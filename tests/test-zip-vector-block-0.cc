#undef NDEBUG
#define ZIP_VECTOR_TRACE 1
#include <zip_vector.h>
#include "test-zip-vector-common.h"

void t1()
{
    zip_vector<i64> vec;
    size_t o1, o2, o3, o4, o5, o6;
    assert(bitmap_count(vec) == 0);
    o1 = vec.alloc_slab(zvec_size_64);
    o2 = vec.alloc_slab(zvec_size_64);
    o3 = vec.alloc_slab(zvec_size_64);
    assert(bitmap_count(vec) == 192);
    vec.dealloc_slab(zvec_size_64, o1);
    vec.dealloc_slab(zvec_size_64, o2);
    vec.dealloc_slab(zvec_size_64, o3);
    assert(bitmap_count(vec) == 0);
    o1 = vec.alloc_slab(zvec_size_64);
    o2 = vec.alloc_slab(zvec_size_64);
    o3 = vec.alloc_slab(zvec_size_64);
    assert(bitmap_count(vec) == 192);
    o4 = vec.alloc_slab(zvec_size_16);
    o5 = vec.alloc_slab(zvec_size_32);
    o6 = vec.alloc_slab(zvec_size_64);
    assert(bitmap_count(vec) == 304);
    vec.dealloc_slab(zvec_size_16, o4);
    vec.dealloc_slab(zvec_size_32, o5);
    vec.dealloc_slab(zvec_size_64, o6);
    assert(bitmap_count(vec) == 192);
    o4 = vec.alloc_slab(zvec_size_16);
    o5 = vec.alloc_slab(zvec_size_32);
    o6 = vec.alloc_slab(zvec_size_64);
    assert(bitmap_count(vec) == 304);
    vec.dealloc_slab(zvec_size_64, o1);
    vec.dealloc_slab(zvec_size_64, o2);
    vec.dealloc_slab(zvec_size_64, o3);
    vec.dealloc_slab(zvec_size_16, o4);
    vec.dealloc_slab(zvec_size_32, o5);
    vec.dealloc_slab(zvec_size_64, o6);
    assert(bitmap_count(vec) == 0);
    size_t o[16];
    for (size_t i = 0; i < 16; i++) {
        o[i] = vec.alloc_slab(zvec_size_1);
    }
    assert(bitmap_count(vec) == 16);
    for (size_t i = 0; i < 16; i++) {
        vec.dealloc_slab(zvec_size_1, o[i]);
    }
    assert(bitmap_count(vec) == 0);
}

int main(int argc, const char **argv)
{
    parse_options(argc, argv);
    t1();
}
