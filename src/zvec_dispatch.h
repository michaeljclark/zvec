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

template<typename T, typename X48, typename X32, typename X24, typename X16, typename X8>
struct zvec_op_types_64
{
    void (*encode_abs_x8)(T *x, X8 *r, size_t n);
    void (*encode_abs_x16)(T *x, X16 *r, size_t n);
    void (*encode_abs_x24)(T *x, X24 *r, size_t n);
    void (*encode_abs_x32)(T *x, X32 *r, size_t n);
    void (*encode_abs_x48)(T *x, X48 *r, size_t n);
    void (*decode_abs_x8)(T *x, X8 *r, size_t n);
    void (*decode_abs_x16)(T *x, X16 *r, size_t n);
    void (*decode_abs_x24)(T *x, X24 *r, size_t n);
    void (*decode_abs_x32)(T *x, X32 *r, size_t n);
    void (*decode_abs_x48)(T *x, X48 *r, size_t n);
    void (*encode_rel_x8)(T *x, X8 *r, size_t n, T iv);
    void (*encode_rel_x16)(T *x, X16 *r, size_t n, T iv);
    void (*encode_rel_x24)(T *x, X24 *r, size_t n, T iv);
    void (*encode_rel_x32)(T *x, X32 *r, size_t n, T iv);
    void (*encode_rel_x48)(T *x, X48 *r, size_t n, T iv);
    void (*decode_rel_x8)(T *x, X8 *r, size_t n, T iv);
    void (*decode_rel_x16)(T *x, X16 *r, size_t n, T iv);
    void (*decode_rel_x24)(T *x, X24 *r, size_t n, T iv);
    void (*decode_rel_x32)(T *x, X32 *r, size_t n, T iv);
    void (*decode_rel_x48)(T *x, X48 *r, size_t n, T iv);
    zvec_stats<T> (*scan_abs)(T *x, size_t n);
    zvec_stats<T> (*scan_rel)(T *x, size_t n);
    zvec_stats<T> (*scan_both)(T *x, size_t n);
    void (*synth_abs)(T *x, size_t n, T iv);
    void (*synth_rel)(T *x, size_t n, T iv, T dv);
    void (*synth_both)(T *x, size_t n, T iv, T dv);
};

template<typename T, typename X24, typename X16, typename X8>
struct zvec_op_types_32
{
    void (*encode_abs_x8)(T *x, X8 *r, size_t n);
    void (*encode_abs_x16)(T *x, X16 *r, size_t n);
    void (*encode_abs_x24)(T *x, X24 *r, size_t n);
    void (*decode_abs_x8)(T *x, X8 *r, size_t n);
    void (*decode_abs_x16)(T *x, X16 *r, size_t n);
    void (*decode_abs_x24)(T *x, X24 *r, size_t n);
    void (*encode_rel_x8)(T *x, X8 *r, size_t n, T iv);
    void (*encode_rel_x16)(T *x, X16 *r, size_t n, T iv);
    void (*encode_rel_x24)(T *x, X24 *r, size_t n, T iv);
    void (*decode_rel_x8)(T *x, X8 *r, size_t n, T iv);
    void (*decode_rel_x16)(T *x, X16 *r, size_t n, T iv);
    void (*decode_rel_x24)(T *x, X24 *r, size_t n, T iv);
    zvec_stats<T> (*scan_abs)(T *x, size_t n);
    zvec_stats<T> (*scan_rel)(T *x, size_t n);
    zvec_stats<T> (*scan_both)(T *x, size_t n);
    void (*synth_abs)(T *x, size_t n, T iv);
    void (*synth_rel)(T *x, size_t n, T iv, T dv);
    void (*synth_both)(T *x, size_t n, T iv, T dv);
};

using zvec_op_types_i64 = zvec_op_types_64<i64,i48,i32,i24,i16,i8>;
using zvec_op_types_u64 = zvec_op_types_64<u64,u48,u32,u24,u16,u8>;
using zvec_op_types_i32 = zvec_op_types_32<i32,i24,i16,i8>;
using zvec_op_types_u32 = zvec_op_types_32<u32,u24,u16,u8>;

enum zvec_arch {
    zvec_arch_unspecified,
    zvec_arch_generic,
    zvec_arch_x86_avx3
};

void zvec_set_override(zvec_arch arch);

zvec_op_types_i64* get_zvec_ops_i64();
zvec_op_types_u64* get_zvec_ops_u64();
zvec_op_types_i32* get_zvec_ops_i32();
zvec_op_types_u32* get_zvec_ops_u32();
