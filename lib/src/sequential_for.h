#ifndef ARAMID__SEQUENTIAL_FOR_H
#define ARAMID__SEQUENTIAL_FOR_H

#include <stdint.h>

#include <aramid/aramid.h>

typedef struct TAG_ARMD__SequentialForContinuationConstants {
    ARMD_SequentialForCountFunc sequential_for_count_func;
    ARMD_SequentialForContinuationFunc sequential_for_continuation_func;
} ARMD__SequentialForContinuationConstants;

typedef struct TAG_ARMD__SequentialForContinuationFrame {
    ARMD_Bool is_first_time;
    ARMD_Size count;
    ARMD_Size index;
} ARMD__SequentialForContinuationFrame;

#endif // ARAMID__SEQUENTIAL_FOR_H
