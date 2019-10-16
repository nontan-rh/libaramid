#ifndef ARAMID__RANDOM_H
#define ARAMID__RANDOM_H

#include <stdint.h>

#include <aramid/aramid.h>

typedef struct TAG_ARMD__Random {
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint32_t w;
} ARMD__Random;

ARMD_EXTERN_C void armd__random_init(ARMD__Random *rand, uint32_t seed);
ARMD_EXTERN_C uint32_t armd__random_generate(ARMD__Random *rand);

#endif // ARAMID__RANDOM_H
