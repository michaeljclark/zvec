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

enum zvec_size {
    zvec_size_0,
    zvec_size_1,
    zvec_size_2,
    zvec_size_3,
    zvec_size_4,
    zvec_size_6,
    zvec_size_8,
    zvec_size_12,
    zvec_size_16,
    zvec_size_24,
    zvec_size_32,
    zvec_size_48,
    zvec_size_64
};

static const char* zvec_codec_name(zvec_codec codec)
{
    switch (codec) {
    case zvec_codec_none: return "none";
    case zvec_block_rel: return "block-rel";
    case zvec_block_abs: return "block-abs";
    case zvec_block_rel_or_abs: return "block-rel-or-abs";
    case zvec_const_rel: return "const-rel";
    case zvec_const_abs: return "const-abs";
    }
    return nullptr;
}

template <typename T>
constexpr zvec_size zvec_size_abs(zvec_stats<T> s);
template <typename T>
constexpr zvec_size zvec_size_rel(zvec_stats<T> s);

int zvec_size_bits(zvec_size z)
{
    int n = (int)z;
    return ~(~n&1u) << 30 >> ((~n >> 1) & 31) & ~((n-1)>>31);
}

template <>
constexpr zvec_size zvec_size_abs(zvec_stats<i64> s)
{
    if (s.amin == s.amax) return zvec_size_0;
    if (s.amin >= -(1<<7) && s.amax <= ((1<<7)-1)) return zvec_size_8;
    if (s.amin >= -(1<<15) && s.amax <= ((1<<15)-1)) return zvec_size_16;
    if (s.amin >= -(1<<23) && s.amax <= ((1<<23)-1)) return zvec_size_24;
    if (s.amin >= -(1ll<<31) && s.amax <= ((1ll<<31)-1)) return zvec_size_32;
    if (s.amin >= -(1ll<<47) && s.amax <= ((1ll<<47)-1)) return zvec_size_48;
    return zvec_size_64;
}

template <>
constexpr zvec_size zvec_size_abs(zvec_stats<u64> s)
{
    if (s.amin == s.amax) return zvec_size_0;
    if (s.amax <= ((1u<<8)-1)) return zvec_size_8;
    if (s.amax <= ((1u<<16)-1)) return zvec_size_16;
    if (s.amax <= ((1u<<24)-1)) return zvec_size_24;
    if (s.amax <= ((1llu<<32)-1)) return zvec_size_32;
    if (s.amax <= ((1llu<<48)-1)) return zvec_size_48;
    return zvec_size_64;
}

template <>
constexpr zvec_size zvec_size_abs(zvec_stats<i32> s)
{
    if (s.amin == s.amax) return zvec_size_0;
    if (s.amin >= -(1<<7) && s.amax <= ((1<<7)-1)) return zvec_size_8;
    if (s.amin >= -(1<<15) && s.amax <= ((1<<15)-1)) return zvec_size_16;
    if (s.amin >= -(1<<23) && s.amax <= ((1<<23)-1)) return zvec_size_24;
    return zvec_size_32;
}

template <>
constexpr zvec_size zvec_size_abs(zvec_stats<u32> s)
{
    if (s.amin == s.amax) return zvec_size_0;
    if (s.amax <= ((1u<<8)-1)) return zvec_size_8;
    if (s.amax <= ((1u<<16)-1)) return zvec_size_16;
    if (s.amax <= ((1u<<24)-1)) return zvec_size_24;
    return zvec_size_32;
}

template <typename T>
constexpr zvec_size zvec_size_rel(zvec_stats<T> s)
{
    if (s.dmin == s.dmax) return zvec_size_0;
    if (s.dmin >= -(1<<7) && s.dmax <= ((1<<7)-1)) return zvec_size_8;
    if (s.dmin >= -(1<<15) && s.dmax <= ((1<<15)-1)) return zvec_size_16;
    if (s.dmin >= -(1<<23) && s.dmax <= ((1<<23)-1)) return zvec_size_24;
    if constexpr (sizeof(T) == 8) {
        if (s.dmin >= -(1ll<<31) && s.dmax <= ((1ll<<31)-1)) return zvec_size_32;
        if (s.dmin >= -(1ll<<47) && s.dmax <= ((1ll<<47)-1)) return zvec_size_48;
        return zvec_size_64;
    }
    if constexpr (sizeof(T) == 4) {
        return zvec_size_32;
    }
    return zvec_size_0;
}

template <typename T>
zvec_stats<T> zvec_block_scan_abs(T * __restrict x, size_t n)
{
    if constexpr (sizeof(T) == 8) {
        typedef typename std::conditional<std::is_signed<T>::value,zvec_op_types_i64,zvec_op_types_u64>::type zvec_ops;
        zvec_ops *ops = (std::is_signed<T>::value) ? (zvec_ops*)get_zvec_ops_i64() : (zvec_ops*)get_zvec_ops_u64();
        return ops->scan_abs(x, n);
    }
    if constexpr (sizeof(T) == 4) {
        typedef typename std::conditional<std::is_signed<T>::value,zvec_op_types_i32,zvec_op_types_u32>::type zvec_ops;
        zvec_ops *ops = (std::is_signed<T>::value) ? (zvec_ops*)get_zvec_ops_i32() : (zvec_ops*)get_zvec_ops_u32();
        return ops->scan_abs(x, n);
    }
}

template <typename T>
zvec_stats<T> zvec_block_scan_rel(T * __restrict x, size_t n)
{
    if constexpr (sizeof(T) == 8) {
        typedef typename std::conditional<std::is_signed<T>::value,zvec_op_types_i64,zvec_op_types_u64>::type zvec_ops;
        zvec_ops *ops = (std::is_signed<T>::value) ? (zvec_ops*)get_zvec_ops_i64() : (zvec_ops*)get_zvec_ops_u64();
        return ops->scan_rel(x, n);
    }
    if constexpr (sizeof(T) == 4) {
        typedef typename std::conditional<std::is_signed<T>::value,zvec_op_types_i32,zvec_op_types_u32>::type zvec_ops;
        zvec_ops *ops = (std::is_signed<T>::value) ? (zvec_ops*)get_zvec_ops_i32() : (zvec_ops*)get_zvec_ops_u32();
        return ops->scan_rel(x, n);
    }
}

template <typename T>
zvec_stats<T> zvec_block_scan_both(T * __restrict x, size_t n)
{
    if constexpr (sizeof(T) == 8) {
        typedef typename std::conditional<std::is_signed<T>::value,zvec_op_types_i64,zvec_op_types_u64>::type zvec_ops;
        zvec_ops *ops = (std::is_signed<T>::value) ? (zvec_ops*)get_zvec_ops_i64() : (zvec_ops*)get_zvec_ops_u64();
        return ops->scan_both(x, n);
    }
    if constexpr (sizeof(T) == 4) {
        typedef typename std::conditional<std::is_signed<T>::value,zvec_op_types_i32,zvec_op_types_u32>::type zvec_ops;
        zvec_ops *ops = (std::is_signed<T>::value) ? (zvec_ops*)get_zvec_ops_i32() : (zvec_ops*)get_zvec_ops_u32();
        return ops->scan_both(x, n);
    }
}

template <typename T>
void zvec_block_synth_abs(T * __restrict x, size_t n, T iv)
{
    if constexpr (sizeof(T) == 8) {
        typedef typename std::conditional<std::is_signed<T>::value,zvec_op_types_i64,zvec_op_types_u64>::type zvec_ops;
        zvec_ops *ops = (std::is_signed<T>::value) ? (zvec_ops*)get_zvec_ops_i64() : (zvec_ops*)get_zvec_ops_u64();
        ops->synth_abs(x, n, iv);
    }
    if constexpr (sizeof(T) == 4) {
        typedef typename std::conditional<std::is_signed<T>::value,zvec_op_types_i32,zvec_op_types_u32>::type zvec_ops;
        zvec_ops *ops = (std::is_signed<T>::value) ? (zvec_ops*)get_zvec_ops_i32() : (zvec_ops*)get_zvec_ops_u32();
        ops->synth_abs(x, n, iv);
    }
}

template <typename T>
void zvec_block_synth_rel(T * __restrict x, size_t n, T iv, T dv)
{
    if constexpr (sizeof(T) == 8) {
        typedef typename std::conditional<std::is_signed<T>::value,zvec_op_types_i64,zvec_op_types_u64>::type zvec_ops;
        zvec_ops *ops = (std::is_signed<T>::value) ? (zvec_ops*)get_zvec_ops_i64() : (zvec_ops*)get_zvec_ops_u64();
        ops->synth_rel(x, n, iv, dv);
    }
    if constexpr (sizeof(T) == 4) {
        typedef typename std::conditional<std::is_signed<T>::value,zvec_op_types_i32,zvec_op_types_u32>::type zvec_ops;
        zvec_ops *ops = (std::is_signed<T>::value) ? (zvec_ops*)get_zvec_ops_i32() : (zvec_ops*)get_zvec_ops_u32();
        ops->synth_rel(x, n, iv, dv);
    }
}

template <typename T>
void zvec_block_synth_both(T * __restrict x, size_t n, T iv, T dv)
{
    if constexpr (sizeof(T) == 8) {
        typedef typename std::conditional<std::is_signed<T>::value,zvec_op_types_i64,zvec_op_types_u64>::type zvec_ops;
        zvec_ops *ops = (std::is_signed<T>::value) ? (zvec_ops*)get_zvec_ops_i64() : (zvec_ops*)get_zvec_ops_u64();
        ops->synth_both(x, n, iv, dv);
    }
    if constexpr (sizeof(T) == 4) {
        typedef typename std::conditional<std::is_signed<T>::value,zvec_op_types_i32,zvec_op_types_u32>::type zvec_ops;
        zvec_ops *ops = (std::is_signed<T>::value) ? (zvec_ops*)get_zvec_ops_i32() : (zvec_ops*)get_zvec_ops_u32();
        ops->synth_both(x, n, iv, dv);
    }
}

template <typename T>
void zvec_block_encode_abs(T * __restrict in, void * __restrict comp, size_t n, zvec_size z)
{
    if constexpr (sizeof(T) == 8) {
        typedef typename std::conditional<std::is_signed<T>::value,i8,u8>::type x8;
        typedef typename std::conditional<std::is_signed<T>::value,i16,u16>::type x16;
        typedef typename std::conditional<std::is_signed<T>::value,i24,u24>::type x24;
        typedef typename std::conditional<std::is_signed<T>::value,i32,u32>::type x32;
        typedef typename std::conditional<std::is_signed<T>::value,i48,u48>::type x48;
        typedef typename std::conditional<std::is_signed<T>::value,zvec_op_types_i64,zvec_op_types_u64>::type zvec_ops;
        zvec_ops *ops = (std::is_signed<T>::value) ? (zvec_ops*)get_zvec_ops_i64() : (zvec_ops*)get_zvec_ops_u64();
        switch (z) {
            case zvec_size_0: break;
            case zvec_size_8: ops->encode_abs_x8(in, (x8*)comp, n); break;
            case zvec_size_16: ops->encode_abs_x16(in, (x16*)comp, n); break;
            case zvec_size_24: ops->encode_abs_x24(in, (x24*)comp, n); break;
            case zvec_size_32: ops->encode_abs_x32(in, (x32*)comp, n); break;
            case zvec_size_48: ops->encode_abs_x48(in, (x48*)comp, n); break;
            default: abort(); break;
        }
    }
    if constexpr (sizeof(T) == 4) {
        typedef typename std::conditional<std::is_signed<T>::value,i8,u8>::type x8;
        typedef typename std::conditional<std::is_signed<T>::value,i16,u16>::type x16;
        typedef typename std::conditional<std::is_signed<T>::value,i24,u24>::type x24;
        typedef typename std::conditional<std::is_signed<T>::value,zvec_op_types_i32,zvec_op_types_u32>::type zvec_ops;
        zvec_ops *ops = (std::is_signed<T>::value) ? (zvec_ops*)get_zvec_ops_i32() : (zvec_ops*)get_zvec_ops_u32();
        switch (z) {
            case zvec_size_0: break;
            case zvec_size_8: ops->encode_abs_x8(in, (x8*)comp, n); break;
            case zvec_size_16: ops->encode_abs_x16(in, (x16*)comp, n); break;
            case zvec_size_24: ops->encode_abs_x24(in, (x24*)comp, n); break;
            default: abort(); break;
        }
    }
}

template <typename T>
void zvec_block_decode_abs(T * __restrict out, void * __restrict comp, size_t n, zvec_size z)
{
    if constexpr (sizeof(T) == 8) {
        typedef typename std::conditional<std::is_signed<T>::value,i8,u8>::type x8;
        typedef typename std::conditional<std::is_signed<T>::value,i16,u16>::type x16;
        typedef typename std::conditional<std::is_signed<T>::value,i24,u24>::type x24;
        typedef typename std::conditional<std::is_signed<T>::value,i32,u32>::type x32;
        typedef typename std::conditional<std::is_signed<T>::value,i48,u48>::type x48;
        typedef typename std::conditional<std::is_signed<T>::value,zvec_op_types_i64,zvec_op_types_u64>::type zvec_ops;
        zvec_ops *ops = (std::is_signed<T>::value) ? (zvec_ops*)get_zvec_ops_i64() : (zvec_ops*)get_zvec_ops_u64();
        switch (z) {
            case zvec_size_8: ops->decode_abs_x8(out, (x8*)comp, n); break;
            case zvec_size_16: ops->decode_abs_x16(out, (x16*)comp, n); break;
            case zvec_size_24: ops->decode_abs_x24(out, (x24*)comp, n); break;
            case zvec_size_32: ops->decode_abs_x32(out, (x32*)comp, n); break;
            case zvec_size_48: ops->decode_abs_x48(out, (x48*)comp, n); break;
            default: abort(); break;
        }
    }
    if constexpr (sizeof(T) == 4) {
        typedef typename std::conditional<std::is_signed<T>::value,i8,u8>::type x8;
        typedef typename std::conditional<std::is_signed<T>::value,i16,u16>::type x16;
        typedef typename std::conditional<std::is_signed<T>::value,i24,u24>::type x24;
        typedef typename std::conditional<std::is_signed<T>::value,zvec_op_types_i32,zvec_op_types_u32>::type zvec_ops;
        zvec_ops *ops = (std::is_signed<T>::value) ? (zvec_ops*)get_zvec_ops_i32() : (zvec_ops*)get_zvec_ops_u32();
        switch (z) {
            case zvec_size_8: ops->decode_abs_x8(out, (x8*)comp, n); break;
            case zvec_size_16: ops->decode_abs_x16(out, (x16*)comp, n); break;
            case zvec_size_24: ops->decode_abs_x24(out, (x24*)comp, n); break;
            default: abort(); break;
        }
    }
}

template <typename T>
void zvec_block_encode_rel(T * __restrict in, void * __restrict comp, size_t n, zvec_size z, T iv)
{
    if constexpr (sizeof(T) == 8) {
        typedef typename std::conditional<std::is_signed<T>::value,i8,u8>::type x8;
        typedef typename std::conditional<std::is_signed<T>::value,i16,u16>::type x16;
        typedef typename std::conditional<std::is_signed<T>::value,i24,u24>::type x24;
        typedef typename std::conditional<std::is_signed<T>::value,i32,u32>::type x32;
        typedef typename std::conditional<std::is_signed<T>::value,i48,u48>::type x48;
        typedef typename std::conditional<std::is_signed<T>::value,zvec_op_types_i64,zvec_op_types_u64>::type zvec_ops;
        zvec_ops *ops = (std::is_signed<T>::value) ? (zvec_ops*)get_zvec_ops_i64() : (zvec_ops*)get_zvec_ops_u64();
        switch (z) {
            case zvec_size_8: ops->encode_rel_x8(in, (x8*)comp, n, iv); break;
            case zvec_size_16: ops->encode_rel_x16(in, (x16*)comp, n, iv); break;
            case zvec_size_24: ops->encode_rel_x24(in, (x24*)comp, n, iv); break;
            case zvec_size_32: ops->encode_rel_x32(in, (x32*)comp, n, iv); break;
            case zvec_size_48: ops->encode_rel_x48(in, (x48*)comp, n, iv); break;
            default: abort(); break;
        }
    }
    if constexpr (sizeof(T) == 4) {
        typedef typename std::conditional<std::is_signed<T>::value,i8,u8>::type x8;
        typedef typename std::conditional<std::is_signed<T>::value,i16,u16>::type x16;
        typedef typename std::conditional<std::is_signed<T>::value,i24,u24>::type x24;
        typedef typename std::conditional<std::is_signed<T>::value,zvec_op_types_i32,zvec_op_types_u32>::type zvec_ops;
        zvec_ops *ops = (std::is_signed<T>::value) ? (zvec_ops*)get_zvec_ops_i32() : (zvec_ops*)get_zvec_ops_u32();
        switch (z) {
            case zvec_size_8: ops->encode_rel_x8(in, (x8*)comp, n, iv); break;
            case zvec_size_16: ops->encode_rel_x16(in, (x16*)comp, n, iv); break;
            case zvec_size_24: ops->encode_rel_x24(in, (x24*)comp, n, iv); break;
            default: abort(); break;
        }
    }
}

template <typename T>
void zvec_block_decode_rel(T * __restrict out, void * __restrict comp, size_t n, zvec_size z, T iv)
{
    if constexpr (sizeof(T) == 8) {
        typedef typename std::conditional<std::is_signed<T>::value,i8,u8>::type x8;
        typedef typename std::conditional<std::is_signed<T>::value,i16,u16>::type x16;
        typedef typename std::conditional<std::is_signed<T>::value,i24,u24>::type x24;
        typedef typename std::conditional<std::is_signed<T>::value,i32,u32>::type x32;
        typedef typename std::conditional<std::is_signed<T>::value,i48,u48>::type x48;
        typedef typename std::conditional<std::is_signed<T>::value,zvec_op_types_i64,zvec_op_types_u64>::type zvec_ops;
        zvec_ops *ops = (std::is_signed<T>::value) ? (zvec_ops*)get_zvec_ops_i64() : (zvec_ops*)get_zvec_ops_u64();
        switch (z) {
            case zvec_size_8: ops->decode_rel_x8(out, (x8*)comp, n, iv); break;
            case zvec_size_16: ops->decode_rel_x16(out, (x16*)comp, n, iv); break;
            case zvec_size_24: ops->decode_rel_x24(out, (x24*)comp, n, iv); break;
            case zvec_size_32: ops->decode_rel_x32(out, (x32*)comp, n, iv); break;
            case zvec_size_48: ops->decode_rel_x48(out, (x48*)comp, n, iv); break;
            default: abort(); break;
        }
    }
    if constexpr (sizeof(T) == 4) {
        typedef typename std::conditional<std::is_signed<T>::value,i8,u8>::type x8;
        typedef typename std::conditional<std::is_signed<T>::value,i16,u16>::type x16;
        typedef typename std::conditional<std::is_signed<T>::value,i24,u24>::type x24;
        typedef typename std::conditional<std::is_signed<T>::value,zvec_op_types_i32,zvec_op_types_u32>::type zvec_ops;
        zvec_ops *ops = (std::is_signed<T>::value) ? (zvec_ops*)get_zvec_ops_i32() : (zvec_ops*)get_zvec_ops_u32();
        switch (z) {
            case zvec_size_8: ops->decode_rel_x8(out, (x8*)comp, n, iv); break;
            case zvec_size_16: ops->decode_rel_x16(out, (x16*)comp, n, iv); break;
            case zvec_size_24: ops->decode_rel_x24(out, (x24*)comp, n, iv); break;
            default: abort(); break;
        }
    }
}

/*
 * high-level interface to scan, encode, and decode blocks
 *
 * - scan block for statistics with codec
 *   zvec_stats<T> zvec_block_scan(T * in, size_t n, zvec_codec codec);
 *
 * - select block codec and size from statistics
 *   zvec_format zvec_block_format(zvec_stats<T> s);
 *
 * - gather block iv and delta from statistics
 *   zvec_meta<T> zvec_block_metadata(zvec_stats<T> s);
 *
 * - block size from format
 *   size_t zvec_block_size(zvec_format fmt, size_t n);
 *
 * - block alignment from format
 *   size_t zvec_block_align(zvec_format fmt, size_t n);
 *
 * - encode block using metadata
 *   void zvec_block_encode(T * in, void * comp, size_t n, zvec_format fmt, zvec_meta<T> meta);
 *
 * - decode block using metadata
 *   void zvec_block_decode(T * out, void * comp, size_t n, zvec_format fmt, zvec_meta<T> meta);
 */

struct zvec_format
{
    u8 codec : 3;
    u8 size : 4;
};

template <typename T>
struct zvec_meta
{
    T iv;
    T dv;
};

/* scan block for statistics with codec */

template <typename T>
zvec_stats<T> zvec_block_scan(T * __restrict in, size_t n, zvec_codec codec)
{
    switch (codec) {
    case zvec_block_abs: return zvec_block_scan_abs(in, n);
    case zvec_block_rel: return zvec_block_scan_rel(in, n);
    case zvec_block_rel_or_abs: return zvec_block_scan_both(in, n);
    default: return zvec_stats<T> { zvec_codec_none };
    }    
}

/* select block codec and size from statistics */

template <typename T>
constexpr zvec_format zvec_block_format(zvec_stats<T> s)
{
    bool is_signed = std::is_signed<T>::value;
    switch (s.codec) {
    case zvec_block_abs:
        if (s.amin == s.amax) {
            return zvec_format { (u8)zvec_const_abs, 0 };
        } else {
            zvec_size size_abs = zvec_size_abs(s);
            return zvec_format { (u8)zvec_block_abs, (u8)size_abs };
        }
    case zvec_block_rel:
        if (s.dmin == s.dmax) {
            if (s.dmin == 0) {
                return zvec_format { (u8)zvec_const_abs, 0 };
            } else {
                return zvec_format { (u8)zvec_const_rel, 0 };
            }
        } else {
            zvec_size size_rel = zvec_size_rel(s);
            return zvec_format { (u8)zvec_block_rel, (u8)size_rel };
        }
    case zvec_block_rel_or_abs:
        if (s.dmin == s.dmax) {
            if (s.dmin == 0) {
                return zvec_format { (u8)zvec_const_abs, 0 };
            } else {
                return zvec_format { (u8)zvec_const_rel, 0 };
            }
        } else {
            zvec_size size_abs = zvec_size_abs(s);
            zvec_size size_rel = zvec_size_rel(s);
            if (size_abs <= size_rel) {
                return zvec_format { (u8)zvec_block_abs, (u8)size_abs };
            } else {
                return zvec_format { (u8)zvec_block_rel, (u8)size_rel };
            }
        }
    default:
        abort();
    }
    return zvec_format { (u8)zvec_codec_none, 0 };
}

/* gather block iv and delta from statistics */

template <typename T>
zvec_meta<T> zvec_block_metadata(zvec_stats<T> s)
{
    zvec_format fmt = zvec_block_format(s);
    switch (fmt.codec) {
    case zvec_block_abs:
        assert(s.codec == zvec_block_abs || s.codec == zvec_block_rel_or_abs);
        assert(s.amin != s.amax);
        return zvec_meta<T> { 0, 0 };
    case zvec_block_rel:
        assert(s.codec == zvec_block_rel || s.codec == zvec_block_rel_or_abs);
        assert(s.dmin != s.dmax);
        return zvec_meta<T> { s.iv, 0 };
    case zvec_const_abs:
        assert(s.codec == zvec_block_abs || s.codec == zvec_block_rel_or_abs);
        assert(s.amin == s.amax);
        return zvec_meta<T> { s.iv, 0 };
    case zvec_const_rel:
        assert(s.dmin == s.dmax);
        assert(s.codec == zvec_block_rel || s.codec == zvec_block_rel_or_abs);
        return zvec_meta<T> { s.iv, s.dmin };
    default:
        break;
    }
    return zvec_meta<T> { 0, 0 };
}

/* block size from format */

template <typename T>
size_t zvec_block_size(zvec_format fmt, size_t n)
{
    switch (fmt.codec) {
    case zvec_block_abs:
    case zvec_block_rel:
        return (zvec_size_bits((zvec_size)fmt.size) * n) >> 3;
    case zvec_const_abs:
    case zvec_const_rel:
        return 0;
    default:
        break;
    }
    return 0;
}

/* block alignment from format */

template <typename T>
size_t zvec_block_align(zvec_format fmt, size_t n)
{
    switch (fmt.codec) {
    case zvec_block_abs:
    case zvec_block_rel:
        return 64;
    case zvec_const_abs:
    case zvec_const_rel:
        return 0;
    default:
        break;
    }
    return 0;
}

/* encode block using metadata */

template <typename T>
void zvec_block_encode(T * __restrict in, void * __restrict comp, size_t n, zvec_format fmt, zvec_meta<T> meta)
{
    switch (fmt.codec) {
    case zvec_block_abs:
        zvec_block_encode_abs(in, comp, n, (zvec_size)fmt.size);
        break;
    case zvec_block_rel:
        zvec_block_encode_rel(in, comp, n, (zvec_size)fmt.size, meta.iv);
        break;
    case zvec_const_abs:
        break;
    case zvec_const_rel:
        break;
    }
}

/* decode block using metadata */

template <typename T>
void zvec_block_decode(T * __restrict out, void * __restrict comp, size_t n, zvec_format fmt, zvec_meta<T> meta)
{
    T dv;
    switch (fmt.codec) {
    case zvec_block_abs:
        zvec_block_decode_abs(out, comp, n, (zvec_size)fmt.size);
        break;
    case zvec_block_rel:
        zvec_block_decode_rel(out, comp, n, (zvec_size)fmt.size, meta.iv);
        break;
    case zvec_const_abs:
        zvec_block_synth_abs(out, n, meta.iv);
        break;
    case zvec_const_rel:
        zvec_block_synth_rel(out, n, meta.iv, meta.dv);
        break;
    }
}
