#include "test_utils.h"
#include "data.h"

#include <limits.h>
#include <string.h>

static void test_eo_writer_init_with_capacity()
{
    size_t capacity = 128;
    EoWriter writer = eo_writer_init_with_capacity(capacity);

    expect("eo_writer_init_with_capacity should allocate data", writer.data != NULL);
    expect("eo_writer_init_with_capacity should initialize length to 0", writer.length == 0);
    expect("eo_writer_init_with_capacity should set capacity correctly", writer.capacity == capacity);

    free(writer.data);
}

static void test_eo_writer_ensure_capacity()
{
    EoWriter writer = eo_writer_init_with_capacity(4);

    // Test ensuring capacity within current capacity
    expect_equal_int("eo_writer_ensure_capacity should succeed when capacity is sufficient", eo_writer_ensure_capacity(&writer, 2), 0);
    expect_equal_size("eo_writer_ensure_capacity should not change capacity when sufficient", writer.capacity, 4);

    writer.length = 4; // Simulate that the writer is full

    // Test ensuring capacity that requires resizing
    expect_equal_int("eo_writer_ensure_capacity should succeed when resizing is needed", eo_writer_ensure_capacity(&writer, 4), 0);
    expect_equal_size("eo_writer_ensure_capacity should increase capacity when needed", writer.capacity, 8);

    // Test ensuring capacity with NULL writer
    expect("eo_writer_ensure_capacity should fail with NULL writer", eo_writer_ensure_capacity(NULL, 1) == EO_NULL_PTR);

    free(writer.data);
}

static EoReader make_reader(const uint8_t *data, size_t length)
{
    EoReader reader;
    reader.chunked_reading_mode = false;
    reader.data = data;
    reader.length = length;
    reader.offset = 0;
    reader.chunk_offset = 0;
    reader.next_chunk_offset = 0;
    return reader;
}

static void test_eo_encode_decode_number_roundtrip()
{
    int32_t values[] = {
        0,
        1,
        EO_CHAR_MAX - 1,
        EO_CHAR_MAX,
        EO_SHORT_MAX - 1,
        EO_SHORT_MAX,
        EO_THREE_MAX - 1,
        1234567,
        INT32_MAX};

    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
    {
        uint8_t bytes[4];
        expect_equal_int("eo_encode_number should succeed", eo_encode_number(values[i], bytes), 0);
        expect_equal_int32("eo_encode_number roundtrip", eo_decode_number(bytes, 4), values[i]);
    }

    expect_equal_int("eo_encode_number should fail with NULL out_bytes", eo_encode_number(5, NULL), EO_NULL_PTR);
    expect_equal_int32("eo_decode_number should treat NULL bytes as zero", eo_decode_number(NULL, 0), 0);
}

static void test_eo_encode_decode_string_roundtrip()
{
    uint8_t buf[] = "Hello, World!";
    uint8_t expected[] = "Hello, World!";
    size_t length = sizeof(buf) - 1;

    eo_encode_string(buf, length);
    eo_decode_string(buf, length);

    expect_equal_bytes("eo_encode_string roundtrip", buf, expected, length);
}

static void test_eo_encode_string_known_value()
{
    uint8_t expected[] = {0x69, 0x36, 0x5E, 0x49};
    uint8_t input[] = "Void";

    eo_encode_string(input, 4);
    expect_equal_bytes("eo_encode_string known value", input, expected, 4);
}

static void test_eo_decode_string_known_value()
{
    uint8_t input[] = {0x69, 0x36, 0x5E, 0x49};
    uint8_t expected[] = "Void";

    eo_decode_string(input, 4);
    expect_equal_bytes("eo_decode_string known value", input, expected, 4);
}

static void test_eo_writer_add_numbers()
{
    EoWriter writer = eo_writer_init_with_capacity(0);
    uint8_t char_bytes[4];
    uint8_t short_bytes[4];
    uint8_t three_bytes[4];
    uint8_t int_bytes[4];

    expect_equal_int("eo_encode_number char", eo_encode_number(123, char_bytes), 0);
    expect_equal_int("eo_encode_number short", eo_encode_number(4567, short_bytes), 0);
    expect_equal_int("eo_encode_number three", eo_encode_number(890123, three_bytes), 0);
    expect_equal_int("eo_encode_number int", eo_encode_number(4567890, int_bytes), 0);

    expect_equal_int("eo_writer_add_byte", eo_writer_add_byte(&writer, 0xAB), 0);
    expect_equal_int("eo_writer_add_char", eo_writer_add_char(&writer, 123), 0);
    expect_equal_int("eo_writer_add_short", eo_writer_add_short(&writer, 4567), 0);
    expect_equal_int("eo_writer_add_three", eo_writer_add_three(&writer, 890123), 0);
    expect_equal_int("eo_writer_add_int", eo_writer_add_int(&writer, 4567890), 0);

    uint8_t expected[1 + 1 + 2 + 3 + 4];
    size_t offset = 0;
    expected[offset++] = 0xAB;
    expected[offset++] = char_bytes[0];
    expected[offset++] = short_bytes[0];
    expected[offset++] = short_bytes[1];
    expected[offset++] = three_bytes[0];
    expected[offset++] = three_bytes[1];
    expected[offset++] = three_bytes[2];
    expected[offset++] = int_bytes[0];
    expected[offset++] = int_bytes[1];
    expected[offset++] = int_bytes[2];
    expected[offset++] = int_bytes[3];

    expect_equal_size("eo_writer_add_numbers length", writer.length, sizeof(expected));
    expect_equal_bytes("eo_writer_add_numbers bytes", writer.data, expected, sizeof(expected));

    free(writer.data);
}

static void test_eo_writer_add_strings_and_bytes()
{
    EoWriter writer = eo_writer_init_with_capacity(0);
    const char *plain = "abc";
    const char *encoded = "Hello";
    const uint8_t raw_bytes[] = {1, 2, 3, 4};

    expect_equal_int("eo_writer_add_string", eo_writer_add_string(&writer, plain), 0);
    expect_equal_int("eo_writer_add_encoded_string", eo_writer_add_encoded_string(&writer, encoded), 0);
    expect_equal_int("eo_writer_add_bytes", eo_writer_add_bytes(&writer, raw_bytes, sizeof(raw_bytes)), 0);

    uint8_t encoded_bytes[6];
    memcpy(encoded_bytes, encoded, 5);
    eo_encode_string(encoded_bytes, 5);

    expect_equal_bytes("eo_writer_add_string bytes", writer.data, (const uint8_t *)plain, 3);
    expect_equal_bytes("eo_writer_add_encoded_string bytes", writer.data + 3, encoded_bytes, 5);
    expect_equal_bytes("eo_writer_add_bytes bytes", writer.data + 8, raw_bytes, sizeof(raw_bytes));

    free(writer.data);
}

static void test_eo_writer_string_sanitization_mode()
{
    EoWriter writer = eo_writer_init_with_capacity(3);
    expect("eo_writer_get_string_sanitization_mode defaults false", !eo_writer_get_string_sanitization_mode(&writer));
    eo_writer_add_string(&writer, "he\xFF");
    expect_equal_str("eo_writer_add_string should not sanitize when mode is disabled", (const char *)writer.data, "he\xFF");
    free(writer.data);

    writer = eo_writer_init_with_capacity(3);
    eo_writer_set_string_sanitization_mode(&writer, true);
    expect("eo_writer_set_string_sanitization_mode true", eo_writer_get_string_sanitization_mode(&writer));
    eo_writer_add_string(&writer, "he\xFF");
    expect_equal_str("eo_writer_add_string should sanitize when mode is enabled", (const char *)writer.data, "hey");

    free(writer.data);
    writer = eo_writer_init_with_capacity(3);
    eo_writer_set_string_sanitization_mode(&writer, true);
    eo_writer_add_encoded_string(&writer, "he\xFF");
    expect_equal_str("eo_writer_add_encoded_string should sanitize when mode is enabled", (const char *)writer.data, "T:e");

    eo_writer_set_string_sanitization_mode(&writer, false);
    expect("eo_writer_set_string_sanitization_mode false", !eo_writer_get_string_sanitization_mode(&writer));

    free(writer.data);
}

static void test_eo_reader_number_reads()
{
    uint8_t char_bytes[4];
    uint8_t short_bytes[4];
    uint8_t three_bytes[4];
    uint8_t int_bytes[4];

    eo_encode_number(12, char_bytes);
    eo_encode_number(3456, short_bytes);
    eo_encode_number(789012, three_bytes);
    eo_encode_number(1234567, int_bytes);

    uint8_t buffer[] = {
        0xAA,
        char_bytes[0],
        short_bytes[0],
        short_bytes[1],
        three_bytes[0],
        three_bytes[1],
        three_bytes[2],
        int_bytes[0],
        int_bytes[1],
        int_bytes[2],
        int_bytes[3]};

    EoReader reader = make_reader(buffer, sizeof(buffer));
    uint8_t byte_value = 0;
    int32_t number_value = 0;

    expect_equal_int("eo_reader_get_byte", eo_reader_get_byte(&reader, &byte_value), 0);
    expect_equal_int("eo_reader_get_byte value", byte_value, 0xAA);

    expect_equal_int("eo_reader_get_char", eo_reader_get_char(&reader, &number_value), 0);
    expect_equal_int32("eo_reader_get_char value", number_value, 12);

    expect_equal_int("eo_reader_get_short", eo_reader_get_short(&reader, &number_value), 0);
    expect_equal_int32("eo_reader_get_short value", number_value, 3456);

    expect_equal_int("eo_reader_get_three", eo_reader_get_three(&reader, &number_value), 0);
    expect_equal_int32("eo_reader_get_three value", number_value, 789012);

    expect_equal_int("eo_reader_get_int", eo_reader_get_int(&reader, &number_value), 0);
    expect_equal_int32("eo_reader_get_int value", number_value, 1234567);
}

static void test_eo_reader_string_reads()
{
    const char *plain = "segment";
    uint8_t encoded_bytes[8];
    memcpy(encoded_bytes, plain, 7);
    eo_encode_string(encoded_bytes, 7);

    uint8_t buffer[] = {'s', 'e', 'g', 'm', 'e', 'n', 't'};
    EoReader reader = make_reader(buffer, sizeof(buffer));
    char *out = NULL;

    expect_equal_int("eo_reader_get_string", eo_reader_get_string(&reader, &out), 0);
    expect_equal_str("eo_reader_get_string value", out, plain);
    expect_equal_size("eo_reader_get_string offset", reader.offset, sizeof(buffer));
    free(out);

    uint8_t encoded_buffer[7];
    memcpy(encoded_buffer, encoded_bytes, 7);
    reader = make_reader(encoded_buffer, 7);
    out = NULL;
    expect_equal_int("eo_reader_get_encoded_string", eo_reader_get_encoded_string(&reader, &out), 0);
    expect_equal_str("eo_reader_get_encoded_string value", out, plain);
    free(out);

    reader = make_reader(buffer, sizeof(buffer));
    out = NULL;
    expect_equal_int("eo_reader_get_fixed_string", eo_reader_get_fixed_string(&reader, 4, &out), 0);
    expect_equal_str("eo_reader_get_fixed_string value", out, "segm");
    free(out);

    memcpy(encoded_buffer, encoded_bytes, 7);
    reader = make_reader(encoded_buffer, 7);
    out = NULL;
    expect_equal_int("eo_reader_get_fixed_encoded_string", eo_reader_get_fixed_encoded_string(&reader, 7, &out), 0);
    expect_equal_str("eo_reader_get_fixed_encoded_string value", out, plain);
    free(out);

    reader = make_reader(buffer, sizeof(buffer));
    out = NULL;
    reader.offset = reader.length;
    expect_equal_int("eo_reader_get_string empty", eo_reader_get_string(&reader, &out), 0);
    expect("eo_reader_get_string empty value", out != NULL && strcmp(out, "") == 0);
    free(out);
}

static void test_eo_reader_get_bytes()
{
    uint8_t buffer[] = {9, 8, 7, 6, 5};
    EoReader reader = make_reader(buffer, sizeof(buffer));
    uint8_t *out = NULL;

    expect_equal_int("eo_reader_get_bytes", eo_reader_get_bytes(&reader, 3, &out), 0);
    expect_equal_bytes("eo_reader_get_bytes value", out, buffer, 3);
    expect_equal_size("eo_reader_get_bytes offset", reader.offset, 3);
    free(out);

    out = NULL;
    expect_equal_int("eo_reader_get_bytes overrun", eo_reader_get_bytes(&reader, 4, &out), EO_BUFFER_UNDERRUN);
}

static void test_eo_reader_chunked_mode()
{
    uint8_t buffer[] = {1, 2, 0xFF, 3, 4, 0xFF, 5};
    EoReader reader = make_reader(buffer, sizeof(buffer));

    eo_reader_set_chunked_reading_mode(&reader, true);
    expect_equal_size("eo_reader_remaining first chunk", eo_reader_remaining(&reader), 2);

    uint8_t value = 0;
    eo_reader_get_byte(&reader, &value);
    eo_reader_get_byte(&reader, &value);
    expect_equal_size("eo_reader_remaining after chunk read", eo_reader_remaining(&reader), 0);

    expect_equal_int("eo_reader_next_chunk", eo_reader_next_chunk(&reader), 0);
    expect_equal_size("eo_reader_remaining second chunk", eo_reader_remaining(&reader), 2);
    eo_reader_get_byte(&reader, &value);
    expect_equal_int("eo_reader_get_byte second chunk", value, 3);

    eo_reader_get_byte(&reader, &value);
    expect_equal_int("eo_reader_next_chunk to final", eo_reader_next_chunk(&reader), 0);
    expect_equal_size("eo_reader_remaining final chunk", eo_reader_remaining(&reader), 1);
    eo_reader_get_byte(&reader, &value);
    expect_equal_int("eo_reader_get_byte final", value, 5);
}

static const TestCase eo_data_tests[] = {
    {"eo_writer_init_with_capacity", test_eo_writer_init_with_capacity},
    {"eo_writer_ensure_capacity", test_eo_writer_ensure_capacity},
    {"eo_encode_decode_number_roundtrip", test_eo_encode_decode_number_roundtrip},
    {"eo_encode_decode_string_roundtrip", test_eo_encode_decode_string_roundtrip},
    {"eo_encode_string_known_value", test_eo_encode_string_known_value},
    {"eo_decode_string_known_value", test_eo_decode_string_known_value},
    {"eo_writer_add_numbers", test_eo_writer_add_numbers},
    {"eo_writer_add_strings_and_bytes", test_eo_writer_add_strings_and_bytes},
    {"eo_writer_string_sanitization_mode", test_eo_writer_string_sanitization_mode},
    {"eo_reader_number_reads", test_eo_reader_number_reads},
    {"eo_reader_string_reads", test_eo_reader_string_reads},
    {"eo_reader_get_bytes", test_eo_reader_get_bytes},
    {"eo_reader_chunked_mode", test_eo_reader_chunked_mode},
};

int main(int argc, char **argv)
{
    return run_tests(eo_data_tests, sizeof(eo_data_tests) / sizeof(eo_data_tests[0]), argc, argv);
}
