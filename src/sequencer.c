#include "eolib/sequencer.h"
#include "eolib/data.h"

#if defined(_WIN32)
#include <windows.h>
#include <bcrypt.h>
static uint32_t csprng_uniform(uint32_t upper_bound)
{
    uint32_t val;
    BCryptGenRandom(NULL, (PUCHAR)&val, sizeof(val), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    return val % upper_bound;
}
#elif defined(__EMSCRIPTEN__)
#include <stdlib.h>
static uint32_t csprng_uniform(uint32_t upper_bound)
{
    return (uint32_t)rand() % upper_bound;
}
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include <stdlib.h>
static uint32_t csprng_uniform(uint32_t upper_bound)
{
    return arc4random_uniform(upper_bound);
}
#else
#include <sys/random.h>
static uint32_t csprng_uniform(uint32_t upper_bound)
{
    uint32_t val;
    getrandom(&val, sizeof(val), 0);
    return val % upper_bound;
}
#endif

EoSequencer eo_sequencer_init(int32_t start)
{
    EoSequencer seq;
    seq.start = start;
    seq.counter = 0;
    return seq;
}

EoResult eo_sequencer_next(EoSequencer *sequencer, int32_t *out_value)
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

int32_t eo_generate_sequence_start()
{
    return (int32_t)csprng_uniform((uint32_t)(EO_CHAR_MAX - 9));
}

int32_t eo_sequence_start_from_init(int32_t s1, int32_t s2)
{
    // Protocol formula: start = s1 * 7 + s2 - 13
    // Inverse of the eo_sequence_init_bytes encoding (seq1*7 + seq2 == start + 13).
    return s1 * 7 + s2 - 13;
}

int32_t eo_sequence_start_from_ping(int32_t s1, int32_t s2)
{
    return s1 - s2;
}

EoResult eo_sequence_init_bytes(int32_t start, uint8_t *out_bytes)
{
    if (!out_bytes)
    {
        return EO_NULL_PTR;
    }

    // Encode start as: start + 13 = seq1 * 7 + seq2
    // where seq1 and seq2 are both valid single EO bytes (0..EO_CHAR_MAX-1).
    // Pick a random seq1 in the valid range and derive seq2.
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
    int32_t seq1 = (range > 0) ? (int32_t)(csprng_uniform((uint32_t)range) + (uint32_t)seq1_min) : seq1_min;
    int32_t seq2 = start - seq1 * 7 + 13;

    if (seq2 < 0 || seq2 > (EO_CHAR_MAX - 1))
    {
        return EO_SEQUENCE_OUT_OF_RANGE;
    }

    out_bytes[0] = (uint8_t)seq1;
    out_bytes[1] = (uint8_t)seq2;
    return EO_SUCCESS;
}

EoResult eo_sequence_ping_bytes(int32_t start, uint8_t *out_bytes)
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
    int32_t seq1 = (range > 0) ? (int32_t)(csprng_uniform((uint32_t)range) + (uint32_t)low) : low;
    int32_t seq2 = seq1 - start;

    if (seq2 < 0 || seq2 > (EO_CHAR_MAX - 1))
    {
        return EO_SEQUENCE_OUT_OF_RANGE;
    }

    out_bytes[0] = (uint8_t)seq1;
    out_bytes[1] = (uint8_t)seq2;
    return EO_SUCCESS;
}
