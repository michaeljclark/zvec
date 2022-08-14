#undef NDEBUG
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <climits>
#include <cinttypes>
#include <cassert>

#include <hwy/highway.h>
#include <zvec_codecs.h>
#include <zvec_dispatch.h>
#include <zvec_block.h>

#include <map>
#include <vector>
#include <string>
#include <random>
#include <chrono>

using namespace std::chrono;

static std::string output_file;
static std::string cpu_arch;
static bool help_text = false;
static int bench_num = -1;
static FILE* data;

template <typename T = i64>
struct bench_random
{
    std::random_device rng_device;
    std::default_random_engine rng_engine;
    std::uniform_int_distribution<T> rng_dist_i7;
    std::uniform_int_distribution<T> rng_dist_i15;
    std::uniform_int_distribution<T> rng_dist_i23;
    std::uniform_int_distribution<T> rng_dist_i31;
    std::uniform_int_distribution<T> rng_dist_i47;

    bench_random() :
        rng_engine(rng_device()),
        rng_dist_i7(-(1ull<<6)+1,(1ull<<6)-1ull),
        rng_dist_i15(-(1ull<<14)+1,(1ull<<14)-1ull),
        rng_dist_i23(-(1ull<<22)+1,(1ull<<22)-1ull),
        rng_dist_i31(-(1ull<<30)+1,(1ull<<30)-1ull),
        rng_dist_i47(-(1ull<<46)+1,(1ull<<46)-1ull) {}
    bench_random(bench_random&&) : bench_random() {}

    T i7() { /* (-2^6:2^6-1) */ return rng_dist_i7(rng_engine); }
    T i15() { /* (-2^14:2^14-1) */ return rng_dist_i15(rng_engine); }
    T i23() { /* (-2^22:2^22-1) */ return rng_dist_i23(rng_engine); }
    T i31() { /* (-2^30:2^30-1) */ return rng_dist_i31(rng_engine); }
    T i47() { /* (-2^46:2^46-1) */ return rng_dist_i47(rng_engine); }
};

template <typename T>
struct bench_con
{
    T c5() { return 5; }
};

template <typename T>
struct bench_seq
{
    T s;

    bench_seq() : s(5) {}

    T s5() { return (s += 5); }
};

template <typename T, typename R>
__attribute__((noinline)) void array_random(T * __restrict arr, size_t n, T(R::*func)())
{
    R rng;
    for (size_t i = 0; i < n; i++) {
        arr[i] = (rng.*func)();
    }
}

template <typename T>
__attribute__((noinline)) T array_sum(T * __restrict arr, size_t n)
{
    T s = 0;
    for (size_t i = 0; i < n; i++) {
        s += arr[i];
    }
    return s;
}

static const char* format_unit(llong n)
{
    static char buf[32];
    if (n % 1000000000 == 0) {
        snprintf(buf, sizeof(buf), "%lluG", n / 1000000000);
    } else if (n % 1000000 == 0) {
        snprintf(buf, sizeof(buf), "%lluM", n / 1000000);
    } else if (n % 1000 == 0) {
        snprintf(buf, sizeof(buf), "%lluK", n / 1000);
    } else {
        snprintf(buf, sizeof(buf), "%llu", n);
    }
    return buf;
}

static const char* format_binary(llong n)
{
    static char buf[32];
    if (n % (1<<30) == 0) {
        snprintf(buf, sizeof(buf), "%lluGi", n / (1<<30));
    } else if (n % (1<<20) == 0) {
        snprintf(buf, sizeof(buf), "%lluMi", n / (1<<20));
    } else if (n % (1<<10) == 0) {
        snprintf(buf, sizeof(buf), "%lluKi", n / (1<<10));
    } else {
        snprintf(buf, sizeof(buf), "%llu", n);
    }
    return buf;
}

static const char* format_comma(llong n)
{
    static char buf[32];
    char buf1[32];

    snprintf(buf1, sizeof(buf1), "%llu", n);

    llong l = strlen(buf1), i = 0, j = 0;
    for (; i < l; i++, j++) {
        buf[j] = buf1[i];
        if ((l-i-1) % 3 == 0 && i != l -1) {
            buf[++j] = ',';
        }
    }
    buf[j] = '\0';

    return buf;
}

static std::string format_string(const char* fmt, ...)
{
    std::vector<char> buf;
    va_list args1, args2;
    int len, ret;

    va_start(args1, fmt);
    len = vsnprintf(NULL, 0, fmt, args1);
    assert(len >= 0);
    va_end(args1);

    buf.resize(len + 1);
    va_start(args2, fmt);
    ret = vsnprintf(buf.data(), buf.capacity(), fmt, args2);
    assert(len == ret);
    va_end(args2);
    
    return std::string(buf.data(), len);
}

static void print_header(std::string name, size_t runs, size_t lanes, size_t word_size, size_t array_size)
{
    int w = 2;
    size_t total_size = word_size * array_size;
    size_t vec_size = word_size * lanes;
    printf("\n");
    printf("name        : %-20s %*s test runs   : %-20zu\n",
        name.c_str(), w, "", runs);
    printf("array size  : %-20s %*s word size   : %-20s\n",
        format_string("%zu items", array_size).c_str(), w, "", format_string("%zu bits", word_size * 8).c_str());
    printf("vector size : %-20s %*s lane count  : %-20zu\n",
        format_string("%zu bits", vec_size * 8).c_str(), w, "", lanes);
    printf("data size   : %-20s %*s loop count  : %-20zu\n",
        format_string("%sB", format_binary(total_size)).c_str(), w, "", array_size/lanes);
    printf("\n");
    printf("%-16s %8s %8s %8s %8s %10s %16s %10s\n",
        "benchmark",
        "runs",
        "size(B)",
        "cyc(L)",
        "cyc(B)",
        "time(s)",
        "word/sec",
        "MiB/s"
    );

    printf("%-16s %8s %8s %8s %8s %10s %16s %10s\n",
        "----------------",
        "--------",
        "--------",
        "--------",
        "--------",
        "----------",
        "----------------",
        "----------"
    );
}

static void print_footer()
{
    printf("\n");
}

static const float cycles_ns = 4.3f; /* core i9-7980XE AVX512 fine-tuned */

template <typename P>
static std::map<std::string,double> print_result(std::string name, size_t runs, size_t lanes, size_t word_size, size_t array_size, P p1, P p2)
{
    size_t total_size = word_size * array_size;
    size_t vec_size = word_size * lanes;
    double t = (double)duration_cast<nanoseconds>(p2 - p1).count();
    double ns_byte = t / (runs * total_size);
    double ns_lane = ns_byte * vec_size;
    double cycles_byte = ns_byte * cycles_ns;
    double cycles_lane = ns_lane * cycles_ns;
    double time_sec = t / 1e9;
    double word_sec = runs * array_size * (1e9 / t);
    double mib_sec = runs * total_size * (1e9 / t) / (1024*1024);
    printf("%-16s %8s %8s %8.2f %8.2f %10.6f %16s %10.3f\n",
        name.c_str(),
        format_unit(runs),
        format_string("%sB", format_binary(total_size)).c_str(),
        cycles_lane,
        cycles_byte,
        time_sec,
        format_comma((llong)word_sec),
        mib_sec
    );

    std::map<std::string,double> result;
    result[format_string("%s_cycles_lane", name.c_str())] = cycles_lane;
    result[format_string("%s_cycles_byte", name.c_str())] = cycles_byte;
    result[format_string("%s_ns_lane", name.c_str())] = ns_lane;
    result[format_string("%s_ns_byte", name.c_str())] = ns_byte;
    result[format_string("%s_time_sec", name.c_str())] = time_sec;
    result[format_string("%s_word_sec", name.c_str())] = word_sec;
    result[format_string("%s_mib_sec", name.c_str())] = mib_sec;
    return result;
}

static std::map<std::string,double> merge(std::map<std::string,double> a, std::map<std::string,double> b)
{
    std::map<std::string,double> c;
    for (const auto& [k, v] : a) {
        c[k] = v;
    }
    for (const auto& [k, v] : b) {
        if (c.find(k) != c.end()) {
            if (c[k] > v) c[k] = v;
        } else {
            c[k] = v;
        }
    }
    return c;
}

template <typename T, typename R>
static std::map<std::string,double> bench_array_copy(size_t runs, size_t n, T(R::*func)())
{
    T *in, *out;
    T x1, x2;
    std::map<std::string,double> results{};

    if (posix_memalign((void**)&in, 64, n * sizeof(T)) < 0) abort();
    if (posix_memalign((void**)&out, 64, n * sizeof(T)) < 0) abort();
    array_random(in, n, func);

    x1 = array_sum(in, n);
    memcpy(out, in, n * sizeof(T));
    auto t1 = high_resolution_clock::now();
    for (size_t i = 0; i < runs; i++) {
        memcpy(out, in, n * sizeof(T));
    }
    auto t2 = high_resolution_clock::now();
    x2 = array_sum(out, n);

    results = merge(results, print_result("memcpy", runs, HWY_LANES(T), sizeof(T), n, t1, t2));

    free(in);
    free(out);

    if (x1 != x2) abort();

    return results;
}

template <typename T, typename R>
static std::map<std::string,double> bench_array_scan(size_t runs, size_t n, const char *suffix, T(R::*func)())
{
    T *in, *out;
    zvec_stats<T> sa, sr, sb;
    T x1, x2;
    void *comp;
    std::map<std::string,double> results{};

    if (posix_memalign((void**)&in, 64, n * sizeof(T)) < 0) abort();
    if (posix_memalign((void**)&out, 64, n * sizeof(T)) < 0) abort();
    array_random(in, n, func);

    sa = zvec_block_scan_abs(in, n);
    auto t1 = high_resolution_clock::now();
    for (size_t i = 0; i < runs; i++) {
        sa = zvec_block_scan_abs(in, n);
    }
    auto t2 = high_resolution_clock::now();

    sr = zvec_block_scan_rel(in, n);
    auto t3 = high_resolution_clock::now();
    for (size_t i = 0; i < runs; i++) {
        sr = zvec_block_scan_rel(in, n);
    }
    auto t4 = high_resolution_clock::now();

    sb = zvec_block_scan_both(in, n);
    auto t5 = high_resolution_clock::now();
    for (size_t i = 0; i < runs; i++) {
        sb = zvec_block_scan_both(in, n);
    }
    auto t6 = high_resolution_clock::now();

    assert(sr.dmin == sb.dmin);
    assert(sa.amin == sb.amin);

    results = merge(results, print_result("scan_abs", runs, HWY_LANES(T), sizeof(T), n, t1, t2));
    results = merge(results, print_result("scan_rel", runs, HWY_LANES(T), sizeof(T), n, t3, t4));
    results = merge(results, print_result("scan_both", runs, HWY_LANES(T), sizeof(T), n, t5, t6));

    free(in);
    free(out);
    free(comp);

    return results;
}

template <typename T, typename R>
static std::map<std::string,double> bench_array_abs(size_t runs, size_t n, const char *suffix, T(R::*func)())
{
    T *in, *out;
    zvec_stats<T> s;
    zvec_format f;
    zvec_meta<T> m;
    size_t a;
    T x1, x2;
    void *comp;
    std::map<std::string,double> results{};

    if (posix_memalign((void**)&in, 64, n * sizeof(T)) < 0) abort();
    if (posix_memalign((void**)&out, 64, n * sizeof(T)) < 0) abort();
    array_random(in, n, func);
    x1 = array_sum(in, n);
    s = zvec_block_scan_abs(in, n);
    f = zvec_block_format(s);
    m = zvec_block_metadata(s);
    a = zvec_block_size<T>(f, n);
    if (posix_memalign(&comp, 64, a) < 0) abort();

    zvec_block_encode(in, comp, n, f, m);
    auto t1 = high_resolution_clock::now();
    for (size_t i = 0; i < runs; i++) {
        zvec_block_encode(in, comp, n, f, m);
    }
    auto t2 = high_resolution_clock::now();

    zvec_block_decode(out, comp, n, f, m);
    auto t3 = high_resolution_clock::now();
    for (size_t i = 0; i < runs; i++) {
        zvec_block_decode(out, comp, n, f, m);
    }
    auto t4 = high_resolution_clock::now();

    x2 = array_sum(out, n);

    results = merge(results, print_result(format_string("encode_abs_%s", suffix), runs, HWY_LANES(T), sizeof(T), n, t1, t2));
    results = merge(results, print_result(format_string("decode_abs_%s", suffix), runs, HWY_LANES(T), sizeof(T), n, t3, t4));

    free(in);
    free(out);
    free(comp);

    if (x1 != x2) abort();

    return results;
}

template <typename T, typename R>
static std::map<std::string,double> bench_array_rel(size_t runs, size_t n, const char *suffix, T(R::*func)())
{
    T *in, *out;
    zvec_stats<T> s;
    zvec_format f;
    zvec_meta<T> m;
    size_t a;
    T x1, x2;
    void *comp;
    std::map<std::string,double> results{};

    if (posix_memalign((void**)&in, 64, n * sizeof(T)) < 0) abort();
    if (posix_memalign((void**)&out, 64, n * sizeof(T)) < 0) abort();
    array_random(in, n, func);
    x1 = array_sum(in, n);
    s = zvec_block_scan_rel(in, n);
    f = zvec_block_format(s);
    m = zvec_block_metadata(s);
    a = zvec_block_size<T>(f, n);
    if (posix_memalign(&comp, 64, a) < 0) abort();

    zvec_block_encode(in, comp, n, f, m);
    auto t1 = high_resolution_clock::now();
    for (size_t i = 0; i < runs; i++) {
        zvec_block_encode(in, comp, n, f, m);
    }
    auto t2 = high_resolution_clock::now();

    zvec_block_decode(out, comp, n, f, m);
    auto t3 = high_resolution_clock::now();
    for (size_t i = 0; i < runs; i++) {
        zvec_block_decode(out, comp, n, f, m);
    }
    auto t4 = high_resolution_clock::now();

    x2 = array_sum(out, n);

    results = merge(results, print_result(format_string("encode_rel_%s", suffix), runs, HWY_LANES(T), sizeof(T), n, t1, t2));
    results = merge(results, print_result(format_string("decode_rel_%s", suffix), runs, HWY_LANES(T), sizeof(T), n, t3, t4));

    free(in);
    free(out);
    free(comp);

    if (x1 != x2) abort();

    return results;
}

template <typename T, typename R>
static std::map<std::string,double> bench_array_const(size_t runs, size_t n, const char *suffix, T(R::*func)())
{
    T *in, *out;
    zvec_stats<T> s;
    T x1, x2;
    void *comp;
    std::map<std::string,double> results{};

    if (posix_memalign((void**)&in, 64, n * sizeof(T)) < 0) abort();
    if (posix_memalign((void**)&out, 64, n * sizeof(T)) < 0) abort();
    array_random(in, n, func);
    x1 = array_sum(in, n);
    s = zvec_block_scan_rel(in, n);

    zvec_block_synth_both(out, n, s.iv, s.dmin);
    auto t1 = high_resolution_clock::now();
    for (size_t i = 0; i < runs; i++) {
        zvec_block_synth_both(out, n, s.iv, s.dmin);
    }
    auto t2 = high_resolution_clock::now();

    x2 = array_sum(out, n);

    results = merge(results, print_result(format_string("synth_%s", suffix), runs, HWY_LANES(T), sizeof(T), n, t1, t2));

    free(in);
    free(out);
    free(comp);

    if (x1 != x2) abort();

    return results;
}

template <typename T>
static void output_results(std::map<std::string,double> results, size_t runs, size_t n)
{
    static int line;
    if (data) {
        if (line++ == 0) {
            fprintf(data, "size\truns");
            for (const auto& [k, v] : results) {
                fprintf(data, "\t%s", k.c_str());
            }
            fprintf(data,"\n");
        }
        fprintf(data, "%zu\t%zu", n * sizeof(T), runs);
        int col = 0;
        for (const auto& [k, v] : results) {
            fprintf(data, "\t%.9g", v);
        }
        fprintf(data,"\n");
        fflush(data);
    }

}

bool run_bench(int num) { return bench_num == num || bench_num == -1; }

template <typename T = i64>
static void bench_array_combo(size_t runs, size_t n)
{
    std::map<std::string,double> results;

    size_t lanes = HWY_LANES(T);
    print_header("array-rel-64", runs, lanes, sizeof(T), n);

    if (run_bench(1)) results = merge(results, bench_array_copy<T>(runs, n, &bench_random<T>::i7));
    if (run_bench(2)) results = merge(results, bench_array_scan<T>(runs, n, "8", &bench_random<T>::i7));
    if (run_bench(3)) results = merge(results, bench_array_abs<T>(runs, n, "8", &bench_random<T>::i7));
    if (run_bench(4)) results = merge(results, bench_array_abs<T>(runs, n, "16", &bench_random<T>::i15));
    if (run_bench(5)) results = merge(results, bench_array_abs<T>(runs, n, "24", &bench_random<T>::i23));
    if (run_bench(6)) results = merge(results, bench_array_abs<T>(runs, n, "32", &bench_random<T>::i31));
    if (run_bench(7)) results = merge(results, bench_array_abs<T>(runs, n, "48", &bench_random<T>::i47));
    if (run_bench(8)) results = merge(results, bench_array_rel<T>(runs, n, "8", &bench_random<T>::i7));
    if (run_bench(9)) results = merge(results, bench_array_rel<T>(runs, n, "16", &bench_random<T>::i15));
    if (run_bench(10)) results = merge(results, bench_array_rel<T>(runs, n, "24", &bench_random<T>::i23));
    if (run_bench(11)) results = merge(results, bench_array_rel<T>(runs, n, "32", &bench_random<T>::i31));
    if (run_bench(12)) results = merge(results, bench_array_rel<T>(runs, n, "48", &bench_random<T>::i47));
    if (run_bench(13)) results = merge(results, bench_array_const<T>(runs, n, "con", &bench_con<T>::c5));
    if (run_bench(14)) results = merge(results, bench_array_const<T>(runs, n, "seq", &bench_seq<T>::s5));

    output_results<T>(results, runs, n);

    print_footer();
}

/* benchmark option processing */

void print_help(int argc, char **argv)
{
    fprintf(stderr,
        "Usage: %s [options] [args]\n"
        "  -h, --help                            command line help\n"
        "  -w, --output-file [file]              benchmark data file\n"
        "  -n, --bench-num [num]                 run specific benchmark\n"
        "  -a, --cpu-arch {generic,avx3}         override cpu detection\n",
        argv[0]);
}

bool check_param(bool cond, const char *param)
{
    if (cond) {
        printf("error: %s requires parameter\n", param);
    }
    return (help_text = cond);
}

bool match_opt(const char *arg, const char *opt, const char *longopt)
{
    return strcmp(arg, opt) == 0 || strcmp(arg, longopt) == 0;
}

void parse_options(int argc, char **argv)
{
    int i = 1;
    while (i < argc) {
        if (match_opt(argv[i], "-h", "--help")) {
            help_text = true;
            i++;
        } else if (match_opt(argv[i], "-w", "--output-file")) {
            if (check_param(++i == argc, "--output-file")) break;
            output_file = argv[i++];
        } else if (match_opt(argv[i], "-n", "--bench-num")) {
            if (check_param(++i == argc, "--bench-num")) break;
            bench_num = atoi(argv[i++]);
        } else if (match_opt(argv[i], "-a", "--cpu-arch")) {
            if (check_param(++i == argc, "--cpu-arch")) break;
            cpu_arch = argv[i++];
        } else {
            fprintf(stderr, "error: unknown option: %s\n", argv[i]);
            help_text = true;
            break;
        }
    }

    if (cpu_arch.size() > 0) {
        if (cpu_arch == "generic") zvec_set_override(zvec_arch_generic);
        else if (cpu_arch == "avx3") zvec_set_override(zvec_arch_x86_avx3);
        else {
            fprintf(stderr, "error: invalid cpu arch: %s\n", cpu_arch.c_str());
            help_text = true;
        }
    }

    if (help_text) {
        print_help(argc, argv);
        exit(1);
    }

}

int main(int argc, char **argv)
{
    parse_options(argc, argv);

    if (output_file.size() > 0) {
        data = fopen(output_file.c_str(), "w");
    }

    // comments show cache breaks for Core i9-7980XE
    // (Core i9-7980XE: L1 32KiB, L2 1024KiB, L3 25344KiB)
    bench_array_combo(1000000, 1<<7); // 1 KiB (L1)
    bench_array_combo(1000000, 1<<8); // 2 KiB (L1)
    bench_array_combo(1000000, 1<<9); // 4 KiB (L1)
    bench_array_combo(100000, 1<<10); // 8 KiB (L1)
    bench_array_combo(100000, 1<<11); // 16 KiB (L1 peak)
    bench_array_combo(100000, 1<<12); // 32 KiB (L2 boundary)
    bench_array_combo(10000, 1<<13); // 64 KiB (L2)
    bench_array_combo(10000, 1<<14); // 128 KiB (L2)
    bench_array_combo(10000, 1<<15); // 256 KiB (L2)
    bench_array_combo(1000, 1<<16); // 512 KiB (L2 peak)
    bench_array_combo(1000, 1<<17); // 1 MiB (L3 boundary)
    bench_array_combo(1000, 1<<18); // 2 MiB (L3)
    bench_array_combo(100, 1<<19); // 4 MiB (L3)
    bench_array_combo(100, 1<<20); // 8 MiB (L3)
    bench_array_combo(100, 1<<21); // 16 MiB (L3 peak)
    bench_array_combo(10, 1<<22); // 32 MiB (RAM boundary)
    bench_array_combo(10, 1<<23); // 64 MiB (RAM)
    bench_array_combo(10, 1<<24); // 128 MiB (RAM)
    bench_array_combo(1, 1<<25); // 256 MiB (RAM)
    bench_array_combo(1, 1<<26); // 512 MiB (RAM)
    bench_array_combo(1, 1<<27); // 1 GiB (RAM)

    if (data) {
        fclose(data);
    }
}
