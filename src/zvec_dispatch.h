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

template<typename X64,typename X48, typename X32, typename X24, typename X16, typename X8>
struct zvec_op_types
{
    void (*encode_abs_x64_x8)(X64 *x, X8 *r, size_t n);
    void (*encode_abs_x64_x16)(X64 *x, X16 *r, size_t n);
    void (*encode_abs_x64_x24)(X64 *x, X24 *r, size_t n);
    void (*encode_abs_x64_x32)(X64 *x, X32 *r, size_t n);
    void (*encode_abs_x64_x48)(X64 *x, X48 *r, size_t n);
    void (*decode_abs_x64_x8)(X64 *x, X8 *r, size_t n);
    void (*decode_abs_x64_x16)(X64 *x, X16 *r, size_t n);
    void (*decode_abs_x64_x24)(X64 *x, X24 *r, size_t n);
    void (*decode_abs_x64_x32)(X64 *x, X32 *r, size_t n);
    void (*decode_abs_x64_x48)(X64 *x, X48 *r, size_t n);
    void (*encode_rel_x64_x8)(X64 *x, X8 *r, size_t n, X64 iv);
    void (*encode_rel_x64_x16)(X64 *x, X16 *r, size_t n, X64 iv);
    void (*encode_rel_x64_x24)(X64 *x, X24 *r, size_t n, X64 iv);
    void (*encode_rel_x64_x32)(X64 *x, X32 *r, size_t n, X64 iv);
    void (*encode_rel_x64_x48)(X64 *x, X48 *r, size_t n, X64 iv);
    void (*decode_rel_x64_x8)(X64 *x, X8 *r, size_t n, X64 iv);
    void (*decode_rel_x64_x16)(X64 *x, X16 *r, size_t n, X64 iv);
    void (*decode_rel_x64_x24)(X64 *x, X24 *r, size_t n, X64 iv);
    void (*decode_rel_x64_x32)(X64 *x, X32 *r, size_t n, X64 iv);
    void (*decode_rel_x64_x48)(X64 *x, X48 *r, size_t n, X64 iv);
    zvec_stats<X64> (*scan_abs_x64)(X64 *x, size_t n);
    zvec_stats<X64> (*scan_rel_x64)(X64 *x, size_t n);
    zvec_stats<X64> (*scan_both_x64)(X64 *x, size_t n);
    void (*synth_abs_x64)(X64 *x, size_t n, X64 iv);
    void (*synth_rel_x64)(X64 *x, size_t n, X64 iv, X64 dv);
    void (*synth_both_x64)(X64 *x, size_t n, X64 iv, X64 dv);
};

using zvec_op_types_i64 = zvec_op_types<i64,i48,i32,i24,i16,i8>;
using zvec_op_types_u64 = zvec_op_types<u64,u48,u32,u24,u16,u8>;

enum zvec_arch {
    zvec_arch_unspecified,
    zvec_arch_generic,
    zvec_arch_x86_avx1,
    zvec_arch_x86_avx3
};

void zvec_set_override(zvec_arch arch);

zvec_op_types_i64* get_zvec_ops_i64();
zvec_op_types_u64* get_zvec_ops_u64();
