#ifndef ARAMID__EXECUTOR_H
#define ARAMID__EXECUTOR_H

#include <aramid/aramid.h>

#include "deque.h"
#include "spinlock.h"
#include "thread.h"
#include "types.h"

struct TAG_ARMD__Executor {
    ARMD_Context *context;
    ARMD_Size id;
    ARMD__Spinlock lock;
    ARMD__Thread thread;
    ARMD__Deque *deque;
    volatile ARMD_Bool thread_should_continue_running;
    volatile ARMD_Bool context_ready;
};

ARMD_EXTERN_C ARMD__Executor *armd__executor_create(ARMD_Context *context,
                                                    ARMD_Size id);
ARMD_EXTERN_C int armd__executor_destroy(ARMD__Executor *executor);

#endif // ARAMID__EXECUTOR_H
