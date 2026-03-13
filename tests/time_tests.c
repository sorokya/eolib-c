#include "eolib/time.h"
#include "test_utils.h"

static void test_eo_time_valid_range()
{
    uint32_t t = eo_time();
    // Should return a positive value. The Borland-compatible formula computes
    // seconds relative to 1970 + a 18000-second offset, so values for 2025+
    // should be above ~1740000000 (approximately Jan 1 2025 UTC).
    expect("eo_time returns positive value", t > 0 && t != (uint32_t)-1);
    expect("eo_time is above year-2025 floor", t > 1740000000u);
}

static void test_eo_time_monotonic()
{
    // Two consecutive calls should return the same or a slightly higher value
    // (within a 2-second window to be safe against second boundaries).
    uint32_t t1 = eo_time();
    uint32_t t2 = eo_time();
    expect("eo_time t2 >= t1", t2 >= t1);
    expect("eo_time t2 < t1 + 2", t2 < t1 + 2);
}

static const TestCase time_tests[] = {
    {"eo_time_valid_range", test_eo_time_valid_range},
    {"eo_time_monotonic", test_eo_time_monotonic},
};

int main(int argc, char **argv)
{
    return run_tests(time_tests, sizeof(time_tests) / sizeof(time_tests[0]), argc, argv);
}
