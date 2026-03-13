#include "eolib/rng.h"

static uint32_t state_high;
static uint32_t current_seed;

void eo_srand(uint32_t seed)
{
    current_seed = 0x1586B75 * seed + 1;
    state_high = 0;
}

uint32_t eo_rand()
{
    uint32_t temp_state_product = state_high ? state_high * 20021 : 0;
    uint32_t temp_seed_product = current_seed * 346;

    // multiply current_seed by 20021 using 64-bit integer
    uint64_t full_product = (uint64_t)current_seed * 20021;

    // extract high and low 32-bit parts
    uint32_t seed_lo = (uint32_t)full_product;
    uint32_t seed_hi = (uint32_t)(full_product >> 32);

    // add contributions to high word
    seed_hi += temp_state_product + temp_seed_product;

    // increment the low word
    seed_lo += 1;

    // update state
    current_seed = seed_lo;
    state_high = seed_hi;

    // return upper 31 bits
    return seed_hi & 0x7FFFFFFF;
}