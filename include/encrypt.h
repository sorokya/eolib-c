#ifndef EOLIB_ENCRYPT_H
#define EOLIB_ENCRYPT_H

#include <stdint.h>
#include <stddef.h>

/**
 * Calculates the server verification hash based on the given challenge.
 */
int32_t server_verification_hash(int32_t challenge);

void swap_multiples(uint8_t *data, size_t length, uint8_t multiple);

#endif