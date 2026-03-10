#include <stdio.h>
#include <string.h>

#include "encrypt.h"

static int failures = 0;

static void expect_equal_bytes(const char *name, const uint8_t *actual, const uint8_t *expected, size_t length)
{
    if (memcmp(actual, expected, length) != 0)
    {
        fprintf(stderr, "%s failed\n", name);
        fprintf(stderr, "  actual  : ");
        for (size_t i = 0; i < length; i++)
        {
            fprintf(stderr, "%u%s", actual[i], (i + 1 == length) ? "" : ", ");
        }
        fprintf(stderr, "\n  expected: ");
        for (size_t i = 0; i < length; i++)
        {
            fprintf(stderr, "%u%s", expected[i], (i + 1 == length) ? "" : ", ");
        }
        fprintf(stderr, "\n");
        failures++;
    }
}

static void expect_equal_int(const char *name, int32_t actual, int32_t expected)
{
    if (actual != expected)
    {
        fprintf(stderr, "%s failed: actual=%d, expected=%d\n", name, actual, expected);
        failures++;
    }
}

static void test_encrypt_packet_known_vector()
{
    uint8_t buf[] = {21, 18, 145, 72, 101, 108, 108, 111, 44, 32, 119, 111, 114, 108, 100, 33};
    const uint8_t expected[] = {149, 161, 146, 228, 17, 242, 200, 236, 229, 239, 236, 247, 236, 160, 239, 172};

    encrypt_packet(buf, sizeof(buf), 6);

    expect_equal_bytes("encrypt_packet_known_vector", buf, expected, sizeof(buf));
}

static void test_decrypt_packet_known_vector()
{
    uint8_t buf[] = {149, 161, 146, 228, 17, 242, 200, 236, 229, 239, 236, 247, 236, 160, 239, 172};
    const uint8_t expected[] = {21, 18, 145, 72, 101, 108, 108, 111, 44, 32, 119, 111, 114, 108, 100, 33};

    decrypt_packet(buf, sizeof(buf), 6);

    expect_equal_bytes("decrypt_packet_known_vector", buf, expected, sizeof(buf));
}

static void test_server_verification_hash()
{
    int32_t challenge = 123456;
    int32_t result = server_verification_hash(challenge);
    expect_equal_int("server_verification_hash", result, 300733);
}

static void test_generate_swap_multiple()
{
    for (int i = 0; i < 100; i++)
    {
        uint8_t multiple = generate_swap_multiple();
        if (multiple < 6 || multiple > 12)
        {
            fprintf(stderr, "generate_swap_multiple failed: got %u\n", multiple);
            failures++;
            break;
        }
    }
}

int main(void)
{
    test_encrypt_packet_known_vector();
    test_decrypt_packet_known_vector();
    test_server_verification_hash();
    test_generate_swap_multiple();

    if (failures != 0)
    {
        fprintf(stderr, "%d test(s) failed\n", failures);
        return 1;
    }

    return 0;
}
