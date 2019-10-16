#ifndef ARAMID__MUTEX_H
#define ARAMID__MUTEX_H

#include <aramid/aramid.h>

typedef struct TAG_ARMD__Mutex ARMD__Mutex;

ARMD_EXTERN_C int armd__mutex_init(ARMD__Mutex *mutex);
ARMD_EXTERN_C int armd__mutex_deinit(ARMD__Mutex *mutex);
ARMD_EXTERN_C int armd__mutex_lock(ARMD__Mutex *mutex);
ARMD_EXTERN_C int armd__mutex_unlock(ARMD__Mutex *mutex);

#if defined(ARAMID_USE_PTHREAD)

#include <pthread.h>

struct TAG_ARMD__Mutex {
    pthread_mutex_t mutex;
};

#elif defined(ARAMID_USE_WIN32THREAD)

#include <windows.h>

struct TAG_ARMD__Mutex {
    CRITICAL_SECTION critical_section;
};

#elif defined(ARAMID_EDITOR)

struct TAG_ARMD__Mutex {
    int _x;
};

#else
#error Thread implementation is not specified
#endif

#endif // ARAMID__MUTEX_H
