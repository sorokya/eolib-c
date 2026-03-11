#include "data.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static size_t eo_reader_next_break_index(const EoReader *reader);

EoWriter eo_writer_init_with_capacity(size_t capacity)
{
    EoWriter writer;
    writer.string_sanitization_mode = false;
    writer.length = 0;
    writer.capacity = capacity;
    writer.data = capacity > 0 ? (uint8_t *)malloc(capacity) : NULL;
    return writer;
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

    out_bytes[0] = 254;
    out_bytes[1] = 254;
    out_bytes[2] = 254;
    out_bytes[3] = 254;

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
    uint8_t data[4] = {254, 254, 254, 254};

    for (size_t i = 0; i < 4; ++i)
    {
        if (bytes && length > i && bytes[i] != 0)
        {
            data[i] = bytes[i];
        }

        if (data[i] == 254)
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

    size_t length = strlen(value);
    EoResult result;
    if ((result = eo_writer_ensure_capacity(writer, length)) != EO_SUCCESS)
    {
        return result;
    }

    if (writer->string_sanitization_mode)
    {
        for (const char *c = value; *c != '\0'; ++c)
        {
            if (*c == (char)0xff)
            {
                writer->data[writer->length++] = 0x79;
            }
            else
            {
                writer->data[writer->length++] = *c;
            }
        }
    }
    else
    {
        memcpy(writer->data + writer->length, value, length);
        writer->length += length;
    }

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

    size_t length = strlen(value);
    EoResult result;
    if ((result = eo_writer_ensure_capacity(writer, length)) != EO_SUCCESS)
    {
        return result;
    }

    uint8_t *encoded = (uint8_t *)malloc(length);
    if (!encoded)
    {
        return EO_ALLOC_FAILED;
    }

    if (writer->string_sanitization_mode)
    {
        size_t i = 0;
        for (const char *c = value; *c != '\0'; ++c)
        {
            char ch = *c;
            if (ch == (char)0xff)
            {
                encoded[i] = 0x79;
            }
            else
            {
                encoded[i] = *c;
            }
            i++;
        }
    }
    else
    {
        memcpy(encoded, value, length);
    }

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

    if (padded && strlen(value) > length)
    {
        return EO_STR_OUT_OF_RANGE;
    }

    if (!padded && strlen(value) != length)
    {
        return EO_STR_TOO_SHORT;
    }

    EoResult result;
    if ((result = eo_writer_ensure_capacity(writer, length)) != EO_SUCCESS)
    {
        return result;
    }

    if (writer->string_sanitization_mode)
    {
        for (const char *c = value; *c != '\0'; ++c)
        {
            if (*c == (char)0xff)
            {
                writer->data[writer->length++] = 0x79;
            }
            else
            {
                writer->data[writer->length++] = *c;
            }
        }
    }
    else
    {
        memcpy(writer->data + writer->length, value, length);
        writer->length += length;
    }

    if (padded)
    {
        size_t remaining = length > strlen(value) ? length - strlen(value) : 0;
        for (size_t i = 0; i < remaining; ++i)
        {
            writer->data[writer->length + i] = '\xff';
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

    if (padded && strlen(value) > length)
    {
        return EO_STR_OUT_OF_RANGE;
    }

    if (!padded && strlen(value) != length)
    {
        return EO_STR_TOO_SHORT;
    }

    EoResult result;
    if ((result = eo_writer_ensure_capacity(writer, length)) != EO_SUCCESS)
    {
        return result;
    }

    uint8_t *encoded = (uint8_t *)malloc(length);
    if (!encoded)
    {
        return EO_ALLOC_FAILED;
    }

    if (writer->string_sanitization_mode)
    {
        size_t i = 0;
        for (const char *c = value; *c != '\0'; ++c)
        {
            if (*c == (char)0xff)
            {
                encoded[i] = 0x79;
            }
            else
            {
                encoded[i] = *c;
            }
            i++;
        }
    }
    else
    {
        memcpy(encoded, value, length);
    }

    if (padded)
    {
        size_t remaining = length > strlen(value) ? length - strlen(value) : 0;
        for (size_t i = 0; i < remaining; ++i)
        {
            encoded[strlen(value) + i] = '\xff';
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
        reader->offset += 1; // Skip the 0xFF break byte
    }

    reader->chunk_offset = reader->offset;
    reader->next_chunk_offset = eo_reader_next_break_index(reader);

    return EO_SUCCESS;
}

static size_t eo_reader_next_break_index(const EoReader *reader)
{
    if (!reader || !reader->chunked_reading_mode)
    {
        return -1;
    }

    for (size_t i = reader->chunk_offset; i < reader->length; ++i)
    {
        if (reader->data[i] == (uint8_t)0xFF)
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
