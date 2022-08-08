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

#undef zvec_ll_block_scan_abs
#undef zvec_ll_block_scan_rel
#undef zvec_ll_block_encode_abs
#undef zvec_ll_block_encode_rel
#undef zvec_ll_block_decode_abs
#undef zvec_ll_block_decode_rel
#undef zvec_ll_block_synth_abs
#undef zvec_ll_block_synth_rel
#undef zvec_ll_block_synth_both

void ZVEC_ARCH_FN3(zvec_ll_block_encode_abs,i64,i8)(i64 *x, i8 *r, size_t n);
void ZVEC_ARCH_FN3(zvec_ll_block_encode_abs,i64,i16)(i64 *x, i16 *r, size_t n);
void ZVEC_ARCH_FN3(zvec_ll_block_encode_abs,i64,i24)(i64 *x, i24 *r, size_t n);
void ZVEC_ARCH_FN3(zvec_ll_block_encode_abs,i64,i32)(i64 *x, i32 *r, size_t n);
void ZVEC_ARCH_FN3(zvec_ll_block_encode_abs,i64,i48)(i64 *x, i48 *r, size_t n);
void ZVEC_ARCH_FN3(zvec_ll_block_decode_abs,i64,i8)(i64 *x, i8 *r, size_t n);
void ZVEC_ARCH_FN3(zvec_ll_block_decode_abs,i64,i16)(i64 *x, i16 *r, size_t n);
void ZVEC_ARCH_FN3(zvec_ll_block_decode_abs,i64,i24)(i64 *x, i24 *r, size_t n);
void ZVEC_ARCH_FN3(zvec_ll_block_decode_abs,i64,i32)(i64 *x, i32 *r, size_t n);
void ZVEC_ARCH_FN3(zvec_ll_block_decode_abs,i64,i48)(i64 *x, i48 *r, size_t n);
void ZVEC_ARCH_FN3(zvec_ll_block_encode_rel,i64,i8)(i64 *x, i8 *r, size_t n, i64 iv);
void ZVEC_ARCH_FN3(zvec_ll_block_encode_rel,i64,i16)(i64 *x, i16 *r, size_t n, i64 iv);
void ZVEC_ARCH_FN3(zvec_ll_block_encode_rel,i64,i24)(i64 *x, i24 *r, size_t n, i64 iv);
void ZVEC_ARCH_FN3(zvec_ll_block_encode_rel,i64,i32)(i64 *x, i32 *r, size_t n, i64 iv);
void ZVEC_ARCH_FN3(zvec_ll_block_encode_rel,i64,i48)(i64 *x, i48 *r, size_t n, i64 iv);
void ZVEC_ARCH_FN3(zvec_ll_block_decode_rel,i64,i8)(i64 *x, i8 *r, size_t n, i64 iv);
void ZVEC_ARCH_FN3(zvec_ll_block_decode_rel,i64,i16)(i64 *x, i16 *r, size_t n, i64 iv);
void ZVEC_ARCH_FN3(zvec_ll_block_decode_rel,i64,i24)(i64 *x, i24 *r, size_t n, i64 iv);
void ZVEC_ARCH_FN3(zvec_ll_block_decode_rel,i64,i32)(i64 *x, i32 *r, size_t n, i64 iv);
void ZVEC_ARCH_FN3(zvec_ll_block_decode_rel,i64,i48)(i64 *x, i48 *r, size_t n, i64 iv);
zvec_stats<i64> ZVEC_ARCH_FN2(zvec_ll_block_scan_abs,i64)(i64 *x, size_t n);
zvec_stats<i64> ZVEC_ARCH_FN2(zvec_ll_block_scan_rel,i64)(i64 *x, size_t n);
zvec_stats<i64> ZVEC_ARCH_FN2(zvec_ll_block_scan_both,i64)(i64 *x, size_t n);
void ZVEC_ARCH_FN2(zvec_ll_block_synth_abs,i64)(i64 *x, size_t n, i64 iv);
void ZVEC_ARCH_FN2(zvec_ll_block_synth_rel,i64)(i64 *x, size_t n, i64 iv, i64 dv);
void ZVEC_ARCH_FN2(zvec_ll_block_synth_both,i64)(i64 *x, size_t n, i64 iv, i64 dv);

void ZVEC_ARCH_FN3(zvec_ll_block_encode_abs,u64,u8)(u64 *x, u8 *r, size_t n);
void ZVEC_ARCH_FN3(zvec_ll_block_encode_abs,u64,u16)(u64 *x, u16 *r, size_t n);
void ZVEC_ARCH_FN3(zvec_ll_block_encode_abs,u64,u24)(u64 *x, u24 *r, size_t n);
void ZVEC_ARCH_FN3(zvec_ll_block_encode_abs,u64,u32)(u64 *x, u32 *r, size_t n);
void ZVEC_ARCH_FN3(zvec_ll_block_encode_abs,u64,u48)(u64 *x, u48 *r, size_t n);
void ZVEC_ARCH_FN3(zvec_ll_block_decode_abs,u64,u8)(u64 *x, u8 *r, size_t n);
void ZVEC_ARCH_FN3(zvec_ll_block_decode_abs,u64,u16)(u64 *x, u16 *r, size_t n);
void ZVEC_ARCH_FN3(zvec_ll_block_decode_abs,u64,u24)(u64 *x, u24 *r, size_t n);
void ZVEC_ARCH_FN3(zvec_ll_block_decode_abs,u64,u32)(u64 *x, u32 *r, size_t n);
void ZVEC_ARCH_FN3(zvec_ll_block_decode_abs,u64,u48)(u64 *x, u48 *r, size_t n);
void ZVEC_ARCH_FN3(zvec_ll_block_encode_rel,u64,u8)(u64 *x, u8 *r, size_t n, u64 iv);
void ZVEC_ARCH_FN3(zvec_ll_block_encode_rel,u64,u16)(u64 *x, u16 *r, size_t n, u64 iv);
void ZVEC_ARCH_FN3(zvec_ll_block_encode_rel,u64,u24)(u64 *x, u24 *r, size_t n, u64 iv);
void ZVEC_ARCH_FN3(zvec_ll_block_encode_rel,u64,u32)(u64 *x, u32 *r, size_t n, u64 iv);
void ZVEC_ARCH_FN3(zvec_ll_block_encode_rel,u64,u48)(u64 *x, u48 *r, size_t n, u64 iv);
void ZVEC_ARCH_FN3(zvec_ll_block_decode_rel,u64,u8)(u64 *x, u8 *r, size_t n, u64 iv);
void ZVEC_ARCH_FN3(zvec_ll_block_decode_rel,u64,u16)(u64 *x, u16 *r, size_t n, u64 iv);
void ZVEC_ARCH_FN3(zvec_ll_block_decode_rel,u64,u24)(u64 *x, u24 *r, size_t n, u64 iv);
void ZVEC_ARCH_FN3(zvec_ll_block_decode_rel,u64,u32)(u64 *x, u32 *r, size_t n, u64 iv);
void ZVEC_ARCH_FN3(zvec_ll_block_decode_rel,u64,u48)(u64 *x, u48 *r, size_t n, u64 iv);
zvec_stats<u64> ZVEC_ARCH_FN2(zvec_ll_block_scan_abs,u64)(u64 *x, size_t n);
zvec_stats<u64> ZVEC_ARCH_FN2(zvec_ll_block_scan_rel,u64)(u64 *x, size_t n);
zvec_stats<u64> ZVEC_ARCH_FN2(zvec_ll_block_scan_both,u64)(u64 *x, size_t n);
void ZVEC_ARCH_FN2(zvec_ll_block_synth_abs,u64)(u64 *x, size_t n, u64 iv);
void ZVEC_ARCH_FN2(zvec_ll_block_synth_rel,u64)(u64 *x, size_t n, u64 iv, u64 dv);
void ZVEC_ARCH_FN2(zvec_ll_block_synth_both,u64)(u64 *x, size_t n, u64 iv, u64 dv);

#ifdef ZVEC_INSTANTIATE
void ZVEC_ARCH_FN3(zvec_ll_block_encode_abs,i64,i8)(i64 *x, i8 *r, size_t n) { ZVEC_ARCH_FN1(zvec_ll_block_encode_abs)(x,r,n); }
void ZVEC_ARCH_FN3(zvec_ll_block_encode_abs,i64,i16)(i64 *x, i16 *r, size_t n) { ZVEC_ARCH_FN1(zvec_ll_block_encode_abs)(x,r,n); }
void ZVEC_ARCH_FN3(zvec_ll_block_encode_abs,i64,i24)(i64 *x, i24 *r, size_t n) { ZVEC_ARCH_FN1(zvec_ll_block_encode_abs)(x,r,n); }
void ZVEC_ARCH_FN3(zvec_ll_block_encode_abs,i64,i32)(i64 *x, i32 *r, size_t n) { ZVEC_ARCH_FN1(zvec_ll_block_encode_abs)(x,r,n); }
void ZVEC_ARCH_FN3(zvec_ll_block_encode_abs,i64,i48)(i64 *x, i48 *r, size_t n) { ZVEC_ARCH_FN1(zvec_ll_block_encode_abs)(x,r,n); }
void ZVEC_ARCH_FN3(zvec_ll_block_decode_abs,i64,i8)(i64 *x, i8 *r, size_t n) { ZVEC_ARCH_FN1(zvec_ll_block_decode_abs)(x,r,n); }
void ZVEC_ARCH_FN3(zvec_ll_block_decode_abs,i64,i16)(i64 *x, i16 *r, size_t n) { ZVEC_ARCH_FN1(zvec_ll_block_decode_abs)(x,r,n); }
void ZVEC_ARCH_FN3(zvec_ll_block_decode_abs,i64,i24)(i64 *x, i24 *r, size_t n) { ZVEC_ARCH_FN1(zvec_ll_block_decode_abs)(x,r,n); }
void ZVEC_ARCH_FN3(zvec_ll_block_decode_abs,i64,i32)(i64 *x, i32 *r, size_t n) { ZVEC_ARCH_FN1(zvec_ll_block_decode_abs)(x,r,n); }
void ZVEC_ARCH_FN3(zvec_ll_block_decode_abs,i64,i48)(i64 *x, i48 *r, size_t n) { ZVEC_ARCH_FN1(zvec_ll_block_decode_abs)(x,r,n); }
void ZVEC_ARCH_FN3(zvec_ll_block_encode_rel,i64,i8)(i64 *x, i8 *r, size_t n, i64 iv) { ZVEC_ARCH_FN1(zvec_ll_block_encode_rel)(x,r,n,iv); }
void ZVEC_ARCH_FN3(zvec_ll_block_encode_rel,i64,i16)(i64 *x, i16 *r, size_t n, i64 iv) { ZVEC_ARCH_FN1(zvec_ll_block_encode_rel)(x,r,n,iv); }
void ZVEC_ARCH_FN3(zvec_ll_block_encode_rel,i64,i24)(i64 *x, i24 *r, size_t n, i64 iv) { ZVEC_ARCH_FN1(zvec_ll_block_encode_rel)(x,r,n,iv); }
void ZVEC_ARCH_FN3(zvec_ll_block_encode_rel,i64,i32)(i64 *x, i32 *r, size_t n, i64 iv) { ZVEC_ARCH_FN1(zvec_ll_block_encode_rel)(x,r,n,iv); }
void ZVEC_ARCH_FN3(zvec_ll_block_encode_rel,i64,i48)(i64 *x, i48 *r, size_t n, i64 iv) { ZVEC_ARCH_FN1(zvec_ll_block_encode_rel)(x,r,n,iv); }
void ZVEC_ARCH_FN3(zvec_ll_block_decode_rel,i64,i8)(i64 *x, i8 *r, size_t n, i64 iv) { ZVEC_ARCH_FN1(zvec_ll_block_decode_rel)(x,r,n,iv); }
void ZVEC_ARCH_FN3(zvec_ll_block_decode_rel,i64,i16)(i64 *x, i16 *r, size_t n, i64 iv) { ZVEC_ARCH_FN1(zvec_ll_block_decode_rel)(x,r,n,iv); }
void ZVEC_ARCH_FN3(zvec_ll_block_decode_rel,i64,i24)(i64 *x, i24 *r, size_t n, i64 iv) { ZVEC_ARCH_FN1(zvec_ll_block_decode_rel)(x,r,n,iv); }
void ZVEC_ARCH_FN3(zvec_ll_block_decode_rel,i64,i32)(i64 *x, i32 *r, size_t n, i64 iv) { ZVEC_ARCH_FN1(zvec_ll_block_decode_rel)(x,r,n,iv); }
void ZVEC_ARCH_FN3(zvec_ll_block_decode_rel,i64,i48)(i64 *x, i48 *r, size_t n, i64 iv) { ZVEC_ARCH_FN1(zvec_ll_block_decode_rel)(x,r,n,iv); }
zvec_stats<i64> ZVEC_ARCH_FN2(zvec_ll_block_scan_abs,i64)(i64 *x, size_t n) { return ZVEC_ARCH_FN1(zvec_ll_block_scan_abs)(x,n); }
zvec_stats<i64> ZVEC_ARCH_FN2(zvec_ll_block_scan_rel,i64)(i64 *x, size_t n) { return ZVEC_ARCH_FN1(zvec_ll_block_scan_rel)(x,n); }
zvec_stats<i64> ZVEC_ARCH_FN2(zvec_ll_block_scan_both,i64)(i64 *x, size_t n) { return ZVEC_ARCH_FN1(zvec_ll_block_scan_both)(x,n); }
void ZVEC_ARCH_FN2(zvec_ll_block_synth_abs,i64)(i64 *x, size_t n, i64 iv) { ZVEC_ARCH_FN1(zvec_ll_block_synth_abs)(x,n,iv); }
void ZVEC_ARCH_FN2(zvec_ll_block_synth_rel,i64)(i64 *x, size_t n, i64 iv, i64 dv) { ZVEC_ARCH_FN1(zvec_ll_block_synth_rel)(x,n,iv,dv); }
void ZVEC_ARCH_FN2(zvec_ll_block_synth_both,i64)(i64 *x, size_t n, i64 iv, i64 dv) { ZVEC_ARCH_FN1(zvec_ll_block_synth_both)(x,n,iv,dv); }

void ZVEC_ARCH_FN3(zvec_ll_block_encode_abs,u64,u8)(u64 *x, u8 *r, size_t n) { ZVEC_ARCH_FN1(zvec_ll_block_encode_abs)(x,r,n); }
void ZVEC_ARCH_FN3(zvec_ll_block_encode_abs,u64,u16)(u64 *x, u16 *r, size_t n) { ZVEC_ARCH_FN1(zvec_ll_block_encode_abs)(x,r,n); }
void ZVEC_ARCH_FN3(zvec_ll_block_encode_abs,u64,u24)(u64 *x, u24 *r, size_t n) { ZVEC_ARCH_FN1(zvec_ll_block_encode_abs)(x,r,n); }
void ZVEC_ARCH_FN3(zvec_ll_block_encode_abs,u64,u32)(u64 *x, u32 *r, size_t n) { ZVEC_ARCH_FN1(zvec_ll_block_encode_abs)(x,r,n); }
void ZVEC_ARCH_FN3(zvec_ll_block_encode_abs,u64,u48)(u64 *x, u48 *r, size_t n) { ZVEC_ARCH_FN1(zvec_ll_block_encode_abs)(x,r,n); }
void ZVEC_ARCH_FN3(zvec_ll_block_decode_abs,u64,u8)(u64 *x, u8 *r, size_t n) { ZVEC_ARCH_FN1(zvec_ll_block_decode_abs)(x,r,n); }
void ZVEC_ARCH_FN3(zvec_ll_block_decode_abs,u64,u16)(u64 *x, u16 *r, size_t n) { ZVEC_ARCH_FN1(zvec_ll_block_decode_abs)(x,r,n); }
void ZVEC_ARCH_FN3(zvec_ll_block_decode_abs,u64,u24)(u64 *x, u24 *r, size_t n) { ZVEC_ARCH_FN1(zvec_ll_block_decode_abs)(x,r,n); }
void ZVEC_ARCH_FN3(zvec_ll_block_decode_abs,u64,u32)(u64 *x, u32 *r, size_t n) { ZVEC_ARCH_FN1(zvec_ll_block_decode_abs)(x,r,n); }
void ZVEC_ARCH_FN3(zvec_ll_block_decode_abs,u64,u48)(u64 *x, u48 *r, size_t n) { ZVEC_ARCH_FN1(zvec_ll_block_decode_abs)(x,r,n); }
void ZVEC_ARCH_FN3(zvec_ll_block_encode_rel,u64,u8)(u64 *x, u8 *r, size_t n, u64 iv) { ZVEC_ARCH_FN1(zvec_ll_block_encode_rel)(x,r,n,iv); }
void ZVEC_ARCH_FN3(zvec_ll_block_encode_rel,u64,u16)(u64 *x, u16 *r, size_t n, u64 iv) { ZVEC_ARCH_FN1(zvec_ll_block_encode_rel)(x,r,n,iv); }
void ZVEC_ARCH_FN3(zvec_ll_block_encode_rel,u64,u24)(u64 *x, u24 *r, size_t n, u64 iv) { ZVEC_ARCH_FN1(zvec_ll_block_encode_rel)(x,r,n,iv); }
void ZVEC_ARCH_FN3(zvec_ll_block_encode_rel,u64,u32)(u64 *x, u32 *r, size_t n, u64 iv) { ZVEC_ARCH_FN1(zvec_ll_block_encode_rel)(x,r,n,iv); }
void ZVEC_ARCH_FN3(zvec_ll_block_encode_rel,u64,u48)(u64 *x, u48 *r, size_t n, u64 iv) { ZVEC_ARCH_FN1(zvec_ll_block_encode_rel)(x,r,n,iv); }
void ZVEC_ARCH_FN3(zvec_ll_block_decode_rel,u64,u8)(u64 *x, u8 *r, size_t n, u64 iv) { ZVEC_ARCH_FN1(zvec_ll_block_decode_rel)(x,r,n,iv); }
void ZVEC_ARCH_FN3(zvec_ll_block_decode_rel,u64,u16)(u64 *x, u16 *r, size_t n, u64 iv) { ZVEC_ARCH_FN1(zvec_ll_block_decode_rel)(x,r,n,iv); }
void ZVEC_ARCH_FN3(zvec_ll_block_decode_rel,u64,u24)(u64 *x, u24 *r, size_t n, u64 iv) { ZVEC_ARCH_FN1(zvec_ll_block_decode_rel)(x,r,n,iv); }
void ZVEC_ARCH_FN3(zvec_ll_block_decode_rel,u64,u32)(u64 *x, u32 *r, size_t n, u64 iv) { ZVEC_ARCH_FN1(zvec_ll_block_decode_rel)(x,r,n,iv); }
void ZVEC_ARCH_FN3(zvec_ll_block_decode_rel,u64,u48)(u64 *x, u48 *r, size_t n, u64 iv) { ZVEC_ARCH_FN1(zvec_ll_block_decode_rel)(x,r,n,iv); }
zvec_stats<u64> ZVEC_ARCH_FN2(zvec_ll_block_scan_abs,u64)(u64 *x, size_t n) { return ZVEC_ARCH_FN1(zvec_ll_block_scan_abs)(x,n); }
zvec_stats<u64> ZVEC_ARCH_FN2(zvec_ll_block_scan_rel,u64)(u64 *x, size_t n) { return ZVEC_ARCH_FN1(zvec_ll_block_scan_rel)(x,n); }
zvec_stats<u64> ZVEC_ARCH_FN2(zvec_ll_block_scan_both,u64)(u64 *x, size_t n) { return ZVEC_ARCH_FN1(zvec_ll_block_scan_both)(x,n); }
void ZVEC_ARCH_FN2(zvec_ll_block_synth_abs,u64)(u64 *x, size_t n, u64 iv) { ZVEC_ARCH_FN1(zvec_ll_block_synth_abs)(x,n,iv); }
void ZVEC_ARCH_FN2(zvec_ll_block_synth_rel,u64)(u64 *x, size_t n, u64 iv, u64 dv) { ZVEC_ARCH_FN1(zvec_ll_block_synth_rel)(x,n,iv,dv); }
void ZVEC_ARCH_FN2(zvec_ll_block_synth_both,u64)(u64 *x, size_t n, u64 iv, u64 dv) { ZVEC_ARCH_FN1(zvec_ll_block_synth_both)(x,n,iv,dv); }
#endif
