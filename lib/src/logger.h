#ifndef ARAMID__LOGGER_H
#define ARAMID__LOGGER_H

#include <aramid/aramid.h>

#include "condvar.h"
#include "mutex.h"

typedef struct TAG_ARMD__LogNode {
    struct TAG_ARMD__LogNode *prev;
    struct TAG_ARMD__LogNode *next;
    ARMD_LogElement *log_element;
} ARMD__LogNode;

struct TAG_ARMD_Logger {
    ARMD_MemoryRegion *memory_region;
    ARMD__Mutex mutex;
    ARMD_Size reference_count;
    ARMD__LogNode *ring;
    struct {
        ARMD_LoggerCallbackFunc func;
        void *context;
    } callback;
};

#endif // ARAMID__LOGGER_H
