#include <assert.h>

#include <aramid/aramid.h>

#include "executor.h"

#include "context.h"
#include "deque.h"
#include "job.h"
#include "promise.h"
#include "random.h"
#include "spinlock.h"

static void *executor_thread_main(void *args) {
    int res = 0;
    (void)res;

    ARMD__Executor *executor = (ARMD__Executor *)args;
    ARMD_Context *context = executor->context;

    ARMD__Random rand;
    armd__random_init(&rand, (uint32_t)executor->id);

    // Waiting for context ready
    {
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
        ARMD_Bool context_ready = executor->context_ready;
        (void)context_ready;

        res = armd__mutex_unlock(&context->executor_mutex);
        assert(res == 0);

        if (!thread_should_continue_running) {
            return NULL;
        }

        assert(context_ready);
    }

    while (1) {
        ARMD_Job *job = NULL;

        // Get free job
        while (1) {
            if (!executor->thread_should_continue_running) {
                return NULL;
            }

            // Check local
            res = armd__spinlock_lock(&executor->lock);
            assert(res == 0);
            armd__deque_dequeue_forward(executor->deque, &job);
            res = armd__spinlock_unlock(&executor->lock);
            assert(res == 0);

            if (job != NULL) {
                res = armd__mutex_lock(&context->executor_mutex);
                assert(res == 0);

                --executor->context->free_job_count;

                res = armd__mutex_unlock(&context->executor_mutex);
                assert(res == 0);
                break;
            }

            if (!executor->thread_should_continue_running) {
                return NULL;
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
                    return NULL;
                }
            }

            // There are some free jobs in some executors

            // Steal
            ARMD_Size victim_index = ((ARMD_Size)armd__random_generate(&rand)) %
                                     context->num_executors;
            ARMD__Executor *victim_executor = context->executors[victim_index];

            res = armd__spinlock_lock(&victim_executor->lock);
            assert(res == 0);
            armd__deque_dequeue_back(victim_executor->deque, &job);
            res = armd__spinlock_unlock(&victim_executor->lock);
            assert(res == 0);

            if (job != NULL) {
                job->executor = executor;

                // implicit memory fence

                res = armd__mutex_lock(&context->executor_mutex);
                assert(res == 0);

                --executor->context->free_job_count;

                res = armd__mutex_unlock(&context->executor_mutex);
                assert(res == 0);
                break;
            }
        }

        // Execute job
        ARMD_Job *executing_job = job;
        while (executing_job != NULL) {
            if (!executor->thread_should_continue_running) {
                return NULL;
            }

            ARMD__JobExecuteStepStatus job_execute_step_status =
                armd__job_execute_step(executing_job, executor);
            switch (job_execute_step_status) {
            case ARMD__JobExecuteStepStatus_WaitingForOtherJobs:
                // Cannot continue
                executing_job = NULL;
                break;
            case ARMD__JobExecuteStepStatus_CanContinue:
                // Just continue
                break;
            case ARMD__JobExecuteStepStatus_Ended:
                switch (executing_job->awaiter.type) {
                case JobAwaiterType_ParentJob: {
                    ARMD_Job *parent_job =
                        executing_job->awaiter.body.parent_job.parent_job;

                    res = armd__spinlock_lock(&parent_job->lock);
                    assert(res == 0);

                    ARMD_Size num_all_waiting_jobs =
                        parent_job->num_all_waiting_jobs;
                    ARMD_Size num_ended_waiting_jobs =
                        ++parent_job->num_ended_waiting_jobs;
                    ARMD_Bool parent_finished = parent_job->parent_finished;
                    assert(num_ended_waiting_jobs <= num_all_waiting_jobs);

                    res = armd__spinlock_unlock(&parent_job->lock);
                    assert(res == 0);

                    armd__job_destroy(executing_job);

                    if (parent_finished &&
                        num_ended_waiting_jobs >= num_all_waiting_jobs) {
                        // parent_job is now enabled, and stealing it
                        parent_job->executor = executor;
                        executing_job = parent_job;
                    } else {
                        executing_job = NULL;
                    }
                } break;
                case JobAwaiterType_Promise: {
                    ARMD_Handle handle =
                        executing_job->awaiter.body.promise.handle;
                    armd__context_complete_promise(context, handle);

                    executing_job = NULL;
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
