#undef NDEBUG
#define ZIP_VECTOR_TRACE 0
#include <zip_vector.h>
#include "test-zip-vector-common.h"

void t1()
{
    block_random<i64> rng;
    zip_vector<i64> vec;

    vec.resize(8192);
    for (size_t i = 0; i < 8192; i++) {
        vec[i] = rng.rel_i7();
    }
    vec.sync();
    
    dump_index(vec);
}

int main(int argc, char **argv)
{
    #if ZIP_VECTOR_TRACE
        zvec_logger::set_level(zvec_logger::Ltrace);
    #endif
    t1();
}
