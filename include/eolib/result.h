#ifndef EOLIB_RESULT_H
#define EOLIB_RESULT_H

/**
 * Return codes for eolib operations.
 *
 * All functions that can fail return an EoResult. A return value of
 * EO_SUCCESS (0) indicates the operation completed without error.
 * Any other value describes what went wrong.
 */
typedef enum EoResult
{
    /** The operation completed successfully. */
    EO_SUCCESS = 0,

    /**
     * A required pointer argument was NULL.
     *
     * Returned when a caller passes NULL for a mandatory parameter such as a
     * writer, reader, sequencer, or output buffer pointer.
     */
    EO_NULL_PTR = 1,

    /**
     * An internal size calculation would overflow.
     *
     * Returned when growing a writer's buffer would exceed the addressable
     * memory limit (SIZE_MAX).
     */
    EO_OVERFLOW = 2,

    /**
     * A memory allocation failed.
     *
     * Returned when malloc or realloc returns NULL, indicating the system
     * could not satisfy the requested allocation.
     */
    EO_ALLOC_FAILED = 3,

    /**
     * A numeric value exceeds the EO encoding range.
     *
     * Returned when a number passed to an EO encoding function is too large
     * to be represented in the EO integer format (must be less than
     * EO_INT_MAX).
     */
    EO_NUMBER_TOO_LARGE = 4,

    /**
     * The reader does not contain enough bytes for the requested read.
     *
     * Returned when a reader function tries to consume more bytes than are
     * available in the current read window.
     */
    EO_BUFFER_UNDERRUN = 5,

    /**
     * The reader is not in chunked reading mode.
     *
     * Returned when eo_reader_next_chunk is called but chunked reading mode
     * has not been enabled on the reader.
     */
    EO_CHUNKED_MODE_DISABLED = 6,

    /**
     * The calculated sequence range is invalid (max < min).
     *
     * Returned by sequence_init_bytes when the derived seq1 bounds produce an
     * empty range, indicating the start value is out of the representable
     * sequence space.
     */
    EO_INVALID_SEQUENCE_RANGE = 7,

    /**
     * A calculated sequence value falls outside [0, EO_CHAR_MAX - 1].
     *
     * Returned by sequence_init_bytes or sequence_ping_bytes when the derived
     * seq2 value cannot be encoded as a single EO byte.
     */
    EO_SEQUENCE_OUT_OF_RANGE = 8,

    /**
     * The data being deserialized is structurally invalid.
     *
     * Returned during protocol deserialization when a required sentinel value
     * does not match, a file magic string is wrong, or a required field
     * contains an unexpected value.
     */
    EO_INVALID_DATA = 9,

    /**
     * A string operation would exceed the allowed length.
     *
     * Returned when a string operation, such as adding a fixed-length string,
     * would result in a string that is too long for the allocated buffer.
     */
    EO_STR_OUT_OF_RANGE = 10,

    /**
     * A string is shorter than the expected length.
     *
     * Returned when a fixed-length string added to a writer is shorter than the specified length,
     * and padding is not enabled to fill the remaining space.
     */
    EO_STR_TOO_SHORT = 11,

    /**
     * A character value is outside the valid range (0 to EO_CHAR_MAX).
     *
     * Returned when attempting to encode a character that cannot be represented
     * in the EO encoding format.
     */
    EO_INVALID_CHAR = 12,

    /**
     * A short integer value is outside the valid range (0 to EO_SHORT_MAX).
     *
     * Returned when attempting to encode a short integer that cannot be represented
     * in the EO encoding format.
     */
    EO_INVALID_SHORT = 13,

    /**
     * A three-byte integer value is outside the valid range (0 to EO_THREE_MAX).
     *
     * Returned when attempting to encode a three-byte integer that cannot be represented
     * in the EO encoding format.
     */
    EO_INVALID_THREE = 14,

    /**
     * An integer value is outside the valid range (0 to EO_INT_MAX).
     *
     * Returned when attempting to encode an integer that cannot be represented
     * in the EO encoding format.
     */
    EO_INVALID_INT = 15,
} EoResult;

/**
 * Returns a human-readable string describing an EoResult value.
 * @param result The result code to describe.
 * @return A static string describing the result. Never returns NULL.
 */
const char *eo_result_string(EoResult result);

#endif
