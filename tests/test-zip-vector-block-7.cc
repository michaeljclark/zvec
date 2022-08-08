#undef NDEBUG
#define ZIP_VECTOR_TRACE 0
#include <zip_vector.h>
#include "test-zip-vector-common.h"

struct vertex
{
    float pos[3];
    float norm[3];
    float uv[2];
    int col;
};

void t1()
{
    block_random<i64> rng;
    zip_vector<i64> vec;

    printf("sizeof(vertex)=%zu\n", sizeof(vertex));

    vec.resize(512);
    for (size_t i = 0; i < 512; i++) {
        vec[i] = (intptr_t)(void*)malloc(sizeof(vertex));
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
