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
    T sum1 = 0, sum2 = 0;
    vec.resize(n);
    for (size_t i = 0; i < n; i++) {
        sum1 += (vec[i] = (rng.*func)());
    }

    /* check sum */
    for (auto x : vec) sum2 += x;
    assert(sum1 == sum2);

    vec.sync();

    dump_index(vec);
}

void t1()
{
    test_zip_vector_1D<i64>(1024*1024, &block_random<i64>::abs_i7);
}

int main(int argc, const char **argv)
{
    parse_options(argc, argv);
    t1();
}
