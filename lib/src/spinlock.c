#include <assert.h>
#include <stddef.h>

#include <aramid/aramid.h>

#include "spinlock.h"

#if defined(ARAMID_USE_GCC_INTRINSIC_SPINLOCK)

int armd__spinlock_init(ARMD__Spinlock *spinlock) {
    __atomic_store_n(&spinlock->is_locked, 0, __ATOMIC_RELEASE);

    return 0;
}

int armd__spinlock_deinit(ARMD__Spinlock *spinlock) {
    (void)spinlock;
    return 0;
}

int armd__spinlock_lock(ARMD__Spinlock *spinlock) {
    ARMD_Bool expect;
    do {
        expect = 0;
    } while (!__atomic_compare_exchange_n(&spinlock->is_locked, &expect, 1, 0,
                                          __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE));
    __atomic_thread_fence(__ATOMIC_ACQUIRE);

    return 0;
}

int armd__spinlock_unlock(ARMD__Spinlock *spinlock) {
    __atomic_thread_fence(__ATOMIC_RELEASE);
    __atomic_store_n(&spinlock->is_locked, 0, __ATOMIC_RELEASE);

    return 0;
}

#elif defined(ARAMID_USE_MSVC_INTRINSIC_SPINLOCK)

#include <intrin.h>

int armd__spinlock_init(ARMD__Spinlock *spinlock) {
    _InterlockedExchange((volatile long *)&spinlock->is_locked, 0);

    return 0;
}

int armd__spinlock_deinit(ARMD__Spinlock *spinlock) {
    (void)spinlock;
    return 0;
}

int armd__spinlock_lock(ARMD__Spinlock *spinlock) {
    while (_InterlockedCompareExchange((volatile long *)&spinlock->is_locked, 1,
                                       0) == 1) {
    }

    return 0;
}

int armd__spinlock_unlock(ARMD__Spinlock *spinlock) {
    _InterlockedExchange((volatile long *)&spinlock->is_locked, 0);

    return 0;
}

#elif defined(ARAMID_EDITOR)

int armd__spinlock_init(ARMD__Spinlock *spinlock) {
    assert(0);
    return 0;
}

int armd__spinlock_deinit(ARMD__Spinlock *spinlock) {
    assert(0);
    return 0;
}

int armd__spinlock_lock(ARMD__Spinlock *spinlock) {
    assert(0);
    return 0;
}

int armd__spinlock_unlock(ARMD__Spinlock *spinlock) {
    assert(0);
    return 0;
}

#else
#error Spinlock implementation is not specified
#endif
