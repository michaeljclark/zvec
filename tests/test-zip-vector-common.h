#pragma once

#include <cstdio>
#include <cassert>
#include <cinttypes>

#include <random>

template <typename T = i64, int Q = 512>
struct block_random
{
    std::seed_seq seed;
    std::mt19937 engine;

    std::uniform_int_distribution<T> dist_state;
    std::uniform_int_distribution<T> dist_i7;
    std::uniform_int_distribution<T> dist_i15;
    std::uniform_int_distribution<T> dist_i23;
    std::uniform_int_distribution<T> dist_i31;
    std::uniform_int_distribution<T> dist_i47;
    std::uniform_int_distribution<T> dist_i63;

    int counter;
    int state;
    T value;
    T delta;

    block_random() :
        seed{2},
        engine(seed),
        dist_state(0,12),
        dist_i7(-(1ull<<6)+1,(1ull<<6)-1ull),
        dist_i15(-(1ull<<14)+1,(1ull<<14)-1ull),
        dist_i23(-(1ull<<22)+1,(1ull<<22)-1ull),
        dist_i31(-(1ull<<30)+1,(1ull<<30)-1ull),
        dist_i47(-(1ull<<46)+1,(1ull<<46)-1ull),
        dist_i63(-(1ull<<62)+1,(1ull<<62)-1ull),
        counter(0),
        state(-1),
        value(0),
        delta(0)
    {}

    block_random(block_random&&) : block_random() {}

    int rstate(){ /* random state (0-12) */ return dist_state(engine); }

    T abs_i7()  { /*  0 + (-2^6:2^6-1)   */ return dist_i7(engine); }
    T abs_i15() { /*  0 + (-2^14:2^14-1) */ return dist_i15(engine); }
    T abs_i23() { /*  0 + (-2^22:2^22-1) */ return dist_i23(engine); }
    T abs_i31() { /*  0 + (-2^30:2^30-1) */ return dist_i31(engine); }
    T abs_i47() { /*  0 + (-2^46:2^46-1) */ return dist_i47(engine); }
    T rel_i7()  { /* iv + (-2^6:2^6-1)   */ return value + dist_i7(engine); }
    T rel_i15() { /* iv + (-2^14:2^14-1) */ return value + dist_i15(engine); }
    T rel_i23() { /* iv + (-2^22:2^22-1) */ return value + dist_i23(engine); }
    T rel_i31() { /* iv + (-2^30:2^30-1) */ return value + dist_i31(engine); }
    T rel_i47() { /* iv + (-2^46:2^46-1) */ return value + dist_i47(engine); }
    T abs_i63() { /* iv + (-2^62:2^62-1) */ return dist_i63(engine); }
    T con_i63() { /* iv                  */ return value; }
    T seq_i63() { /* iv + delta          */ value += delta; return value; }

    T val() {
        if (++counter == Q || state == -1) {
            state = rstate();
            value = dist_i63(engine);
            delta = dist_i47(engine);
            counter = 0;
        }
        switch (state) {
        case 0: return abs_i7();
        case 1: return abs_i15();
        case 2: return abs_i23();
        case 3: return abs_i31();
        case 4: return abs_i47();
        case 5: return rel_i7();
        case 6: return rel_i15();
        case 7: return rel_i23();
        case 8: return rel_i31();
        case 9: return rel_i47();
        case 10: return abs_i63();
        case 11: return con_i63();
        case 12: return seq_i63();
        }
        return 0;
    }
};

template <typename ZV>
void dump_index(ZV& vec)
{
    printf("--- dump index ---\n");
    size_t page_size = sizeof(typename ZV::value_type) * ZV::page_interval;
    size_t page_count = vec._page_count;
    size_t meta_total = sizeof(typename ZV::page_idx) * vec._page_count + vec._slab_limit >> 9;
    size_t slab_total = vec._slab_limit;
    size_t vec_total = meta_total + slab_total;
    size_t vec_pages = page_size * page_count;
    size_t vec_used = 0;

    for (size_t i = 0; i < vec.f_page_num(vec._count); i++) {
        typename ZV::page_idx p = vec._page_idx[i];
        size_t size = zvec_size_bits((zvec_size)p.format.size);
        const char *codec = zvec_codec_name((zvec_codec)p.format.codec);
        size_t block_size = ((size_t)size * ZV::page_interval) >> 3;
        float ratio = ((float)block_size / (float)page_size) * 100.0f;
        printf("block[%-5zd] fmt=%-10s:%-3zd size=[%5zu/%-5zu] (%5.1f%%) offset=%-9zd iv=%" PRId64 " dv=%" PRId64 "\n",
            i, codec, size, block_size, page_size, ratio, p.offset, p.meta.iv, p.meta.dv);
        vec_used += block_size;
    }

    float page_ratio = ((float)vec_used / (float)vec_pages) * 100.0f;
    float total_ratio = ((float)vec_total / (float)vec_pages) * 100.0f;
    float meta_ratio = ((float)meta_total / (float)vec_total) * 100.0f;

    printf("--- statistics ---\n");
    printf("page_size   : %-9zu\n", page_size);
    printf("page_count  : %-9zu\n", page_count);
    printf("vec_used    : %-9zu\n", vec_used);
    printf("vec_pages   : %-9zu\n", vec_pages);
    printf("slab_total  : %-9zu\n", slab_total);
    printf("meta_total  : %-9zu\n", meta_total);
    printf("vec_total   : %-9zu\n", vec_total);
    printf("page_ratio  : %5.1f%%\n", page_ratio);
    printf("total_ratio : %5.1f%%\n", total_ratio);
    printf("meta_ratio  : %5.1f%%\n", meta_ratio);
}
