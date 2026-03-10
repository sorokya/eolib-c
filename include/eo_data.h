#ifndef EOLIB_EO_DATA_H
#define EOLIB_EO_DATA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define EO_CHAR_MAX 253
#define EO_SHORT_MAX 64009
#define EO_THREE_MAX 16194277
#define EO_INT_MAX 4097152081

typedef struct EoWriter
{
    bool string_sanitization_mode;
    uint8_t *data;
    size_t length;
    size_t capacity;
} EoWriter;

typedef struct EoReader
{
    bool chunked_reading_mode;
    const uint8_t *data;
    size_t length;
    size_t offset;
    size_t chunk_offset;
    size_t next_chunk_offset;
} EoReader;

EoWriter eo_writer_init_with_capacity(size_t capacity);
int eo_writer_ensure_capacity(EoWriter *writer, size_t additional);

int eo_encode_number(int32_t number, uint8_t out_bytes[4]);
int32_t eo_decode_number(const uint8_t *bytes, size_t length);
void eo_decode_string(uint8_t *buf, size_t length);
void eo_encode_string(uint8_t *buf, size_t length);

bool eo_writer_get_string_sanitization_mode(const EoWriter *writer);
void eo_writer_set_string_sanitization_mode(EoWriter *writer, bool enabled);

int eo_writer_add_byte(EoWriter *writer, uint8_t value);
int eo_writer_add_char(EoWriter *writer, int32_t value);
int eo_writer_add_short(EoWriter *writer, int32_t value);
int eo_writer_add_three(EoWriter *writer, int32_t value);
int eo_writer_add_int(EoWriter *writer, int32_t value);
int eo_writer_add_string(EoWriter *writer, const char *value);
int eo_writer_add_encoded_string(EoWriter *writer, const char *value);
int eo_writer_add_bytes(EoWriter *writer, const uint8_t *data, size_t length);

bool eo_reader_get_chunked_reading_mode(const EoReader *reader);
void eo_reader_set_chunked_reading_mode(EoReader *reader, bool enabled);

size_t eo_reader_remaining(const EoReader *reader);
int eo_reader_next_chunk(EoReader *reader);

int eo_reader_get_byte(EoReader *reader, uint8_t *out_value);
int eo_reader_get_char(EoReader *reader, int32_t *out_value);
int eo_reader_get_short(EoReader *reader, int32_t *out_value);
int eo_reader_get_three(EoReader *reader, int32_t *out_value);
int eo_reader_get_int(EoReader *reader, int32_t *out_value);
int eo_reader_get_string(EoReader *reader, char **out_value);
int eo_reader_get_encoded_string(EoReader *reader, char **out_value);
int eo_reader_get_fixed_string(EoReader *reader, size_t length, char **out_value);
int eo_reader_get_fixed_encoded_string(EoReader *reader, size_t length, char **out_value);
int eo_reader_get_bytes(EoReader *reader, size_t length, uint8_t **out_value);

#endif
