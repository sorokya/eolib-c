#ifndef EOLIB_ENCRYPT_H
#define EOLIB_ENCRYPT_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "eolib/rng.h"

/**
 * Generates a server verification challenge value, which is a random integer used in the EO protocol's
 * challenge-response authentication process. The client receives this challenge from the server and must compute
 * the correct response using the `eo_server_verification_hash` function to authenticate itself to the server.
 * @return The generated server verification challenge value.
 * @remarks Emulates the behavior of the original game client: eo_rand() % 1000000
 */
static inline int32_t eo_generate_server_verification_challenge()
{
    return eo_rand() % (int32_t)1000000;
}

/**
 * Calculates the server verification hash for the given challenge value.
 * @param challenge The server challenge value (from ServerInitInitServerPacket).
 * @return The verification hash to be sent back to the server.
 * @remarks Implements the EO protocol challenge-response formula:
 *          hash = 110905 + ((c%9)+1) * ((11092004-c) % (((c%11)+1)*119)) * 119 + (c%2004)
 *          where c = challenge + 1. The constants are protocol-defined magic values.
 */
int32_t eo_server_verification_hash(int32_t challenge);

/**
 * Reverses contiguous sequences of bytes where each value is a multiple of `multiple`.
 * @param data The packet data to mutate.
 * @param length Number of bytes in `data`.
 * @param multiple The divisor used to determine which bytes are in a sequence.
 */
void eo_swap_multiples(uint8_t *data, size_t length, uint8_t multiple);

/**
 * Generates a cryptographically random swap multiple between 6 and 12 inclusive.
 * @return The generated swap multiple.
 */
uint8_t eo_generate_swap_multiple();

/**
 * Encrypts a packet in place using the specified swap multiple.
 * @param data The packet data to encrypt.
 * @param length Number of bytes in `data`.
 * @param swap_multiple The swap multiple for the packet.
 */
void eo_encrypt_packet(uint8_t *data, size_t length, uint8_t swap_multiple);

/**
 * Decrypts a packet in place using the specified swap multiple.
 * @param data The packet data to decrypt.
 * @param length Number of bytes in `data`.
 * @param swap_multiple The swap multiple for the packet.
 */
void eo_decrypt_packet(uint8_t *data, size_t length, uint8_t swap_multiple);

#endif