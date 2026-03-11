#include "sequencer.h"
#include "eo_data.h"

#include <stdlib.h>

Sequencer sequencer_init(int32_t start)
{
    Sequencer seq;
    seq.start = start;
    seq.counter = 0;
    return seq;
}

int32_t sequencer_next(Sequencer *sequencer)
{
    if (!sequencer)
    {
        return -1;
    }

    sequencer->counter = (sequencer->counter + 1) % 10;
    return sequencer->start + sequencer->counter;
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

int sequence_init_bytes(int32_t start, uint8_t *out_bytes)
{
    if (!out_bytes)
    {
        return -1;
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
        return -1;
    }

    int32_t range = seq1_max - seq1_min + 1;
    int32_t seq1 = (range > 0) ? (rand() % range + seq1_min) : seq1_min;
    int32_t seq2 = start - seq1 * 7 + 13;

    if (seq2 < 0 || seq2 > (EO_CHAR_MAX - 1))
    {
        return -1;
    }

    out_bytes[0] = (uint8_t)seq1;
    out_bytes[1] = (uint8_t)seq2;
    return 0;
}

int sequence_ping_bytes(int32_t start, uint8_t *out_bytes)
{
    if (!out_bytes)
    {
        return -1;
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
        return -1;
    }

    int32_t range = high - low + 1;
    int32_t seq1 = (range > 0) ? (rand() % range + low) : low;
    int32_t seq2 = seq1 - start;

    if (seq2 < 0 || seq2 > (EO_CHAR_MAX - 1))
    {
        return -1;
    }

    out_bytes[0] = (uint8_t)seq1;
    out_bytes[1] = (uint8_t)seq2;
    return 0;
}
