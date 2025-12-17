// rng.h
#ifndef RNG_H
#define RNG_H
#include <stdint.h>
void rng_seed(uint64_t seed);
double rng_uniform01(void);
#endif
