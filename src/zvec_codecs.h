/*
 * PLEASE LICENSE 11/2020, Michael Clark <michaeljclark@mac.com>
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

#pragma once

#include <cstddef>
#include <cstdlib>
#include <cstdint>

#include <tuple>
#include <utility>
#include <algorithm>
#include <functional>
#include <type_traits>
#include <limits>

#include <hwy/highway.h>

#if defined(ZVECTOR_ARCH_X86_AVX3)
#define ZVECTOR_ARCH x86_avx3
#elif defined(ZVECTOR_ARCH_X86_AVX1)
#define ZVECTOR_ARCH x86_avx1
#define ZVECTOR_USE_SCALAR
#else
#define ZVECTOR_ARCH generic
#define ZVECTOR_USE_SCALAR
#endif

#define ZVEC_CAT4(a,b,c,d) a ## _ ## b ## _ ## c ## _ ## d
#define ZVEC_FN4(a,b,c,d) ZVEC_CAT4(a,b,c,d)
#define ZVEC_CAT3(a,b,c) a ## _ ## b ## _ ## c
#define ZVEC_FN3(a,b,c) ZVEC_CAT3(a,b,c)
#define ZVEC_CAT2(a,b) a ## _ ## b
#define ZVEC_FN2(a,b) ZVEC_CAT2(a,b)

#define ZVEC_ARCH_FN1(x) ZVEC_FN2(x,ZVECTOR_ARCH)
#define ZVEC_ARCH_FN2(x,y) ZVEC_FN3(x,y,ZVECTOR_ARCH)
#define ZVEC_ARCH_FN3(x,y,z) ZVEC_FN4(x,y,z,ZVECTOR_ARCH)

typedef long long llong;

constexpr size_t ilog2(size_t v)
{
	for (size_t i = 0; i < (sizeof(v) << 3); i++) {
		if (v <= (1ull << i)) return i;
	}
	return (sizeof(v) << 3);
}

template <auto Start, auto End, auto Inc, class F>
constexpr void constexpr_for(F&& f)
{
    if constexpr (Start < End)
    {
        f(std::integral_constant<decltype(Start), Start>());
        constexpr_for<Start + Inc, End, Inc>(f);
    }
}

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

struct si24 { i8 raw[3]; };
struct si48 { i16 raw[3]; };
struct ui24 { u8 raw[3]; };
struct ui48 { u16 raw[3]; };

typedef struct si24 i24;
typedef struct si48 i48;
typedef struct ui24 u24;
typedef struct ui48 u48;

using namespace hwy::HWY_NAMESPACE;

//#define VEC_DUMP
#undef VEC_DUMP

#ifdef VEC_DUMP
template <typename T>
static void zvec_dump(const char *name, T *r, size_t n)
{
    printf("%s = {", name);
    for (size_t i = 0; i < n; i++) {
        printf("%s%s%" PRId64,
            i > 0 ? "," : "", i % 16 == 0 ? "\n  " : " ",
            (i64)r[i]);
    }
    printf("\n}\n");
}

static void hex_dump(const char *name, i8 *r, size_t n)
{
    printf("%s = {", name);
    for (size_t i = 0; i < n; i++) {
        printf("%s%s0x%x",
            i > 0 ? "," : "", i % 16 == 0 ? "\n  " : " ",
            (u8)r[i]);
    }
    printf("\n}\n");
}

template <typename I, typename V>
static void vec_dump(const char *name, const char *fmt, V v)
{
    using D = DFromV<V>;
    using T = TFromD<D>;
    D d;
    const size_t N = sizeof(v)/sizeof(T);
    T arr[N];
    Store(v, d, arr);
    printf("%s = {", name);
    for (size_t i = 0; i < N; i++) {
        printf("%s ", i > 0 ? "," : "");
        printf(fmt, (I)arr[i]);
    }
    printf(" }\n");
}

template <typename I, typename V>
static void vec_dump(const char *name, const char *fmt, V v, size_t N)
{
    using D = DFromV<V>;
    using T = TFromD<D>;
    D d;
    const size_t Z = sizeof(v)/sizeof(T);
    T arr[Z];
    Store(v, d, arr);
    printf("%s = {", name);
    for (size_t i = 0; i < N; i++) {
        printf("%s ", i > 0 ? "," : "");
        printf(fmt, (I)arr[i]);
    }
    printf(" }\n");
}
#endif

enum zvec_codec
{
    zvec_codec_none = 0,
    zvec_block_rel = 1,
    zvec_block_abs = 2,
    zvec_block_rel_or_abs = 3,
    zvec_const_rel = 5,
    zvec_const_abs = 6,
};

template <typename T>
struct zvec_stats
{
    using TS = typename std::make_signed<T>::type;
    zvec_codec codec;
	T iv;
    TS dmin;
    TS dmax;
    T amin;
    T amax;
};

template <typename T>
zvec_stats<T> ZVEC_ARCH_FN1(zvec_ll_block_scan_abs)(T * __restrict x, size_t N)
{
    const ScalableTag<T> d;
    const RebindToSigned<decltype(d)> ds;

    const size_t L = Lanes(d);

    Vec<decltype(d)> v0 = Zero(d);
    Vec<decltype(d)> v1, v2, v3;
    Vec<decltype(d)> vamax = Set(d, hwy::LowestValue<T>());
    Vec<decltype(d)> vamin = Set(d, hwy::HighestValue<T>());

    for (size_t i = 0; i < N; i += L) {
        v1 = Load(d, x+i);
        vamin = Min(vamin, v1);
        vamax = Max(vamax, v1);
    }

    T amin = GetLane(MinOfLanes(d, vamin));
    T amax = GetLane(MaxOfLanes(d, vamax));
    T iv = amin == amax ? x[0] : 0;

    return zvec_stats<T>{ zvec_block_abs, iv, 0, 0, amin, amax };
}

template <typename T>
zvec_stats<T> ZVEC_ARCH_FN1(zvec_ll_block_scan_rel)(T * __restrict x, size_t N)
{
    const ScalableTag<T> d;
    const RebindToSigned<decltype(d)> ds;

    using TS = TFromD<decltype(ds)>;

    const size_t L = Lanes(d);

    Vec<decltype(d)> v0 = Zero(d);
    Vec<decltype(d)> v1, v2, v3;
    Vec<decltype(d)> vamax = Set(d, hwy::LowestValue<T>());
    Vec<decltype(d)> vamin = Set(d, hwy::HighestValue<T>());
    Vec<decltype(ds)> vdmax = Set(ds, hwy::LowestValue<TS>());
    Vec<decltype(ds)> vdmin = Set(ds, hwy::HighestValue<TS>());

    size_t i = 0;
    if (i < N) {
        v1 = Load(d, x+i);
        v2 = CombineShiftRightLanes<HWY_LANES(T)-1>(d, v1, v0);
        v3 = Sub(v1, v2);
        v0 = v1;
        // hoisted iteration 0 due to this exception where we duplicate
        // lane 1 into lane 0 because its zero delta makes it impossible
        // to detect constant delta sequences i.e. where min(𝛿) == max(𝛿).
        v3 = IfThenElse(FirstN(d, 1), DupOdd(v3), v3);
        vdmin = Min(vdmin, BitCast(ds, v3));
        vdmax = Max(vdmax, BitCast(ds, v3));
        i += L;
    }
    for (; i < N; i += L) {
    	v1 = Load(d, x+i);
        v2 = CombineShiftRightLanes<HWY_LANES(T)-1>(d, v1, v0);
        v3 = Sub(v1, v2);
        v0 = v1;
        vdmin = Min(vdmin, BitCast(ds, v3));
        vdmax = Max(vdmax, BitCast(ds, v3));
    }

    TS dmin = GetLane(MinOfLanes(ds, vdmin));
    TS dmax = GetLane(MaxOfLanes(ds, vdmax));
    T iv = dmin == dmax ? x[0] - (x[1] - x[0]) : x[0];

    return zvec_stats<T>{ zvec_block_rel, iv, dmin, dmax, 0, 0 };
}

template <typename T>
zvec_stats<T> ZVEC_ARCH_FN1(zvec_ll_block_scan_both)(T * __restrict x, size_t N)
{
    const ScalableTag<T> d;
    const RebindToSigned<decltype(d)> ds;

    using TS = TFromD<decltype(ds)>;

    const size_t L = Lanes(d);

    Vec<decltype(d)> v0 = Zero(d);
    Vec<decltype(d)> v1, v2, v3;
    Vec<decltype(d)> vamax = Set(d, hwy::LowestValue<T>());
    Vec<decltype(d)> vamin = Set(d, hwy::HighestValue<T>());
    Vec<decltype(ds)> vdmax = Set(ds, hwy::LowestValue<TS>());
    Vec<decltype(ds)> vdmin = Set(ds, hwy::HighestValue<TS>());

    size_t i = 0;
    if (i < N) {
        v1 = Load(d, x+i);
        vamin = Min(vamin, v1);
        vamax = Max(vamax, v1);
        v2 = CombineShiftRightLanes<HWY_LANES(T)-1>(d, v1, v0);
        v3 = Sub(v1, v2);
        v0 = v1;
        // hoisted iteration 0 due to this exception where we duplicate
        // lane 1 into lane 0 because its zero delta makes it impossible
        // to detect constant delta sequences i.e. where min(𝛿) == max(𝛿).
        v3 = IfThenElse(FirstN(d, 1), DupOdd(v3), v3);
        vdmin = Min(vdmin, BitCast(ds, v3));
        vdmax = Max(vdmax, BitCast(ds, v3));
        i += L;
    }
    for (; i < N; i += L) {
        v1 = Load(d, x+i);
        vamin = Min(vamin, v1);
        vamax = Max(vamax, v1);
        v2 = CombineShiftRightLanes<HWY_LANES(T)-1>(d, v1, v0);
        v3 = Sub(v1, v2);
        v0 = v1;
        vdmin = Min(vdmin, BitCast(ds, v3));
        vdmax = Max(vdmax, BitCast(ds, v3));
    }

    T amin = GetLane(MinOfLanes(d, vamin));
    T amax = GetLane(MaxOfLanes(d, vamax));
    TS dmin = GetLane(MinOfLanes(ds, vdmin));
    TS dmax = GetLane(MaxOfLanes(ds, vdmax));
    T iv = dmin == dmax ? x[0] - (x[1] - x[0]) : x[0];

    return zvec_stats<T>{ zvec_block_rel_or_abs, iv, dmin, dmax, amin, amax };
}

template <typename T>
void ZVEC_ARCH_FN1(zvec_ll_block_synth_abs)(T * __restrict x, size_t N, T iv)
{
    ScalableTag<T> d;

    const size_t L = Lanes(d);

    Vec<decltype(d)> v0 = Set(d, iv);
    for (size_t i = 0; i < N; i += L) {
        Store(v0, d, x+i);
    }
}

template <typename T>
void ZVEC_ARCH_FN1(zvec_ll_block_synth_rel)(T * __restrict x, size_t N, T iv, T dv)
{
    ScalableTag<T> d;

    const size_t L = Lanes(d);

    const auto shuf_last = IndicesFromVec(d, Set(d, L - 1));

    Vec<decltype(d)> v0 = Set(d, iv), v1 = Set(d, dv), v2 = Zero(d);
    constexpr_for<0, ilog2(HWY_LANES(T)), 1>([&](auto j){
        v1 = v1 + CombineShiftRightLanes<HWY_LANES(T)-(1 << j)>(d, v1, Zero(d));
    });
    for (size_t i = 0; i < N; i += L) {
        v2 = v1 + v0;
        v0 = TableLookupLanes(v2, shuf_last);
        Store(v2, d, x+i);
    }
}

template <typename T>
void ZVEC_ARCH_FN1(zvec_ll_block_synth_both)(T * __restrict x, size_t N, T iv, T dv)
{
    ScalableTag<T> d;

    const size_t L = Lanes(d);

    const auto shuf_last = IndicesFromVec(d, Set(d, L - 1));

    if (dv == 0) {
        Vec<decltype(d)> v0 = Set(d, iv);
        for (size_t i = 0; i < N; i += L) {
            Store(v0, d, x+i);
        }
    } else {
        Vec<decltype(d)> v0 = Set(d, iv), v1 = Set(d, dv), v2 = Zero(d);
        constexpr_for<0, ilog2(HWY_LANES(T)), 1>([&](auto j){
            v1 = v1 + CombineShiftRightLanes<HWY_LANES(T)-(1 << j)>(d, v1, Zero(d));
        });
        for (size_t i = 0; i < N; i += L) {
            v2 = v1 + v0;
            v0 = TableLookupLanes(v2, shuf_last);
            Store(v2, d, x+i);
        }
    }
}

template <typename T, typename S>
void ZVEC_ARCH_FN1(zvec_ll_block_encode_abs)(T * __restrict x, S * __restrict r, size_t N)
{
    using x32 = typename std::conditional<std::is_signed<T>::value,i32,u32>::type;

    const ScalableTag<T> d;
    const ScalableTag<x32> w;
    const ScalableTag<i32> ws;
    const Rebind<S, decltype(d)> dw;

    const size_t L = Lanes(d);
    const size_t K = Lanes(w);

    alignas(64) i32 idx_demote[K];
    for (size_t i = 0; i < K; i++) {
        idx_demote[i] = i % L * (K/L); /* little-endian dword-0 */
    }
    const auto shuf_demote = SetTableIndices(w, idx_demote);

    Vec<decltype(d)> v1;
    Vec<decltype(w)> v2;
    for (size_t i = 0; i < N; i += L) {
        v1 = Load(d, x+i);
        v2 = TableLookupLanes(BitCast(w, v1), shuf_demote);
        if constexpr (sizeof(S) == 4) {
            Store(LowerHalf(v2), dw, r+i);
        } else {
            Store(DemoteTo(dw, LowerHalf(BitCast(ws, v2))), dw, r+i);
        }
    }
}

template <typename T, typename S>
void ZVEC_ARCH_FN1(zvec_ll_block_decode_abs)(T * __restrict x, S * __restrict r, size_t N)
{
    const ScalableTag<T> d;
    const Rebind<S, decltype(d)> dw;

    const size_t L = Lanes(d);

    const auto shuf_last = IndicesFromVec(d, Set(d, L - 1));

    Vec<decltype(dw)> v1;
    Vec<decltype(d)> v2;
    for (size_t i = 0; i < N; i += L) {
        v1 = Load(dw, r+i);
        v2 = PromoteTo(d, v1);
        Store(v2, d, x+i);
    }
}

template <typename T, typename S>
void ZVEC_ARCH_FN1(zvec_ll_block_encode_rel)(T * __restrict x, S * __restrict r, size_t N, T iv)
{
    using x32 = typename std::conditional<std::is_signed<T>::value,i32,u32>::type;

    const ScalableTag<T> d;
    const ScalableTag<x32> w;
    const ScalableTag<i32> ws;
    const Rebind<S, decltype(d)> dw;

    const size_t L = Lanes(d);
    const size_t K = Lanes(w);

    alignas(64) i32 idx_demote[K];
    for (size_t i = 0; i < K; i++) {
        idx_demote[i] = i % L * (K/L); /* little-endian dword-0 */
    }
    const auto shuf_demote = SetTableIndices(w, idx_demote);

    Vec<decltype(d)> v0 = Set(d, iv);
    Vec<decltype(d)> v1, v2, v3;
    Vec<decltype(w)> v4;
    for (size_t i = 0; i < N; i += L) {
    	v1 = Load(d, x+i);
        v2 = CombineShiftRightLanes<HWY_LANES(T)-1>(d, v1, v0);
        v3 = Sub(v1, v2);
        v0 = v1;
        v4 = TableLookupLanes(BitCast(w, v3), shuf_demote);
        if constexpr (sizeof(S) == 4) {
            Store(LowerHalf(v4), dw, r+i);
        } else {
            Store(DemoteTo(dw, LowerHalf(BitCast(ws, v4))), dw, r+i);
        }
    }
}

template <typename T, typename S>
void ZVEC_ARCH_FN1(zvec_ll_block_decode_rel)(T * __restrict x, S * __restrict r, size_t N, T iv)
{
    const ScalableTag<T> d;
    const Rebind<S, decltype(d)> dw;

    const size_t L = Lanes(d);

    const auto shuf_last = IndicesFromVec(d, Set(d, L - 1));

    Vec<decltype(dw)> v1;
    Vec<decltype(d)> v0 = Set(d, iv), v2;
    for (size_t i = 0; i < N; i += L) {
    	v1 = Load(dw, r+i);
    	v2 = PromoteTo(d, v1);
    	constexpr_for<0, ilog2(HWY_LANES(T)), 1>([&](auto j){
    	 	v2 = v2 + CombineShiftRightLanes<HWY_LANES(T)-(1 << j)>(d, v2, Zero(d));
    	});
    	v2 = v2 + v0;
        v0 = TableLookupLanes(v2, shuf_last);
    	Store(v2, d, x+i);
    }
}

#if defined(ZVECTOR_USE_SCALAR)

template<zvec_codec codec, typename T>
void ZVEC_ARCH_FN2(zvec_ll_block_encode,x24)(T * __restrict x, typename std::conditional<std::is_signed<T>::value,i24,u24>::type * __restrict r, size_t N, i64 iv)
{
    T v0 = iv;
    T d0, d1, d2, d3, w0, w1, w2, w3;
    for (size_t i = 0, j = 0; i < N; i += 4, j += 3)
    {
        if constexpr (codec == zvec_block_rel)
        {
            d0 = x[i + 0];
            d1 = x[i + 1];
            d2 = x[i + 2];
            d3 = x[i + 3];

            w0 = d0 - v0;
            w1 = d1 - d0;
            w2 = d2 - d1;
            w3 = d3 - d2;

            v0 = d3;
        } else {
            w0 = x[i + 0];
            w1 = x[i + 1];
            w2 = x[i + 2];
            w3 = x[i + 3];
        }

        u32 r0 = (w0        & 0xffffff) | ((w1 & 0xff) << 24);
        u32 r1 = ((w1 >> 8) & 0xffff)   | ((w2 & 0xffff) << 16);
        u32 r2 = ((w2 >> 16) & 0xff)    | ((w3 & 0xffffff) << 8);

        *((u32*)r + j + 0) = r0;
        *((u32*)r + j + 1) = r1;
        *((u32*)r + j + 2) = r2;
    }
}

template<zvec_codec codec, typename T>
void ZVEC_ARCH_FN2(zvec_ll_block_decode,x24)(T * __restrict x, typename std::conditional<std::is_signed<T>::value,i24,u24>::type * __restrict r, size_t N, i64 iv)
{
    T v0 = iv;
    T r0, r1, r2, s0, s1, s2, s3;
    for (size_t i = 0, j = 0; i < N; i += 4, j += 3)
    {
        r0 = *((u32*)r + j + 0);
        r1 = *((u32*)r + j + 1);
        r2 = *((u32*)r + j + 2);

        s0 =                          (r0 & 0xffffff);
        s1 = ((r0 >> 24) & 0xff)   | ((r1 & 0xffff) << 8);
        s2 = ((r1 >> 16) & 0xffff) | ((r2 & 0xff) << 16);
        s3 =  (r2 >> 8);

        if (std::is_signed<T>::value) {
            s0 = (s0 << 40) >> 40;
            s1 = (s1 << 40) >> 40;
            s2 = (s2 << 40) >> 40;
            s3 = (s3 << 40) >> 40;
        }

        if constexpr (codec == zvec_block_rel)
        {
            s0 += v0;
            s1 += s0;
            s2 += s1;
            s3 += s2;

            v0 = s3;
        }

        x[i + 0] = s0;
        x[i + 1] = s1;
        x[i + 2] = s2;
        x[i + 3] = s3;
    }
}

template<zvec_codec codec, typename T>
void ZVEC_ARCH_FN2(zvec_ll_block_encode,x48)(T * __restrict x, typename std::conditional<std::is_signed<T>::value,i48,u48>::type * __restrict r, size_t N, T iv)
{
    T v0 = iv;
    T d0, d1, d2, d3, w0, w1, w2, w3;
    for (size_t i = 0, j = 0; i < N; i += 4, j += 3)
    {
        if constexpr (codec == zvec_block_rel)
        {
            d0 = x[i + 0];
            d1 = x[i + 1];
            d2 = x[i + 2];
            d3 = x[i + 3];

            w0 = d0 - v0;
            w1 = d1 - d0;
            w2 = d2 - d1;
            w3 = d3 - d2;

            v0 = d3;
        } else {
            w0 = x[i + 0];
            w1 = x[i + 1];
            w2 = x[i + 2];
            w3 = x[i + 3];
        }

        u64 r0 = ((w0 >> 0) & 0xffffffffffff) | ((w1 & 0xffff) << 48);
        u64 r1 = ((w1 >> 16) & 0xffffffff)   | ((w2 & 0xffffffff) << 32);
        u64 r2 = ((w2 >> 32) & 0xffff)    | ((w3 & 0xffffffffffff) << 16);

        *((u64*)r + j + 0) = r0;
        *((u64*)r + j + 1) = r1;
        *((u64*)r + j + 2) = r2;
    }
}

template<zvec_codec codec, typename T>
void ZVEC_ARCH_FN2(zvec_ll_block_decode,x48)(T * __restrict x, typename std::conditional<std::is_signed<T>::value,i48,u48>::type * __restrict r, size_t N, T iv)
{
    T v0 = iv;
    T r0, r1, r2, s0, s1, s2, s3;
    for (size_t i = 0, j = 0; i < N; i += 4, j += 3)
    {
        r0 = *((u64*)r + j + 0);
        r1 = *((u64*)r + j + 1);
        r2 = *((u64*)r + j + 2);

        s0 =                                 (r0 & 0xffffffffffffull);
        s1 = ((r0 >> 48) & 0xffffull)     | ((r1 & 0xffffffffull) << 16);
        s2 = ((r1 >> 32) & 0xffffffffull) | ((r2 & 0xffffull) << 32);
        s3 =  (r2 >> 16);

        if (std::is_signed<T>::value) {
            s0 = (s0 << 16) >> 16;
            s1 = (s1 << 16) >> 16;
            s2 = (s2 << 16) >> 16;
            s3 = (s3 << 16) >> 16;
        }

        if constexpr (codec == zvec_block_rel)
        {
            s0 += v0;
            s1 += s0;
            s2 += s1;
            s3 += s2;

            v0 = s3;
        }

        x[i + 0] = s0;
        x[i + 1] = s1;
        x[i + 2] = s2;
        x[i + 3] = s3;
    }
}

#else

template<zvec_codec codec, typename T>
void ZVEC_ARCH_FN2(zvec_ll_block_encode,x24)(T * __restrict x, typename std::conditional<std::is_signed<T>::value,i24,u24>::type * __restrict r, size_t N, i64 iv)
{
    const ScalableTag<T> d;
    const ScalableTag<i32> w;
    const ScalableTag<i8> b;
    const Rebind<i32, decltype(d)> dw;
    const Repartition<i8, decltype(dw)> db;

    const size_t L = Lanes(d);
    const size_t K = Lanes(w);
    const size_t W = Lanes(dw);
    const size_t V = Lanes(db);

    alignas(64) i32 idx_demote[K];
    alignas(64) i8 idx_enc[V];
    alignas(64) i8 idx_mask[V];
    Vec<decltype(db)> shuf_enc;
    Mask<decltype(db)> shuf_mask;

    for (size_t i = 0; i < K; i++) {
        idx_demote[i] = i % L * (K/L); /* little-endian dword-0 */
    }
    const auto shuf_demote = SetTableIndices(w, idx_demote);

    for (size_t i = 0; i < V; i++) {
        i8 x = (i % 3) + (i / 3) * 4;
        idx_enc[i] = x < V ? x : -1;
    }
    shuf_enc = Load(db, idx_enc);

    if constexpr (HWY_LANES(i8) > 16) {
        for (size_t i = 0; i < V; i++) {
            i8 x = (i % 3) + (i / 3) * 4;
            idx_mask[i] = (x / 16) == (i / 16) ? -1 : 0;
        }
        shuf_mask = MaskFromVec(Load(db, idx_mask));
    }

    auto delta = [&] (size_t i, Vec<decltype(d)> v0) -> std::tuple<Vec<decltype(d)>,Vec<decltype(dw)>> {
        auto v1 = Load(d, x+i);
        auto v2 = CombineShiftRightLanes<HWY_LANES(i64)-1>(d, v1, v0);
        auto v3 = Sub(v1, v2);
        auto v4 = LowerHalf(TableLookupLanes(BitCast(w, v3), shuf_demote));
        return std::tie(v1, v4);
    };

    auto demote = [&] (Vec<decltype(d)> v0) -> Vec<decltype(dw)> {
        return LowerHalf(TableLookupLanes(BitCast(w, v0), shuf_demote));
    };

    auto convert24 = [&] (Vec<decltype(dw)> v) -> Vec<decltype(w)>
    {
        if constexpr (HWY_LANES(i8) == 64) {
            Vec<decltype(dw)> u = CombineShiftRightLanes<4>(dw, Zero(dw), v);
            return ZeroExtendVector(w, BitCast(dw, IfThenElse(shuf_mask,
                TableLookupBytes(BitCast(db, v), shuf_enc),
                TableLookupBytes(BitCast(db, u), shuf_enc))));
        } else if constexpr (HWY_LANES(i8) == 16) {
            return ZeroExtendVector(w, BitCast(dw, TableLookupBytes(BitCast(db, v), shuf_enc)));
        }
    };

    Vec<decltype(d)> v0 = Set(d, iv);
    Vec<decltype(w)> w0, w1, w2, w3;
    Vec<decltype(w)> r0, r1, r2, r3;
    for (size_t i = 0, j = 0; i < N; i += L * 4, j+= L * 3)
    {
        if constexpr (codec == zvec_block_rel)
        {
            auto d0 = delta(i + L * 0, v0);
            auto d1 = delta(i + L * 1, std::get<0>(d0));
            auto d2 = delta(i + L * 2, std::get<0>(d1));
            auto d3 = delta(i + L * 3, std::get<0>(d2));

            v0 = std::get<0>(d3);

            w0 = convert24(std::get<1>(d0));
            w1 = convert24(std::get<1>(d1));
            w2 = convert24(std::get<1>(d2));
            w3 = convert24(std::get<1>(d3));
        }
        else
        {
            w0 = convert24(demote(Load(d, x + i + L * 0)));
            w1 = convert24(demote(Load(d, x + i + L * 1)));
            w2 = convert24(demote(Load(d, x + i + L * 2)));
            w3 = convert24(demote(Load(d, x + i + L * 3)));
        }

        if constexpr (HWY_LANES(i8) == 64)
        {
            r0 = IfThenElse(FirstN(w, 6),
                BitCast(w, w0),
                CombineShiftRightLanes<HWY_LANES(i32)-6>(w, BitCast(w, w1), Zero(w)));
            r1 = IfThenElse(FirstN(w, 4),
                CombineShiftRightLanes<2>(w, Zero(w), BitCast(w, w1)),
                CombineShiftRightLanes<HWY_LANES(i32)-4>(w, BitCast(w, w2), Zero(w)));
            r2 = IfThenElse(FirstN(w, 2),
                CombineShiftRightLanes<4>(w, Zero(w), BitCast(w, w2)),
                CombineShiftRightLanes<HWY_LANES(i32)-2>(w, BitCast(w, w3), Zero(w)));
        }
        else if constexpr (HWY_LANES(i8) == 16)
        {
            r0 = BitCast(w, IfThenElse(FirstN(b, 6),
                BitCast(b, w0),
                CombineShiftRightBytes<HWY_LANES(i8)-6>(b, BitCast(b, w1), Zero(b))));
            r1 = BitCast(w, IfThenElse(FirstN(b, 4),
                CombineShiftRightBytes<2>(b, Zero(b), BitCast(b, w1)),
                CombineShiftRightBytes<HWY_LANES(i8)-4>(b, BitCast(b, w2), Zero(b))));
            r2 = BitCast(w, IfThenElse(FirstN(b, 2),
                CombineShiftRightBytes<4>(b, Zero(b), BitCast(b, w2)),
                CombineShiftRightBytes<HWY_LANES(i8)-2>(b, BitCast(b, w3), Zero(b))));
        }

        Store(LowerHalf(r0), dw, (i32*)r + j + L * 0);
        Store(LowerHalf(r1), dw, (i32*)r + j + L * 1);
        Store(LowerHalf(r2), dw, (i32*)r + j + L * 2);
    }
}

template<zvec_codec codec, typename T>
void ZVEC_ARCH_FN2(zvec_ll_block_decode,x24)(T * __restrict x, typename std::conditional<std::is_signed<T>::value,i24,u24>::type * __restrict r, size_t N, i64 iv)
{
    using S = typename std::conditional<std::is_signed<T>::value,i24,u24>::type;
    using x32 = typename std::conditional<std::is_signed<T>::value,i32,u32>::type;

    const ScalableTag<T> d;
    const ScalableTag<x32> w;
    const ScalableTag<i8> b;
    const Rebind<x32, decltype(d)> dw;
    const RebindToSigned<decltype(dw)> dws;
    const Repartition<i8, decltype(dw)> db;

    const size_t L = Lanes(d);
    const size_t W = Lanes(dw);
    const size_t V = Lanes(db);

    alignas(64) i8 idx_dec[V];
    alignas(64) i8 idx_mask[V];
    Vec<decltype(db)> shuf_dec;
    Mask<decltype(db)> shuf_mask;

    const auto shuf_last = IndicesFromVec(d, Set(d, L - 1));

    for (size_t i = 0; i < V; i++) {
        i8 x = (i % 4) + (i / 4) * 3;
        idx_dec[(i+1)&(V-1)] = (i % 4) < 3 ? x : -1;
    }
    shuf_dec = Load(db, idx_dec);

    if constexpr (HWY_LANES(i8) == 64) {
        for (size_t i = 0; i < V; i++) {
            i8 x = (i % 4) + (i / 4) * 3;
            if (std::is_signed<T>::value || codec == zvec_block_rel) {
                idx_mask[(i+1)&(V-1)] = (x / 16) == (i / 16) ? -1 : 0;
            } else {
                idx_mask[i] = (x / 16) == (i / 16) ? -1 : 0;
            }
        }
        shuf_mask = MaskFromVec(Load(db, idx_mask));
    }

    auto convert24 = [&] (Vec<decltype(dw)> v) -> Vec<decltype(dw)>
    {
        if constexpr (std::is_signed<T>::value || codec == zvec_block_rel)
        {
            if constexpr (HWY_LANES(i8) == 64) {
                Vec<decltype(dw)> u = CombineShiftRightLanes<HWY_LANES(i32)-4>(dw, v, Zero(dw));
                return BitCast(dw, ShiftRight<8>(BitCast(dws, IfThenElse(shuf_mask,
                    TableLookupBytes(BitCast(db, v), shuf_dec),
                    TableLookupBytes(BitCast(db, u), shuf_dec)))));
            } else if constexpr (HWY_LANES(i8) == 16) {
                return BitCast(dw, ShiftRight<8>(BitCast(dws,
                    TableLookupBytes(BitCast(db, v), shuf_dec))));
            }
        }
        else
        {
            if constexpr (HWY_LANES(i8) == 64) {
                Vec<decltype(dw)> u = CombineShiftRightLanes<HWY_LANES(i32)-4>(dw, v, Zero(dw));
                return BitCast(dw, IfThenElse(shuf_mask,
                    TableLookupBytes(BitCast(db, v), shuf_dec),
                    TableLookupBytes(BitCast(db, u), shuf_dec)));
            } else if constexpr (HWY_LANES(i8) == 16) {
                return BitCast(dw,
                    TableLookupBytes(BitCast(db, v), shuf_dec));
            }
        }
    };

    Vec<decltype(d)> v0 = Set(d, iv);
    Vec<decltype(w)> r0, r1, r2;
    Vec<decltype(w)> w0, w1, w2, w3;
    Vec<decltype(d)> d0, d1, d2, d3;
    Vec<decltype(d)> s0, s1, s2, s3;
    for (size_t i = 0, j = 0; i < N; i += L * 4, j+= L * 3)
    {
        r0 = ZeroExtendVector(w, Load(dw, (x32*)r + j + L * 0));
        r1 = ZeroExtendVector(w, Load(dw, (x32*)r + j + L * 1));
        r2 = ZeroExtendVector(w, Load(dw, (x32*)r + j + L * 2));

        if constexpr (HWY_LANES(i8) == 64)
        {
            w0 = r0;
            w1 = BitCast(w, IfThenElse(FirstN(w, 2),
                CombineShiftRightLanes<6>(w, Zero(w), BitCast(w, r0)),
                CombineShiftRightLanes<HWY_LANES(i32)-2>(w, BitCast(w, r1), Zero(w))));
            w2 = BitCast(w, IfThenElse(FirstN(w, 4),
                CombineShiftRightLanes<4>(w, Zero(w), BitCast(w, r1)),
                CombineShiftRightLanes<HWY_LANES(i32)-4>(w, BitCast(w, r2), Zero(w))));
            w3 = BitCast(w, CombineShiftRightLanes<2>(w, Zero(w), BitCast(w, r2)));
        }
        else if constexpr (HWY_LANES(i8) == 16)
        {
            w0 = r0;
            w1 = BitCast(w, IfThenElse(FirstN(b, 2),
                CombineShiftRightBytes<6>(b, Zero(b), BitCast(b, r0)),
                CombineShiftRightBytes<HWY_LANES(i8)-2>(b, BitCast(b, r1), Zero(b))));
            w2 = BitCast(w, IfThenElse(FirstN(b, 4),
                CombineShiftRightBytes<4>(b, Zero(b), BitCast(b, r1)),
                CombineShiftRightBytes<HWY_LANES(i8)-4>(b, BitCast(b, r2), Zero(b))));
            w3 = BitCast(w, CombineShiftRightBytes<2>(b, Zero(b), BitCast(b, r2)));
        }

        d0 = PromoteTo(d, convert24(LowerHalf(dw,w0)));
        d1 = PromoteTo(d, convert24(LowerHalf(dw,w1)));
        d2 = PromoteTo(d, convert24(LowerHalf(dw,w2)));
        d3 = PromoteTo(d, convert24(LowerHalf(dw,w3)));

        s0 = d0;
        s1 = d1;
        s2 = d2;
        s3 = d3;

        if (codec == zvec_block_rel)
        {
            constexpr_for<0, ilog2(HWY_LANES(T)), 1>([&](auto j){
                s0 = s0 + CombineShiftRightLanes<HWY_LANES(T)-(1 << j)>(d, s0, Zero(d));
                s1 = s1 + CombineShiftRightLanes<HWY_LANES(T)-(1 << j)>(d, s1, Zero(d));
                s2 = s2 + CombineShiftRightLanes<HWY_LANES(T)-(1 << j)>(d, s2, Zero(d));
                s3 = s3 + CombineShiftRightLanes<HWY_LANES(T)-(1 << j)>(d, s3, Zero(d));
            });

            s0 = s0 + v0;
            s1 = s1 + TableLookupLanes(s0, shuf_last);
            s2 = s2 + TableLookupLanes(s1, shuf_last);
            s3 = s3 + TableLookupLanes(s2, shuf_last);
            v0 = TableLookupLanes(s3, shuf_last);
        }

        Store(s0, d, x + i + L * 0);
        Store(s1, d, x + i + L * 1);
        Store(s2, d, x + i + L * 2);
        Store(s3, d, x + i + L * 3);
    }
}

template<zvec_codec codec, typename T>
void ZVEC_ARCH_FN2(zvec_ll_block_encode,x48)(T * __restrict x, typename std::conditional<std::is_signed<T>::value,i48,u48>::type * __restrict r, size_t N, T iv)
{
    const ScalableTag<T> d;
    const ScalableTag<i32> w;
    const ScalableTag<i16> s;
    const ScalableTag<i8> b;

    const size_t L = Lanes(d);
    const size_t W = Lanes(s);
    const size_t V = Lanes(b);

    alignas(64) i16 idx_encs[W];
    alignas(64) i8 idx_encb[V];
    Vec<decltype(s)> shuf_encs;
    Vec<decltype(b)> shuf_encb;

    if constexpr (HWY_LANES(i8) == 64)
    {
        for (size_t i = 0; i < W; i++) {
            i16 x = (i % 3) + (i / 3) * 4;
            idx_encs[i] = x < W ? x : -1;
        }
        shuf_encs = Load(s, idx_encs);
    }
    else if constexpr (HWY_LANES(i8) == 16)
    {
        for (size_t i = 0; i < V; i++) {
            i8 x = (i % 6) + (i / 6) * 8;
            idx_encb[i] = x < V ? x : -1;
        }
        shuf_encb = Load(b, idx_encb);
    }

    auto delta = [&] (size_t i, Vec<decltype(d)> v0) -> std::tuple<Vec<decltype(d)>,Vec<decltype(d)>> {
        auto v1 = Load(d, x+i);
        auto v2 = CombineShiftRightLanes<HWY_LANES(i64)-1>(d, v1, v0);
        auto v3 = Sub(v1, v2);
        return std::tie(v1, v3);
    };

    auto convert48 = [&] (Vec<decltype(d)> v) -> Vec<decltype(d)>
    {
        if constexpr (HWY_LANES(i8) == 64) {
            return BitCast(d, TableLookupLanes(BitCast(s, v), IndicesFromVec(s, shuf_encs)));
        } else if constexpr (HWY_LANES(i8) == 16) {
            return BitCast(d, TableLookupBytes(BitCast(b, v), shuf_encb));
        }
    };

    Vec<decltype(d)> v0 = Set(d, iv);
    Vec<decltype(d)> w0, w1, w2, w3;
    Vec<decltype(d)> r0, r1, r2;
    for (size_t i = 0, j = 0; i < N; i += L * 4, j+= L * 3)
    {
        if constexpr (codec == zvec_block_rel)
        {
            auto d0 = delta(i + L * 0, v0);
            auto d1 = delta(i + L * 1, std::get<0>(d0));
            auto d2 = delta(i + L * 2, std::get<0>(d1));
            auto d3 = delta(i + L * 3, std::get<0>(d2));

            v0 = std::get<0>(d3);

            w0 = convert48(std::get<1>(d0));
            w1 = convert48(std::get<1>(d1));
            w2 = convert48(std::get<1>(d2));
            w3 = convert48(std::get<1>(d3));
        }
        else
        {
            w0 = convert48(Load(d, x + i + L * 0));
            w1 = convert48(Load(d, x + i + L * 1));
            w2 = convert48(Load(d, x + i + L * 2));
            w3 = convert48(Load(d, x + i + L * 3));            
        }

        if constexpr (HWY_LANES(i8) == 64)
        {
            r0 = BitCast(d, IfThenElse(FirstN(w, 12),
                BitCast(w, w0),
                CombineShiftRightLanes<HWY_LANES(i32)-12>(w, BitCast(w, w1), Zero(w))));
            r1 = BitCast(d, IfThenElse(FirstN(w, 8),
                CombineShiftRightLanes<4>(w, Zero(w), BitCast(w, w1)),
                CombineShiftRightLanes<HWY_LANES(i32)-8>(w, BitCast(w, w2), Zero(w))));
            r2 = BitCast(d, IfThenElse(FirstN(w, 4),
                CombineShiftRightLanes<8>(w, Zero(w), BitCast(w, w2)),
                CombineShiftRightLanes<HWY_LANES(i32)-4>(w, BitCast(w, w3), Zero(w))));
        }
        else if constexpr (HWY_LANES(i8) == 16)
        {
            r0 = BitCast(d, IfThenElse(FirstN(b, 12),
                BitCast(b, w0),
                CombineShiftRightBytes<HWY_LANES(i8)-12>(b, BitCast(b, w1), Zero(b))));
            r1 = BitCast(d, IfThenElse(FirstN(b, 8),
                CombineShiftRightBytes<4>(b, Zero(b), BitCast(b, w1)),
                CombineShiftRightBytes<HWY_LANES(i8)-8>(b, BitCast(b, w2), Zero(b))));
            r2 = BitCast(d, IfThenElse(FirstN(b, 4),
                CombineShiftRightBytes<8>(b, Zero(b), BitCast(b, w2)),
                CombineShiftRightBytes<HWY_LANES(i8)-4>(b, BitCast(b, w3), Zero(b))));
        }

        Store(r0, d, (T*)r + j + L * 0);
        Store(r1, d, (T*)r + j + L * 1);
        Store(r2, d, (T*)r + j + L * 2);
    }
}

template<zvec_codec codec, typename T>
void ZVEC_ARCH_FN2(zvec_ll_block_decode,x48)(T * __restrict x, typename std::conditional<std::is_signed<T>::value,i48,u48>::type * __restrict r, size_t N, T iv)
{
    using S = typename std::conditional<std::is_signed<T>::value,i48,u48>::type;

    const ScalableTag<T> d;
    const RebindToSigned<decltype(d)> ds;
    const ScalableTag<i32> w;
    const ScalableTag<i16> s;
    const ScalableTag<i8> b;

    const size_t L = Lanes(d);
    const size_t W = Lanes(s);
    const size_t V = Lanes(b);

    alignas(64) i16 idx_decs[W];
    alignas(64) i8 idx_decb[V];
    Vec<decltype(s)> shuf_decs;
    Vec<decltype(b)> shuf_decb;

    const auto shuf_last = IndicesFromVec(d, Set(d, L - 1));

    if constexpr (HWY_LANES(i8) == 64)
    {
        for (size_t i = 0; i < W; i++) {
            i16 x = (i % 4) + (i / 4) * 3;
            if (std::is_signed<T>::value || codec == zvec_block_rel) {
                idx_decs[(i+1)&(W-1)] = (i % 4) < 3 ? x : -1;
            } else {
                idx_decs[i] = (i % 4) < 3 ? x : -1;
            }
        }
        shuf_decs = Load(s, idx_decs);
    }
    else if constexpr (HWY_LANES(i8) == 16)
    {
        for (size_t i = 0; i < V; i++) {
            i8 x = (i % 8) + (i / 8) * 6;
            if (std::is_signed<T>::value || codec == zvec_block_rel) {
                idx_decb[(i+2)&(V-1)] = (i % 8) < 6 ? x : -1;
            } else {
                idx_decb[i] = (i % 8) < 6 ? x : -1;
            }
        }
        shuf_decb = Load(b, idx_decb);
    }

    auto convert48 = [&] (Vec<decltype(d)> v) -> Vec<decltype(d)>
    {
        if (std::is_signed<T>::value || codec == zvec_block_rel)
        {
            if constexpr (HWY_LANES(i8) == 64) {
                return BitCast(d, ShiftRight<16>(BitCast(ds, TableLookupLanes(BitCast(s, v), IndicesFromVec(s, shuf_decs)))));
            } else if constexpr (HWY_LANES(i8) == 16) {
                return BitCast(d, ShiftRight<16>(BitCast(ds, TableLookupBytes(BitCast(b, v), shuf_decb))));
            }
        }
        else
        {
            if constexpr (HWY_LANES(i8) == 64) {
                return BitCast(d, TableLookupLanes(BitCast(s, v), IndicesFromVec(s, shuf_decs)));
            } else if constexpr (HWY_LANES(i8) == 16) {
                return BitCast(d, TableLookupBytes(BitCast(b, v), shuf_decb));
            }
        }
    };

    Vec<decltype(d)> v0 = Set(d, iv);
    Vec<decltype(d)> r0, r1, r2;
    Vec<decltype(d)> w0, w1, w2, w3;
    Vec<decltype(d)> d0, d1, d2, d3;
    Vec<decltype(d)> s0, s1, s2, s3;
    for (size_t i = 0, j = 0; i < N; i += L * 4, j+= L * 3)
    {
        r0 = Load(d, (T*)r + j + L * 0);
        r1 = Load(d, (T*)r + j + L * 1);
        r2 = Load(d, (T*)r + j + L * 2);

        if constexpr (HWY_LANES(i8) == 64)
        {
            w0 = r0;
            w1 = BitCast(d, IfThenElse(FirstN(w, 4),
                CombineShiftRightLanes<12>(w, Zero(w), BitCast(w,r0)),
                CombineShiftRightLanes<HWY_LANES(i32)-4>(w, BitCast(w,r1), Zero(w))));
            w2 = BitCast(d, IfThenElse(FirstN(w, 8),
                CombineShiftRightLanes<8>(w, Zero(w), BitCast(w,r1)),
                CombineShiftRightLanes<HWY_LANES(i32)-8>(w, BitCast(w,r2), Zero(w))));
            w3 = BitCast(d, CombineShiftRightLanes<4>(w, Zero(w), BitCast(w,r2)));
        }
        else if constexpr (HWY_LANES(i8) == 16)
        {
            w0 = r0;
            w1 = BitCast(d, IfThenElse(FirstN(b, 4),
                CombineShiftRightBytes<12>(b, Zero(b), BitCast(b, r0)),
                CombineShiftRightBytes<HWY_LANES(i8)-4>(b, BitCast(b, r1), Zero(b))));
            w2 = BitCast(d, IfThenElse(FirstN(b, 8),
                CombineShiftRightBytes<8>(b, Zero(b), BitCast(b, r1)),
                CombineShiftRightBytes<HWY_LANES(i8)-8>(b, BitCast(b, r2), Zero(b))));
            w3 = BitCast(d, CombineShiftRightBytes<4>(b, Zero(b), BitCast(b, r2)));
        }

        d0 = convert48(w0);
        d1 = convert48(w1);
        d2 = convert48(w2);
        d3 = convert48(w3);

        s0 = d0;
        s1 = d1;
        s2 = d2;
        s3 = d3;

        if (codec == zvec_block_rel)
        {
            constexpr_for<0, ilog2(HWY_LANES(T)), 1>([&](auto j){
                s0 = s0 + CombineShiftRightLanes<HWY_LANES(T)-(1 << j)>(d, s0, Zero(d));
                s1 = s1 + CombineShiftRightLanes<HWY_LANES(T)-(1 << j)>(d, s1, Zero(d));
                s2 = s2 + CombineShiftRightLanes<HWY_LANES(T)-(1 << j)>(d, s2, Zero(d));
                s3 = s3 + CombineShiftRightLanes<HWY_LANES(T)-(1 << j)>(d, s3, Zero(d));
            });

            s0 = s0 + v0;
            s1 = s1 + TableLookupLanes(s0, shuf_last);
            s2 = s2 + TableLookupLanes(s1, shuf_last);
            s3 = s3 + TableLookupLanes(s2, shuf_last);
            v0 = TableLookupLanes(s3, shuf_last);
        }

        Store(s0, d, x + i + L * 0);
        Store(s1, d, x + i + L * 1);
        Store(s2, d, x + i + L * 2);
        Store(s3, d, x + i + L * 3);
    }
}

#endif

template <typename T>
void ZVEC_ARCH_FN1(zvec_ll_block_encode_rel)(T * __restrict x, i24 * __restrict r, size_t N, T iv)
{ ZVEC_ARCH_FN2(zvec_ll_block_encode,x24)<zvec_block_rel,T>(x, r, N, iv); }
template <typename T>
void ZVEC_ARCH_FN1(zvec_ll_block_decode_rel)(T * __restrict x, i24 * __restrict r, size_t N, T iv)
{ ZVEC_ARCH_FN2(zvec_ll_block_decode,x24)<zvec_block_rel,T>(x, r, N, iv); }
template <typename T>
void ZVEC_ARCH_FN1(zvec_ll_block_encode_rel)(T * __restrict x, i48 * __restrict r, size_t N, T iv)
{ ZVEC_ARCH_FN2(zvec_ll_block_encode,x48)<zvec_block_rel,T>(x, r, N, iv); }
template <typename T>
void ZVEC_ARCH_FN1(zvec_ll_block_decode_rel)(T * __restrict x, i48 * __restrict r, size_t N, T iv)
{ ZVEC_ARCH_FN2(zvec_ll_block_decode,x48)<zvec_block_rel,T>(x, r, N, iv); }

template <typename T>
void ZVEC_ARCH_FN1(zvec_ll_block_encode_rel)(T * __restrict x, u24 * __restrict r, size_t N, T iv)
{ ZVEC_ARCH_FN2(zvec_ll_block_encode,x24)<zvec_block_rel,T>(x, r, N, iv); }
template <typename T>
void ZVEC_ARCH_FN1(zvec_ll_block_decode_rel)(T * __restrict x, u24 * __restrict r, size_t N, T iv)
{ ZVEC_ARCH_FN2(zvec_ll_block_decode,x24)<zvec_block_rel,T>(x, r, N, iv); }
template <typename T>
void ZVEC_ARCH_FN1(zvec_ll_block_encode_rel)(T * __restrict x, u48 * __restrict r, size_t N, T iv)
{ ZVEC_ARCH_FN2(zvec_ll_block_encode,x48)<zvec_block_rel,T>(x, r, N, iv); }
template <typename T>
void ZVEC_ARCH_FN1(zvec_ll_block_decode_rel)(T * __restrict x, u48 * __restrict r, size_t N, T iv)
{ ZVEC_ARCH_FN2(zvec_ll_block_decode,x48)<zvec_block_rel,T>(x, r, N, iv); }

template <typename T>
void ZVEC_ARCH_FN1(zvec_ll_block_encode_abs)(T * __restrict x, i24 * __restrict r, size_t N)
{ ZVEC_ARCH_FN2(zvec_ll_block_encode,x24)<zvec_block_abs,T>(x, r, N, 0); }
template <typename T>
void ZVEC_ARCH_FN1(zvec_ll_block_decode_abs)(T * __restrict x, i24 * __restrict r, size_t N)
{ ZVEC_ARCH_FN2(zvec_ll_block_decode,x24)<zvec_block_abs,T>(x, r, N, 0); }
template <typename T>
void ZVEC_ARCH_FN1(zvec_ll_block_encode_abs)(T * __restrict x, i48 * __restrict r, size_t N)
{ ZVEC_ARCH_FN2(zvec_ll_block_encode,x48)<zvec_block_abs,T>(x, r, N, 0); }
template <typename T>
void ZVEC_ARCH_FN1(zvec_ll_block_decode_abs)(T * __restrict x, i48 * __restrict r, size_t N)
{ ZVEC_ARCH_FN2(zvec_ll_block_decode,x48)<zvec_block_abs,T>(x, r, N, 0); }

template <typename T>
void ZVEC_ARCH_FN1(zvec_ll_block_encode_abs)(T * __restrict x, u24 * __restrict r, size_t N)
{ ZVEC_ARCH_FN2(zvec_ll_block_encode,x24)<zvec_block_abs,T>(x, r, N, 0); }
template <typename T>
void ZVEC_ARCH_FN1(zvec_ll_block_decode_abs)(T * __restrict x, u24 * __restrict r, size_t N)
{ ZVEC_ARCH_FN2(zvec_ll_block_decode,x24)<zvec_block_abs,T>(x, r, N, 0); }
template <typename T>
void ZVEC_ARCH_FN1(zvec_ll_block_encode_abs)(T * __restrict x, u48 * __restrict r, size_t N)
{ ZVEC_ARCH_FN2(zvec_ll_block_encode,x48)<zvec_block_abs,T>(x, r, N, 0); }
template <typename T>
void ZVEC_ARCH_FN1(zvec_ll_block_decode_abs)(T * __restrict x, u48 * __restrict r, size_t N)
{ ZVEC_ARCH_FN2(zvec_ll_block_decode,x48)<zvec_block_abs,T>(x, r, N, 0); }

#define zvec_ll_block_scan_abs ZVEC_ARCH_FN1(zvec_ll_block_scan_abs)
#define zvec_ll_block_scan_rel ZVEC_ARCH_FN1(zvec_ll_block_scan_rel)
#define zvec_ll_block_encode_abs ZVEC_ARCH_FN1(zvec_ll_block_encode_abs)
#define zvec_ll_block_encode_rel ZVEC_ARCH_FN1(zvec_ll_block_encode_rel)
#define zvec_ll_block_decode_abs ZVEC_ARCH_FN1(zvec_ll_block_decode_abs)
#define zvec_ll_block_decode_rel ZVEC_ARCH_FN1(zvec_ll_block_decode_rel)
#define zvec_ll_block_synth_abs ZVEC_ARCH_FN1(zvec_ll_block_synth_abs)
#define zvec_ll_block_synth_rel ZVEC_ARCH_FN1(zvec_ll_block_synth_rel)
#define zvec_ll_block_synth_both ZVEC_ARCH_FN1(zvec_ll_block_synth_both)

