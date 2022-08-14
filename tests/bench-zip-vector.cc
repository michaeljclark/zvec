#undef NDEBUG
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <climits>
#include <cinttypes>
#include <cassert>

#include <zip_vector.h>

#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <utility>

using namespace std::chrono;

using timepoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

static std::string cpu_arch;
static bool help_text = false;
static int bench_num = -1;
static size_t bench_size = 128 * 1024 * 1024;

struct bench_result
{
    std::string name;
    size_t n;
    timepoint p1;
    timepoint p2;
};

static std::vector<bench_result> results;

static const char* format_unit(i64 n)
{
    static char buf[32];
    if (n % 1000000000 == 0) {
        snprintf(buf, sizeof(buf), "%" PRId64 "G", n / 1000000000);
    } else if (n % 1000000 == 0) {
        snprintf(buf, sizeof(buf), "%" PRId64 "M", n / 1000000);
    } else if (n % 1000 == 0) {
        snprintf(buf, sizeof(buf), "%" PRId64 "K", n / 1000);
    } else {
        snprintf(buf, sizeof(buf), "%" PRId64 "", n);
    }
    return buf;
}

static const char* format_binary(i64 n)
{
    static char buf[32];
    if (n % (1<<30) == 0) {
        snprintf(buf, sizeof(buf), "%" PRId64 "Gi", n / (1<<30));
    } else if (n % (1<<20) == 0) {
        snprintf(buf, sizeof(buf), "%" PRId64 "Mi", n / (1<<20));
    } else if (n % (1<<10) == 0) {
        snprintf(buf, sizeof(buf), "%" PRId64 "Ki", n / (1<<10));
    } else {
        snprintf(buf, sizeof(buf), "%" PRId64 "", n);
    }
    return buf;
}

static const char* format_comma(i64 n)
{
    static char buf[32];
    char buf1[32];

    snprintf(buf1, sizeof(buf1), "%" PRId64 "", n);

    i64 l = strlen(buf1), i = 0, j = 0;
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

static void print_header()
{
    printf("%-30s %8s %8s %16s %10s\n",
        "benchmark",
        "size(W)",
        "time(ns)",
        "word/sec",
        "MiB/s"
    );

    printf("%-30s %8s %8s %16s %10s\n",
        "------------------------------",
        "--------",
        "--------",
        "----------------",
        "----------"
    );
}

static void print_footer()
{
        printf("\n");
}

static void print_result(const bench_result &r)
{
    size_t n = r.n;
    double t = (double)duration_cast<nanoseconds>(r.p2 - r.p1).count();
    double ns_byte = t / n;
    double time_nsec = t / n;
    double word_sec = n * (1e9 / t);
    double mib_sec = n * sizeof(float) * (1e9 / t) / (1024*1024);
    printf("%-30s %8s %8.3f %16s %10.3f\n",
        r.name.c_str(),
        format_string("%sW", format_binary(n)).c_str(),
        time_nsec,
        format_comma((i64)word_sec),
        mib_sec
    );
}

static void collect_result(bool last, const bench_result &r)
{
    if (results.size() > 0 && results.back().name == r.name) {
        bench_result &l = results.back();
        if ((r.p2 - r.p1) < (l.p2 - l.p1)) {
            results.pop_back();
            results.push_back(r);
        }
    } else {
        results.push_back(r);
    }
    if (last) {
        print_result(results.back());
    }
}

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

    T abs_i7()  { /*    0 + (-2^6:2^6-1)   */ return rng_dist_i7(rng_engine); }
    T abs_i15() { /*    0 + (-2^14:2^14-1) */ return rng_dist_i15(rng_engine); }
    T abs_i23() { /*    0 + (-2^22:2^22-1) */ return rng_dist_i23(rng_engine); }
    T abs_i31() { /*    0 + (-2^30:2^30-1) */ return rng_dist_i31(rng_engine); }
    T abs_i47() { /*    0 + (-2^46:2^46-1) */ return rng_dist_i47(rng_engine); }
    T rel_i7()  { /* 2^56 + (-2^6:2^6-1)   */ return (1ll << 56) + rng_dist_i7(rng_engine); }
    T rel_i15() { /* 2^56 + (-2^14:2^14-1) */ return (1ll << 56) + rng_dist_i15(rng_engine); }
    T rel_i23() { /* 2^56 + (-2^22:2^22-1) */ return (1ll << 56) + rng_dist_i23(rng_engine); }
    T rel_i31() { /* 2^56 + (-2^30:2^30-1) */ return (1ll << 56) + rng_dist_i31(rng_engine); }
    T rel_i47() { /* 2^56 + (-2^46:2^46-1) */ return (1ll << 56) + rng_dist_i47(rng_engine); }
};

template <typename T, typename R>
static __attribute__((noinline)) void bench_std_vector_1D(std::string suffix, size_t runs, size_t n, T(R::*func)())
{
    R rng;
    std::vector<T> vec;

    T x1 = 0, x2 = 0;

    vec.resize(n);
    for (size_t i = 0; i < n; i++) {
        x1 += (vec[i] = (rng.*func)());
    }

    for (auto x : vec) x2 += x;
    if (x1 != x2) abort();

    for (size_t h = 0; h < runs; h++) {
        x2 = 0;
        timepoint t1 = high_resolution_clock::now();
        for (auto x : vec) x2 += x;
        timepoint t2 = high_resolution_clock::now();
        if (x1 != x2) abort();
        collect_result(h == runs - 1,
            {format_string("std_vector_1D%s", suffix.c_str()), n, t1, t2});
    }
}

template <typename T, typename R>
static __attribute__((noinline)) void bench_zip_vector_1D(std::string suffix, size_t runs, size_t n, T(R::*func)())
{
    R rng;
    zip_vector<T> vec;

    T x1 = 0, x2 = 0;

    vec.resize(n);
    for (size_t i = 0; i < n; i++) {
        x1 += (vec[i] = (rng.*func)());
    }

    for (auto x : vec) x2 += x;
    if (x1 != x2) abort();

    for (size_t h = 0; h < runs; h++) {
        x2 = 0;
        timepoint t1 = high_resolution_clock::now();
        for (auto x : vec) x2 += x;
        timepoint t2 = high_resolution_clock::now();
        if (x1 != x2) abort();
        collect_result(h == runs - 1,
            {format_string("zip_vector_1D%s", suffix.c_str()), n, t1, t2});
    }
}

template <typename T, typename R>
static __attribute__((noinline)) void bench_zip_vector_2D(std::string suffix, size_t runs, size_t n, T(R::*func)())
{
    R rng;
    zip_vector<T> vec;

    T x1 = 0, x2 = 0;

    vec.resize(n);
    for (size_t i = 0; i < n; i++) {
        x1 += (vec[i] = (rng.*func)());
    }

    for (size_t i = 0; i < n; i += decltype(vec)::page_interval) {
        T *cur = &vec[i], *end = cur + decltype(vec)::page_interval;
        while(cur != end) x2 += *cur++;
    }
    if (x1 != x2) abort();

    for (size_t h = 0; h < runs; h++) {
        x2 = 0;
        timepoint t1 = high_resolution_clock::now();
        for (size_t i = 0; i < n; i += decltype(vec)::page_interval) {
            T *cur = &vec[i], *end = cur + decltype(vec)::page_interval;
            while(cur != end) x2 += *cur++;
        }
        timepoint t2 = high_resolution_clock::now();
        if (x1 != x2) abort();
        collect_result(h == runs - 1,
            {format_string("zip_vector_2D%s", suffix.c_str()), n, t1, t2});
    }
}

template <typename T, typename R>
static void bench_vector(std::string suffix, size_t runs, size_t n, T(R::*func)())
{
    bench_std_vector_1D(suffix, runs, n, func);
    bench_zip_vector_1D(suffix, runs, n, func);
    bench_zip_vector_2D(suffix, runs, n, func);
}

/* benchmark option processing */

void print_help(int argc, char **argv)
{
    fprintf(stderr,
        "Usage: %s [options] [args]\n"
        "  -h, --help                            command line help\n"
        "  -n, --bench-num [num]                 run specific benchmark\n"
        "  -s, --bench-size [size(K|M|G)?]       specify benchmark size\n"
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

size_t parse_size(const char *str)
{
    size_t l = strlen(str);
    size_t n = (size_t)atoll(str);
    if (l >= 2 && isdigit(str[l-2])) {
        switch (str[l-1]) {
        case 'G': n *= 1024;
        case 'M': n *= 1024;
        case 'K': n *= 1024;
        default: break;
        }
    }
    return n;
}

void parse_options(int argc, char **argv)
{
    int i = 1;
    while (i < argc) {
        if (match_opt(argv[i], "-h", "--help")) {
            help_text = true;
            i++;
        } else if (match_opt(argv[i], "-n", "--bench-num")) {
            if (check_param(++i == argc, "--bench-num")) break;
            bench_num = atoi(argv[i++]);
        } else if (match_opt(argv[i], "-s", "--bench-size")) {
            if (check_param(++i == argc, "--bench-size")) break;
            bench_size = parse_size(argv[i++]);
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

bool run_bench(int num) { return bench_num == num || bench_num == -1; }

int main(int argc, char **argv)
{
    parse_options(argc, argv);
    print_header();
    if (run_bench(1)) bench_vector<i64>("-abs-8", 5, bench_size, &bench_random<i64>::abs_i7);
    if (run_bench(2)) bench_vector<i64>("-rel-8", 5, bench_size, &bench_random<i64>::rel_i7);
    if (run_bench(3)) bench_vector<i64>("-abs-16", 5, bench_size, &bench_random<i64>::abs_i15);
    if (run_bench(4)) bench_vector<i64>("-rel-16", 5, bench_size, &bench_random<i64>::rel_i15);
    if (run_bench(5)) bench_vector<i64>("-abs-24", 5, bench_size, &bench_random<i64>::abs_i23);
    if (run_bench(6)) bench_vector<i64>("-rel-24", 5, bench_size, &bench_random<i64>::rel_i23);
    if (run_bench(7)) bench_vector<i64>("-abs-32", 5, bench_size, &bench_random<i64>::abs_i31);
    if (run_bench(8)) bench_vector<i64>("-rel-32", 5, bench_size, &bench_random<i64>::rel_i31);
    if (run_bench(9)) bench_vector<i64>("-abs-48", 5, bench_size, &bench_random<i64>::abs_i47);
    if (run_bench(10)) bench_vector<i64>("-rel-48", 5, bench_size, &bench_random<i64>::rel_i47);
    print_footer();
}
