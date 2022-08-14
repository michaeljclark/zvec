#undef NDEBUG
#define ZIP_VECTOR_TRACE 1
#include <zip_vector.h>
#include "test-zip-vector-common.h"

void t1()
{
    block_random<i64> rng;
    std::vector<i64> cvec;
    zip_vector<i64> zvec;

    enum test : size_t { test_size = 65536, num_rewrites = 128 };

    /* write pathologically distributed random values to array
     * and a check array. this test check sums all block modes */
    cvec.resize(test_size);
    zvec.resize(test_size);
    for (size_t j = 0; j < num_rewrites; j++) {
        for (size_t i = 0; i < cvec.size(); i++) {
            cvec[i] = zvec[i] = rng.val();
        }
    }

    /* check sum against control vector */
    i64 s1 = 0, s2 = 0;
    for (auto v : cvec) {
        s1 += (i64)v;
    }
    for (auto v : zvec) {
        s2 += (i64)v;
    }
    assert(s1 == s2);

    zvec.sync();

    dump_index(zvec);
}

int main(int argc, const char **argv)
{
    parse_options(argc, argv);
    t1();
}
