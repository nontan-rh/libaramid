#include <assert.h>

#include "thread.h"

#if defined(ARAMID_USE_PTHREAD)

#include <pthread.h>

int armd__thread_create(ARMD__Thread *thread, ThreadMainFunc thread_main_func,
                        void *arg) {
    int res;
    (void)res;

    pthread_attr_t attr;
    if (pthread_attr_init(&attr)) {
        return -1;
    }

    int result = pthread_create(&thread->thread, &attr, thread_main_func, arg);

    res = pthread_attr_destroy(&attr);
    assert(res == 0);

    return result;
}

int armd__thread_join(ARMD__Thread *thread, void **result) {
    return pthread_join(thread->thread, result);
}

#elif defined(ARAMID_USE_WIN32THREAD)

#include <windows.h>

static DWORD WINAPI ThreadEntryPoint(void *arg) {
    ARMD__Thread *thread = (ARMD__Thread *)arg;
    ThreadMainFunc main_func = thread->main_func;
    void *main_func_arg = thread->main_func_arg;

    thread->result = main_func(main_func_arg);

    return 0;
}

int armd__thread_create(ARMD__Thread *thread, ThreadMainFunc thread_main_func,
                        void *arg) {
    thread->main_func = thread_main_func;
    thread->main_func_arg = arg;
    thread->result = NULL;
    thread->thread = CreateThread(NULL, 0, ThreadEntryPoint, thread, 0, NULL);
    return thread->thread == NULL;
}

int armd__thread_join(ARMD__Thread *thread, void **result) {
    int ret = WaitForSingleObject(thread->thread, INFINITE);
    *result = thread->result;
    return ret;
}

#elif defined(ARAMID_EDITOR)

int armd__thread_create(ARMD__Thread *thread, ThreadMainFunc thread_main_func,
                        void *arg) {
    assert(0);
    return 0;
}

int armd__thread_join(ARMD__Thread *thread, void **result) {
    assert(0);
    return 0;
}

#else
#error Thread implementation is not specified
#endif
