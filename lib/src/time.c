#include <aramid/aramid.h>

#if defined(_WIN32)

#include <windows.h>

int armd_get_time(ARMD_Timespec *result) {
    FILETIME filetime;
    GetSystemTimeAsFileTime(&filetime);

    int64_t filetime_int64 =
        filetime.dwLowDateTime | (((int64_t)filetime.dwHighDateTime) << 32);

    filetime_int64 -= 116444736000000000ll; // Fix Epoch to Unix
    result->seconds = filetime_int64 / 10000000ll;
    result->nanoseconds = filetime_int64 % 10000000ll * 100ll;

    return 0;
}

#elif defined(unix) || defined(__unix__) || defined(__unix) ||                 \
    defined(__APPLE__)

#include <time.h>

int armd_get_time(ARMD_Timespec *result) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    result->seconds = ts.tv_sec;
    result->nanoseconds = ts.tv_nsec;

    return 0;
}

#elif ARAMID_EDITOR
#else
#error OS not supported
#endif
