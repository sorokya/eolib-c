#include "sequencer.h"
#include "data.h"
#include "test_utils.h"

#include <stdlib.h>

static void test_sequencer_next_wraps()
{
    Sequencer seq = sequencer_init(100);
    int32_t expected[] = {101, 102, 103, 104, 105, 106, 107, 108, 109, 100, 101};

    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++)
    {
        int32_t value;
        sequencer_next(&seq, &value);
        expect_equal_int32("sequencer_next wrap", value, expected[i]);
    }

    expect_equal_int32("sequencer_next null", sequencer_next(NULL, NULL), EO_NULL_PTR);
}

static void test_generate_sequence_start_range()
{
    srand(1);
    for (int i = 0; i < 50; i++)
    {
        int32_t start = generate_sequence_start();
        expect("generate_sequence_start >= 0", start >= 0);
        expect("generate_sequence_start upper bound", start <= (EO_CHAR_MAX - 10));
    }
}

static void test_sequence_init_bytes_roundtrip()
{
    int32_t starts[] = {-13, 0, 1, 50, 100, 200};
    srand(2);

    for (size_t i = 0; i < sizeof(starts) / sizeof(starts[0]); i++)
    {
        uint8_t bytes[2] = {0};
        EoResult result = sequence_init_bytes(starts[i], bytes);
        expect_equal_int("sequence_init_bytes result", result, EO_SUCCESS);
        if (result != EO_SUCCESS)
        {
            continue;
        }
        expect("sequence_init_bytes seq1 range", bytes[0] <= (EO_CHAR_MAX - 1));
        expect("sequence_init_bytes seq2 range", bytes[1] <= (EO_CHAR_MAX - 1));
        expect_equal_int32("sequence_init_bytes roundtrip",
                           sequence_start_from_init(bytes[0], bytes[1]),
                           starts[i]);
    }
}

static void test_sequence_ping_bytes_roundtrip()
{
    int32_t starts[] = {-10, 0, 5, 100};
    srand(3);

    for (size_t i = 0; i < sizeof(starts) / sizeof(starts[0]); i++)
    {
        uint8_t bytes[2] = {0};
        EoResult result = sequence_ping_bytes(starts[i], bytes);
        expect_equal_int("sequence_ping_bytes result", result, EO_SUCCESS);
        if (result != EO_SUCCESS)
        {
            continue;
        }
        expect("sequence_ping_bytes seq1 range", bytes[0] <= (EO_CHAR_MAX - 1));
        expect("sequence_ping_bytes seq2 range", bytes[1] <= (EO_CHAR_MAX - 1));
        expect_equal_int32("sequence_ping_bytes roundtrip",
                           sequence_start_from_ping(bytes[0], bytes[1]),
                           starts[i]);
    }

    uint8_t bytes[2] = {0};
    expect_equal_int("sequence_ping_bytes invalid high",
                     sequence_ping_bytes(EO_CHAR_MAX, bytes),
                     EO_INVALID_SEQUENCE_RANGE);
}

static const TestCase sequencer_tests[] = {
    {"sequencer_next_wraps", test_sequencer_next_wraps},
    {"generate_sequence_start_range", test_generate_sequence_start_range},
    {"sequence_init_bytes_roundtrip", test_sequence_init_bytes_roundtrip},
    {"sequence_ping_bytes_roundtrip", test_sequence_ping_bytes_roundtrip},
};

int main(int argc, char **argv)
{
    return run_tests(sequencer_tests, sizeof(sequencer_tests) / sizeof(sequencer_tests[0]), argc, argv);
}
