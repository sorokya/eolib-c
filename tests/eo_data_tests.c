#include "test_utils.h"
#include "eo_data.h"

static void test_eo_writer_init_with_capacity()
{
    size_t capacity = 128;
    EoWriter writer = eo_writer_init_with_capacity(capacity);

    expect("eo_writer_init_with_capacity should allocate data", writer.data != NULL);
    expect("eo_writer_init_with_capacity should initialize length to 0", writer.length == 0);
    expect("eo_writer_init_with_capacity should set capacity correctly", writer.capacity == capacity);

    free(writer.data);
}

static void test_eo_writer_ensure_capacity()
{
    EoWriter writer = eo_writer_init_with_capacity(4);

    // Test ensuring capacity within current capacity
    expect_equal_int("eo_writer_ensure_capacity should succeed when capacity is sufficient", eo_writer_ensure_capacity(&writer, 2), 0);
    expect_equal_int("eo_writer_ensure_capacity should not change capacity when sufficient", writer.capacity, 4);

    writer.length = 4; // Simulate that the writer is full

    // Test ensuring capacity that requires resizing
    expect_equal_int("eo_writer_ensure_capacity should succeed when resizing is needed", eo_writer_ensure_capacity(&writer, 4), 0);
    expect_equal_int("eo_writer_ensure_capacity should increase capacity when needed", writer.capacity, 8);

    // Test ensuring capacity with NULL writer
    expect("eo_writer_ensure_capacity should fail with NULL writer", eo_writer_ensure_capacity(NULL, 1) == -1);

    free(writer.data);
}

int main(void)
{
    test_eo_writer_init_with_capacity();
    test_eo_writer_ensure_capacity();

    return test_failures != 0 ? 1 : 0;
}
