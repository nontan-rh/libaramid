#include <aramid/aramid.h>

#if defined(_WIN32)

#include <stdio.h>
#include <time.h>

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

char *armd_format_time_iso8601(ARMD_MemoryRegion *memory_region,
                               const ARMD_Timespec *timespec) {
    time_t time = timespec->seconds;
    struct tm tm;
    gmtime_s(&time, &tm);

    char secstr[128];
    strftime(secstr, sizeof(secstr), "%Y-%m-%dT%H:%M:%S", &tm);

    char buf[128];
    snprintf(buf, sizeof(buf), "%s.%03dZ", secstr,
             (int)(timespec->nanoseconds / 1000000ll));

    return armd_memory_region_strdup(memory_region, buf);
}

#elif defined(unix) || defined(__unix__) || defined(__unix) ||                 \
    defined(__APPLE__)

#include <stdio.h>
#include <time.h>

int armd_get_time(ARMD_Timespec *result) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    result->seconds = ts.tv_sec;
    result->nanoseconds = ts.tv_nsec;

    return 0;
}

char *armd_format_time_iso8601(ARMD_MemoryRegion *memory_region,
                               const ARMD_Timespec *timespec) {
    time_t time = timespec->seconds;
    struct tm tm;
    gmtime_r(&time, &tm);

    char secstr[128];
    strftime(secstr, sizeof(secstr), "%Y-%m-%dT%H:%M:%S", &tm);

    char buf[128];
    snprintf(buf, sizeof(buf), "%s.%03dZ", secstr,
             (int)(timespec->nanoseconds / 1000000ll));

    return armd_memory_region_strdup(memory_region, buf);
}

#elif ARAMID_EDITOR
#else
#error OS not supported
#endif
