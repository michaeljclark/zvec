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
#include <cstdarg>
#include <cassert>
#include <cstdint>

#include <zvec_codecs.h>
#include <zvec_dispatch.h>
#include <zvec_block.h>
#include <zvec_bits.h>
#include <zvec_logger.h>

#define _sizebits(T) (sizeof(T)<<3)

template<typename T>
static inline T* _align_ptr(T* ptr, size_t n)
{
    return (char*)((intptr_t)(ptr+n-1) & ~(n-1));
}

static inline size_t _align_offset(size_t offset, size_t n)
{
    return (offset + (n-1)) & ~(n-1);
}

static inline size_t _pow2_ge(size_t x)
{
    return 1ull << (_sizebits(size_t) - clz(x-1));
}

template <typename V = i64, typename I = i64, size_t Q = (4096/sizeof(V))>
struct zip_vector
{
    static constexpr I page_size = Q * sizeof(V);
    static constexpr I page_interval = Q;
    static constexpr I page_shift = ilog2(page_interval);

    static constexpr size_t invalid_offset = (size_t)(-1ll);

    typedef I index_type;
    typedef V value_type;

    struct ref;
    struct iterator;

    struct ref
    {
        zip_vector &vec;
        size_t y, x;

        operator V();
        V* operator&();
        ref& operator=(V val);

        ref operator++();
        ref operator++(int);
        bool operator==(const ref &o) const;
        bool operator!=(const ref &o) const;
    };

    struct iterator
    {
        ref r;

        iterator operator++();
        iterator operator++(int);
        ref operator*();
        ref* operator->();
        bool operator==(const iterator &o) const;
        bool operator!=(const iterator &o) const;
    };

    struct page_idx { zvec_meta<V> meta; size_t offset; zvec_format format; };

    page_idx       *_page_idx;     /* compressed page IV, delta, offset, fmt */
    size_t          _page_count;   /* number of metadata pages allocated */
    char           *_slab_ptr;     /* slab of compressed data (base) */
    char           *_slab_data;    /* slab of compressed data (aligned) */
    size_t          _slab_limit;   /* size of slab array */
    u64            *_bmap_data;    /* bitmap of allocated space */
    size_t          _bmap_last;    /* last bitmap allocation offset */
    size_t          _active_page;  /* page number of active area */
    size_t          _active_area;  /* offset to active area within slab */
    I               _count;        /* number of elements in the vector */
    bool            _dirty;        /* active area is dirty */

    constexpr I f_page_round(I count) { return (count + Q - 1) & ~(Q - 1); }
    constexpr size_t f_page_num(I count) { return (size_t)(count >> page_shift); }
    constexpr size_t f_page_offset(I count) { return (size_t)(count & (Q - 1)); }

    static constexpr zvec_size zvec_max_size =
        sizeof(V) == 8 ? zvec_size_64 :
        sizeof(V) == 4 ? zvec_size_32 :
                         zvec_size_0;

    zip_vector(I count);
    zip_vector();
    ~zip_vector();

    iterator begin();
    iterator end();

    void resize(I count);
    I size();
    void sync();

    void resize_slab(size_t next_limit);
    size_t alloc_slab(zvec_size size);
    void dealloc_slab(zvec_size size, size_t offset);

    static std::pair<size_t,size_t> scan_bitmap(u64 *x, size_t offset,
        size_t limit, std::function<bool(size_t,size_t)> f);
    static void set_bitmap(u64 *x, size_t o, size_t l, bool v);

    size_t alloc_bitmap(zvec_size size);
    void dealloc_bitmap(zvec_size size, size_t offset);

    void switch_page(size_t y);
    void write_element(size_t y, size_t x, V val);
    V read_element(size_t y, size_t x);
    V* addr_element(size_t y, size_t x);

    ref operator[](I idx);
};

template <typename V, typename I, size_t Q>
inline zip_vector<V,I,Q>::zip_vector()
    : _page_idx(nullptr),
      _page_count(0),
      _slab_ptr(nullptr),
      _slab_data(nullptr),
      _slab_limit(0),
      _bmap_data(nullptr),
      _bmap_last(0),
      _active_page((size_t)-1ll),
      _active_area((size_t)-1ll),
      _count(0),
      _dirty(false)
{
    resize_slab(page_size * 2);
 }

template <typename V, typename I, size_t Q>
inline zip_vector<V,I,Q>::zip_vector(I count) : zip_vector()
{
    resize(count);
}

template <typename V, typename I, size_t Q>
inline zip_vector<V,I,Q>::~zip_vector()
{
    free(_page_idx);
    free(_slab_ptr);
    free(_bmap_data);
}

template <typename V, typename I, size_t Q>
inline I zip_vector<V,I,Q>::size()
{
    return _count;
}

template <typename V, typename I, size_t Q>
inline void zip_vector<V,I,Q>::resize(I count)
{
    size_t next_count = f_page_num(f_page_round(1ull << ilog2(count)));

    if (next_count > _page_count)
    {
        size_t prev_count = _page_count;
        size_t prev_size = sizeof(page_idx) * prev_count;
        size_t next_size = sizeof(page_idx) * next_count;
        _page_count = next_count;
        _page_idx = (page_idx*)realloc(_page_idx, next_size);
        memset((char*)_page_idx + prev_size, 0, next_size - prev_size);
    }

    _count = count;
}


template <typename V, typename I, size_t Q>
inline void zip_vector<V,I,Q>::sync()
{
    switch_page(_page_count);
}

template <typename V, typename I, size_t Q>
inline void zip_vector<V,I,Q>::resize_slab(size_t next_limit)
{
    if (next_limit > _slab_limit)
    {
        /* bitmap is 512:1, 1-bit = 64-bytes, 1-byte = 512-bytes */
        size_t old_bmap_size = _slab_limit >> 9;
        size_t new_bmap_size = next_limit >> 9;
        _bmap_data = (u64*)realloc(_bmap_data, new_bmap_size);
        memset((char*)_bmap_data + old_bmap_size, 0, new_bmap_size - old_bmap_size);

        /* use malloc/memcpy/free for the slab. we need realloc_aligned
         * which can't be emulated with realloc because the alignment
         * adjustment can change messing up the alignment of objects */
        char* prev_slab_data = _slab_data;
        char* next_slab_ptr = (char*)malloc(next_limit + 64);
        char* next_slab_data = _align_ptr<char>(next_slab_ptr, 64);
        memcpy(next_slab_data, prev_slab_data, _slab_limit);
        free(_slab_ptr);
        _slab_ptr = next_slab_ptr;
        _slab_data = next_slab_data;
        _slab_limit = next_limit;
        Trace("resize_slab %zu\n", next_limit);
    }
}

template <typename V, typename I, size_t Q>
inline size_t zip_vector<V,I,Q>::alloc_slab(zvec_size size)
{
    size_t offset = alloc_bitmap(size);
    size_t nbytes = (zvec_size_bits(size) * Q) >> 3;
    if (offset == invalid_offset) {
        resize_slab(_slab_limit << 1);
        offset =  alloc_bitmap(size);
    }
    Trace("alloc_slab: allocated %zd bytes at offset %zd", nbytes, offset);
    return offset;
}

template <typename V, typename I, size_t Q>
inline void zip_vector<V,I,Q>::dealloc_slab(zvec_size size, size_t offset)
{
    size_t nbytes = (zvec_size_bits(size) * Q) >> 3;
    Trace("dealloc_slab: freeing %zd bytes at offset %zd", nbytes, offset);
    dealloc_bitmap(size, offset);
}

template <typename V, typename I, size_t Q>
inline std::pair<size_t,size_t> zip_vector<V,I,Q>::scan_bitmap(u64 *x,
    size_t offset, size_t limit, std::function<bool(size_t,size_t)> f)
{
    u64 v = 0;
    size_t i = 0, c = 0, lo, ll = 0, no, nl, ni, b0, b1;

    auto next = [&] () -> u64 { ni = (offset + i++) % limit; return x[ni]; };

    do {
        /* fetch next word if there are no bits */
        if (c == 0 && i < limit) {
            v = next();
            c = 64;
        }

        /* find range of used bits */
        b1 = std::min((size_t)ctz(~v), c);
        v >>= b1;
        c -= b1;

        /* fetch next word if there are no bits */
        if (c == 0 && i < limit) {
            v = next();
            c = 64;
        }

        /* find range of free bits */
        b0 = std::min((size_t)ctz(v), c);
        v >>= b0;
        c -= b0;

        /* create bit address */
        no = (ni << 6) + 64 - c - b0;
        nl = b0;

        /* merge with range in last round or emit */
        if (lo + ll == no) {
            no = lo;
            nl += ll;
        } else if (ll != 0 && f(lo, ll)) {
            return { lo, ll };
        }

        /* save next range */
        lo = no;
        ll = nl;
    }
    while (i < limit || c > 0);

    if (ll != 0 && f(lo, ll)) {
        return { lo, ll };
    } else {
        return { invalid_offset, invalid_offset };
    }
}

template <typename V, typename I, size_t Q>
inline void zip_vector<V,I,Q>::set_bitmap(u64 *x, size_t o, size_t l, bool v)
{
    size_t w = o >> 6, b = o & 63;
    if (v) {
        x[w] |= (((u64)(-1ll)) >> (64-l)) << b;
        if (b + l > 64) {
            x[w+1] |= (((u64)(-1ll)) >> (128-b-l));
        }
    } else {
        x[w] &= ~((((u64)(-1ll)) >> (64-l)) << b);
        if (b + l > 64) {
            x[w+1] &= ~((((u64)(-1ll)) >> (128-b-l)));
        }
    }
}

template <typename V, typename I, size_t Q>
inline size_t zip_vector<V,I,Q>::alloc_bitmap(zvec_size size)
{
    /*
     * find block matching our size request.
     *
     * currently finds the first block. there is potential to scan several
     * blocks and find a block that when split creates a popular size.
     * requires global block size statistics and scanned size statistics.
     */

    size_t lines = (zvec_size_bits(size) * Q) >> 9;
    auto scan = [=] (size_t o, size_t l) -> bool { return (l >= lines); };
    auto r = scan_bitmap(_bmap_data, _bmap_last, _slab_limit >> 12, scan);

    if (r.first == invalid_offset) {
        return invalid_offset;
    } else {
        _bmap_last = r.first >> 6;
        set_bitmap(_bmap_data, r.first, lines, true);
        return r.first << 6;
    }
}

template <typename V, typename I, size_t Q>
inline void zip_vector<V,I,Q>::dealloc_bitmap(zvec_size size, size_t offset)
{
    set_bitmap(_bmap_data, offset >> 6, (zvec_size_bits(size) * Q) >> 9, false);
}

template <typename V, typename I, size_t Q>
inline void zip_vector<V,I,Q>::switch_page(size_t y1)
{
    size_t y0 = _active_page;
    size_t a = _active_area;

    page_idx prev_idx = y0 < _page_count ? _page_idx[y0] : page_idx{};
    zvec_format prev_format = prev_idx.format;
    size_t prev_offset = prev_idx.offset;
    zvec_codec prev_codec = (zvec_codec)prev_format.codec;
    zvec_size prev_size = (zvec_size)prev_format.size;

    zvec_stats<V> mod_stats = {};
    zvec_format mod_format = {};
    size_t mod_offset = 0;
    zvec_codec mod_codec = zvec_codec_none;
    zvec_size mod_size = zvec_size_0;
    zvec_meta<V> mod_meta;

    Trace("switch_page: y0=%zu y1=%zu", y0, y1);
    Trace("switch_page: index y0=%zd a=%zd format=%s:%zd offset=%zd",
        y0, a, zvec_codec_name(prev_codec), zvec_size_bits(prev_size), prev_offset);

    if (_dirty)
    {
        mod_stats = zvec_block_scan((V*)(_slab_data + a), Q,
                                          zvec_block_rel_or_abs);
        mod_format = zvec_block_format(mod_stats);
        mod_codec = (zvec_codec)mod_format.codec;
        mod_size = (zvec_size)mod_format.size;
        mod_meta = zvec_block_metadata(mod_stats);

        Trace("switch_page: scan y0=%zd a=%zd format=%s:%zd "
            "(amin=%lld amax=%lld dmin=%lld dmax=%lld)", y0, a,
            zvec_codec_name(mod_codec), zvec_size_bits(mod_size),
            mod_stats.amin, mod_stats.amax, mod_stats.dmin, mod_stats.dmax);

        if (mod_size == zvec_size_0) {
            mod_offset = invalid_offset;
            if (mod_size != prev_size && prev_size != zvec_size_0) {
                dealloc_slab(prev_size, prev_offset);
            }
        }
        else if (mod_size == zvec_max_size) {
            mod_offset = a;
            a = invalid_offset;
            if (mod_size != prev_size && prev_size != zvec_size_0) {
                dealloc_slab(prev_size, prev_offset);
            }
        }
        else {
            if (mod_size == prev_size) {
                mod_offset = prev_offset;
            } else {
                mod_offset = alloc_slab(mod_size);
            }
            Trace("switch_page: compress y0=%zd a=%zd fmt=%s:%zd dst=%zd",
                y0, a, zvec_codec_name(mod_codec), zvec_size_bits(mod_size), mod_offset);
            zvec_block_encode((V*)(_slab_data + a),
                              (void*)(_slab_data + mod_offset),
                              Q, mod_format, mod_meta);
            if (mod_size != prev_size && prev_size != zvec_size_0) {
                dealloc_slab(prev_size, prev_offset);
            }
        }

        _page_idx[y0].offset = mod_offset;
        _page_idx[y0].format = mod_format;
        _page_idx[y0].meta = mod_meta;

        _dirty = false;
    }

    /*
     * if last page was accessed or updated inplace we clear active area as
     * it is still inuse. if it an inplace page changes size then the active
     * area can be reused, hence we check modified size for dirty pages.
     */
    if (prev_codec == zvec_block_abs && prev_size == zvec_max_size &&
        (!_dirty || (_dirty && mod_codec == zvec_block_abs && mod_size == zvec_max_size))) {
        a = invalid_offset;
    }

    page_idx next_idx = y1 < _page_count ? _page_idx[y1] : page_idx{};
    zvec_format next_format = next_idx.format;
    size_t next_offset = next_idx.offset;
    zvec_codec next_codec = (zvec_codec)next_format.codec;
    zvec_size next_size = (zvec_size)next_format.size;
    zvec_meta<V> next_meta = next_idx.meta;

    if (y1 < _page_count)
    {
        _active_page = y1;

        if (next_size == zvec_max_size) {
            if (a != invalid_offset) {
                dealloc_slab(zvec_max_size, a);
            }
            a = next_offset;
            Trace("switch_page: inplace y1=%zd a=%zd fmt=%s:%zd",
                y1, a, zvec_codec_name(next_codec), zvec_size_bits(next_size));
        } else {
            if (a == invalid_offset) {
                a = alloc_slab(zvec_max_size);
            }
            if (next_codec == zvec_codec_none) {
                Trace("switch_page: zero y1=%zd a=%zd", y1, a)
                memset(_slab_data + a, 0, Q * sizeof(V));
            } else {
                Trace("switch_page: decompress y1=%zd a=%zd fmt=%s:%zd src=%zd",
                    y1, a, zvec_codec_name(next_codec), zvec_size_bits(next_size),
                    next_offset);
                zvec_block_decode((V*)(_slab_data + a),
                                  (void*)(_slab_data + next_offset),
                                  Q, next_format, next_meta);
            }
        }
    }

    _active_area = a;
}

template <typename V, typename I, size_t Q>
inline typename zip_vector<V,I,Q>::ref zip_vector<V,I,Q>::operator[](I idx)
{
    size_t y = f_page_num(idx), x = f_page_offset(idx);
    return ref { *this, y, x };
}

template <typename V, typename I, size_t Q>
inline void zip_vector<V,I,Q>::write_element(size_t y, size_t x, V val)
{
    if (y != _active_page) switch_page(y);
    _dirty |= true;
    ((V*)(_slab_data + _active_area))[x] = val;
}

template <typename V, typename I, size_t Q>
inline V zip_vector<V,I,Q>::read_element(size_t y, size_t x)
{
    if (y != _active_page) switch_page(y);
    return ((V*)(_slab_data + _active_area))[x];
}

template <typename V, typename I, size_t Q>
inline V* zip_vector<V,I,Q>::addr_element(size_t y, size_t x)
{
    if (y != _active_page) switch_page(y);
    return ((V*)(_slab_data + _active_area)) + x;
}

template <typename V, typename I, size_t Q>
inline zip_vector<V,I,Q>::ref::operator V()
{
    return vec.read_element(y, x);
}

template <typename V, typename I, size_t Q>
inline V* zip_vector<V,I,Q>::ref::operator&()
{
    return vec.addr_element(y, x);
}

template <typename V, typename I, size_t Q>
inline typename zip_vector<V,I,Q>::ref& zip_vector<V,I,Q>::ref::operator=(V val)
{
    vec.write_element(y, x, val);
    return *this;
}

template <typename V, typename I, size_t Q>
inline typename zip_vector<V,I,Q>::iterator zip_vector<V,I,Q>::begin()
{
    return iterator { ref { *this, 0, 0 } };
}

template <typename V, typename I, size_t Q>
inline typename zip_vector<V,I,Q>::iterator zip_vector<V,I,Q>::end()
{
    return iterator { ref { *this, f_page_num(_count), f_page_offset(_count) } };
}

template <typename V, typename I, size_t Q>
inline typename zip_vector<V,I,Q>::ref zip_vector<V,I,Q>::ref::operator++()
{
    if (++x == Q) {
        ++y;
        x = 0;
    }
    return *this;
}

template <typename V, typename I, size_t Q>
inline typename zip_vector<V,I,Q>::ref zip_vector<V,I,Q>::ref::operator++(int)
{
    ref q = *this;
    operator++();
    return q;
}

template <typename V, typename I, size_t Q>
inline bool zip_vector<V,I,Q>::ref::operator==(const typename zip_vector<V,I,Q>::ref &o) const
{
    return std::tie(y, x) == std::tie(o.y, o.x);
}

template <typename V, typename I, size_t Q>
inline bool zip_vector<V,I,Q>::ref::operator!=(const typename zip_vector<V,I,Q>::ref &o) const
{
    return std::tie(y, x) != std::tie(o.y, o.x);
}

template <typename V, typename I, size_t Q>
inline typename zip_vector<V,I,Q>::iterator zip_vector<V,I,Q>::iterator::operator++()
{
    ++r;
    return *this;
}

template <typename V, typename I, size_t Q>
inline typename zip_vector<V,I,Q>::iterator zip_vector<V,I,Q>::iterator::operator++(int)
{
    ref q = r;
    ++r;
    return typename zip_vector<V,I,Q>::iterator { q };
}

template <typename V, typename I, size_t Q>
inline typename zip_vector<V,I,Q>::ref zip_vector<V,I,Q>::iterator::operator*()
{
    return r;
}

template <typename V, typename I, size_t Q>
inline typename zip_vector<V,I,Q>::ref * zip_vector<V,I,Q>::iterator::operator->()
{
    return &r;
}

template <typename V, typename I, size_t Q>
inline bool zip_vector<V,I,Q>::iterator::operator==(const typename zip_vector<V,I,Q>::iterator &o) const
{
    return r == o.r;
}

template <typename V, typename I, size_t Q>
inline bool zip_vector<V,I,Q>::iterator::operator!=(const typename zip_vector<V,I,Q>::iterator &o) const
{
    return r != o.r;
}
