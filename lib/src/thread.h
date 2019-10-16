#ifndef ARAMID__THREAD_H
#define ARAMID__THREAD_H

#include <aramid/aramid.h>

typedef struct TAG_ARMD__Thread ARMD__Thread;

typedef void *(*ThreadMainFunc)(void *arg);

ARMD_EXTERN_C int armd__thread_create(ARMD__Thread *thread,
                                      ThreadMainFunc thread_main_func,
                                      void *arg);
ARMD_EXTERN_C int armd__thread_join(ARMD__Thread *thread, void **result);

#if defined(ARAMID_USE_PTHREAD)

#include <pthread.h>

struct TAG_ARMD__Thread {
    pthread_t thread;
};

#elif defined(ARAMID_USE_WIN32THREAD)

#include <windows.h>

struct TAG_ARMD__Thread {
    HANDLE thread;
    ThreadMainFunc main_func;
    void *main_func_arg;
    void *result;
};

#elif defined(ARAMID_EDITOR)

struct TAG_ARMD__Thread {
    int _x;
};

#else
#error Thread implementation is not specified
#endif

#endif // ARAMID__THREAD_H
