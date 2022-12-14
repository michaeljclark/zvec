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

    /* cause a sequence to be written to the array */
    vec.resize(test_size);
    for (size_t i = 0; i < test_size; i++) {
        vec[i] = i;
    }
    vec.sync();

    /* check sum */
    size_t s1 = 0, s2 = 0;
    for (size_t i = 0; i < test_size; i++) {
        s1 += i;
        s2 += vec[i];
    }
    assert(s1 == s2);

    /* check index */
    for (size_t i = 0; i < test_size/page_interval; i++) {
        assert(vec._page_idx[i].format.codec == zvec_const_rel);
        assert(vec._page_idx[i].meta.iv == (i * page_interval) - 1);
        assert(vec._page_idx[i].meta.dv == 1);
    }

    dump_index(vec);
}

int main(int argc, const char **argv)
{
    parse_options(argc, argv);
    t1<i64>();
    t1<i32>();
}
