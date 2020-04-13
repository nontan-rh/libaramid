#include <assert.h>

#include <aramid/aramid.h>

#include "context.h"
#include "executor.h"
#include "job.h"
#include "job_awaiter.h"
#include "memory_region.h"
#include "procedure.h"

ARMD_Job *armd__job_create(ARMD_MemoryRegion *memory_region,
                           ARMD__Executor *executor,
                           const ARMD_Procedure *procedure,
                           const ARMD__JobAwaiter *awaiter, void *args) {
    assert(memory_region != NULL);
    assert(executor != NULL);
    assert(procedure != NULL);
    assert(awaiter != NULL);

    int res = 0;
    (void)res;
    int job_initialized = 0;
    int frame_initialized = 0;
    int spinlock_initialized = 0;
    ARMD_Job *job;

    job = armd_memory_region_allocate(memory_region, sizeof(ARMD_Job));
    if (job == NULL) {
        goto error;
    }
    job_initialized = 1;

    job->memory_region = memory_region;
    job->procedure = procedure;
    job->awaiter = *awaiter;
    job->frame = armd_memory_region_allocate(
        memory_region, procedure->frame_size == 0 ? 1 : procedure->frame_size);
    if (job->frame == NULL) {
    }
    frame_initialized = 1;
    job->args = args;
    job->continuation_index = 0;
    job->continuation_frame = NULL;
    job->executor = executor;
    if (armd__spinlock_init(&job->lock)) {
        goto error;
    }
    spinlock_initialized = 1; // NOLINT(clang-analyzer-deadcode.DeadStores)

    return job;

error:
    if (spinlock_initialized) {
        res = armd__spinlock_deinit(&job->lock);
        assert(res == 0);
    }

    if (frame_initialized) {
        armd_memory_region_free(memory_region, job->frame);
    }

    if (job_initialized) {
        armd_memory_region_free(memory_region, job);
    }

    return NULL;
}

int armd__job_destroy(ARMD_Job *job) {
    assert(job != NULL);

    int res = 0;
    (void)res;

    res = armd__spinlock_deinit(&job->lock);
    assert(res == 0);

    // args are owned by owner
    job->args = NULL;

    armd_memory_region_free(job->memory_region, job->frame);
    job->frame = NULL;

    armd_memory_region_free(job->memory_region, job);

    return 0;
}

ARMD_Size armd_job_get_num_executors(ARMD_Job *job) {
    return job->executor->context->num_executors;
}

ARMD_Size armd_job_get_executor_id(ARMD_Job *job) { return job->executor->id; }

void armd__job_cleanup_continuation_frame(ARMD_Job *job) {
    assert(job->continuation_frame != NULL);

    ARMD__Continuation *continuation =
        &job->procedure->continuations[job->continuation_index];

    continuation->continuation_frame_destroyer(job->memory_region,
                                               job->continuation_frame);
    job->continuation_frame = NULL;
}

void armd__job_increment_continuation_index(ARMD_Job *job) {
    ++job->continuation_index;
}

ARMD_Bool armd__job_notify_to_parent_and_steal(ARMD_Job *job,
                                               ARMD__Executor *executor,
                                               ARMD_Job **next_job) {
    int res = 0;
    (void)res;

    ARMD_Job *parent_job = job->awaiter.body.parent_job.parent_job;

    res = armd__spinlock_lock(&parent_job->lock);
    assert(res == 0);

    ARMD_Size num_all_waiting_jobs = parent_job->num_all_waiting_jobs;
    ARMD_Size num_ended_waiting_jobs = ++parent_job->num_ended_waiting_jobs;
    ARMD_Bool parent_finished = parent_job->parent_finished;
    assert(num_ended_waiting_jobs <= num_all_waiting_jobs);

    if (job->has_error) {
        parent_job->has_error = 1;
    }

    ARMD_Bool parent_enabled =
        parent_finished && num_ended_waiting_jobs >= num_all_waiting_jobs;
    if (parent_enabled) {
        parent_job->executor = executor;
        *next_job = parent_job;
    } else {
        *next_job = NULL;
    }

    res = armd__spinlock_unlock(&parent_job->lock);
    assert(res == 0);

    return parent_enabled;
}

ARMD__JobExecuteStepStatus armd__job_execute_step(ARMD_Job *job,
                                                  ARMD__Executor *executor) {
    (void)executor;

    int res = 0;
    (void)res;

    assert(job->continuation_index <= job->procedure->num_continuations);
    assert(job->num_all_waiting_jobs <=
           job->num_ended_waiting_jobs); // Other jobs are already ended

    if (job->continuation_index == job->procedure->num_continuations) {
        return ARMD__JobExecuteStepStatus_Ended;
    }

    // Run step
    job->parent_finished = 0;
    job->num_all_waiting_jobs = 1; // For myself
    job->num_ended_waiting_jobs = 0;

    ARMD__Continuation *continuation =
        &job->procedure->continuations[job->continuation_index];

    if (job->continuation_frame == NULL) {
        job->continuation_frame =
            continuation->continuation_frame_creator(job->memory_region);
        assert(job->continuation_frame != NULL);
    }

    ARMD_ContinuationResult result = continuation->continuation_func(
        job, job->procedure->constants, job->args, job->frame,
        continuation->continuation_constants, job->continuation_frame);

    res = armd__spinlock_lock(&job->lock);
    assert(res == 0);

    job->continuation_result = result;
    if (result == ARMD_ContinuationResult_Error) {
        job->has_error = 1;
    }

    job->parent_finished = 1;
    ARMD_Size num_ended_waiting_jobs = ++job->num_ended_waiting_jobs;
    ARMD_Size num_all_waiting_jobs = job->num_all_waiting_jobs;
    assert(num_ended_waiting_jobs <= num_all_waiting_jobs);

    res = armd__spinlock_unlock(&job->lock);
    assert(res == 0);

    assert(num_all_waiting_jobs >= num_ended_waiting_jobs);
    if (num_all_waiting_jobs <= num_ended_waiting_jobs) {
        return ARMD__JobExecuteStepStatus_CanContinue;
    }

    return ARMD__JobExecuteStepStatus_WaitingForOtherJobs;
}
