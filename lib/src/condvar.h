#ifndef ARAMID__CONDVAR_H
#define ARAMID__CONDVAR_H

#include <aramid/aramid.h>

#include "mutex.h"

typedef struct TAG_ARMD__Condvar ARMD__Condvar;

ARMD_EXTERN_C int armd__condvar_init(ARMD__Condvar *condvar);
ARMD_EXTERN_C int armd__condvar_deinit(ARMD__Condvar *condvar);
ARMD_EXTERN_C int armd__condvar_wait(ARMD__Condvar *condvar,
                                     ARMD__Mutex *mutex);
ARMD_EXTERN_C int armd__condvar_notify(ARMD__Condvar *condvar);
ARMD_EXTERN_C int armd__condvar_broadcast(ARMD__Condvar *condvar);

#if defined(ARAMID_USE_PTHREAD)

#include <pthread.h>

struct TAG_ARMD__Condvar {
    pthread_cond_t cond;
};

#elif defined(ARAMID_USE_WIN32THREAD)

#include <windows.h>

struct TAG_ARMD__Condvar {
    CONDITION_VARIABLE cond;
};

#elif defined(ARAMID_EDITOR)

struct TAG_ARMD__Condvar {
    int _x;
};

#else
#error Thread implementation is not specified
#endif

#endif // ARAMID__CONDVAR_H
