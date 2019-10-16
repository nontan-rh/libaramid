#ifndef ARAMID__JOB_AWAITER_H
#define ARAMID__JOB_AWAITER_H

#include <aramid/aramid.h>

typedef enum TAG_ARMD__JobAwaiterType {
    JobAwaiterType_Promise,
    JobAwaiterType_ParentJob,
} ARMD__JobAwaiterType;

typedef struct ARMD__TAG_JobAwaiter {
    ARMD__JobAwaiterType type;
    union {
        struct {
            ARMD_Handle handle;
        } promise;
        struct {
            ARMD_Job *parent_job;
        } parent_job;
    } body;
} ARMD__JobAwaiter;

#endif
