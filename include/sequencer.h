#ifndef EOLIB_SEQUENCER_H
#define EOLIB_SEQUENCER_H

#include <stdint.h>

#include "result.h"

typedef struct Sequencer
{
    int32_t start;
    int32_t counter;
} Sequencer;

/**
 * Initializes a sequencer with the given starting value.
 * @param start The initial value for the sequencer.
 * @return The initialized sequencer.
 */
Sequencer sequencer_init(int32_t start);

/**
 * Gets the next value in the sequence, advances the counter, and writes it
 * to out_value.
 * @param sequencer The sequencer to advance.
 * @param out_value Output for the next sequence value. May be NULL to advance
 *                  without capturing the value.
 * @return EO_SUCCESS on success, or EO_NULL_PTR if sequencer is NULL.
 */
EoResult sequencer_next(Sequencer *sequencer, int32_t *out_value);

/**
 * Generates a cryptographically random starting value for a sequencer.
 * @return A random starting value.
 * @remarks The value is guaranteed to fit in the ServerInitInitPacket sequence fields.
 */
int32_t generate_sequence_start();

/**
 * Calculates the starting sequence value based on the initial server sequence values.
 * @param s1 The first sequence value from the server.
 * @param s2 The second sequence value from the server.
 * @return The calculated starting sequence value.
 */
int32_t sequence_start_from_init(int32_t s1, int32_t s2);

/**
 * Calculates the starting sequence value based on the ping sequence values.
 * @param s1 The first sequence value from the server ping response.
 * @param s2 The second sequence value from the server ping response.
 * @return The calculated starting sequence value.
 */
int32_t sequence_start_from_ping(int32_t s1, int32_t s2);

/**
 * Initializes a sequence of bytes based on the starting value.
 * @param start The starting value for the sequence.
 * @param out_bytes A pointer to the buffer where the initialized byte sequence will be stored. Must have space for at least 2 bytes.
 * @return EO_SUCCESS on success, EO_NULL_PTR if out_bytes is NULL,
 *         EO_INVALID_SEQUENCE_RANGE if the derived range is empty, or
 *         EO_SEQUENCE_OUT_OF_RANGE if seq2 cannot be encoded as a single EO byte.
 */
EoResult sequence_init_bytes(int32_t start, uint8_t *out_bytes);

/**
 * Generates a sequence of bytes for the ping packet based on the starting value.
 * @param start The starting value for the sequence.
 * @param out_bytes A pointer to the buffer where the ping byte sequence will be stored. Must have space for at least 2 bytes.
 * @return EO_SUCCESS on success, EO_NULL_PTR if out_bytes is NULL,
 *         EO_INVALID_SEQUENCE_RANGE if the derived range is empty, or
 *         EO_SEQUENCE_OUT_OF_RANGE if seq2 cannot be encoded as a single EO byte.
 */
EoResult sequence_ping_bytes(int32_t start, uint8_t *out_bytes);

#endif