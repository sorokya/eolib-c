#ifndef EOLIB_EO_DATA_H
#define EOLIB_EO_DATA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "result.h"

/** Maximum value encodable in one EO byte. */
#define EO_CHAR_MAX 253
/** Maximum value encodable in two EO bytes. */
#define EO_SHORT_MAX 64009
/** Maximum value encodable in three EO bytes. */
#define EO_THREE_MAX 16194277
/** Maximum value encodable in four EO bytes. */
#define EO_INT_MAX 4097152081
/** Value used to represent missing bytes in EO number encoding. */
#define EO_NUMBER_PADDING 0xFE
/** Delimiter byte used to mark chunk boundaries in EO string/packet data. */
#define EO_BREAK_BYTE 0xFF

/** Writer that accumulates EO-encoded bytes. */
typedef struct EoWriter
{
    bool string_sanitization_mode; /**< Accessible: see eo_writer_get/set_string_sanitization_mode(). */
    uint8_t *data;                 /**< Read-only: pointer to the accumulated bytes. Do not modify directly. */
    size_t length;                 /**< Read-only: number of bytes written so far. Do not modify directly. */
    size_t capacity;               /**< Private: current allocated capacity. Do not access or modify directly. */
} EoWriter;

/** Reader that parses EO-encoded bytes. */
typedef struct EoReader
{
    bool chunked_reading_mode; /**< Accessible: see eo_reader_get/set_chunked_reading_mode(). */
    const uint8_t *data;       /**< Read-only: pointer to the underlying data buffer. Do not modify directly. */
    size_t length;             /**< Read-only: total length of the data buffer. Do not modify directly. */
    size_t offset;             /**< Read-only: current read position. Do not modify directly. */
    size_t chunk_offset;       /**< Private: start of the current chunk. Do not access or modify directly. */
    size_t next_chunk_offset;  /**< Private: start of the next chunk. Do not access or modify directly. */
} EoReader;

/**
 * Converts a UTF-8 string to Windows-1252 (CP1252) encoding.
 * @param value UTF-8 input string. If NULL, @p out_value is set to NULL and EO_SUCCESS is returned.
 * @param out_value Output string in Windows-1252, heap-allocated — caller must free.
 * @return EO_SUCCESS on success, EO_NULL_PTR if @p out_value is NULL, or
 *         EO_ALLOC_FAILED if memory allocation fails.
 * @remarks Valid UTF-8 multi-byte sequences are decoded and mapped to their Windows-1252
 *          equivalents. Characters with no Windows-1252 representation are replaced with '?'.
 *          Bytes that do not form a valid UTF-8 sequence are passed through unchanged, so
 *          strings that are already Windows-1252 encoded are returned as-is.
 */
EoResult eo_string_to_windows_1252(const char *value, char **out_value);

/**
 * Initializes a writer with a default initial capacity.
 * @return The initialized writer.
 */
EoWriter eo_writer_init(void);

/**
 * Initializes a writer with a specified initial capacity.
 * @param capacity Initial allocation size in bytes.
 * @return The initialized writer.
 */
EoWriter eo_writer_init_with_capacity(size_t capacity);

/**
 * Frees the memory allocated by a writer and zeroes the struct.
 * @param writer Writer to free. If NULL, this function does nothing.
 * @remarks The caller is responsible for calling this function when the writer
 *          is no longer needed. Failure to do so will leak the writer's data buffer.
 */
void eo_writer_free(EoWriter *writer);

/**
 * Ensures a writer has space for additional bytes, reallocating if necessary.
 * @param writer Writer to grow.
 * @param additional Additional bytes required.
 * @return EO_SUCCESS on success, EO_NULL_PTR if writer is NULL,
 *         EO_OVERFLOW if the required size would overflow, or
 *         EO_ALLOC_FAILED if memory reallocation fails.
 * @remarks This is an internal utility function exposed for advanced use cases.
 *          Prefer using the `eo_writer_add_*` functions which call this automatically.
 */
EoResult eo_writer_ensure_capacity(EoWriter *writer, size_t additional);

/**
 * Encodes a signed integer into EO byte format.
 * @param number Value to encode.
 * @param out_bytes Buffer for up to 4 encoded bytes.
 * @return EO_SUCCESS on success, EO_NULL_PTR if out_bytes is NULL, or
 *         EO_NUMBER_TOO_LARGE if the value exceeds the EO encoding range.
 */
EoResult eo_encode_number(int32_t number, uint8_t out_bytes[4]);

/**
 * Decodes an EO-encoded integer.
 * @param bytes Input bytes.
 * @param length Number of bytes to decode (1-4).
 * @return The decoded value.
 * @remarks Inputs must be valid EO-encoded bytes (values produced by `eo_encode_number`).
 *          Passing arbitrary bytes may produce values outside the representable range,
 *          resulting in a silent cast to int32_t with implementation-defined behavior.
 */
int32_t eo_decode_number(const uint8_t *bytes, size_t length);

/**
 * Decodes a string in place using the EO string transform.
 * @param buf Buffer to decode.
 * @param length Number of bytes in the buffer.
 * @remarks This is a low-level transform exposed for advanced use. Prefer
 *          `eo_reader_get_encoded_string` and related functions which apply
 *          this transform automatically. Calling this twice on the same buffer
 *          (double-decoding) will corrupt the data.
 */
void eo_decode_string(uint8_t *buf, size_t length);

/**
 * Encodes a string in place using the EO string transform.
 * @param buf Buffer to encode.
 * @param length Number of bytes in the buffer.
 * @remarks This is a low-level transform exposed for advanced use. Prefer
 *          `eo_writer_add_encoded_string` and related functions which apply
 *          this transform automatically. Calling this twice on the same buffer
 *          (double-encoding) will corrupt the data.
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
 * @return EO_SUCCESS on success, or a non-zero EoResult on failure.
 */
EoResult eo_writer_add_byte(EoWriter *writer, uint8_t value);

/**
 * Appends a 1-byte EO-encoded number.
 * @param writer Writer to append to.
 * @param value Value to encode.
 * @return EO_SUCCESS on success, or a non-zero EoResult on failure.
 */
EoResult eo_writer_add_char(EoWriter *writer, int32_t value);

/**
 * Appends a 2-byte EO-encoded number.
 * @param writer Writer to append to.
 * @param value Value to encode.
 * @return EO_SUCCESS on success, or a non-zero EoResult on failure.
 */
EoResult eo_writer_add_short(EoWriter *writer, int32_t value);

/**
 * Appends a 3-byte EO-encoded number.
 * @param writer Writer to append to.
 * @param value Value to encode.
 * @return EO_SUCCESS on success, or a non-zero EoResult on failure.
 */
EoResult eo_writer_add_three(EoWriter *writer, int32_t value);

/**
 * Appends a 4-byte EO-encoded number.
 * @param writer Writer to append to.
 * @param value Value to encode.
 * @return EO_SUCCESS on success, or a non-zero EoResult on failure.
 */
EoResult eo_writer_add_int(EoWriter *writer, int32_t value);

/**
 * Appends a raw string to the writer.
 * @param writer Writer to append to.
 * @param value String to append (without terminator).
 * @return EO_SUCCESS on success, or a non-zero EoResult on failure.
 */
EoResult eo_writer_add_string(EoWriter *writer, const char *value);

/**
 * Appends a string encoded with the EO string transform.
 * @param writer Writer to append to.
 * @param value String to encode and append.
 * @return EO_SUCCESS on success, or a non-zero EoResult on failure.
 */
EoResult eo_writer_add_encoded_string(EoWriter *writer, const char *value);

/**
 * Appends a fixed-length string to the writer, with optional padding.
 * @param writer Writer to append to.
 * @param value String to append (without terminator).
 * @param length Fixed length of the string in bytes.
 * @param padded If true, shorter strings will be padded with 0xFF bytes.
 * @return EO_SUCCESS on success, EO_STR_OUT_OF_RANGE if the string is too long,
 *         EO_STR_TOO_SHORT if the string is too short and padding is disabled,
 *         or a non-zero EoResult on other failures.
 */
EoResult eo_writer_add_fixed_string(EoWriter *writer, const char *value, size_t length, bool padded);

/**
 * Appends a fixed-length string encoded with the EO string transform, with optional padding.
 * @param writer Writer to append to.
 * @param value String to encode and append (without terminator).
 * @param length Fixed length of the string in bytes.
 * @param padded If true, shorter strings will be padded with 0xFF bytes.
 * @return EO_SUCCESS on success, EO_STR_OUT_OF_RANGE if the string is too long,
 *         EO_STR_TOO_SHORT if the string is too short and padding is disabled,
 *         or a non-zero EoResult on other failures.
 */
EoResult eo_writer_add_fixed_encoded_string(EoWriter *writer, const char *value, size_t length, bool padded);

/**
 * Appends raw bytes to the writer.
 * @param writer Writer to append to.
 * @param data Bytes to append.
 * @param length Number of bytes to append.
 * @return EO_SUCCESS on success, or a non-zero EoResult on failure.
 */
EoResult eo_writer_add_bytes(EoWriter *writer, const uint8_t *data, size_t length);

/**
 * Initializes a reader over an existing byte buffer.
 * @param data Pointer to the byte buffer. The caller must ensure it remains valid
 *             for the lifetime of the reader.
 * @param length Number of bytes in the buffer.
 * @return The initialized reader with chunked reading mode disabled.
 */
EoReader eo_reader_init(const uint8_t *data, size_t length);

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
 * @return EO_SUCCESS on success, EO_NULL_PTR if reader is NULL, or
 *         EO_CHUNKED_MODE_DISABLED if chunked reading mode is not active.
 */
EoResult eo_reader_next_chunk(EoReader *reader);

/**
 * Reads a raw byte from the reader.
 * @param reader Reader to consume from.
 * @param out_value Output for the byte.
 * @return EO_SUCCESS on success, EO_NULL_PTR if reader or out_value is NULL, or
 *         EO_BUFFER_UNDERRUN if not enough bytes remain (non-chunked mode only).
 * @remarks In chunked reading mode, reading past the end of a chunk returns
 *          EO_SUCCESS with @p out_value set to 0 rather than EO_BUFFER_UNDERRUN.
 *          Use `eo_reader_remaining()` to check available bytes before reading.
 */
EoResult eo_reader_get_byte(EoReader *reader, uint8_t *out_value);

/**
 * Reads a 1-byte EO-encoded number.
 * @param reader Reader to consume from.
 * @param out_value Output for the value.
 * @return EO_SUCCESS on success, EO_NULL_PTR if reader or out_value is NULL, or
 *         EO_BUFFER_UNDERRUN if not enough bytes remain (non-chunked mode only).
 * @remarks In chunked reading mode, reading past the end of a chunk returns
 *          EO_SUCCESS with @p out_value set to 0 rather than EO_BUFFER_UNDERRUN.
 *          Use `eo_reader_remaining()` to check available bytes before reading.
 */
EoResult eo_reader_get_char(EoReader *reader, int32_t *out_value);

/**
 * Reads a 2-byte EO-encoded number.
 * @param reader Reader to consume from.
 * @param out_value Output for the value.
 * @return EO_SUCCESS on success, EO_NULL_PTR if reader or out_value is NULL, or
 *         EO_BUFFER_UNDERRUN if not enough bytes remain (non-chunked mode only).
 * @remarks In chunked reading mode, reading past the end of a chunk returns
 *          EO_SUCCESS with @p out_value set to 0 rather than EO_BUFFER_UNDERRUN.
 *          Use `eo_reader_remaining()` to check available bytes before reading.
 */
EoResult eo_reader_get_short(EoReader *reader, int32_t *out_value);

/**
 * Reads a 3-byte EO-encoded number.
 * @param reader Reader to consume from.
 * @param out_value Output for the value.
 * @return EO_SUCCESS on success, EO_NULL_PTR if reader or out_value is NULL, or
 *         EO_BUFFER_UNDERRUN if not enough bytes remain (non-chunked mode only).
 * @remarks In chunked reading mode, reading past the end of a chunk returns
 *          EO_SUCCESS with @p out_value set to 0 rather than EO_BUFFER_UNDERRUN.
 *          Use `eo_reader_remaining()` to check available bytes before reading.
 */
EoResult eo_reader_get_three(EoReader *reader, int32_t *out_value);

/**
 * Reads a 4-byte EO-encoded number.
 * @param reader Reader to consume from.
 * @param out_value Output for the value.
 * @return EO_SUCCESS on success, EO_NULL_PTR if reader or out_value is NULL, or
 *         EO_BUFFER_UNDERRUN if not enough bytes remain (non-chunked mode only).
 * @remarks In chunked reading mode, reading past the end of a chunk returns
 *          EO_SUCCESS with @p out_value set to 0 rather than EO_BUFFER_UNDERRUN.
 *          Use `eo_reader_remaining()` to check available bytes before reading.
 */
EoResult eo_reader_get_int(EoReader *reader, int32_t *out_value);

/**
 * Reads the remaining bytes as a raw string.
 * @param reader Reader to consume from.
 * @param out_value Output string, heap-allocated — caller must free.
 * @return EO_SUCCESS on success, EO_NULL_PTR if reader or out_value is NULL,
 *         or EO_ALLOC_FAILED if memory allocation fails.
 */
EoResult eo_reader_get_string(EoReader *reader, char **out_value);

/**
 * Reads the remaining bytes as an EO-encoded string.
 * @param reader Reader to consume from.
 * @param out_value Output string, heap-allocated — caller must free.
 * @return EO_SUCCESS on success, EO_NULL_PTR if reader or out_value is NULL,
 *         or EO_ALLOC_FAILED if memory allocation fails.
 */
EoResult eo_reader_get_encoded_string(EoReader *reader, char **out_value);

/**
 * Reads a fixed-length raw string.
 * @param reader Reader to consume from.
 * @param length Number of bytes to read.
 * @param out_value Output string, heap-allocated — caller must free.
 * @return EO_SUCCESS on success, EO_NULL_PTR if reader or out_value is NULL,
 *         EO_BUFFER_UNDERRUN if not enough bytes remain, or
 *         EO_ALLOC_FAILED if memory allocation fails.
 */
EoResult eo_reader_get_fixed_string(EoReader *reader, size_t length, char **out_value);

/**
 * Reads a fixed-length EO-encoded string.
 * @param reader Reader to consume from.
 * @param length Number of bytes to read.
 * @param out_value Output string, heap-allocated — caller must free.
 * @return EO_SUCCESS on success, EO_NULL_PTR if reader or out_value is NULL,
 *         EO_BUFFER_UNDERRUN if not enough bytes remain, or
 *         EO_ALLOC_FAILED if memory allocation fails.
 */
EoResult eo_reader_get_fixed_encoded_string(EoReader *reader, size_t length, char **out_value);

/**
 * Reads raw bytes from the reader.
 * @param reader Reader to consume from.
 * @param length Number of bytes to read.
 * @param out_value Output buffer, heap-allocated — caller must free.
 * @return EO_SUCCESS on success, EO_NULL_PTR if reader or out_value is NULL,
 *         EO_BUFFER_UNDERRUN if not enough bytes remain, or
 *         EO_ALLOC_FAILED if memory allocation fails.
 */
EoResult eo_reader_get_bytes(EoReader *reader, size_t length, uint8_t **out_value);

#endif
