#ifndef EOLIB_ENCRYPT_H
#define EOLIB_ENCRYPT_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

/**
 * Calculates the server verification hash for the given challenge value.
 * @param challenge The server challenge value (from ServerInitInitServerPacket).
 * @return The verification hash to be sent back to the server.
 * @remarks Implements the EO protocol challenge-response formula:
 *          hash = 110905 + ((c%9)+1) * ((11092004-c) % (((c%11)+1)*119)) * 119 + (c%2004)
 *          where c = challenge + 1. The constants are protocol-defined magic values.
 */
int32_t server_verification_hash(int32_t challenge);

/**
 * Reverses contiguous sequences of bytes where each value is a multiple of `multiple`.
 * @param data The packet data to mutate.
 * @param length Number of bytes in `data`.
 * @param multiple The divisor used to determine which bytes are in a sequence.
 */
void swap_multiples(uint8_t *data, size_t length, uint8_t multiple);

/**
 * Generates a cryptographically random swap multiple between 6 and 12 inclusive.
 * @return The generated swap multiple.
 */
uint8_t generate_swap_multiple();

/**
 * Encrypts a packet in place using the specified swap multiple.
 * @param data The packet data to encrypt.
 * @param length Number of bytes in `data`.
 * @param swap_multiple The swap multiple for the packet.
 */
void encrypt_packet(uint8_t *data, size_t length, uint8_t swap_multiple);

/**
 * Decrypts a packet in place using the specified swap multiple.
 * @param data The packet data to decrypt.
 * @param length Number of bytes in `data`.
 * @param swap_multiple The swap multiple for the packet.
 */
void decrypt_packet(uint8_t *data, size_t length, uint8_t swap_multiple);

#endif