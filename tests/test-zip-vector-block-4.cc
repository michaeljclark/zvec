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

    /* cause 7-bit random (with base) to be written to the array */
    u64 s1 = 0, s2 = 0;
    vec.resize(test_size);
    for (size_t i = 0; i < test_size; i++) {
        s1 += (vec[i] = rng.rel_i7());
    }
    vec.sync();

    /* check sum */
    for (size_t i = 0; i < test_size; i++) {
        s2 += vec[i];
    }
    assert(s1 == s2);

    /* check index */
    for (size_t i = 0; i < test_size/page_interval; i++) {
        assert(vec._page_idx[i].format.codec == zvec_block_rel);
        assert(vec._page_idx[i].meta.iv != 0);
        assert(vec._page_idx[i].meta.dv == 0);
    }

    dump_index(vec);
}

int main(int argc, const char **argv)
{
    parse_options(argc, argv);
    t1<i64>();
    t1<i32>();
}
