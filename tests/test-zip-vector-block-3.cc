#undef NDEBUG
#define ZIP_VECTOR_TRACE 1
#include <zip_vector.h>
#include "test-zip-vector-common.h"

void t1()
{
    block_random<i64> rng;
    zip_vector<i64> vec;

    enum test : size_t { test_size = 8192, page_interval = 512 };

    /* cause 7-bit random to be written to the array */
    u64 sum1 = 0, sum2 = 0, r;
    vec.resize(test_size);
    for (size_t i = 0; i < test_size; i++) {
        sum1 += (vec[i] = rng.abs_i7());
    }
    vec.sync();

    /* check sum */
    for (size_t i = 0; i < test_size; i++) {
        sum2 += vec[i];
    }
    assert(sum1 == sum2);

    /* check index */
    for (size_t i = 0; i < test_size/page_interval; i++) {
        assert(vec._page_idx[i].format.codec == zvec_block_abs);
        assert(vec._page_idx[i].meta.iv == 0);
        assert(vec._page_idx[i].meta.dv == 0);
    }

    dump_index(vec);
}

int main(int argc, const char **argv)
{
    parse_options(argc, argv);
    t1();
}
