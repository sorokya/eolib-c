#include "test_utils.h"

#include <stdio.h>
#include <string.h>

int test_failures = 0;

void expect_equal_bytes(const char *name, const uint8_t *actual, const uint8_t *expected, size_t length)
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
        test_failures++;
    }
}

void expect_equal_int32(const char *name, int32_t actual, int32_t expected)
{
    if (actual != expected)
    {
        fprintf(stderr, "%s failed: actual=%d, expected=%d\n", name, actual, expected);
        test_failures++;
    }
}

void expect_equal_int(const char *name, int actual, int expected)
{
    if (actual != expected)
    {
        fprintf(stderr, "%s failed: actual=%d, expected=%d\n", name, actual, expected);
        test_failures++;
    }
}

void expect_equal_size(const char *name, size_t actual, size_t expected)
{
    if (actual != expected)
    {
        fprintf(stderr, "%s failed: actual=%zu, expected=%zu\n", name, actual, expected);
        test_failures++;
    }
}

void expect_equal_str(const char *name, const char *actual, const char *expected)
{
    if (!actual && !expected)
    {
        return;
    }

    if (!actual || !expected || strcmp(actual, expected) != 0)
    {
        fprintf(stderr, "%s failed: actual=%s, expected=%s\n",
                name,
                actual ? actual : "(null)",
                expected ? expected : "(null)");
        test_failures++;
    }
}

void expect(const char *name, int condition)
{
    if (!condition)
    {
        fprintf(stderr, "%s failed\n", name);
        test_failures++;
    }
}
