#undef NDEBUG
#define ZIP_VECTOR_TRACE 1
#include <zip_vector.h>
#include "test-zip-vector-common.h"

template <typename T, typename R>
static __attribute__((noinline)) void test_zip_vector_1D(size_t n, T(R::*func)())
{
    R rng;
    zip_vector<T> vec;

    /* checksum test largish vector for consistency after slab resize */
    T s1 = 0, s2 = 0;
    vec.resize(n);
    for (size_t i = 0; i < n; i++) {
        s1 += (vec[i] = (rng.*func)());
    }

    /* check sum */
    for (auto x : vec) s2 += x;
    assert(s1 == s2);

    vec.sync();
}

void t1()
{
    test_zip_vector_1D<i64>(1024*1024, &block_random<i64>::abs_i7);
    test_zip_vector_1D<i64>(1024*1024, &block_random<i64>::abs_i15);
    test_zip_vector_1D<i64>(1024*1024, &block_random<i64>::abs_i23);
    test_zip_vector_1D<i64>(1024*1024, &block_random<i64>::abs_i31);
    test_zip_vector_1D<i64>(1024*1024, &block_random<i64>::abs_i47);
    test_zip_vector_1D<i32>(1024*1024, &block_random<i32>::abs_i7);
    test_zip_vector_1D<i32>(1024*1024, &block_random<i32>::abs_i15);
    test_zip_vector_1D<i32>(1024*1024, &block_random<i32>::abs_i23);
}

int main(int argc, const char **argv)
{
    parse_options(argc, argv);
    t1();
}
