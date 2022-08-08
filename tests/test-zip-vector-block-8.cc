#undef NDEBUG
#define ZIP_VECTOR_TRACE 0
#include <zip_vector.h>
#include "test-zip-vector-common.h"

template <typename T, typename R>
static __attribute__((noinline)) void test_zip_vector_1D(size_t n, T(R::*func)())
{
    R rng;
    zip_vector<T> vec;

    T x1 = 0, x2 = 0;

    vec.resize(n);
    for (size_t i = 0; i < n; i++) {
        x1 += (vec[i] = (rng.*func)());
    }

    for (auto x : vec) x2 += x;

    if (x1 != x2) abort();
}

int main(int argc, char **argv)
{
    #if ZIP_VECTOR_TRACE
        zvec_logger::set_level(zvec_logger::Ltrace);
    #endif
    test_zip_vector_1D<i64>(8192, &block_random<i64>::abs_i7);
}
