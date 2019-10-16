#include <assert.h>

#include "condvar.h"

#if defined(ARAMID_USE_PTHREAD)

#include <pthread.h>

int armd__condvar_init(ARMD__Condvar *condvar) {
    int res;
    (void)res;

    pthread_condattr_t attr;
    if (pthread_condattr_init(&attr)) {
        return -1;
    }
    if (pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE)) {
        return -1;
    }

    int result = pthread_cond_init(&condvar->cond, &attr);

    res = pthread_condattr_destroy(&attr);
    assert(res == 0);

    return result;
}

int armd__condvar_deinit(ARMD__Condvar *condvar) {
    return pthread_cond_destroy(&condvar->cond);
}

int armd__condvar_wait(ARMD__Condvar *condvar, ARMD__Mutex *mutex) {
    return pthread_cond_wait(&condvar->cond, &mutex->mutex);
}

int armd__condvar_signal(ARMD__Condvar *condvar) {
    return pthread_cond_signal(&condvar->cond);
}

int armd__condvar_broadcast(ARMD__Condvar *condvar) {
    return pthread_cond_broadcast(&condvar->cond);
}

#elif defined(ARAMID_USE_WIN32THREAD)

#include <windows.h>

int armd__condvar_init(ARMD__Condvar *condvar) {
    InitializeConditionVariable(&condvar->cond);
    return 0;
}

int armd__condvar_deinit(ARMD__Condvar *condvar) {
    (void)condvar;
    return 0;
}

int armd__condvar_wait(ARMD__Condvar *condvar, ARMD__Mutex *mutex) {
    BOOL result = SleepConditionVariableCS(&condvar->cond,
                                           &mutex->critical_section, INFINITE);
    return result == 0;
}

int armd__condvar_signal(ARMD__Condvar *condvar) {
    WakeConditionVariable(&condvar->cond);
    return 0;
}

int armd__condvar_broadcast(ARMD__Condvar *condvar) {
    WakeAllConditionVariable(&condvar->cond);
    return 0;
}

#elif defined(ARAMID_EDITOR)

int armd__condvar_init(ARMD__Condvar *condvar) {
    assert(0);
    return 0;
}

int armd__condvar_deinit(ARMD__Condvar *condvar) {
    assert(0);
    return 0;
}

int armd__condvar_wait(ARMD__Condvar *condvar, ARMD__Mutex *mutex) {
    assert(0);
    return 1;
}

int armd__condvar_signal(ARMD__Condvar *condvar) {
    assert(0);
    return 0;
}

int armd__condvar_broadcast(ARMD__Condvar *condvar) {
    assert(0);
    return 0;
}

#else
#error Thread implementation is not specified
#endif
