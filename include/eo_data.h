#ifndef EOLIB_EO_DATA_H
#define EOLIB_EO_DATA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Maximum value encodable in one EO byte. */
#define EO_CHAR_MAX 253
/** Maximum value encodable in two EO bytes. */
#define EO_SHORT_MAX 64009
/** Maximum value encodable in three EO bytes. */
#define EO_THREE_MAX 16194277
/** Maximum value encodable in four EO bytes. */
#define EO_INT_MAX 4097152081

/** Writer that accumulates EO-encoded bytes. */
typedef struct EoWriter
{
    bool string_sanitization_mode;
    uint8_t *data;
    size_t length;
    size_t capacity;
} EoWriter;

/** Reader that parses EO-encoded bytes. */
typedef struct EoReader
{
    bool chunked_reading_mode;
    const uint8_t *data;
    size_t length;
    size_t offset;
    size_t chunk_offset;
    size_t next_chunk_offset;
} EoReader;

/**
 * Initializes a writer with a specified initial capacity.
 * @param capacity Initial allocation size in bytes.
 * @return The initialized writer.
 */
EoWriter eo_writer_init_with_capacity(size_t capacity);

/**
 * Ensures a writer has space for additional bytes.
 * @param writer Writer to grow.
 * @param additional Additional bytes required.
 * @return 0 on success, -1 on failure.
 */
int eo_writer_ensure_capacity(EoWriter *writer, size_t additional);

/**
 * Encodes a signed integer into EO byte format.
 * @param number Value to encode.
 * @param out_bytes Buffer for up to 4 encoded bytes.
 * @return 0 on success, -1 on failure.
 */
int eo_encode_number(int32_t number, uint8_t out_bytes[4]);

/**
 * Decodes an EO-encoded integer.
 * @param bytes Input bytes.
 * @param length Number of bytes to decode (1-4).
 * @return The decoded value.
 */
int32_t eo_decode_number(const uint8_t *bytes, size_t length);

/**
 * Decodes a string in place using the EO string transform.
 * @param buf Buffer to decode.
 * @param length Number of bytes in the buffer.
 */
void eo_decode_string(uint8_t *buf, size_t length);

/**
 * Encodes a string in place using the EO string transform.
 * @param buf Buffer to encode.
 * @param length Number of bytes in the buffer.
 */
void eo_encode_string(uint8_t *buf, size_t length);

/**
 * Returns the current writer string sanitization mode.
 * @param writer Writer to query.
 * @return True if sanitization mode is enabled.
 */
bool eo_writer_get_string_sanitization_mode(const EoWriter *writer);
/**
 * Enables or disables string sanitization mode.
 * @param writer Writer to update.
 * @param enabled True to enable sanitization.
 */
void eo_writer_set_string_sanitization_mode(EoWriter *writer, bool enabled);

/**
 * Appends a raw byte to the writer.
 * @param writer Writer to append to.
 * @param value Byte value to append.
 * @return 0 on success, -1 on failure.
 */
int eo_writer_add_byte(EoWriter *writer, uint8_t value);

/**
 * Appends a 1-byte EO-encoded number.
 * @param writer Writer to append to.
 * @param value Value to encode.
 * @return 0 on success, -1 on failure.
 */
int eo_writer_add_char(EoWriter *writer, int32_t value);

/**
 * Appends a 2-byte EO-encoded number.
 * @param writer Writer to append to.
 * @param value Value to encode.
 * @return 0 on success, -1 on failure.
 */
int eo_writer_add_short(EoWriter *writer, int32_t value);

/**
 * Appends a 3-byte EO-encoded number.
 * @param writer Writer to append to.
 * @param value Value to encode.
 * @return 0 on success, -1 on failure.
 */
int eo_writer_add_three(EoWriter *writer, int32_t value);

/**
 * Appends a 4-byte EO-encoded number.
 * @param writer Writer to append to.
 * @param value Value to encode.
 * @return 0 on success, -1 on failure.
 */
int eo_writer_add_int(EoWriter *writer, int32_t value);

/**
 * Appends a raw string to the writer.
 * @param writer Writer to append to.
 * @param value String to append (without terminator).
 * @return 0 on success, -1 on failure.
 */
int eo_writer_add_string(EoWriter *writer, const char *value);

/**
 * Appends a string encoded with the EO string transform.
 * @param writer Writer to append to.
 * @param value String to encode and append.
 * @return 0 on success, -1 on failure.
 */
int eo_writer_add_encoded_string(EoWriter *writer, const char *value);

/**
 * Appends raw bytes to the writer.
 * @param writer Writer to append to.
 * @param data Bytes to append.
 * @param length Number of bytes to append.
 * @return 0 on success, -1 on failure.
 */
int eo_writer_add_bytes(EoWriter *writer, const uint8_t *data, size_t length);

/**
 * Returns the current chunked reading mode.
 * @param reader Reader to query.
 * @return True if chunked reading is enabled.
 */
bool eo_reader_get_chunked_reading_mode(const EoReader *reader);

/**
 * Enables or disables chunked reading mode.
 * @param reader Reader to update.
 * @param enabled True to enable chunked reading.
 */
void eo_reader_set_chunked_reading_mode(EoReader *reader, bool enabled);

/**
 * Returns the number of bytes remaining in the current read window.
 * @param reader Reader to query.
 * @return Remaining bytes.
 */
size_t eo_reader_remaining(const EoReader *reader);

/**
 * Advances to the next chunk when chunked reading is enabled.
 * @param reader Reader to advance.
 * @return 0 on success, -1 on failure.
 */
int eo_reader_next_chunk(EoReader *reader);

/**
 * Reads a raw byte from the reader.
 * @param reader Reader to consume from.
 * @param out_value Optional output for the byte.
 * @return 0 on success, -1 on failure.
 */
int eo_reader_get_byte(EoReader *reader, uint8_t *out_value);

/**
 * Reads a 1-byte EO-encoded number.
 * @param reader Reader to consume from.
 * @param out_value Optional output for the value.
 * @return 0 on success, -1 on failure.
 */
int eo_reader_get_char(EoReader *reader, int32_t *out_value);

/**
 * Reads a 2-byte EO-encoded number.
 * @param reader Reader to consume from.
 * @param out_value Optional output for the value.
 * @return 0 on success, -1 on failure.
 */
int eo_reader_get_short(EoReader *reader, int32_t *out_value);

/**
 * Reads a 3-byte EO-encoded number.
 * @param reader Reader to consume from.
 * @param out_value Optional output for the value.
 * @return 0 on success, -1 on failure.
 */
int eo_reader_get_three(EoReader *reader, int32_t *out_value);

/**
 * Reads a 4-byte EO-encoded number.
 * @param reader Reader to consume from.
 * @param out_value Optional output for the value.
 * @return 0 on success, -1 on failure.
 */
int eo_reader_get_int(EoReader *reader, int32_t *out_value);

/**
 * Reads the remaining bytes as a raw string.
 * @param reader Reader to consume from.
 * @param out_value Output string (allocated by the reader).
 * @return 0 on success, -1 on failure.
 */
int eo_reader_get_string(EoReader *reader, char **out_value);

/**
 * Reads the remaining bytes as an EO-encoded string.
 * @param reader Reader to consume from.
 * @param out_value Output string (allocated by the reader).
 * @return 0 on success, -1 on failure.
 */
int eo_reader_get_encoded_string(EoReader *reader, char **out_value);

/**
 * Reads a fixed-length raw string.
 * @param reader Reader to consume from.
 * @param length Number of bytes to read.
 * @param out_value Output string (allocated by the reader).
 * @return 0 on success, -1 on failure.
 */
int eo_reader_get_fixed_string(EoReader *reader, size_t length, char **out_value);

/**
 * Reads a fixed-length EO-encoded string.
 * @param reader Reader to consume from.
 * @param length Number of bytes to read.
 * @param out_value Output string (allocated by the reader).
 * @return 0 on success, -1 on failure.
 */
int eo_reader_get_fixed_encoded_string(EoReader *reader, size_t length, char **out_value);

/**
 * Reads raw bytes from the reader.
 * @param reader Reader to consume from.
 * @param length Number of bytes to read.
 * @param out_value Output buffer (allocated by the reader).
 * @return 0 on success, -1 on failure.
 */
int eo_reader_get_bytes(EoReader *reader, size_t length, uint8_t **out_value);

#endif
