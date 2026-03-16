#include "eolib/data.h"
#include "eolib/result.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(_WIN32)
#define strdup _strdup
#endif

static size_t eo_reader_next_break_index(const EoReader *reader);

/*
 * Unicode code points for Windows-1252 bytes 0x80-0x9F.
 * Index 0 maps to byte 0x80, index 31 maps to byte 0x9F.
 * A value of 0 means the byte is undefined in Windows-1252.
 */
static const uint32_t cp1252_unicode_extras[32] = {
    0x20AC,
    0,
    0x201A,
    0x0192,
    0x201E,
    0x2026,
    0x2020,
    0x2021,
    0x02C6,
    0x2030,
    0x0160,
    0x2039,
    0x0152,
    0,
    0x017D,
    0,
    0,
    0x2018,
    0x2019,
    0x201C,
    0x201D,
    0x2022,
    0x2013,
    0x2014,
    0x02DC,
    0x2122,
    0x0161,
    0x203A,
    0x0153,
    0,
    0x017E,
    0x0178,
};

static uint8_t unicode_to_cp1252(uint32_t codepoint)
{
    if (codepoint <= 0x7F)
        return (uint8_t)codepoint;

    if (codepoint >= 0xA0 && codepoint <= 0xFF)
        return (uint8_t)codepoint;

    for (int i = 0; i < 32; ++i)
    {
        if (cp1252_unicode_extras[i] == codepoint)
            return (uint8_t)(0x80 + i);
    }

    return '?';
}

/*
 * Converts a UTF-8 string to Windows-1252 in-place into `output`.
 * `output` must be at least `input_len + 1` bytes.
 * Returns the number of bytes written (excluding the null terminator).
 * Valid UTF-8 multi-byte sequences are decoded and mapped to Windows-1252.
 * Characters with no Windows-1252 representation are replaced with '?'.
 * Bytes that do not form a valid UTF-8 sequence are passed through as-is.
 */
static size_t normalize_to_cp1252(const char *input, char *output, size_t input_len)
{
    size_t out_pos = 0;
    size_t i = 0;

    while (i < input_len)
    {
        uint8_t b0 = (uint8_t)input[i];

        if (b0 < 0x80)
        {
            output[out_pos++] = (char)b0;
            i++;
        }
        else if (b0 >= 0xC2 && b0 <= 0xDF && i + 1 < input_len)
        {
            uint8_t b1 = (uint8_t)input[i + 1];
            if ((b1 & 0xC0) == 0x80)
            {
                uint32_t cp = ((uint32_t)(b0 & 0x1F) << 6) | (b1 & 0x3F);
                output[out_pos++] = (char)unicode_to_cp1252(cp);
                i += 2;
            }
            else
            {
                output[out_pos++] = (char)b0;
                i++;
            }
        }
        else if (b0 >= 0xE0 && b0 <= 0xEF && i + 2 < input_len)
        {
            uint8_t b1 = (uint8_t)input[i + 1];
            uint8_t b2 = (uint8_t)input[i + 2];
            if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80)
            {
                uint32_t cp = ((uint32_t)(b0 & 0x0F) << 12) | ((uint32_t)(b1 & 0x3F) << 6) | (b2 & 0x3F);
                output[out_pos++] = (char)unicode_to_cp1252(cp);
                i += 3;
            }
            else
            {
                output[out_pos++] = (char)b0;
                i++;
            }
        }
        else if (b0 >= 0xF0 && b0 <= 0xF4 && i + 3 < input_len)
        {
            uint8_t b1 = (uint8_t)input[i + 1];
            uint8_t b2 = (uint8_t)input[i + 2];
            uint8_t b3 = (uint8_t)input[i + 3];
            if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80)
            {
                /* Code points above U+FFFF have no Windows-1252 representation. */
                output[out_pos++] = '?';
                i += 4;
            }
            else
            {
                output[out_pos++] = (char)b0;
                i++;
            }
        }
        else
        {
            /* Continuation byte in isolation, or other non-UTF-8 byte: treat as raw CP1252. */
            output[out_pos++] = (char)b0;
            i++;
        }
    }

    output[out_pos] = '\0';
    return out_pos;
}

EoResult eo_string_to_windows_1252(const char *value, char **out_value)
{
    if (!out_value)
        return EO_NULL_PTR;

    if (!value)
    {
        *out_value = NULL;
        return EO_SUCCESS;
    }

    size_t in_len = strlen(value);
    char *out = (char *)malloc(in_len + 1);
    if (!out)
        return EO_ALLOC_FAILED;

    normalize_to_cp1252(value, out, in_len);
    *out_value = out;
    return EO_SUCCESS;
}

const char *eo_result_string(EoResult result)
{
    switch (result)
    {
    case EO_SUCCESS:
        return "EO_SUCCESS: operation completed successfully";
    case EO_NULL_PTR:
        return "EO_NULL_PTR: a required pointer argument was NULL";
    case EO_OVERFLOW:
        return "EO_OVERFLOW: internal size calculation would overflow";
    case EO_ALLOC_FAILED:
        return "EO_ALLOC_FAILED: memory allocation failed";
    case EO_NUMBER_TOO_LARGE:
        return "EO_NUMBER_TOO_LARGE: numeric value exceeds EO encoding range";
    case EO_BUFFER_UNDERRUN:
        return "EO_BUFFER_UNDERRUN: not enough bytes available for read";
    case EO_CHUNKED_MODE_DISABLED:
        return "EO_CHUNKED_MODE_DISABLED: chunked reading mode is not active";
    case EO_INVALID_SEQUENCE_RANGE:
        return "EO_INVALID_SEQUENCE_RANGE: calculated sequence range is empty";
    case EO_SEQUENCE_OUT_OF_RANGE:
        return "EO_SEQUENCE_OUT_OF_RANGE: sequence value out of encodable range";
    case EO_INVALID_DATA:
        return "EO_INVALID_DATA: data is structurally invalid";
    case EO_STR_OUT_OF_RANGE:
        return "EO_STR_OUT_OF_RANGE: string exceeds the allowed fixed length";
    case EO_STR_TOO_SHORT:
        return "EO_STR_TOO_SHORT: string is shorter than the required fixed length";
    case EO_INVALID_CHAR:
        return "EO_INVALID_CHAR: character value is outside the valid range";
    case EO_INVALID_SHORT:
        return "EO_INVALID_SHORT: short integer value is outside the valid range";
    case EO_INVALID_THREE:
        return "EO_INVALID_THREE: three-byte integer value is outside the valid range";
    case EO_INVALID_INT:
        return "EO_INVALID_INT: integer value is outside the valid range";
    default:
        return "unknown EoResult value";
    }
}

EoWriter eo_writer_init(void)
{
    return eo_writer_init_with_capacity(128);
}

EoWriter eo_writer_init_with_capacity(size_t capacity)
{
    EoWriter writer;
    writer.string_sanitization_mode = false;
    writer.length = 0;
    writer.capacity = capacity;
    writer.data = capacity > 0 ? (uint8_t *)malloc(capacity) : NULL;
    return writer;
}

void eo_writer_free(EoWriter *writer)
{
    if (!writer)
    {
        return;
    }
    free(writer->data);
    writer->data = NULL;
    writer->length = 0;
    writer->capacity = 0;
}

EoResult eo_writer_ensure_capacity(EoWriter *writer, size_t additional)
{
    if (!writer)
    {
        return EO_NULL_PTR;
    }

    if (SIZE_MAX - writer->length < additional)
    {
        return EO_OVERFLOW;
    }

    size_t required = writer->length + additional;
    if (writer->capacity >= required)
    {
        return EO_SUCCESS;
    }

    size_t new_capacity = required == 0 ? 1 : required;
    uint8_t *new_data = (uint8_t *)realloc(writer->data, new_capacity);
    if (!new_data)
    {
        return EO_ALLOC_FAILED;
    }

    writer->data = new_data;
    writer->capacity = new_capacity;
    return EO_SUCCESS;
}

EoResult eo_encode_number(int32_t number, uint8_t out_bytes[4])
{
    if (!out_bytes)
    {
        return EO_NULL_PTR;
    }

    out_bytes[0] = EO_NUMBER_PADDING;
    out_bytes[1] = EO_NUMBER_PADDING;
    out_bytes[2] = EO_NUMBER_PADDING;
    out_bytes[3] = EO_NUMBER_PADDING;

    int64_t value = number;
    if (value < 0)
    {
        value = -value + (int64_t)INT32_MAX;
    }

    int64_t original = value;
    if (original >= (int64_t)EO_INT_MAX)
    {
        return EO_NUMBER_TOO_LARGE;
    }

    if (original >= (int64_t)EO_THREE_MAX)
    {
        out_bytes[3] = (uint8_t)(value / (int64_t)EO_THREE_MAX) + 1;
        value %= (int64_t)EO_THREE_MAX;
    }

    if (original >= (int64_t)EO_SHORT_MAX)
    {
        out_bytes[2] = (uint8_t)(value / (int64_t)EO_SHORT_MAX) + 1;
        value %= (int64_t)EO_SHORT_MAX;
    }

    if (original >= (int64_t)EO_CHAR_MAX)
    {
        out_bytes[1] = (uint8_t)(value / (int64_t)EO_CHAR_MAX) + 1;
        value %= (int64_t)EO_CHAR_MAX;
    }

    out_bytes[0] = (uint8_t)value + 1;
    return EO_SUCCESS;
}

int32_t eo_decode_number(const uint8_t *bytes, size_t length)
{
    uint8_t data[4] = {EO_NUMBER_PADDING, EO_NUMBER_PADDING, EO_NUMBER_PADDING, EO_NUMBER_PADDING};

    for (size_t i = 0; i < 4; ++i)
    {
        if (bytes && length > i && bytes[i] != 0)
        {
            data[i] = bytes[i];
        }

        if (data[i] == EO_NUMBER_PADDING)
        {
            data[i] = 1;
        }

        data[i] -= 1;
    }

    uint32_t result = (uint32_t)data[3] * (uint32_t)EO_THREE_MAX;
    result += (uint32_t)data[2] * (uint32_t)EO_SHORT_MAX;
    result += (uint32_t)data[1] * (uint32_t)EO_CHAR_MAX;
    result += data[0];
    return (int32_t)result;
}

void eo_decode_string(uint8_t *buf, size_t length)
{
    if (!buf || length == 0)
    {
        return;
    }

    for (size_t i = 0; i < length / 2; ++i)
    {
        uint8_t tmp = buf[i];
        buf[i] = buf[length - 1 - i];
        buf[length - 1 - i] = tmp;
    }

    size_t parity = (length + 1) % 2;
    for (size_t i = 0; i < length; ++i)
    {
        uint8_t ch = buf[i];
        int odd = ((i % 2) != parity);

        if (odd && ch >= 34 && ch <= 125)
        {
            buf[i] = (uint8_t)(125 - ch + 34);
        }
        else if (!odd && ch >= 34 && ch <= 79)
        {
            buf[i] = (uint8_t)(79 - ch + 34);
        }
        else if (!odd && ch >= 80 && ch <= 125)
        {
            buf[i] = (uint8_t)(125 - ch + 80);
        }
    }
}

void eo_encode_string(uint8_t *buf, size_t length)
{
    if (!buf || length == 0)
    {
        return;
    }

    size_t parity = (length + 1) % 2;
    for (size_t i = 0; i < length; ++i)
    {
        uint8_t ch = buf[i];
        int odd = ((i % 2) != parity);

        if (odd && ch >= 34 && ch <= 125)
        {
            buf[i] = (uint8_t)(125 - ch + 34);
        }
        else if (!odd && ch >= 34 && ch <= 79)
        {
            buf[i] = (uint8_t)(79 - ch + 34);
        }
        else if (!odd && ch >= 80 && ch <= 125)
        {
            buf[i] = (uint8_t)(125 - ch + 80);
        }
    }

    for (size_t i = 0; i < length / 2; ++i)
    {
        uint8_t tmp = buf[i];
        buf[i] = buf[length - 1 - i];
        buf[length - 1 - i] = tmp;
    }
}

bool eo_writer_get_string_sanitization_mode(const EoWriter *writer)
{
    return writer ? writer->string_sanitization_mode : false;
}

void eo_writer_set_string_sanitization_mode(EoWriter *writer, bool enabled)
{
    if (writer)
    {
        writer->string_sanitization_mode = enabled;
    }
}

EoResult eo_writer_add_byte(EoWriter *writer, uint8_t value)
{
    if (!writer)
    {
        return EO_NULL_PTR;
    }

    EoResult result;
    if ((result = eo_writer_ensure_capacity(writer, 1)) != EO_SUCCESS)
    {
        return result;
    }

    writer->data[writer->length++] = value;
    return EO_SUCCESS;
}

EoResult eo_writer_add_char(EoWriter *writer, int32_t value)
{
    if (!writer)
    {
        return EO_NULL_PTR;
    }

    if (value < 0 || value > EO_CHAR_MAX)
    {
        return EO_INVALID_CHAR;
    }

    EoResult result;
    if ((result = eo_writer_ensure_capacity(writer, 1)) != EO_SUCCESS)
    {
        return result;
    }

    uint8_t bytes[4];
    if ((result = eo_encode_number(value, bytes)) != EO_SUCCESS)
    {
        return result;
    }

    writer->data[writer->length++] = bytes[0];

    return EO_SUCCESS;
}

EoResult eo_writer_add_short(EoWriter *writer, int32_t value)
{
    if (!writer)
    {
        return EO_NULL_PTR;
    }

    if (value < 0 || value > EO_SHORT_MAX)
    {
        return EO_INVALID_SHORT;
    }

    EoResult result;
    if ((result = eo_writer_ensure_capacity(writer, 2)) != EO_SUCCESS)
    {
        return result;
    }

    uint8_t bytes[4];
    if ((result = eo_encode_number(value, bytes)) != EO_SUCCESS)
    {
        return result;
    }

    writer->data[writer->length++] = bytes[0];
    writer->data[writer->length++] = bytes[1];

    return EO_SUCCESS;
}

EoResult eo_writer_add_three(EoWriter *writer, int32_t value)
{
    if (!writer)
    {
        return EO_NULL_PTR;
    }

    if (value < 0 || value > EO_THREE_MAX)
    {
        return EO_INVALID_THREE;
    }

    EoResult result;
    if ((result = eo_writer_ensure_capacity(writer, 3)) != EO_SUCCESS)
    {
        return result;
    }

    uint8_t bytes[4];
    if ((result = eo_encode_number(value, bytes)) != EO_SUCCESS)
    {
        return result;
    }

    writer->data[writer->length++] = bytes[0];
    writer->data[writer->length++] = bytes[1];
    writer->data[writer->length++] = bytes[2];
    return EO_SUCCESS;
}

EoResult eo_writer_add_int(EoWriter *writer, int32_t value)
{
    if (!writer)
    {
        return EO_NULL_PTR;
    }

    if (value < 0)
    {
        return EO_INVALID_INT;
    }

    EoResult result;
    if ((result = eo_writer_ensure_capacity(writer, 4)) != EO_SUCCESS)
    {
        return result;
    }

    uint8_t bytes[4];
    if ((result = eo_encode_number(value, bytes)) != EO_SUCCESS)
    {
        return result;
    }

    writer->data[writer->length++] = bytes[0];
    writer->data[writer->length++] = bytes[1];
    writer->data[writer->length++] = bytes[2];
    writer->data[writer->length++] = bytes[3];

    return EO_SUCCESS;
}

EoResult eo_writer_add_string(EoWriter *writer, const char *value)
{
    if (!writer)
    {
        return EO_NULL_PTR;
    }

    // Ignore NULL strings, treat them as empty strings
    if (!value)
    {
        return EO_SUCCESS;
    }

    size_t in_len = strlen(value);
    char *normalized = (char *)malloc(in_len + 1);
    if (!normalized)
    {
        return EO_ALLOC_FAILED;
    }
    size_t length = normalize_to_cp1252(value, normalized, in_len);

    EoResult result;
    if ((result = eo_writer_ensure_capacity(writer, length)) != EO_SUCCESS)
    {
        free(normalized);
        return result;
    }

    if (writer->string_sanitization_mode)
    {
        for (size_t k = 0; k < length; ++k)
        {
            writer->data[writer->length++] =
                (normalized[k] == (char)EO_BREAK_BYTE) ? 0x79 : (uint8_t)normalized[k];
        }
    }
    else
    {
        memcpy(writer->data + writer->length, normalized, length);
        writer->length += length;
    }

    free(normalized);
    return EO_SUCCESS;
}

EoResult eo_writer_add_encoded_string(EoWriter *writer, const char *value)
{
    if (!writer)
    {
        return EO_NULL_PTR;
    }

    // Ignore NULL strings, treat them as empty strings
    if (!value)
    {
        return EO_SUCCESS;
    }

    size_t in_len = strlen(value);
    char *normalized = (char *)malloc(in_len + 1);
    if (!normalized)
    {
        return EO_ALLOC_FAILED;
    }
    size_t length = normalize_to_cp1252(value, normalized, in_len);

    EoResult result;
    if ((result = eo_writer_ensure_capacity(writer, length)) != EO_SUCCESS)
    {
        free(normalized);
        return result;
    }

    uint8_t *encoded = (uint8_t *)malloc(length);
    if (!encoded)
    {
        free(normalized);
        return EO_ALLOC_FAILED;
    }

    if (writer->string_sanitization_mode)
    {
        for (size_t k = 0; k < length; ++k)
        {
            encoded[k] = (normalized[k] == (char)EO_BREAK_BYTE) ? 0x79 : (uint8_t)normalized[k];
        }
    }
    else
    {
        memcpy(encoded, normalized, length);
    }

    free(normalized);

    eo_encode_string(encoded, length);
    memcpy(writer->data + writer->length, encoded, length);
    writer->length += length;

    free(encoded);

    return EO_SUCCESS;
}

EoResult eo_writer_add_fixed_string(EoWriter *writer, const char *value, size_t length, bool padded)
{
    if (!writer)
    {
        return EO_NULL_PTR;
    }

    // Ignore NULL strings, treat them as empty strings
    if (!value)
    {
        return EO_SUCCESS;
    }

    size_t in_len = strlen(value);
    char *normalized = (char *)malloc(in_len + 1);
    if (!normalized)
    {
        return EO_ALLOC_FAILED;
    }
    size_t value_len = normalize_to_cp1252(value, normalized, in_len);

    if (padded && value_len > length)
    {
        free(normalized);
        return EO_STR_OUT_OF_RANGE;
    }

    if (!padded && value_len != length)
    {
        free(normalized);
        return EO_STR_TOO_SHORT;
    }

    EoResult result;
    if ((result = eo_writer_ensure_capacity(writer, length)) != EO_SUCCESS)
    {
        free(normalized);
        return result;
    }

    if (writer->string_sanitization_mode)
    {
        for (size_t k = 0; k < value_len; ++k)
        {
            writer->data[writer->length++] =
                (normalized[k] == (char)EO_BREAK_BYTE) ? 0x79 : (uint8_t)normalized[k];
        }
    }
    else
    {
        size_t to_copy = padded ? value_len : length;
        memcpy(writer->data + writer->length, normalized, to_copy);
        writer->length += to_copy;
    }

    free(normalized);

    if (padded)
    {
        size_t remaining = length > value_len ? length - value_len : 0;
        for (size_t i = 0; i < remaining; ++i)
        {
            writer->data[writer->length++] = EO_BREAK_BYTE;
        }
    }

    return EO_SUCCESS;
}

EoResult eo_writer_add_fixed_encoded_string(EoWriter *writer, const char *value, size_t length, bool padded)
{
    if (!writer)
    {
        return EO_NULL_PTR;
    }

    // Ignore NULL strings, treat them as empty strings
    if (!value)
    {
        return EO_SUCCESS;
    }

    size_t in_len = strlen(value);
    char *normalized = (char *)malloc(in_len + 1);
    if (!normalized)
    {
        return EO_ALLOC_FAILED;
    }
    size_t value_len = normalize_to_cp1252(value, normalized, in_len);

    if (padded && value_len > length)
    {
        free(normalized);
        return EO_STR_OUT_OF_RANGE;
    }

    if (!padded && value_len != length)
    {
        free(normalized);
        return EO_STR_TOO_SHORT;
    }

    EoResult result;
    if ((result = eo_writer_ensure_capacity(writer, length)) != EO_SUCCESS)
    {
        free(normalized);
        return result;
    }

    uint8_t *encoded = (uint8_t *)malloc(length);
    if (!encoded)
    {
        free(normalized);
        return EO_ALLOC_FAILED;
    }

    if (writer->string_sanitization_mode)
    {
        for (size_t k = 0; k < value_len; ++k)
        {
            encoded[k] = (normalized[k] == (char)EO_BREAK_BYTE) ? 0x79 : (uint8_t)normalized[k];
        }
    }
    else
    {
        size_t to_copy = padded ? value_len : length;
        memcpy(encoded, normalized, to_copy);
    }

    free(normalized);

    if (padded)
    {
        size_t remaining = length > value_len ? length - value_len : 0;
        for (size_t i = 0; i < remaining; ++i)
        {
            encoded[value_len + i] = EO_BREAK_BYTE;
        }
    }

    eo_encode_string(encoded, length);

    memcpy(writer->data + writer->length, encoded, length);
    writer->length += length;

    free(encoded);

    return EO_SUCCESS;
}

EoResult eo_writer_add_bytes(EoWriter *writer, const uint8_t *data, size_t length)
{
    if (!writer || !data)
    {
        return EO_NULL_PTR;
    }

    EoResult result;
    if ((result = eo_writer_ensure_capacity(writer, length)) != EO_SUCCESS)
    {
        return result;
    }

    memcpy(writer->data + writer->length, data, length);
    writer->length += length;

    return EO_SUCCESS;
}

EoReader eo_reader_init(const uint8_t *data, size_t length)
{
    EoReader reader;
    reader.data = data;
    reader.length = length;
    reader.offset = 0;
    reader.chunked_reading_mode = false;
    reader.chunk_offset = 0;
    reader.next_chunk_offset = 0;
    return reader;
}

bool eo_reader_get_chunked_reading_mode(const EoReader *reader)
{
    return reader ? reader->chunked_reading_mode : false;
}

void eo_reader_set_chunked_reading_mode(EoReader *reader, bool enabled)
{
    if (reader)
    {
        reader->chunked_reading_mode = enabled;
        if (!reader->next_chunk_offset)
        {
            reader->next_chunk_offset = eo_reader_next_break_index(reader);
        }
    }
}

size_t eo_reader_remaining(const EoReader *reader)
{
    if (!reader)
    {
        return 0;
    }

    if (reader->offset >= reader->length)
    {
        return 0;
    }

    if (reader->chunked_reading_mode)
    {
        size_t next_break = reader->next_chunk_offset;
        if (next_break < reader->offset)
        {
            return 0;
        }

        return next_break - reader->offset;
    }

    return reader->length - reader->offset;
}

EoResult eo_reader_next_chunk(EoReader *reader)
{
    if (!reader)
    {
        return EO_NULL_PTR;
    }

    if (!reader->chunked_reading_mode)
    {
        return EO_CHUNKED_MODE_DISABLED;
    }

    reader->offset = reader->next_chunk_offset;
    if (reader->offset < reader->length)
    {
        reader->offset += 1; // Skip the EO_BREAK_BYTE break byte
    }

    reader->chunk_offset = reader->offset;
    reader->next_chunk_offset = eo_reader_next_break_index(reader);

    return EO_SUCCESS;
}

static size_t eo_reader_next_break_index(const EoReader *reader)
{
    if (!reader || !reader->chunked_reading_mode)
    {
        return reader ? reader->length : 0;
    }

    for (size_t i = reader->chunk_offset; i < reader->length; ++i)
    {
        if (reader->data[i] == (uint8_t)EO_BREAK_BYTE)
        {
            return i;
        }
    }

    return reader->length;
}

EoResult eo_reader_get_byte(EoReader *reader, uint8_t *out_value)
{
    if (!reader || !out_value)
    {
        return EO_NULL_PTR;
    }

    if (reader->offset > reader->length || reader->length - reader->offset < 1)
    {
        if (reader->chunked_reading_mode)
        {
            *out_value = 0;
            return EO_SUCCESS;
        }

        return EO_BUFFER_UNDERRUN;
    }

    uint8_t value = reader->data[reader->offset++];
    if (out_value)
    {
        *out_value = value;
    }

    return EO_SUCCESS;
}

EoResult eo_reader_get_char(EoReader *reader, int32_t *out_value)
{
    if (!reader || !out_value)
    {
        return EO_NULL_PTR;
    }

    if (reader->offset > reader->length || reader->length - reader->offset < 1)
    {
        if (reader->chunked_reading_mode)
        {
            *out_value = 0;
            return EO_SUCCESS;
        }

        return EO_BUFFER_UNDERRUN;
    }

    uint8_t bytes[1] = {reader->data[reader->offset++]};
    if (out_value)
    {
        *out_value = eo_decode_number(bytes, 1);
    }

    return EO_SUCCESS;
}

EoResult eo_reader_get_short(EoReader *reader, int32_t *out_value)
{
    if (!reader || !out_value)
    {
        return EO_NULL_PTR;
    }

    if (reader->offset > reader->length || reader->length - reader->offset < 2)
    {
        if (reader->chunked_reading_mode)
        {
            *out_value = 0;
            return EO_SUCCESS;
        }

        return EO_BUFFER_UNDERRUN;
    }

    uint8_t bytes[2] = {reader->data[reader->offset++], reader->data[reader->offset++]};
    if (out_value)
    {
        *out_value = eo_decode_number(bytes, 2);
    }

    return EO_SUCCESS;
}

EoResult eo_reader_get_three(EoReader *reader, int32_t *out_value)
{
    if (!reader || !out_value)
    {
        return EO_NULL_PTR;
    }

    if (reader->offset > reader->length || reader->length - reader->offset < 3)
    {
        if (reader->chunked_reading_mode)
        {
            *out_value = 0;
            return EO_SUCCESS;
        }

        return EO_BUFFER_UNDERRUN;
    }

    uint8_t bytes[3] = {
        reader->data[reader->offset++],
        reader->data[reader->offset++],
        reader->data[reader->offset++]};
    if (out_value)
    {
        *out_value = eo_decode_number(bytes, 3);
    }

    return EO_SUCCESS;
}

EoResult eo_reader_get_int(EoReader *reader, int32_t *out_value)
{
    if (!reader || !out_value)
    {
        return EO_NULL_PTR;
    }

    if (reader->offset > reader->length || reader->length - reader->offset < 4)
    {
        if (reader->chunked_reading_mode)
        {
            *out_value = 0;
            return EO_SUCCESS;
        }

        return EO_BUFFER_UNDERRUN;
    }

    uint8_t bytes[4] = {
        reader->data[reader->offset++],
        reader->data[reader->offset++],
        reader->data[reader->offset++],
        reader->data[reader->offset++]};
    if (out_value)
    {
        *out_value = eo_decode_number(bytes, 4);
    }

    return EO_SUCCESS;
}

EoResult eo_reader_get_string(EoReader *reader, char **out_value)
{
    if (!reader || !out_value)
    {
        return EO_NULL_PTR;
    }

    size_t length = eo_reader_remaining(reader);
    if (length == 0)
    {
        *out_value = strdup("");
        return EO_SUCCESS;
    }

    return eo_reader_get_fixed_string(reader, length, out_value);
}

EoResult eo_reader_get_encoded_string(EoReader *reader, char **out_value)
{
    if (!reader || !out_value)
    {
        return EO_NULL_PTR;
    }

    size_t length = eo_reader_remaining(reader);
    if (length == 0)
    {
        *out_value = strdup("");
        return EO_SUCCESS;
    }

    return eo_reader_get_fixed_encoded_string(reader, length, out_value);
}

EoResult eo_reader_get_fixed_string(EoReader *reader, size_t length, char **out_value)
{
    if (!reader || !out_value)
    {
        return EO_NULL_PTR;
    }

    if (reader->offset > reader->length ||
        reader->length - reader->offset < length)
    {
        if (reader->chunked_reading_mode)
        {
            *out_value = strdup("");
            return EO_SUCCESS;
        }

        return EO_BUFFER_UNDERRUN;
    }

    *out_value = (char *)malloc(length + 1);
    if (!*out_value)
    {
        return EO_ALLOC_FAILED;
    }

    memcpy(*out_value, reader->data + reader->offset, length);
    (*out_value)[length] = '\0';
    reader->offset += length;
    return EO_SUCCESS;
}

EoResult eo_reader_get_fixed_encoded_string(EoReader *reader, size_t length, char **out_value)
{
    if (!reader || !out_value)
    {
        return EO_NULL_PTR;
    }

    if (reader->offset > reader->length || reader->length - reader->offset < length)
    {
        if (reader->chunked_reading_mode)
        {
            *out_value = strdup("");
            return EO_SUCCESS;
        }

        return EO_BUFFER_UNDERRUN;
    }

    *out_value = (char *)malloc(length + 1);
    if (!*out_value)
    {
        return EO_ALLOC_FAILED;
    }

    memcpy(*out_value, reader->data + reader->offset, length);
    eo_decode_string((uint8_t *)*out_value, length);
    (*out_value)[length] = '\0';
    reader->offset += length;
    return EO_SUCCESS;
}

EoResult eo_reader_get_bytes(EoReader *reader, size_t length, uint8_t **out_value)
{
    if (!reader || !out_value)
    {
        return EO_NULL_PTR;
    }

    if (reader->offset > reader->length || reader->length - reader->offset < length)
    {
        if (reader->chunked_reading_mode)
        {
            *out_value = NULL;
            return EO_SUCCESS;
        }

        return EO_BUFFER_UNDERRUN;
    }

    *out_value = (uint8_t *)malloc(length);
    if (!*out_value)
    {
        return EO_ALLOC_FAILED;
    }

    memcpy(*out_value, reader->data + reader->offset, length);
    reader->offset += length;
    return EO_SUCCESS;
}
