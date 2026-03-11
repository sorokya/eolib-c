#ifndef EOLIB_TEST_UTILS_H
#define EOLIB_TEST_UTILS_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

extern int test_failures;

typedef void (*TestFn)(void);

typedef struct
{
    const char *name;
    TestFn fn;
} TestCase;

void expect_equal_bytes(const char *name, const uint8_t *actual, const uint8_t *expected, size_t length);
void expect_equal_int32(const char *name, int32_t actual, int32_t expected);
void expect_equal_int(const char *name, int actual, int expected);
void expect_equal_size(const char *name, size_t actual, size_t expected);
void expect_equal_str(const char *name, const char *actual, const char *expected);
void expect(const char *name, int condition);
int run_tests(const TestCase *tests, size_t count, int argc, char **argv);

#endif
