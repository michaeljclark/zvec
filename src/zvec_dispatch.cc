/*
 * PLEASE LICENSE 2022, Michael Clark <michaeljclark@mac.com>
 *
 * All rights to this work are granted for all purposes, with exception of
 * author's implied right of copyright to defend the free use of this work.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <cstdlib>
#include <cstring>

#include "zvec_codecs.h"
#include "zvec_dispatch.h"

#undef ZVECTOR_ARCH
#define ZVECTOR_ARCH generic
#include "zvec_arch.inc"

#ifdef ZVEC_HAS_AVX3
#undef ZVECTOR_ARCH
#define ZVECTOR_ARCH x86_avx3
#include "zvec_arch.inc"
#endif

#if defined __GNUC__ && (defined __i386__ || defined __x86_64__)
#define HAS_X86_CPUID 1
#include <cpuid.h>
inline void x86_cpuid(int reg[], int level)
{ __cpuid(level, reg[0], reg[1], reg[2], reg[3]); }
inline void x86_cpuid_count(int reg[], int level, int count)
{ __cpuid_count(level, count, reg[0], reg[1], reg[2], reg[3]); }
#elif defined _MSC_VER && (defined _M_IX86 || defined _M_X64)
#define HAS_X86_CPUID 1
#define x86_cpuid __cpuid
#define x86_cpuid_count __cpuidex
#endif

static zvec_op_types_i64 zvec_ops_i64;
static zvec_op_types_u64 zvec_ops_u64;
static zvec_op_types_i32 zvec_ops_i32;
static zvec_op_types_u32 zvec_ops_u32;

#define ZVEC_INIT_ARCH(arch) \
static void ZVEC_FN2(zvec_init,arch)() { \
    zvec_ops_i64.encode_abs_x8 = &ZVEC_FN2(zvec_ll_block_encode_abs_i64_i8,arch); \
    zvec_ops_i64.encode_abs_x16 = &ZVEC_FN2(zvec_ll_block_encode_abs_i64_i16,arch); \
    zvec_ops_i64.encode_abs_x24 = &ZVEC_FN2(zvec_ll_block_encode_abs_i64_i24,arch); \
    zvec_ops_i64.encode_abs_x32 = &ZVEC_FN2(zvec_ll_block_encode_abs_i64_i32,arch); \
    zvec_ops_i64.encode_abs_x48 = &ZVEC_FN2(zvec_ll_block_encode_abs_i64_i48,arch); \
    zvec_ops_i64.decode_abs_x8 = &ZVEC_FN2(zvec_ll_block_decode_abs_i64_i8,arch); \
    zvec_ops_i64.decode_abs_x16 = &ZVEC_FN2(zvec_ll_block_decode_abs_i64_i16,arch); \
    zvec_ops_i64.decode_abs_x24 = &ZVEC_FN2(zvec_ll_block_decode_abs_i64_i24,arch); \
    zvec_ops_i64.decode_abs_x32 = &ZVEC_FN2(zvec_ll_block_decode_abs_i64_i32,arch); \
    zvec_ops_i64.decode_abs_x48 = &ZVEC_FN2(zvec_ll_block_decode_abs_i64_i48,arch); \
    zvec_ops_i64.encode_rel_x8 = &ZVEC_FN2(zvec_ll_block_encode_rel_i64_i8,arch); \
    zvec_ops_i64.encode_rel_x16 = &ZVEC_FN2(zvec_ll_block_encode_rel_i64_i16,arch); \
    zvec_ops_i64.encode_rel_x24 = &ZVEC_FN2(zvec_ll_block_encode_rel_i64_i24,arch); \
    zvec_ops_i64.encode_rel_x32 = &ZVEC_FN2(zvec_ll_block_encode_rel_i64_i32,arch); \
    zvec_ops_i64.encode_rel_x48 = &ZVEC_FN2(zvec_ll_block_encode_rel_i64_i48,arch); \
    zvec_ops_i64.decode_rel_x8 = &ZVEC_FN2(zvec_ll_block_decode_rel_i64_i8,arch); \
    zvec_ops_i64.decode_rel_x16 = &ZVEC_FN2(zvec_ll_block_decode_rel_i64_i16,arch); \
    zvec_ops_i64.decode_rel_x24 = &ZVEC_FN2(zvec_ll_block_decode_rel_i64_i24,arch); \
    zvec_ops_i64.decode_rel_x32 = &ZVEC_FN2(zvec_ll_block_decode_rel_i64_i32,arch); \
    zvec_ops_i64.decode_rel_x48 = &ZVEC_FN2(zvec_ll_block_decode_rel_i64_i48,arch); \
    zvec_ops_i64.scan_abs = &ZVEC_FN2(zvec_ll_block_scan_abs_i64,arch); \
    zvec_ops_i64.scan_rel = &ZVEC_FN2(zvec_ll_block_scan_rel_i64,arch); \
    zvec_ops_i64.scan_both = &ZVEC_FN2(zvec_ll_block_scan_both_i64,arch); \
    zvec_ops_i64.synth_abs = &ZVEC_FN2(zvec_ll_block_synth_abs_i64,arch); \
    zvec_ops_i64.synth_rel = &ZVEC_FN2(zvec_ll_block_synth_rel_i64,arch); \
    zvec_ops_i64.synth_both = &ZVEC_FN2(zvec_ll_block_synth_both_i64,arch); \
    zvec_ops_u64.encode_abs_x8 = &ZVEC_FN2(zvec_ll_block_encode_abs_u64_u8,arch); \
    zvec_ops_u64.encode_abs_x16 = &ZVEC_FN2(zvec_ll_block_encode_abs_u64_u16,arch); \
    zvec_ops_u64.encode_abs_x24 = &ZVEC_FN2(zvec_ll_block_encode_abs_u64_u24,arch); \
    zvec_ops_u64.encode_abs_x32 = &ZVEC_FN2(zvec_ll_block_encode_abs_u64_u32,arch); \
    zvec_ops_u64.encode_abs_x48 = &ZVEC_FN2(zvec_ll_block_encode_abs_u64_u48,arch); \
    zvec_ops_u64.decode_abs_x8 = &ZVEC_FN2(zvec_ll_block_decode_abs_u64_u8,arch); \
    zvec_ops_u64.decode_abs_x16 = &ZVEC_FN2(zvec_ll_block_decode_abs_u64_u16,arch); \
    zvec_ops_u64.decode_abs_x24 = &ZVEC_FN2(zvec_ll_block_decode_abs_u64_u24,arch); \
    zvec_ops_u64.decode_abs_x32 = &ZVEC_FN2(zvec_ll_block_decode_abs_u64_u32,arch); \
    zvec_ops_u64.decode_abs_x48 = &ZVEC_FN2(zvec_ll_block_decode_abs_u64_u48,arch); \
    zvec_ops_u64.encode_rel_x8 = &ZVEC_FN2(zvec_ll_block_encode_rel_u64_u8,arch); \
    zvec_ops_u64.encode_rel_x16 = &ZVEC_FN2(zvec_ll_block_encode_rel_u64_u16,arch); \
    zvec_ops_u64.encode_rel_x24 = &ZVEC_FN2(zvec_ll_block_encode_rel_u64_u24,arch); \
    zvec_ops_u64.encode_rel_x32 = &ZVEC_FN2(zvec_ll_block_encode_rel_u64_u32,arch); \
    zvec_ops_u64.encode_rel_x48 = &ZVEC_FN2(zvec_ll_block_encode_rel_u64_u48,arch); \
    zvec_ops_u64.decode_rel_x8 = &ZVEC_FN2(zvec_ll_block_decode_rel_u64_u8,arch); \
    zvec_ops_u64.decode_rel_x16 = &ZVEC_FN2(zvec_ll_block_decode_rel_u64_u16,arch); \
    zvec_ops_u64.decode_rel_x24 = &ZVEC_FN2(zvec_ll_block_decode_rel_u64_u24,arch); \
    zvec_ops_u64.decode_rel_x32 = &ZVEC_FN2(zvec_ll_block_decode_rel_u64_u32,arch); \
    zvec_ops_u64.decode_rel_x48 = &ZVEC_FN2(zvec_ll_block_decode_rel_u64_u48,arch); \
    zvec_ops_u64.scan_abs = &ZVEC_FN2(zvec_ll_block_scan_abs_u64,arch); \
    zvec_ops_u64.scan_rel = &ZVEC_FN2(zvec_ll_block_scan_rel_u64,arch); \
    zvec_ops_u64.scan_both = &ZVEC_FN2(zvec_ll_block_scan_both_u64,arch); \
    zvec_ops_u64.synth_abs = &ZVEC_FN2(zvec_ll_block_synth_abs_u64,arch); \
    zvec_ops_u64.synth_rel = &ZVEC_FN2(zvec_ll_block_synth_rel_u64,arch); \
    zvec_ops_u64.synth_both = &ZVEC_FN2(zvec_ll_block_synth_both_u64,arch); \
    zvec_ops_i32.encode_abs_x8 = &ZVEC_FN2(zvec_ll_block_encode_abs_i32_i8,arch); \
    zvec_ops_i32.encode_abs_x16 = &ZVEC_FN2(zvec_ll_block_encode_abs_i32_i16,arch); \
    zvec_ops_i32.encode_abs_x24 = &ZVEC_FN2(zvec_ll_block_encode_abs_i32_i24,arch); \
    zvec_ops_i32.decode_abs_x8 = &ZVEC_FN2(zvec_ll_block_decode_abs_i32_i8,arch); \
    zvec_ops_i32.decode_abs_x16 = &ZVEC_FN2(zvec_ll_block_decode_abs_i32_i16,arch); \
    zvec_ops_i32.decode_abs_x24 = &ZVEC_FN2(zvec_ll_block_decode_abs_i32_i24,arch); \
    zvec_ops_i32.encode_rel_x8 = &ZVEC_FN2(zvec_ll_block_encode_rel_i32_i8,arch); \
    zvec_ops_i32.encode_rel_x16 = &ZVEC_FN2(zvec_ll_block_encode_rel_i32_i16,arch); \
    zvec_ops_i32.encode_rel_x24 = &ZVEC_FN2(zvec_ll_block_encode_rel_i32_i24,arch); \
    zvec_ops_i32.decode_rel_x8 = &ZVEC_FN2(zvec_ll_block_decode_rel_i32_i8,arch); \
    zvec_ops_i32.decode_rel_x16 = &ZVEC_FN2(zvec_ll_block_decode_rel_i32_i16,arch); \
    zvec_ops_i32.decode_rel_x24 = &ZVEC_FN2(zvec_ll_block_decode_rel_i32_i24,arch); \
    zvec_ops_i32.scan_abs = &ZVEC_FN2(zvec_ll_block_scan_abs_i32,arch); \
    zvec_ops_i32.scan_rel = &ZVEC_FN2(zvec_ll_block_scan_rel_i32,arch); \
    zvec_ops_i32.scan_both = &ZVEC_FN2(zvec_ll_block_scan_both_i32,arch); \
    zvec_ops_i32.synth_abs = &ZVEC_FN2(zvec_ll_block_synth_abs_i32,arch); \
    zvec_ops_i32.synth_rel = &ZVEC_FN2(zvec_ll_block_synth_rel_i32,arch); \
    zvec_ops_i32.synth_both = &ZVEC_FN2(zvec_ll_block_synth_both_i32,arch); \
    zvec_ops_u32.encode_abs_x8 = &ZVEC_FN2(zvec_ll_block_encode_abs_u32_u8,arch); \
    zvec_ops_u32.encode_abs_x16 = &ZVEC_FN2(zvec_ll_block_encode_abs_u32_u16,arch); \
    zvec_ops_u32.encode_abs_x24 = &ZVEC_FN2(zvec_ll_block_encode_abs_u32_u24,arch); \
    zvec_ops_u32.decode_abs_x8 = &ZVEC_FN2(zvec_ll_block_decode_abs_u32_u8,arch); \
    zvec_ops_u32.decode_abs_x16 = &ZVEC_FN2(zvec_ll_block_decode_abs_u32_u16,arch); \
    zvec_ops_u32.decode_abs_x24 = &ZVEC_FN2(zvec_ll_block_decode_abs_u32_u24,arch); \
    zvec_ops_u32.encode_rel_x8 = &ZVEC_FN2(zvec_ll_block_encode_rel_u32_u8,arch); \
    zvec_ops_u32.encode_rel_x16 = &ZVEC_FN2(zvec_ll_block_encode_rel_u32_u16,arch); \
    zvec_ops_u32.encode_rel_x24 = &ZVEC_FN2(zvec_ll_block_encode_rel_u32_u24,arch); \
    zvec_ops_u32.decode_rel_x8 = &ZVEC_FN2(zvec_ll_block_decode_rel_u32_u8,arch); \
    zvec_ops_u32.decode_rel_x16 = &ZVEC_FN2(zvec_ll_block_decode_rel_u32_u16,arch); \
    zvec_ops_u32.decode_rel_x24 = &ZVEC_FN2(zvec_ll_block_decode_rel_u32_u24,arch); \
    zvec_ops_u32.scan_abs = &ZVEC_FN2(zvec_ll_block_scan_abs_u32,arch); \
    zvec_ops_u32.scan_rel = &ZVEC_FN2(zvec_ll_block_scan_rel_u32,arch); \
    zvec_ops_u32.scan_both = &ZVEC_FN2(zvec_ll_block_scan_both_u32,arch); \
    zvec_ops_u32.synth_abs = &ZVEC_FN2(zvec_ll_block_synth_abs_u32,arch); \
    zvec_ops_u32.synth_rel = &ZVEC_FN2(zvec_ll_block_synth_rel_u32,arch); \
    zvec_ops_u32.synth_both = &ZVEC_FN2(zvec_ll_block_synth_both_u32,arch); \
}

static zvec_arch override_arch = zvec_arch_unspecified;

void zvec_set_override(zvec_arch arch) { override_arch = arch; }

ZVEC_INIT_ARCH(generic)

#if HAS_X86_CPUID
#ifdef ZVEC_HAS_AVX3
ZVEC_INIT_ARCH(x86_avx3)
#endif

union x86_cpuid_info
{
    struct { int eax, ebx, ecx, edx; };
    int arr[4];
};

static x86_cpuid_info h0;
static x86_cpuid_info h1;
static x86_cpuid_info h7_c0;

#define X86_HAS_AVX         (!!(h1.ecx & (1 << 28)))
#define X86_HAS_AVX512DQ    (!!(h7_c0.ebx & (1 << 17)))

static void zvec_init_x86()
{
    x86_cpuid(h0.arr, 0);
    x86_cpuid(h1.arr, 1);
    if (h0.eax >= 7) {
        x86_cpuid_count(h7_c0.arr, 7, 0);
    }

    switch (override_arch) {
    case zvec_arch_generic: zvec_init_generic(); return;
#ifdef ZVEC_HAS_AVX3
    case zvec_arch_x86_avx3: zvec_init_x86_avx3(); return;
#endif
    default: break;
    }

#ifdef ZVEC_HAS_AVX3
    if (X86_HAS_AVX512DQ) zvec_init_x86_avx3(); else
#endif
    zvec_init_generic();
}
#endif

static void zvec_init()
{
#if HAS_X86_CPUID
    zvec_init_x86();
#else
    zvec_init_generic();
#endif
}

zvec_op_types_i64* get_zvec_ops_i64()
{
    if (zvec_ops_i64.synth_rel == nullptr) zvec_init();
    return &zvec_ops_i64;
}

zvec_op_types_u64* get_zvec_ops_u64()
{
    if (zvec_ops_u64.synth_rel == nullptr) zvec_init();
    return &zvec_ops_u64;
}

zvec_op_types_i32* get_zvec_ops_i32()
{
    if (zvec_ops_i32.synth_rel == nullptr) zvec_init();
    return &zvec_ops_i32;
}

zvec_op_types_u32* get_zvec_ops_u32()
{
    if (zvec_ops_u32.synth_rel == nullptr) zvec_init();
    return &zvec_ops_u32;
}