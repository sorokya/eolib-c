#ifndef EOLIB_SEQUENCER_H
#define EOLIB_SEQUENCER_H

#include <stdint.h>

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
 * Gets the next value in the sequence and advances the counter.
 * @param sequencer The sequencer to get the next value from.
 * @return The next value in the sequence or -1 if the sequencer is NULL.
 */
int32_t sequencer_next(Sequencer *sequencer);

/**
 * Generates a random starting value for a sequencer.
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
 * @return 0 on success, -1 on failure.
 */
int sequence_init_bytes(int32_t start, uint8_t *out_bytes);

/**
 * Generates a sequence of bytes for the ping packet based on the starting value.
 * @param start The starting value for the sequence.
 * @param out_bytes A pointer to the buffer where the ping byte sequence will be stored. Must have space for at least 2 bytes.
 * @return 0 on success, -1 on failure.
 */
int sequence_ping_bytes(int32_t start, uint8_t *out_bytes);

#endif