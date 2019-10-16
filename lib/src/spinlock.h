#ifndef ARAMID__SPINLOCK_H
#define ARAMID__SPINLOCK_H

#include <aramid/aramid.h>

typedef struct TAG_ARMD__Spinlock ARMD__Spinlock;

ARMD_EXTERN_C int armd__spinlock_init(ARMD__Spinlock *spinlock);
ARMD_EXTERN_C int armd__spinlock_deinit(ARMD__Spinlock *spinlock);
ARMD_EXTERN_C int armd__spinlock_lock(ARMD__Spinlock *spinlock);
ARMD_EXTERN_C int armd__spinlock_unlock(ARMD__Spinlock *spinlock);

#if defined(ARAMID_USE_GCC_INTRINSIC_SPINLOCK)

struct TAG_ARMD__Spinlock {
    ARMD_Bool is_locked;
};

#elif defined(ARAMID_USE_MSVC_INTRINSIC_SPINLOCK)

struct TAG_ARMD__Spinlock {
    ARMD_Bool is_locked;
};

#elif defined(ARAMID_EDITOR)

struct TAG_ARMD__Spinlock {
    int _x;
};

#else
#error Spinlock implementation is not specified
#endif

#endif // ARAMID__SPINLOCK_H
