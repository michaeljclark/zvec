#undef NDEBUG
#define ZIP_VECTOR_TRACE 1
#include <zip_vector.h>
#include "test-zip-vector-common.h"

template <typename T>
void t1()
{
    zip_vector<T> vec;

    size_t o = zip_vector<T>::page_interval;

    vec.resize(o * 2);
    Debug("*** resize(%zu)", o * 2);

    T x0 = vec[0];
    assert(x0 == 0);
    Debug("*** read [0] -> %lld", x0);

    vec[0] = -1;
    Debug("*** write 1 -> [0]");

    T x1 = vec[0];
    assert(x1 == -1);
    Debug("*** read [0] -> %lld", x1);

    vec[o] = 5;
    Debug("*** write 5 -> [%zu]", o);

    T x2 = vec[o];
    assert(x2 == 5);
    Debug("*** read [%zu] -> %lld", o, x2);

    T x3 = vec[0];
    assert(x3 == -1);
    Debug("*** read [0] -> %lld", x3);

    T x4 = vec[o];
    assert(x4 == 5);
    Debug("*** read [%zu] -> %lld", o, x4);

    T x5 = vec[0];
    assert(x5 == -1);
    Debug("*** read [0] -> %lld", x5);

    vec[0] = 0;
    Debug("*** write 0 -> [0]");

    T x6 = vec[o];
    assert(x6 == 5);
    Debug("*** read [%zu] -> %lld", o, x6);

    T x7 = vec[0];
    assert(x7 == 0);
    Debug("*** read [0] -> %lld", x7);

    dump_index(vec);
}

int main(int argc, const char **argv)
{
    parse_options(argc, argv);
    t1<i64>();
    t1<i32>();
}
