#ifndef EOLIB_ENCRYPT_H
#define EOLIB_ENCRYPT_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

/**
 * Calculates the server verification hash based on the given challenge.
 */
int32_t server_verification_hash(int32_t challenge);

/**
 * Swaps sequences of bytes in the data array that are multiples of the given value.
 */
void swap_multiples(uint8_t *data, size_t length, uint8_t multiple);

/**
 * Generates a random swap multiple between 6 and 12 inclusive.
 */
uint8_t generate_swap_multiple();

#endif