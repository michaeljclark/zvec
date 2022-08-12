#undef NDEBUG
#define ZIP_VECTOR_TRACE 1
#include <zip_vector.h>
#include "test-zip-vector-common.h"

void t1()
{
    zip_vector<i64> vec;

    vec.resize(1024);
    Debug("*** resize(1024)");

    i64 x0 = vec[0];
    assert(x0 == 0);
    Debug("*** read [0] -> %lld", x0);

    vec[0] = -1;
    Debug("*** write 1 -> [0]");

    i64 x1 = vec[0];
    assert(x1 == -1);
    Debug("*** read [0] -> %lld", x1);

    vec[512] = 5;
    Debug("*** write 5 -> [512]");

    i64 x2 = vec[512];
    assert(x2 == 5);
    Debug("*** read [512] -> %lld", x2);

    i64 x3 = vec[0];
    assert(x3 == -1);
    Debug("*** read [0] -> %lld", x3);

    i64 x4 = vec[512];
    assert(x4 == 5);
    Debug("*** read [512] -> %lld", x4);

    i64 x5 = vec[0];
    assert(x5 == -1);
    Debug("*** read [0] -> %lld", x5);

    vec[0] = 0;
    Debug("*** write 0 -> [0]");

    i64 x6 = vec[512];
    assert(x6 == 5);
    Debug("*** read [512] -> %lld", x6);

    i64 x7 = vec[0];
    assert(x7 == 0);
    Debug("*** read [0] -> %lld", x7);

    dump_index(vec);
}

int main(int argc, const char **argv)
{
    parse_options(argc, argv);
    t1();
}
