#undef NDEBUG
#define ZIP_VECTOR_TRACE 1
#include <zip_vector.h>
#include "test-zip-vector-common.h"

template<typename T>
void t1()
{
    block_random<T> rng;
    zip_vector<T> vec;

    enum test : size_t { test_size = 8192, page_interval = zip_vector<T>::page_interval };

    /* cause 7-bit random to be written to the array */
    vec.resize(test_size);
    for (size_t i = 0; i < vec.size(); i++) {
        vec[i] = rng.abs_i7();
    }

    /* check sum for indexed access vs iterator access */
    T s1 = 0, s2 = 0;
    for (size_t i = 0; i < vec.size(); i++) {
        s1 += vec[i];
    }
    for (auto v : vec) {
        s2 += v;
    }
    assert(s1 == s2);

    dump_index(vec);
}

int main(int argc, const char **argv)
{
    parse_options(argc, argv);
    t1<i64>();
    t1<i32>();
}
