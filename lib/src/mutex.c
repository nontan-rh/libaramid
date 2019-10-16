#include <assert.h>

#include "mutex.h"

#if defined(ARAMID_USE_PTHREAD)

#include <pthread.h>

int armd__mutex_init(ARMD__Mutex *mutex) {
    int res;
    (void)res;

    pthread_mutexattr_t attr;
    if (pthread_mutexattr_init(&attr)) {
        return -1;
    }
    if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK)) {
        return -1;
    }

    int result = pthread_mutex_init(&mutex->mutex, &attr);

    res = pthread_mutexattr_destroy(&attr);
    assert(res == 0);

    return result;
}

int armd__mutex_deinit(ARMD__Mutex *mutex) {
    return pthread_mutex_destroy(&mutex->mutex);
}

int armd__mutex_lock(ARMD__Mutex *mutex) {
    return pthread_mutex_lock(&mutex->mutex);
}

int armd__mutex_unlock(ARMD__Mutex *mutex) {
    return pthread_mutex_unlock(&mutex->mutex);
}

#elif defined(ARAMID_USE_WIN32THREAD)

#include <windows.h>

int armd__mutex_init(ARMD__Mutex *mutex) {
    InitializeCriticalSection(&mutex->critical_section);
    return 0;
}

int armd__mutex_deinit(ARMD__Mutex *mutex) {
    DeleteCriticalSection(&mutex->critical_section);
    return 0;
}

int armd__mutex_lock(ARMD__Mutex *mutex) {
    EnterCriticalSection(&mutex->critical_section);
    return 0;
}

int armd__mutex_unlock(ARMD__Mutex *mutex) {
    LeaveCriticalSection(&mutex->critical_section);
    return 0;
}

#elif defined(ARAMID_EDITOR)

int armd__mutex_init(ARMD__Mutex *mutex) {
    assert(0);
    return 0;
}

int armd__mutex_deinit(ARMD__Mutex *mutex) {
    assert(0);
    return 0;
}

int armd__mutex_lock(ARMD__Mutex *mutex) {
    assert(0);
    return 0;
}

int armd__mutex_unlock(ARMD__Mutex *mutex) {
    assert(0);
    return 0;
}

#else
#error Thread implementation is not specified
#endif
