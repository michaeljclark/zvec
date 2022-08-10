#undef NDEBUG
#define ZIP_VECTOR_TRACE 0
#include <zip_vector.h>
#include "test-zip-vector-common.h"

void t1()
{
    block_random<i64> rng;
    std::vector<i64> cvec;
    zip_vector<i64> zvec;

    cvec.resize(65536);
    zvec.resize(65536);
    for (size_t j = 0; j < 128; j++) {
        for (size_t i = 0; i < cvec.size(); i++) {
            cvec[i] = zvec[i] = rng.val();
        }
    }

    i64 sum1 = 0, sum2 = 0;
    for (auto v : cvec) {
        sum1 += (i64)v;
    }
    for (auto v : zvec) {
        sum2 += (i64)v;
    }

    assert(sum1 == sum2);

    zvec.sync();

    dump_index(zvec);
}

int main(int argc, char **argv)
{
    #if ZIP_VECTOR_TRACE
        zvec_logger::set_level(zvec_logger::Ltrace);
    #endif
    t1();
}
