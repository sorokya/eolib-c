/**
 * WASM API wrappers for eolib-c.
 *
 * These functions provide a WASM-friendly interface to the core library by:
 *  - Exposing number codec via individual byte parameters (avoids pointer sharing)
 *  - Using static buffers for small fixed-size outputs (encode_number result)
 *  - Re-exporting in-place operations (encrypt, decrypt, encode/decode string)
 *    so they can be called with Emscripten heap pointers from JS
 */
#include <stdint.h>
#include <string.h>

#include "../include/eolib/data.h"
#include "../include/eolib/encrypt.h"
#include "../include/eolib/sequencer.h"
#include "../include/eolib/rng.h"

/* Static 4-byte output buffer for eo_encode_number. */
static uint8_t g_encode_number_buf[4];

/**
 * Encode a number in EO format.
 *
 * Returns a pointer to an internal 4-byte buffer containing the encoded bytes.
 * The caller reads exactly `size` bytes (1-4) from the returned pointer.
 * Returns NULL on error.
 */
uint8_t *wasm_encode_number_buf(int32_t number)
{
    memset(g_encode_number_buf, 0xFE, 4);
    EoResult r = eo_encode_number(number, g_encode_number_buf);
    if (r != EO_SUCCESS)
        return NULL;
    return g_encode_number_buf;
}

/**
 * Decode a number from up to 4 raw EO bytes.
 *
 * Pass the bytes individually so JS doesn't need to build a heap pointer.
 * `size` must be 1-4.
 */
int32_t wasm_decode_number_bytes(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3, int size)
{
    uint8_t buf[4] = {b0, b1, b2, b3};
    if (size < 1)
        size = 1;
    if (size > 4)
        size = 4;
    return eo_decode_number(buf, (size_t)size);
}

/**
 * Compute the server verification hash for a given challenge value.
 */
int32_t wasm_server_verification_hash(int32_t challenge)
{
    return eo_server_verification_hash(challenge);
}

/**
 * Generate a random swap multiple in [6, 12].
 */
uint8_t wasm_generate_swap_multiple(void)
{
    return eo_generate_swap_multiple();
}

/**
 * Generate a random sequence start value.
 */
int32_t wasm_generate_sequence_start(void)
{
    return eo_generate_sequence_start();
}

/**
 * Recover a sequence start from init handshake bytes.
 */
int32_t wasm_sequence_start_from_init(int32_t s1, int32_t s2)
{
    return eo_sequence_start_from_init(s1, s2);
}

/**
 * Recover a sequence start from ping handshake bytes.
 */
int32_t wasm_sequence_start_from_ping(int32_t s1, int32_t s2)
{
    return eo_sequence_start_from_ping(s1, s2);
}

/**
 * Encrypt packet data in-place on the WASM heap.
 * `data` is a pointer into the WASM linear memory (allocated by JS via _malloc).
 */
void wasm_encrypt_packet(uint8_t *data, int length, uint8_t swap_multiple)
{
    eo_encrypt_packet(data, (size_t)length, swap_multiple);
}

/**
 * Decrypt packet data in-place on the WASM heap.
 */
void wasm_decrypt_packet(uint8_t *data, int length, uint8_t swap_multiple)
{
    eo_decrypt_packet(data, (size_t)length, swap_multiple);
}

/**
 * EO-encode a string in-place on the WASM heap.
 */
void wasm_encode_string(uint8_t *buf, int length)
{
    eo_encode_string(buf, (size_t)length);
}

/**
 * EO-decode a string in-place on the WASM heap.
 */
void wasm_decode_string(uint8_t *buf, int length)
{
    eo_decode_string(buf, (size_t)length);
}

/* Static output buffer for wasm_string_to_windows_1252. */
static char g_cp1252_buf[4096];

/**
 * Seed the RNG with the given 32-bit value.
 * Call this before using wasm_rand() or wasm_rand_range().
 * To replicate the original client's startup seed, pass (uint32_t)time(NULL).
 */
void wasm_srand(uint32_t seed)
{
    eo_srand(seed);
}

/**
 * Generate a pseudo-random 31-bit unsigned integer using the current RNG state.
 */
uint32_t wasm_rand(void)
{
    return eo_rand();
}

/**
 * Generate a pseudo-random integer in the inclusive range [min, max].
 */
uint32_t wasm_rand_range(uint32_t min, uint32_t max)
{
    return eo_rand_range(min, max);
}

/**
 * Generate a server verification challenge value using the current RNG state.
 * Equivalent to eo_rand() % 1000000.
 */
int32_t wasm_generate_server_verification_challenge(void)
{
    return eo_generate_server_verification_challenge();
}

/**
 * Convert a UTF-8 string to Windows-1252.
 * Characters with no CP1252 mapping are replaced with '?'.
 * Input `ptr` must be a WASM heap pointer to a null-terminated UTF-8 string.
 * Returns a pointer to an internal static buffer containing the result.
 */
const char *wasm_string_to_windows_1252(const char *input)
{
    char *out = NULL;
    EoResult r = eo_string_to_windows_1252(input, &out);
    if (r != EO_SUCCESS || !out)
    {
        g_cp1252_buf[0] = '\0';
        return g_cp1252_buf;
    }
    strncpy(g_cp1252_buf, out, sizeof(g_cp1252_buf) - 1);
    g_cp1252_buf[sizeof(g_cp1252_buf) - 1] = '\0';
    free(out);
    return g_cp1252_buf;
}
