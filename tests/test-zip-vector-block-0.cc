#undef NDEBUG
#define ZIP_VECTOR_TRACE 1
#include <zip_vector.h>
#include "test-zip-vector-common.h"

void t1()
{
    zip_vector<i64> vec;
    size_t o1, o2, o3, o4, o5, o6;
    o1 = vec.alloc_slab(zvec_size_64);
    o2 = vec.alloc_slab(zvec_size_64);
    o3 = vec.alloc_slab(zvec_size_64);
    vec.dealloc_slab(zvec_size_64, o1);
    vec.dealloc_slab(zvec_size_64, o2);
    vec.dealloc_slab(zvec_size_64, o3);
    dump_bins(vec);
    o1 = vec.alloc_slab(zvec_size_64);
    o2 = vec.alloc_slab(zvec_size_64);
    o3 = vec.alloc_slab(zvec_size_64);
    o4 = vec.alloc_slab(zvec_size_16);
    o5 = vec.alloc_slab(zvec_size_32);
    o6 = vec.alloc_slab(zvec_size_64);
    vec.dealloc_slab(zvec_size_16, o4);
    vec.dealloc_slab(zvec_size_32, o5);
    vec.dealloc_slab(zvec_size_64, o6);
    dump_bins(vec);
    o4 = vec.alloc_slab(zvec_size_16);
    o5 = vec.alloc_slab(zvec_size_32);
    o6 = vec.alloc_slab(zvec_size_64);
    vec.dealloc_slab(zvec_size_64, o1);
    vec.dealloc_slab(zvec_size_64, o2);
    vec.dealloc_slab(zvec_size_64, o3);
    vec.dealloc_slab(zvec_size_16, o4);
    vec.dealloc_slab(zvec_size_32, o5);
    vec.dealloc_slab(zvec_size_64, o6);
    dump_bins(vec);
    size_t o[16];
    for (size_t i = 0; i < 16; i++) {
        o[i] = vec.alloc_slab(zvec_size_1);
    }
    for (size_t i = 0; i < 16; i++) {
        vec.dealloc_slab(zvec_size_1, o[i]);
    }
    dump_bins(vec);
}

int main(int argc, char **argv)
{
    #if ZIP_VECTOR_TRACE
        zvec_logger::set_level(zvec_logger::Ltrace);
    #endif
    t1();
}
