#ifndef ARAMID__PARALLEL_FOR_H
#define ARAMID__PARALLEL_FOR_H

#include <aramid/aramid.h>

typedef struct TAG_ARMD__ParallelForContinuationFrame
    ARMD__ParallelForContinuationFrame;

typedef struct TAG_ARMD__ParallelForChildProcedureConstants {
    ARMD_ParallelForContinuationFunc parallel_for_continuation_func;
} ARMD__ParallelForChildProcedureConstants;

typedef struct TAG_ARMD__ParallelForChildProcedureArgs {
    ARMD__ParallelForContinuationFrame *parent_continuation_frame;
    void *parent_frame;
    const void *parent_constants;
    void *parent_args;
} ARMD__ParallelForChildProcedureArgs;

typedef struct TAG_ARMD__ParallelForContinuationConstants {
    ARMD_Procedure *child_procedure;
    ARMD_ParallelForCountFunc parallel_for_count_func;
    ARMD_ParallelForContinuationFunc parallel_for_continuation_func;
} ARMD__ParallelForContinuationConstants;

struct TAG_ARMD__ParallelForContinuationFrame {
    ARMD_Bool is_first_time;
    ARMD_Size count;
    ARMD_Size index;
    ARMD__ParallelForChildProcedureArgs child_args;
};

#endif // ARAMID__PARALLEL_FOR_H
