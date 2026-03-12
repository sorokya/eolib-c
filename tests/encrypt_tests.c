#include "eolib/encrypt.h"
#include "test_utils.h"

static void test_encrypt_packet_known_vector()
{
    uint8_t buf[] = {21, 18, 145, 72, 101, 108, 108, 111, 44, 32, 119, 111, 114, 108, 100, 33};
    const uint8_t expected[] = {149, 161, 146, 228, 17, 242, 200, 236, 229, 239, 236, 247, 236, 160, 239, 172};

    eo_encrypt_packet(buf, sizeof(buf), 6);

    expect_equal_bytes("encrypt_packet_known_vector", buf, expected, sizeof(buf));
}

static void test_decrypt_packet_known_vector()
{
    uint8_t buf[] = {149, 161, 146, 228, 17, 242, 200, 236, 229, 239, 236, 247, 236, 160, 239, 172};
    const uint8_t expected[] = {21, 18, 145, 72, 101, 108, 108, 111, 44, 32, 119, 111, 114, 108, 100, 33};

    eo_decrypt_packet(buf, sizeof(buf), 6);

    expect_equal_bytes("decrypt_packet_known_vector", buf, expected, sizeof(buf));
}

static void test_server_verification_hash()
{
    int32_t challenge = 123456;
    int32_t result = eo_server_verification_hash(challenge);
    expect_equal_int32("eo_server_verification_hash", result, 300733);
}

static void test_generate_swap_multiple()
{
    for (int i = 0; i < 100; i++)
    {
        uint8_t multiple = eo_generate_swap_multiple();
        if (multiple < 6 || multiple > 12)
        {
            fprintf(stderr, "eo_generate_swap_multiple failed: got %u\n", multiple);
            test_failures++;
            break;
        }
    }
}

static const TestCase encrypt_tests[] = {
    {"encrypt_packet_known_vector", test_encrypt_packet_known_vector},
    {"decrypt_packet_known_vector", test_decrypt_packet_known_vector},
    {"eo_server_verification_hash", test_server_verification_hash},
    {"eo_generate_swap_multiple", test_generate_swap_multiple},
};

int main(int argc, char **argv)
{
    return run_tests(encrypt_tests, sizeof(encrypt_tests) / sizeof(encrypt_tests[0]), argc, argv);
}
