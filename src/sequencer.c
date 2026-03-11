#include "sequencer.h"
#include "data.h"

#include <stdlib.h>

Sequencer sequencer_init(int32_t start)
{
    Sequencer seq;
    seq.start = start;
    seq.counter = 0;
    return seq;
}

EoResult sequencer_next(Sequencer *sequencer, int32_t *out_value)
{
    if (!sequencer)
    {
        return EO_NULL_PTR;
    }

    sequencer->counter = (sequencer->counter + 1) % 10;
    if (out_value)
    {
        *out_value = sequencer->start + sequencer->counter;
    }

    return EO_SUCCESS;
}

int32_t generate_sequence_start()
{
    return rand() % (EO_CHAR_MAX - 9);
}

int32_t sequence_start_from_init(int32_t s1, int32_t s2)
{
    return s1 * 7 + s2 - 13;
}

int32_t sequence_start_from_ping(int32_t s1, int32_t s2)
{
    return s1 - s2;
}

EoResult sequence_init_bytes(int32_t start, uint8_t *out_bytes)
{
    if (!out_bytes)
    {
        return EO_NULL_PTR;
    }

    int32_t seq1_min = (start - (EO_CHAR_MAX - 1) + 13 + 6) / 7;
    if (seq1_min < 0)
    {
        seq1_min = 0;
    }

    int32_t seq1_max = (start + 13) / 7;
    if (seq1_max > (EO_CHAR_MAX - 1))
    {
        seq1_max = EO_CHAR_MAX - 1;
    }

    if (seq1_max < seq1_min)
    {
        return EO_INVALID_SEQUENCE_RANGE;
    }

    int32_t range = seq1_max - seq1_min + 1;
    int32_t seq1 = (range > 0) ? (rand() % range + seq1_min) : seq1_min;
    int32_t seq2 = start - seq1 * 7 + 13;

    if (seq2 < 0 || seq2 > (EO_CHAR_MAX - 1))
    {
        return EO_SEQUENCE_OUT_OF_RANGE;
    }

    out_bytes[0] = (uint8_t)seq1;
    out_bytes[1] = (uint8_t)seq2;
    return EO_SUCCESS;
}

EoResult sequence_ping_bytes(int32_t start, uint8_t *out_bytes)
{
    if (!out_bytes)
    {
        return EO_NULL_PTR;
    }

    int32_t low = start;
    if (low < 0)
    {
        low = 0;
    }

    int32_t high = start + (EO_CHAR_MAX - 1);
    if (high > (EO_CHAR_MAX - 1))
    {
        high = EO_CHAR_MAX - 1;
    }

    if (high < low)
    {
        return EO_INVALID_SEQUENCE_RANGE;
    }

    int32_t range = high - low + 1;
    int32_t seq1 = (range > 0) ? (rand() % range + low) : low;
    int32_t seq2 = seq1 - start;

    if (seq2 < 0 || seq2 > (EO_CHAR_MAX - 1))
    {
        return EO_SEQUENCE_OUT_OF_RANGE;
    }

    out_bytes[0] = (uint8_t)seq1;
    out_bytes[1] = (uint8_t)seq2;
    return EO_SUCCESS;
}
