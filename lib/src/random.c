#include <stdint.h>

#include "random.h"

void armd__random_init(ARMD__Random *rand, uint32_t seed) {
    rand->x = 123456789u;
    rand->y = 362436069u;
    rand->z = 521288629u;
    rand->w = seed;
}

uint32_t armd__random_generate(ARMD__Random *rand) {
    uint32_t t = rand->x ^ (rand->x << 11);
    rand->x = rand->y;
    rand->y = rand->z;
    rand->z = rand->w;
    rand->w = (rand->w ^ (rand->w >> 19)) ^ (t ^ (t >> 8));

    return rand->w;
}
