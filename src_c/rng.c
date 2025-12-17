// rng.c (xorshift64*)
#include "rng.h"

static uint64_t g_state = 88172645463325252ull;

void rng_seed(uint64_t seed) {
    g_state = (seed ? seed : 88172645463325252ull);
}

double rng_uniform01(void) {
    uint64_t x = g_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    g_state = x;
    uint64_t r = x * 2685821657736338717ull;
    /* map to [0,1) using 53 bits */
    return (r >> 11) * (1.0 / 9007199254740992.0);
}
