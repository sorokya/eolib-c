#include "test_utils.h"
#include "eolib/data.h"

#include <limits.h>
#include <stdlib.h>
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
        INT32_MAX,
        INT32_MIN,
        -197815216};

    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
    {
        uint8_t bytes[4];
        expect_equal_int("eo_encode_number should succeed", eo_encode_number(values[i], bytes), 0);
        expect_equal_int32("eo_encode_number roundtrip", eo_decode_number(bytes, 4), values[i]);
    }

    expect_equal_int("eo_encode_number should fail with NULL out_bytes", eo_encode_number(5, NULL), EO_NULL_PTR);
    expect_equal_int32("eo_decode_number should treat NULL bytes as zero", eo_decode_number(NULL, 0), 0);
}

static void test_eo_decode_number_negative_wrap()
{
    uint8_t bytes[] = {0xA8, 0xB5, 0x9A, 0x85};

    expect_equal_int32("eo_decode_number should wrap at int32 boundary", eo_decode_number(bytes, 4), INT32_MIN);
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

static void test_eo_writer_number_range_checks()
{
    EoWriter writer = eo_writer_init_with_capacity(0);

    expect_equal_int("eo_writer_add_char negative", eo_writer_add_char(&writer, -1), EO_INVALID_CHAR);
    expect_equal_int("eo_writer_add_char above max", eo_writer_add_char(&writer, EO_CHAR_MAX + 1), EO_INVALID_CHAR);

    expect_equal_int("eo_writer_add_short negative", eo_writer_add_short(&writer, -1), EO_INVALID_SHORT);
    expect_equal_int("eo_writer_add_short above max", eo_writer_add_short(&writer, EO_SHORT_MAX + 1), EO_INVALID_SHORT);

    expect_equal_int("eo_writer_add_three negative", eo_writer_add_three(&writer, -1), EO_INVALID_THREE);
    expect_equal_int("eo_writer_add_three above max", eo_writer_add_three(&writer, EO_THREE_MAX + 1), EO_INVALID_THREE);

    expect_equal_int("eo_writer_add_int negative near zero", eo_writer_add_int(&writer, -1), EO_INVALID_INT);
    expect_equal_int("eo_writer_add_int above wrapped negative max", eo_writer_add_int(&writer, -197815215), EO_INVALID_INT);

    expect_equal_int("eo_writer_add_int minimum wrapped negative", eo_writer_add_int(&writer, -197815216), EO_SUCCESS);
    {
        const uint8_t expected[] = {0xFD, 0xFD, 0xFD, 0xFD};
        expect_equal_bytes("eo_writer_add_int minimum wrapped negative bytes", writer.data, expected, sizeof(expected));
    }

    writer.length = 0;
    expect_equal_int("eo_writer_add_int int32 min", eo_writer_add_int(&writer, INT32_MIN), EO_SUCCESS);
    {
        const uint8_t expected[] = {0xA8, 0xB5, 0x9A, 0x85};
        expect_equal_bytes("eo_writer_add_int int32 min bytes", writer.data, expected, sizeof(expected));
    }

    writer.length = 0;
    expect_equal_int("eo_writer_add_int invalid after valid reset", eo_writer_add_int(&writer, -1), EO_INVALID_INT);
    expect_equal_size("eo_writer_add_* invalid values do not write bytes", writer.length, 0);

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
    {
        char *tmp = malloc(writer.length + 1);
        memcpy(tmp, writer.data, writer.length);
        tmp[writer.length] = '\0';
        expect_equal_str("eo_writer_add_string should not sanitize when mode is disabled", tmp, "he\xFF");
        free(tmp);
    }
    free(writer.data);

    writer = eo_writer_init_with_capacity(3);
    eo_writer_set_string_sanitization_mode(&writer, true);
    expect("eo_writer_set_string_sanitization_mode true", eo_writer_get_string_sanitization_mode(&writer));
    eo_writer_add_string(&writer, "he\xFF");
    {
        char *tmp = malloc(writer.length + 1);
        memcpy(tmp, writer.data, writer.length);
        tmp[writer.length] = '\0';
        expect_equal_str("eo_writer_add_string should sanitize when mode is enabled", tmp, "hey");
        free(tmp);
    }

    free(writer.data);
    writer = eo_writer_init_with_capacity(3);
    eo_writer_set_string_sanitization_mode(&writer, true);
    eo_writer_add_encoded_string(&writer, "he\xFF");
    {
        char *tmp = malloc(writer.length + 1);
        memcpy(tmp, writer.data, writer.length);
        tmp[writer.length] = '\0';
        expect_equal_str("eo_writer_add_encoded_string should sanitize when mode is enabled", tmp, "T:e");
        free(tmp);
    }

    eo_writer_set_string_sanitization_mode(&writer, false);
    expect("eo_writer_set_string_sanitization_mode false", !eo_writer_get_string_sanitization_mode(&writer));

    free(writer.data);
}

static void test_eo_writer_fixed_string()
{
    EoWriter writer = eo_writer_init_with_capacity(5);
    expect_equal_int("eo_writer_add_fixed_string short string", eo_writer_add_fixed_string(&writer, "hi", 5, false), EO_STR_TOO_SHORT);
    expect_equal_int("eo_writer_add_fixed_string long string", eo_writer_add_fixed_string(&writer, "hello world", 5, true), EO_STR_OUT_OF_RANGE);
    expect_equal_int("eo_writer_add_fixed_string exact length", eo_writer_add_fixed_string(&writer, "hello", 5, false), EO_SUCCESS);
    {
        char *tmp = malloc(writer.length + 1);
        memcpy(tmp, writer.data, writer.length);
        tmp[writer.length] = '\0';
        expect_equal_str("eo_writer_add_fixed_string exact length bytes", tmp, "hello");
        free(tmp);
    }

    free(writer.data);
    writer = eo_writer_init_with_capacity(5);
    expect_equal_int("eo_writer_add_fixed_string padded short string", eo_writer_add_fixed_string(&writer, "hi", 5, true), EO_SUCCESS);
    {
        char *tmp = malloc(writer.length + 1);
        memcpy(tmp, writer.data, writer.length);
        tmp[writer.length] = '\0';
        expect_equal_str("eo_writer_add_fixed_string padded short string bytes", tmp, "hi\xFF\xFF\xFF");
        free(tmp);
    }

    free(writer.data);
    writer = eo_writer_init_with_capacity(24);
    expect_equal_int("eo_writer_add_fixed_string padded long string", eo_writer_add_fixed_string(&writer, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@", 24, true), EO_STR_OUT_OF_RANGE);
    expect_equal_int("eo_writer_add_fixed_encoded_string padded short string", eo_writer_add_fixed_encoded_string(&writer, "Aeven", 24, true), EO_SUCCESS);
    const uint8_t expected_encoded[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x31, 0x68, 0x29, 0x68, 0x5E};

    expect_equal_bytes("eo_writer_add_fixed_encoded_string padded short string bytes", writer.data, expected_encoded, 24);

    eo_decode_string(writer.data, 24);
    {
        char *tmp = malloc(writer.length + 1);
        memcpy(tmp, writer.data, writer.length);
        tmp[writer.length] = '\0';
        expect_equal_str("eo_writer_add_fixed_encoded_string padded short string bytes", tmp, "Aeven\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF");
        free(tmp);
    }

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

static void test_eo_string_to_windows_1252()
{
    char *out = NULL;

    /* ASCII strings pass through unchanged */
    expect_equal_int("eo_string_to_windows_1252 ascii", eo_string_to_windows_1252("Hello", &out), EO_SUCCESS);
    expect_equal_str("eo_string_to_windows_1252 ascii value", out, "Hello");
    free(out);
    out = NULL;

    /* UTF-8 é (U+00E9) -> CP1252 0xE9 */
    expect_equal_int("eo_string_to_windows_1252 utf8 latin1", eo_string_to_windows_1252("\xC3\xA9", &out), EO_SUCCESS);
    expect_equal_str("eo_string_to_windows_1252 utf8 latin1 value", out, "\xE9");
    free(out);
    out = NULL;

    /* UTF-8 € (U+20AC) -> CP1252 0x80 */
    expect_equal_int("eo_string_to_windows_1252 utf8 euro", eo_string_to_windows_1252("\xE2\x82\xAC", &out), EO_SUCCESS);
    expect_equal_str("eo_string_to_windows_1252 utf8 euro value", out, "\x80");
    free(out);
    out = NULL;

    /* UTF-8 š (U+0161) -> CP1252 0x9A */
    expect_equal_int("eo_string_to_windows_1252 utf8 0x9A", eo_string_to_windows_1252("\xC5\xA1", &out), EO_SUCCESS);
    expect_equal_str("eo_string_to_windows_1252 utf8 0x9A value", out, "\x9A");
    free(out);
    out = NULL;

    /* Raw CP1252 bytes pass through as-is */
    expect_equal_int("eo_string_to_windows_1252 raw cp1252", eo_string_to_windows_1252("\xE9\xF1", &out), EO_SUCCESS);
    expect_equal_str("eo_string_to_windows_1252 raw cp1252 value", out, "\xE9\xF1");
    free(out);
    out = NULL;

    /* 4-byte UTF-8 emoji (U+1F600) has no CP1252 representation -> '?' */
    expect_equal_int("eo_string_to_windows_1252 emoji", eo_string_to_windows_1252("\xF0\x9F\x98\x80", &out), EO_SUCCESS);
    expect_equal_str("eo_string_to_windows_1252 emoji value", out, "?");
    free(out);
    out = NULL;

    /* Unicode code point with no CP1252 mapping (U+0400, Cyrillic Є) -> '?' */
    expect_equal_int("eo_string_to_windows_1252 no mapping", eo_string_to_windows_1252("\xD0\x80", &out), EO_SUCCESS);
    expect_equal_str("eo_string_to_windows_1252 no mapping value", out, "?");
    free(out);
    out = NULL;

    /* Mixed ASCII and UTF-8 */
    expect_equal_int("eo_string_to_windows_1252 mixed", eo_string_to_windows_1252("caf\xC3\xA9", &out), EO_SUCCESS);
    expect_equal_str("eo_string_to_windows_1252 mixed value", out, "caf\xE9");
    free(out);
    out = NULL;

    /* NULL input returns NULL output */
    expect_equal_int("eo_string_to_windows_1252 null input", eo_string_to_windows_1252(NULL, &out), EO_SUCCESS);
    expect("eo_string_to_windows_1252 null input value", out == NULL);

    /* NULL out_value returns EO_NULL_PTR */
    expect_equal_int("eo_string_to_windows_1252 null out", eo_string_to_windows_1252("test", NULL), EO_NULL_PTR);
}

static const TestCase eo_data_tests[] = {
    {"eo_writer_init_with_capacity", test_eo_writer_init_with_capacity},
    {"eo_writer_ensure_capacity", test_eo_writer_ensure_capacity},
    {"eo_encode_decode_number_roundtrip", test_eo_encode_decode_number_roundtrip},
    {"eo_decode_number_negative_wrap", test_eo_decode_number_negative_wrap},
    {"eo_encode_decode_string_roundtrip", test_eo_encode_decode_string_roundtrip},
    {"eo_encode_string_known_value", test_eo_encode_string_known_value},
    {"eo_decode_string_known_value", test_eo_decode_string_known_value},
    {"eo_writer_add_numbers", test_eo_writer_add_numbers},
    {"eo_writer_number_range_checks", test_eo_writer_number_range_checks},
    {"eo_writer_add_strings_and_bytes", test_eo_writer_add_strings_and_bytes},
    {"eo_writer_string_sanitization_mode", test_eo_writer_string_sanitization_mode},
    {"eo_writer_fixed_string", test_eo_writer_fixed_string},
    {"eo_reader_number_reads", test_eo_reader_number_reads},
    {"eo_reader_string_reads", test_eo_reader_string_reads},
    {"eo_reader_get_bytes", test_eo_reader_get_bytes},
    {"eo_reader_chunked_mode", test_eo_reader_chunked_mode},
    {"eo_string_to_windows_1252", test_eo_string_to_windows_1252},
};

int main(int argc, char **argv)
{
    return run_tests(eo_data_tests, sizeof(eo_data_tests) / sizeof(eo_data_tests[0]), argc, argv);
}
