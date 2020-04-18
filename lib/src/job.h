#ifndef ARAMID__JOB_H
#define ARAMID__JOB_H

#include <aramid/aramid.h>

#include "job_awaiter.h"
#include "memory_region.h"
#include "procedure.h"
#include "spinlock.h"
#include "types.h"

struct TAG_ARMD_Job {
    ARMD_MemoryRegion *memory_region;
    // procedure
    const ARMD_Procedure *procedure;
    // awaiter
    ARMD__JobAwaiter awaiter;
    // frame & args
    void *frame;
    void *args;
    // continuation
    ARMD_Size continuation_index;
    void *continuation_frame;
    // waiting for child jobs
    ARMD__Spinlock lock;
    volatile ARMD_Bool parent_finished;
    volatile ARMD_Size num_all_waiting_jobs;
    volatile ARMD_Size num_ended_waiting_jobs;
    volatile ARMD_Bool has_error;
    volatile ARMD_ContinuationResult continuation_result;
    // executor
    ARMD__Executor *executor;
    // setup
    ARMD_Bool setup_executed;
    // dependency promise
    ARMD_Bool dependency_has_error;
};

typedef enum TAG_ARMD__JobExecuteStepStatus {
    ARMD__JobExecuteStepStatus_Error,
    ARMD__JobExecuteStepStatus_WaitingForOtherJobs,
    ARMD__JobExecuteStepStatus_CanContinue,
    ARMD__JobExecuteStepStatus_Ended,
} ARMD__JobExecuteStepStatus;

ARMD_EXTERN_C ARMD_Job *armd__job_create(ARMD_MemoryRegion *memory_region,
                                         ARMD__Executor *executor,
                                         const ARMD_Procedure *procedure,
                                         const ARMD__JobAwaiter *awaiter,
                                         void *args);
ARMD_EXTERN_C int armd__job_destroy(ARMD_Job *job);

ARMD_EXTERN_C void armd__job_cleanup_continuation_frame(ARMD_Job *job);
ARMD_EXTERN_C void armd__job_increment_continuation_index(ARMD_Job *job);
ARMD_EXTERN_C ARMD_Bool armd__job_notify_to_parent_and_steal(
    ARMD_Job *job, ARMD__Executor *executor, ARMD_Job **next_job);
ARMD_EXTERN_C ARMD_Bool armd__job_execute_setup(ARMD_Job *job,
                                                ARMD__Executor *executor);
ARMD_EXTERN_C ARMD__JobExecuteStepStatus
armd__job_execute_step(ARMD_Job *job, ARMD__Executor *executor);

#endif // ARAMID__JOB_H
