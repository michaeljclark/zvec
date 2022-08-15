#include <cstdio>
#include <cstring>
#include <climits>
#include <cinttypes>

#include <hwy/highway.h>
#include <zvec_codecs.h>
#include <zvec_dispatch.h>
#include <zvec_block.h>

bool verbose = false;
int test_count = 0;
int pass_count = 0;

i64 test0_data_i64[32] __attribute__((aligned(64))) = { 0 };

i64 test1_data_i64[] __attribute__((aligned(64))) = {
     5,  5,  5,  5,
     5,  5,  5,  5,
     5,  5,  5,  5,
     5,  5,  5,  5,
     5,  5,  5,  5,
     5,  5,  5,  5,
     5,  5,  5,  5,
     5,  5,  5,  5
};

i64 test2_data_i64[] __attribute__((aligned(64))) = {
     5,  6,  7,  8,
     9, 10, 11, 12,
    13, 14, 15, 16,
    17, 18, 19, 20,
    21, 22, 23, 24,
    25, 26, 27, 28,
    29, 30, 31, 32,
    33, 34, 35, 36
};

i64 test3_data_i64[] __attribute__((aligned(64))) = {
    72, 70, 68, 66,
    64, 62, 60, 58,
    56, 54, 52, 50,
    48, 46, 44, 42,
    40, 38, 36, 34,
    32, 30, 28, 26,
    24, 22, 20, 18,
    16, 14, 12, 10
};

i64 test4_data_i64[] __attribute__((aligned(64))) = {
    13, 14, 15, 13,
    11,  9, 12, 15,
    18, 14, 10,  6,
    11, 16, 21, 15,
     9,  3, 10, 17,
    24, 32, 40, 48,
    39, 30, 21, 32,
    43, 54, 44,  0
};

i64 test5_data_i64[] __attribute__((aligned(64))) = {
    36, 28, 82, 92,
    90, 46, 56, 76,
    82, 64, 16, 46,
    68, 56, 84, 58,
    20, 56, 26, 24,
    18, 70, 46, 62,
    38, 70, 40, 72,
    36, 86, 48, 42
};

i64 test6_data_i64[] __attribute__((aligned(64))) = {
     2737, 24503,  1841, 22571,
    17358, 23390, 13090, 22112,
    23734, 15713, 27786, 22722,
    15363,  7601, 12648, 29487,
    25786, 26622,  5267, 20941,
    12574, 21041,  2750, 23943,
     9150, 28942,    38,  6935,
    20002,  1017, 11101,  7200,
};

i64 test7_data_i64[] __attribute__((aligned(64))) = {
    992994, 989867, 273769, 747962,
    866981, 228788,  63364, 239641,
    226455, 874047, 548608, 492347,
    585164, 305305, 247083, 601514,
    414348, 799000, 852673, 248168,
     18212, 918588, 627011, 519409,
    101768, 996411, 744105, 859356,
    181567, 110491, 967637, 848872,
};

i64 test8_data_i64[] __attribute__((aligned(64))) = {
    88596766, 83087019,  2762925, 60821993,
    74335111, 30727101, 84351574, 81510871,
    27389871,  7700565, 17442596, 17216836,
    87892788, 63698607, 89867671, 87203665,
    15992885, 18728848, 59455898, 48071469,
    94138769, 35772229, 31957911, 92835244,
    57778019, 40078585, 42550021, 86682957,
    47499301, 15523046, 55817440, 28397707,
};

i64 test9_data_i64[] __attribute__((aligned(64))) = {
    69529791293ll, 69577642707ll, 67631421956ll, 24413889239ll,
     3669836733ll, 74670882998ll, 32568637229ll,  8376201863ll,
    94400052348ll, 58554454790ll, 33847156623ll, 63688675928ll,
    46029309608ll,  5967952565ll, 25876430039ll, 51021822464ll,
    97123943854ll, 22164310876ll, 78721770133ll, 60303927074ll,
    82809236233ll, 88023486193ll, 19717952941ll, 65910403021ll,
    41904644799ll, 80044807540ll,  1384713267ll, 82416623378ll,
    73725855619ll, 17040971092ll, 52055970735ll, 97647384197ll,
};

i64 test0_result_i64[32] __attribute__((aligned(64)));
i64 test1_result_i64[32] __attribute__((aligned(64)));
i64 test2_result_i64[32] __attribute__((aligned(64)));
i64 test3_result_i64[32] __attribute__((aligned(64)));
i64 test4_result_i64[32] __attribute__((aligned(64)));
i64 test5_result_i64[32] __attribute__((aligned(64)));
i64 test6_result_i64[32] __attribute__((aligned(64)));
i64 test7_result_i64[32] __attribute__((aligned(64)));
i64 test8_result_i64[32] __attribute__((aligned(64)));
i64 test9_result_i64[32] __attribute__((aligned(64)));

i32 test0_data_i32[64] __attribute__((aligned(64))) = { 0 };

i32 test1_data_i32[] __attribute__((aligned(64))) = {
     5,  5,  5,  5,
     5,  5,  5,  5,
     5,  5,  5,  5,
     5,  5,  5,  5,
     5,  5,  5,  5,
     5,  5,  5,  5,
     5,  5,  5,  5,
     5,  5,  5,  5,
     5,  5,  5,  5,
     5,  5,  5,  5,
     5,  5,  5,  5,
     5,  5,  5,  5,
     5,  5,  5,  5,
     5,  5,  5,  5,
     5,  5,  5,  5,
     5,  5,  5,  5
};

i32 test2_data_i32[] __attribute__((aligned(64))) = {
     5,  6,  7,  8,
     9, 10, 11, 12,
    13, 14, 15, 16,
    17, 18, 19, 20,
    21, 22, 23, 24,
    25, 26, 27, 28,
    29, 30, 31, 32,
    33, 34, 35, 36,
    37, 38, 39, 40,
    41, 42, 43, 44,
    45, 46, 47, 48,
    49, 50, 51, 52,
    53, 54, 55, 56,
    57, 58, 59, 60,
    61, 62, 63, 64,
    65, 66, 67, 68
};

i32 test3_data_i32[] __attribute__((aligned(64))) = {
    72, 70, 68, 66,
    64, 62, 60, 58,
    56, 54, 52, 50,
    48, 46, 44, 42,
    40, 38, 36, 34,
    32, 30, 28, 26,
    24, 22, 20, 18,
    16, 14, 12, 10,
    72, 70, 68, 66,
    64, 62, 60, 58,
    56, 54, 52, 50,
    48, 46, 44, 42,
    40, 38, 36, 34,
    32, 30, 28, 26,
    24, 22, 20, 18,
    16, 14, 12, 10
};

i32 test4_data_i32[] __attribute__((aligned(64))) = {
    13, 14, 15, 13,
    11,  9, 12, 15,
    18, 14, 10,  6,
    11, 16, 21, 15,
     9,  3, 10, 17,
    24, 32, 40, 48,
    39, 30, 21, 32,
    43, 54, 44,  0,
    13, 14, 15, 13,
    11,  9, 12, 15,
    18, 14, 10,  6,
    11, 16, 21, 15,
     9,  3, 10, 17,
    24, 32, 40, 48,
    39, 30, 21, 32,
    43, 54, 44,  0
};

i32 test5_data_i32[] __attribute__((aligned(64))) = {
    36, 28, 82, 92,
    90, 46, 56, 76,
    82, 64, 16, 46,
    68, 56, 84, 58,
    20, 56, 26, 24,
    18, 70, 46, 62,
    38, 70, 40, 72,
    36, 86, 48, 42,
    36, 28, 82, 92,
    90, 46, 56, 76,
    82, 64, 16, 46,
    68, 56, 84, 58,
    20, 56, 26, 24,
    18, 70, 46, 62,
    38, 70, 40, 72,
    36, 86, 48, 42
};

i32 test6_data_i32[] __attribute__((aligned(64))) = {
     2737, 24503,  1841, 22571,
    17358, 23390, 13090, 22112,
    23734, 15713, 27786, 22722,
    15363,  7601, 12648, 29487,
    25786, 26622,  5267, 20941,
    12574, 21041,  2750, 23943,
     9150, 28942,    38,  6935,
    20002,  1017, 11101,  7200,
     2737, 24503,  1841, 22571,
    17358, 23390, 13090, 22112,
    23734, 15713, 27786, 22722,
    15363,  7601, 12648, 29487,
    25786, 26622,  5267, 20941,
    12574, 21041,  2750, 23943,
     9150, 28942,    38,  6935,
    20002,  1017, 11101,  7200,
};

i32 test7_data_i32[] __attribute__((aligned(64))) = {
    992994, 989867, 273769, 747962,
    866981, 228788,  63364, 239641,
    226455, 874047, 548608, 492347,
    585164, 305305, 247083, 601514,
    414348, 799000, 852673, 248168,
     18212, 918588, 627011, 519409,
    101768, 996411, 744105, 859356,
    181567, 110491, 967637, 848872,
    992994, 989867, 273769, 747962,
    866981, 228788,  63364, 239641,
    226455, 874047, 548608, 492347,
    585164, 305305, 247083, 601514,
    414348, 799000, 852673, 248168,
     18212, 918588, 627011, 519409,
    101768, 996411, 744105, 859356,
    181567, 110491, 967637, 848872,
};

i32 test0_result_i32[64] __attribute__((aligned(64)));
i32 test1_result_i32[64] __attribute__((aligned(64)));
i32 test2_result_i32[64] __attribute__((aligned(64)));
i32 test3_result_i32[64] __attribute__((aligned(64)));
i32 test4_result_i32[64] __attribute__((aligned(64)));
i32 test5_result_i32[64] __attribute__((aligned(64)));
i32 test6_result_i32[64] __attribute__((aligned(64)));
i32 test7_result_i32[64] __attribute__((aligned(64)));

template <typename T>
void array_print(T * __restrict r, size_t n, int w)
{
    printf("{ ");
    for (size_t i = 0; i < n; i++) {
        if (i*w > 156) { printf(" …"); break; }
        printf("%*" PRId64, w, (i64)r[i]);
    }
    printf(" }\n");
}

template <>
void array_print(i24 * __restrict r, size_t n, int w)
{
    printf("{ ");
    for (size_t i = 0; i < n; i++) {
        if (i*w > 156) { printf(" …"); break; }
        i24 x = r[i];
        i64 y = (i64)(u8)x.raw[0] |
               ((i64)(u8)x.raw[1] << 8) |
               ((i64)(i8)x.raw[2] << 16);
        printf("%*" PRId64, w, y);
    }
    printf(" }\n");
}

template <>
void array_print(i48 * __restrict r, size_t n, int w)
{
    printf("{ ");
    for (size_t i = 0; i < n; i++) {
        if (i*w > 156) { printf(" …"); break; }
        i48 x = r[i];
        i64 y = (i64)(u16)x.raw[0] |
               ((i64)(u16)x.raw[1] << 16) |
               ((i64)(i16)x.raw[2] << 32);
        printf("%*" PRId64, w, y);
    }
    printf(" }\n");
}

template <typename T, size_t N>
void test_synth_rel(T (&in)[N], T (&out)[N], zvec_stats<T> s, int w)
{
    if (verbose) {
        printf("%-*s", 16, "enc-rel-0");
        printf("{ iv=%-*" PRId64 " 𝛿=%-*" PRId64 " }\n", w, (i64)s.iv, w, (i64)s.dmin);
    }
    zvec_ll_block_synth_both(out, N, s.iv, s.dmin);
    if (verbose) {
        printf("%-*s", 16, "dec-rel-0");
        array_print(out, N, w);
    }
}

template <typename T, size_t N>
void test_synth_abs(T (&in)[N], T (&out)[N], zvec_stats<T> s, int w)
{
    if (verbose) {
        printf("%-*s", 16, "enc-abs-0");
        printf("{ iv=%-*" PRId64 " }\n", w, (i64)s.iv);
    }
    zvec_ll_block_synth_abs(out, N, s.iv);
    if (verbose) {
        printf("%-*s", 16, "dec-abs-0");
        array_print(out, N, w);
    }
}

template <typename T, typename S, size_t N>
void test_codec_rel(T (&in)[N], S* comp, T (&out)[N], zvec_stats<T> s, int w)
{
    zvec_ll_block_encode_rel(in, comp, N, s.iv);
    if (verbose) {
        char namebuf[16];
        snprintf(namebuf, sizeof(namebuf), "enc-rel-%zu-%zu", sizeof(T) * 8, sizeof(S) * 8);
        printf("%-*s", 16, namebuf);
        array_print(comp, N, w);
    }
    zvec_ll_block_decode_rel(out, comp, N, s.iv);
    if (verbose) {
        char namebuf[16];
        snprintf(namebuf, sizeof(namebuf), "dec-rel-%zu-%zu", sizeof(T) * 8, sizeof(S) * 8);
        printf("%-*s", 16, namebuf);
        array_print(out, N, w);
    }
}

template <typename T, typename S, size_t N>
void test_codec_abs(T (&in)[N], S* comp, T (&out)[N], zvec_stats<T> s, int w)
{
    zvec_ll_block_encode_abs(in, comp, N);
    if (verbose) {
        char namebuf[16];
        snprintf(namebuf, sizeof(namebuf), "enc-abs-%zu-%zu", sizeof(T) * 8, sizeof(S) * 8);
        printf("%-*s", 16, namebuf);
        array_print(comp, N, w);
    }
    zvec_ll_block_decode_abs(out, comp, N);
    if (verbose) {
        char namebuf[16];
        snprintf(namebuf, sizeof(namebuf), "dec-abs-%zu-%zu", sizeof(T) * 8, sizeof(S) * 8);
        printf("%-*s", 16, namebuf);
        array_print(out, N, w);
    }
}

static void print_result(const char *test_name, bool result)
{
    if (!verbose) printf("%s: ", test_name);
    printf("%s\n", result ? "PASS" : "FAIL");
    if (result) pass_count++;
    test_count++;
}

template <typename T, size_t N>
void test_codec_rel(T (&in)[N], T (&out)[N], int w)
{
    using S = typename std::make_signed<T>::type;
    zvec_stats<S> s = zvec_ll_block_scan_rel(in, N);
    int d_size = zvec_size_bits(zvec_size_rel(s));

    char testname[32];
    snprintf(testname, sizeof(testname), "test-codec-rel-%zu-%d ", sizeof(T) * 8, d_size);

    if (verbose) {
        printf("=== %s ===\n", testname);
        printf("%-*s", 16, "scan");
        printf("iters=%-5zu lanes=%-5zu width=%-5zu iv=%-18" PRId64
            "𝛿min=%-18" PRId64 "𝛿max=%-18" PRId64 "encode-width=%-5d ratio=%3.1f\n",
            N/HWY_LANES(T), HWY_LANES(T), sizeof(T) << 3, (i64)s.iv, (i64)s.dmin, (i64)s.dmax,
            d_size, s.dmin == s.dmax ? 512.f : sizeof(T) * 8.f / d_size);
        printf("%-*s", 16, "input");
        array_print(in, N, w);
    }

    void *comp;
    if (posix_memalign(&comp, 64, (N * d_size) >> 3) < 0) abort();
    if constexpr (sizeof(T) == 8) {
        switch (d_size) {
            case 0: test_synth_rel(in, out, s, w); break;
            case 8: test_codec_rel(in, (i8*)comp, out, s, w); break;
            case 16: test_codec_rel(in, (i16*)comp, out, s, w); break;
            case 24: test_codec_rel(in, (i24*)comp, out, s, w); break;
            case 32: test_codec_rel(in, (i32*)comp, out, s, w); break;
            case 48: test_codec_rel(in, (i48*)comp, out, s, w); break;
            default: break;
        }
    }
    if constexpr (sizeof(T) == 4) {
        switch (d_size) {
            case 0: test_synth_rel(in, out, s, w); break;
            case 8: test_codec_rel(in, (i8*)comp, out, s, w); break;
            case 16: test_codec_rel(in, (i16*)comp, out, s, w); break;
            case 24: test_codec_rel(in, (i24*)comp, out, s, w); break;
            default: break;
        }
    }
    print_result(testname, memcmp(in, out, N * sizeof(T)) == 0);
    free(comp);
}

template <typename T, size_t N>
void test_codec_abs(T (&in)[N], T (&out)[N], int w)
{
    using S = typename std::make_signed<T>::type;
    zvec_stats<S> s = zvec_ll_block_scan_abs(in, N);
    int a_size = zvec_size_bits(zvec_size_abs(s));

    char testname[32];
    snprintf(testname, sizeof(testname), "test-codec-abs-%zu-%d ", sizeof(T) * 8, a_size);

    if (verbose) {
        printf("=== %s ===\n", testname);
        printf("%-*s", 16, "scan");
        printf("iters=%-5zu lanes=%-5zu width=%-5zu "
            "𝛿min=%-18" PRId64 "𝛿max=%-18" PRId64 "encode-width=%-5d ratio=%3.1f\n",
            N/HWY_LANES(T), HWY_LANES(T), sizeof(T) << 3, (i64)s.amin, (i64)s.amax,
            a_size, s.amin == s.amax ? 512.f : sizeof(T) * 8.f / a_size);
        printf("%-*s", 16, "input");
        array_print(in, N, w);
    }

    void *comp;
    if (posix_memalign(&comp, 64, (N * a_size) >> 3) < 0) abort();
    if constexpr (sizeof(T) == 8) {
        switch (a_size) {
            case 0: test_synth_abs(in, out, s, w); break;
            case 8: test_codec_abs(in, (i8*)comp, out, s, w); break;
            case 16: test_codec_abs(in, (i16*)comp, out, s, w); break;
            case 24: test_codec_abs(in, (i24*)comp, out, s, w); break;
            case 32: test_codec_abs(in, (i32*)comp, out, s, w); break;
            case 48: test_codec_abs(in, (i48*)comp, out, s, w); break;
            default: break;
        }
    }
    if constexpr (sizeof(T) == 4) {
        switch (a_size) {
            case 0: test_synth_abs(in, out, s, w); break;
            case 8: test_codec_abs(in, (i8*)comp, out, s, w); break;
            case 16: test_codec_abs(in, (i16*)comp, out, s, w); break;
            case 24: test_codec_abs(in, (i24*)comp, out, s, w); break;
            default: break;
        }
    }
    print_result(testname, memcmp(in, out, N * sizeof(T)) == 0);
    free(comp);
}

void test_codec_rel_i64()
{
    test_codec_rel(test0_data_i64, test0_result_i64, 3);
    test_codec_rel(test1_data_i64, test1_result_i64, 4);
    test_codec_rel(test2_data_i64, test2_result_i64, 4);
    test_codec_rel(test3_data_i64, test3_result_i64, 4);
    test_codec_rel(test4_data_i64, test4_result_i64, 4);
    test_codec_rel(test5_data_i64, test5_result_i64, 4);
    test_codec_rel(test6_data_i64, test6_result_i64, 7);
    test_codec_rel(test7_data_i64, test7_result_i64, 8);
    test_codec_rel(test8_data_i64, test8_result_i64, 9);
    test_codec_rel(test9_data_i64, test9_result_i64, 12);
}

void test_codec_abs_i64()
{
    test_codec_abs(test0_data_i64, test0_result_i64, 3);
    test_codec_abs(test1_data_i64, test1_result_i64, 4);
    test_codec_abs(test2_data_i64, test2_result_i64, 4);
    test_codec_abs(test3_data_i64, test3_result_i64, 4);
    test_codec_abs(test4_data_i64, test4_result_i64, 4);
    test_codec_abs(test5_data_i64, test5_result_i64, 4);
    test_codec_abs(test6_data_i64, test6_result_i64, 7);
    test_codec_abs(test7_data_i64, test7_result_i64, 8);
    test_codec_abs(test8_data_i64, test8_result_i64, 9);
    test_codec_abs(test9_data_i64, test9_result_i64, 12);
}

void test_codec_rel_i32()
{
    test_codec_rel(test0_data_i32, test0_result_i32, 3);
    test_codec_rel(test1_data_i32, test1_result_i32, 4);
    test_codec_rel(test2_data_i32, test2_result_i32, 4);
    test_codec_rel(test3_data_i32, test3_result_i32, 4);
    test_codec_rel(test4_data_i32, test4_result_i32, 4);
    test_codec_rel(test5_data_i32, test5_result_i32, 4);
    test_codec_rel(test6_data_i32, test6_result_i32, 7);
    test_codec_rel(test7_data_i32, test7_result_i32, 8);
}

void test_codec_abs_i32()
{
    test_codec_abs(test0_data_i32, test0_result_i32, 3);
    test_codec_abs(test1_data_i32, test1_result_i32, 4);
    test_codec_abs(test2_data_i32, test2_result_i32, 4);
    test_codec_abs(test3_data_i32, test3_result_i32, 4);
    test_codec_abs(test4_data_i32, test4_result_i32, 4);
    test_codec_abs(test5_data_i32, test5_result_i32, 4);
    test_codec_abs(test6_data_i32, test6_result_i32, 7);
    test_codec_abs(test7_data_i32, test7_result_i32, 8);
}

int main(int argc, char **argv)
{
    verbose = (argc == 2 && strcmp(argv[1], "-v") == 0);
    test_codec_rel_i64();
    test_codec_abs_i64();
    test_codec_rel_i32();
    test_codec_abs_i32();
    printf("result: %d/%d\n", pass_count, test_count);
}
