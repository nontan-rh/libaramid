#include <assert.h>

#include <aramid/aramid.h>

#include "executor.h"

#include "context.h"
#include "deque.h"
#include "job.h"
#include "promise.h"
#include "random.h"
#include "spinlock.h"

static ARMD_Bool wait_for_context_ready(ARMD_Context *context,
                                        ARMD__Executor *executor) {
    int res = 0;
    (void)res;

    res = armd__mutex_lock(&context->executor_mutex);
    assert(res == 0);

    while (executor->thread_should_continue_running &&
           !executor->context_ready) {
        res = armd__condvar_wait(&context->executor_condvar,
                                 &context->executor_mutex);
        assert(res == 0);
    }

    ARMD_Bool thread_should_continue_running =
        executor->thread_should_continue_running;
    assert(executor->context_ready);

    res = armd__mutex_unlock(&context->executor_mutex);
    assert(res == 0);

    return thread_should_continue_running;
}

static ARMD_Bool get_free_job(ARMD_Context *context, ARMD__Executor *executor,
                              ARMD__Random *rand, ARMD_Job **job) {
    int res = 0;
    (void)res;

    // Get free job
    while (1) {
        if (!executor->thread_should_continue_running) {
            return 0;
        }

        // Check local
        res = armd__spinlock_lock(&executor->lock);
        assert(res == 0);

        armd__deque_dequeue_forward(executor->deque, job);

        res = armd__spinlock_unlock(&executor->lock);
        assert(res == 0);

        if (*job != NULL) {
            res = armd__mutex_lock(&context->executor_mutex);
            assert(res == 0);

            --executor->context->free_job_count;

            res = armd__mutex_unlock(&context->executor_mutex);
            assert(res == 0);
            return 1;
        }

        if (!executor->thread_should_continue_running) {
            return 0;
        }

        // Waiting for job
        {
            res = armd__mutex_lock(&context->executor_mutex);
            assert(res == 0);

            while (context->free_job_count == 0 &&
                   executor->thread_should_continue_running) {
                res = armd__condvar_wait(&context->executor_condvar,
                                         &context->executor_mutex);
                assert(res == 0);
            }

            ARMD_Bool thread_should_continue_running =
                executor->thread_should_continue_running;

            res = armd__mutex_unlock(&context->executor_mutex);
            assert(res == 0);

            if (!thread_should_continue_running) {
                return 0;
            }
        }

        // There are some free jobs in some executors

        // Steal
        ARMD_Size victim_index =
            ((ARMD_Size)armd__random_generate(rand)) % context->num_executors;
        ARMD__Executor *victim_executor = context->executors[victim_index];

        res = armd__spinlock_lock(&victim_executor->lock);
        assert(res == 0);

        armd__deque_dequeue_back(victim_executor->deque, job);

        res = armd__spinlock_unlock(&victim_executor->lock);
        assert(res == 0);

        if (*job != NULL) {
            (*job)->executor = executor;

            // implicit memory fence

            res = armd__mutex_lock(&context->executor_mutex);
            assert(res == 0);

            --executor->context->free_job_count;

            res = armd__mutex_unlock(&context->executor_mutex);
            assert(res == 0);

            return 1;
        }
    }
}

static void unwind(ARMD_Job *job) {
    ARMD_UnwindFunc unwind_func = job->procedure->unwind_func;
    const void *job_constants = job->procedure->constants;
    void *job_args = job->args;
    void *job_frame = job->frame;
    if (unwind_func != NULL) {
        unwind_func(job, job_constants, job_args, job_frame);
    }
}

static int move_to_next(ARMD_Job *job) {
    int res = 0;
    (void)res;

    res = armd__spinlock_lock(&job->lock);
    assert(res == 0);

    ARMD__Continuation *continuation =
        &job->procedure->continuations[job->continuation_index];

    ARMD_ContinuationResult continuation_result;
    if (job->has_error) {
        if (continuation->error_trap_func != NULL) {
            continuation_result = continuation->error_trap_func(
                job, job->procedure->constants, job->args, job->frame,
                continuation->continuation_constants, job->continuation_frame);
        } else {
            continuation_result = ARMD_ContinuationResult_Error;
        }
    } else {
        continuation_result = job->continuation_result;
    }

    res = armd__spinlock_unlock(&job->lock);
    assert(res == 0);

    int should_abort = 0;
    switch (continuation_result) {
    case ARMD_ContinuationResult_Error:
        armd__job_cleanup_continuation_frame(job);
        should_abort = 1;
        break;
    case ARMD_ContinuationResult_Ended:
        armd__job_cleanup_continuation_frame(job);
        armd__job_increment_continuation_index(job);
        should_abort = 0;
        break;
    case ARMD_ContinuationResult_Repeat:
        should_abort = 0;
        break;
    default:
        assert(0);
        break;
    }

    return should_abort;
}

static ARMD_Job *move_to_next_and_propagate_error(ARMD_Context *context,
                                                  ARMD__Executor *executor,
                                                  ARMD_Job *job) {
    while (job != NULL) {
        int should_abort = move_to_next(job);
        if (!should_abort) {
            break;
        }

        unwind(job);

        switch (job->awaiter.type) {
        case JobAwaiterType_ParentJob: {
            ARMD_Job *next_job;
            ARMD_Bool stole =
                armd__job_notify_to_parent_and_steal(job, executor, &next_job);
            armd__job_destroy(job);

            if (stole) {
                job = next_job;
            } else {
                job = NULL;
            }
        } break;
        case JobAwaiterType_Promise: {
            ARMD_Handle handle = job->awaiter.body.promise.handle;
            int res = armd__context_complete_promise(context, handle, 1);
            assert(res == 0);

            armd__job_destroy(job);
            job = NULL;
        } break;
        default:
            assert(0);
            break;
        }
    }

    return job;
}

static void *executor_thread_main(void *args) {
    int res = 0;
    (void)res;

    ARMD__Executor *executor = (ARMD__Executor *)args;
    ARMD_Context *context = executor->context;

    ARMD__Random rand;
    armd__random_init(&rand, (uint32_t)executor->id);

    if (!wait_for_context_ready(context, executor)) {
        return NULL;
    }

    while (1) {
        ARMD_Job *job;
        if (!get_free_job(context, executor, &rand, &job)) {
            return NULL;
        }

        // Execute job
        while (job != NULL) {
            if (!executor->thread_should_continue_running) {
                return NULL;
            }

            ARMD__JobExecuteStepStatus job_execute_step_status =
                armd__job_execute_step(job, executor);
            switch (job_execute_step_status) {
            case ARMD__JobExecuteStepStatus_WaitingForOtherJobs:
                // Cannot continue
                job = NULL;
                break;
            case ARMD__JobExecuteStepStatus_CanContinue: {
                // Just continue
                job = move_to_next_and_propagate_error(context, executor, job);
            } break;
            case ARMD__JobExecuteStepStatus_Ended:
                unwind(job);

                switch (job->awaiter.type) {
                case JobAwaiterType_ParentJob: {
                    ARMD_Job *next_job;
                    ARMD_Bool stole = armd__job_notify_to_parent_and_steal(
                        job, executor, &next_job);
                    armd__job_destroy(job);

                    if (stole) {
                        job = move_to_next_and_propagate_error(
                            context, executor, next_job);
                    } else {
                        job = NULL;
                    }
                } break;
                case JobAwaiterType_Promise: {
                    ARMD_Handle handle = job->awaiter.body.promise.handle;
                    res = armd__context_complete_promise(context, handle, 0);
                    assert(res == 0);
                    armd__job_destroy(job);

                    job = NULL;
                } break;
                default:
                    assert(0);
                    break;
                }
                break;
            default:
                assert(0);
                break;
            }
        }
    }

    return NULL;
}

ARMD__Executor *armd__executor_create(ARMD_Context *context, ARMD_Size id) {
    assert(context != NULL);

    const ARMD_Size initial_deque_size = 128;
    int res = 0;
    (void)res;

    ARMD_MemoryRegion *memory_region = context->memory_region;

    int executor_initialized = 0;
    int deque_initialized = 0;
    int spinlock_initialized = 0;
    int thread_initialized = 0;

    ARMD__Executor *executor = NULL;

    executor =
        armd_memory_region_allocate(memory_region, sizeof(ARMD__Executor));
    if (executor == NULL) {
        goto error;
    }
    executor_initialized = 1;

    executor->thread_should_continue_running = 1;
    executor->id = id;

    executor->context = context;
    executor->deque =
        armd__deque_create(context->memory_region, initial_deque_size);
    if (executor->deque == NULL) {
        goto error;
    }
    deque_initialized = 1;

    if (armd__spinlock_init(&executor->lock)) {
        goto error;
    }
    spinlock_initialized = 1;

    if (armd__thread_create(&executor->thread, executor_thread_main,
                            executor) != 0) {
        goto error;
    }
    thread_initialized = 1; // NOLINT(clang-analyzer-deadcode.DeadStores)

    return executor;

error:
    if (thread_initialized) {
        res = armd__mutex_lock(&context->executor_mutex);
        assert(res == 0);

        executor->thread_should_continue_running = 0;
        res = armd__condvar_broadcast(&context->executor_condvar);
        assert(res == 0);

        res = armd__mutex_unlock(&context->executor_mutex);
        assert(res == 0);

        void *result;
        res = armd__thread_join(&executor->thread, &result);
        assert(res == 0);
    }

    if (spinlock_initialized) {
        res = armd__spinlock_deinit(&executor->lock);
        assert(res == 0);
    }

    if (deque_initialized) {
        res = armd__deque_destroy(executor->deque);
        assert(res == 0);
    }

    if (executor_initialized) {
        armd_memory_region_free(memory_region, executor);
    }

    return NULL;
}

int armd__executor_destroy(ARMD__Executor *executor) {
    int status = 0;
    int res = 0;
    (void)res;

    if (executor == NULL) {
        return -1;
    }

    ARMD_MemoryRegion *memory_region = executor->context->memory_region;

    res = armd__mutex_lock(&executor->context->executor_mutex);
    assert(res == 0);

    executor->thread_should_continue_running = 0;
    res = armd__condvar_broadcast(&executor->context->executor_condvar);
    assert(res == 0);

    res = armd__mutex_unlock(&executor->context->executor_mutex);
    assert(res == 0);

    void *result;
    res = armd__thread_join(&executor->thread, &result);
    assert(res == 0);

    status = armd__deque_destroy(executor->deque);
    executor->deque = NULL;

    res = armd__spinlock_deinit(&executor->lock);
    assert(res == 0);

    armd_memory_region_free(memory_region, executor);
    return status;
}
