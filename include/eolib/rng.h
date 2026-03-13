#ifndef EOLIB_RNG_H
#define EOLIB_RNG_H

#include <stdint.h>

/**
 * Seed the random number generator with a 32-bit unsigned integer. The same seed will
 * produce the same sequence of random numbers. The seed is transformed internally to
 * initialize the generator's state.
 */
void eo_srand(uint32_t seed);

/**
 * Generate a pseudo-random 32-bit unsigned integer. The sequence of numbers generated
 * depends on the seed set by eo_srand().
 * @remark Emulates the behavior of the original game client.
 */
uint32_t eo_rand();

/**
 * Generate a pseudo-random 32-bit unsigned integer within a specified range [min, max].
 * The sequence of numbers generated depends on the seed set by eo_srand().
 */
uint32_t eo_rand_range(uint32_t min, uint32_t max);

#endif