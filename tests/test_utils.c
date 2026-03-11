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

    int mismatch = 0;

    if (!actual || !expected)
    {
        mismatch = 1;
    }
    else
    {
        size_t actual_len = strlen(actual);
        size_t expected_len = strlen(expected);
        if (actual_len != expected_len || memcmp(actual, expected, actual_len) != 0)
        {
            mismatch = 1;
        }
    }

    if (mismatch)
    {
        fprintf(stderr, "%s failed\n", name);
        fprintf(stderr, "  actual  : ");
        if (actual)
        {
            for (size_t i = 0; actual[i] != '\0'; i++)
            {
                uint8_t b = (uint8_t)actual[i];
                if (b >= 0x20 && b < 0x7F)
                    fprintf(stderr, "%c", b);
                else
                    fprintf(stderr, "\\x%02X", b);
            }
        }
        else
        {
            fprintf(stderr, "(null)");
        }
        fprintf(stderr, "\n  expected: ");
        if (expected)
        {
            for (size_t i = 0; expected[i] != '\0'; i++)
            {
                uint8_t b = (uint8_t)expected[i];
                if (b >= 0x20 && b < 0x7F)
                    fprintf(stderr, "%c", b);
                else
                    fprintf(stderr, "\\x%02X", b);
            }
        }
        else
        {
            fprintf(stderr, "(null)");
        }
        fprintf(stderr, "\n");
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

int run_tests(const TestCase *tests, size_t count, int argc, char **argv)
{
    test_failures = 0;

    if (argc > 1 && strcmp(argv[1], "--list") == 0)
    {
        for (size_t i = 0; i < count; i++)
        {
            printf("%s\n", tests[i].name);
        }
        return 0;
    }

    if (argc > 1)
    {
        for (int arg = 1; arg < argc; arg++)
        {
            const char *name = argv[arg];
            int found = 0;
            for (size_t i = 0; i < count; i++)
            {
                if (strcmp(name, tests[i].name) == 0)
                {
                    tests[i].fn();
                    found = 1;
                    break;
                }
            }

            if (!found)
            {
                fprintf(stderr, "Unknown test: %s\n", name);
                test_failures++;
            }
        }
    }
    else
    {
        for (size_t i = 0; i < count; i++)
        {
            tests[i].fn();
        }
    }

    if (test_failures != 0)
    {
        fprintf(stderr, "%d test(s) failed\n", test_failures);
        return 1;
    }

    return 0;
}
