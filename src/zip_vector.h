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

template <typename V = i64, typename I = i64, size_t Q = 512>
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

    struct slab_bin { size_t _offset; size_t _next; };
    struct page_idx { zvec_meta<V> meta; size_t offset; zvec_format format; };

    page_idx       *_page_idx;     /* compressed page IV, delta, offset, fmt */
    size_t          _page_count;   /* number of metadata pages allocated */
    char           *_slab_ptr;     /* slab of compressed data (base) */
    char           *_slab_data;    /* slab of compressed data (aligned) */
    size_t          _slab_free;    /* slab offset to unallocated space */
    size_t          _slab_limit;   /* size of slab array */
    slab_bin       *_bin_data;     /* bins of freed blocks */
    size_t          _bin_free;     /* bins offset to unallocated space */
    size_t          _bin_limit;    /* size of bins array */
    size_t          _active_page;  /* page number of active area */
    size_t          _active_area;  /* offset to active area within slab */
    I               _count;        /* number of elements in the vector */
    bool            _dirty;        /* active area is dirty */

    static constexpr size_t _slab_align = 64; /* slab alignment for AVX-512 */
    static constexpr size_t _bin_roots = 12;  /* number of roots in bins array */

    constexpr I f_page_round(I count) { return (count + Q - 1) & ~(Q - 1); }
    constexpr size_t f_page_num(I count) { return (size_t)(count >> page_shift); }
    constexpr size_t f_page_offset(I count) { return (size_t)(count & (Q - 1)); }

    zip_vector(I count);
    zip_vector();
    ~zip_vector();

    iterator begin();
    iterator end();

    void resize(I count);
    I size();
    void sync();

    void resize_slab(size_t next_limit);
    size_t alloc_slab(zvec_size size, size_t align = _slab_align);
    void dealloc_slab(zvec_size size, size_t offset);

    void resize_bins(size_t next_limit);
    size_t scan_bins();
    size_t alloc_bin();
    void dealloc_bin(size_t bin);
    void push_bin(zvec_size size, size_t offset);
    size_t pop_bin(zvec_size size);

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
      _slab_free(0),
      _slab_limit(0),
      _bin_data(nullptr),
      _bin_free(0),
      _bin_limit(0),
      _active_page((size_t)-1ll),
      _active_area((size_t)-1ll),
      _count(0),
      _dirty(false)
{
    resize_slab(page_size * 2);
    resize_bins(_bin_roots * 2);
    _bin_free = _bin_roots;
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
    free(_bin_data);
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
    if (next_limit > _slab_limit) {
        /* we use malloc/memcpy/free because we need realloc_aligned
         * which can't be emulated with realloc because the alignment
         * adjustment can change messing up the alignment of objects */
        char* prev_slab_data = _slab_data;
        char* next_slab_ptr = (char*)malloc(next_limit + _slab_align);
        char* next_slab_data = _align_ptr<char>(next_slab_ptr, _slab_align);
        memcpy(next_slab_data, prev_slab_data, _slab_free);
        free(_slab_ptr);
        _slab_ptr = next_slab_ptr;
        _slab_data = next_slab_data;
        _slab_limit = next_limit;
    }
}

template <typename V, typename I, size_t Q>
inline size_t zip_vector<V,I,Q>::alloc_slab(zvec_size size, size_t align)
{
    size_t offset = pop_bin(size);
    size_t nbytes = (zvec_size_bits(size) * Q) >> 3;
    if (offset != invalid_offset) {
        Trace("alloc_slab: reusing %zd bytes at offset %zd", nbytes, offset);
        return offset;
    } else {
        size_t req_base = _align_offset(_slab_free, align);
        size_t req_limit = _pow2_ge(req_base + nbytes);
        offset = req_base;
        resize_slab(req_limit);
        _slab_free += nbytes;
        Trace("alloc_slab: allocated %zd bytes at offset %zd", nbytes, offset);
        return offset;
    }
}

template <typename V, typename I, size_t Q>
inline void zip_vector<V,I,Q>::dealloc_slab(zvec_size size, size_t offset)
{
    size_t nbytes = (zvec_size_bits(size) * Q) >> 3;
    Trace("dealloc_slab: freeing %zd bytes at offset %zd", nbytes, offset);
    push_bin(size, offset);
}

template <typename V, typename I, size_t Q>
inline void zip_vector<V,I,Q>::resize_bins(size_t next_limit)
{
    if (next_limit > _bin_limit) {
        size_t prev_size = sizeof(slab_bin) * _bin_limit;
        size_t next_size = sizeof(slab_bin) * next_limit;
        _bin_limit = next_limit;
        _bin_data = (slab_bin*)realloc(_bin_data, next_size);
        memset((char*)_bin_data + prev_size, -1, next_size - prev_size);
    }
}

template <typename V, typename I, size_t Q>
inline size_t zip_vector<V,I,Q>::scan_bins()
{
    /* circular scan for free bins but skips bin_roots */
    for (size_t i = 0; i < (_bin_limit - _bin_roots); i++) {
        size_t o = _bin_free + i < _bin_limit ? _bin_free + i
                 : _bin_free + i - _bin_limit + _bin_roots;
        if (_bin_data[o]._offset == invalid_offset) {
            return o;
        }
    }
    return invalid_offset;
}

template <typename V, typename I, size_t Q>
inline size_t zip_vector<V,I,Q>::alloc_bin()
{
    size_t bin = scan_bins();
    if (bin == invalid_offset) {
        resize_bins(_bin_limit * 2);
        bin = scan_bins();
        assert(bin != invalid_offset);
    }
    _bin_free = bin + 1;
    return bin;
}

template <typename V, typename I, size_t Q>
inline void zip_vector<V,I,Q>::dealloc_bin(size_t bin)
{
    _bin_data[bin]._offset = invalid_offset;
    _bin_data[bin]._next = invalid_offset;
    if (_bin_free == bin + 1) _bin_free = bin;
}

template <typename V, typename I, size_t Q>
inline void zip_vector<V,I,Q>::push_bin(zvec_size size, size_t offset)
{
    size_t _next = _bin_data[size-1]._next;
    size_t _offset = _bin_data[size-1]._offset;
    if (_offset == invalid_offset) {
        _bin_data[size-1]._offset = offset;
    } else {
        size_t bin = alloc_bin();
        _bin_data[bin]._next = _next;
        _bin_data[bin]._offset = _offset;
        _bin_data[size-1]._next = bin;
        _bin_data[size-1]._offset = offset;
    }
}

template <typename V, typename I, size_t Q>
inline size_t zip_vector<V,I,Q>::pop_bin(zvec_size size)
{
    size_t _next = _bin_data[size-1]._next;
    size_t _offset = _bin_data[size-1]._offset;
    if (_next == invalid_offset) {
        _bin_data[size-1]._offset = invalid_offset;
    } else {
        _bin_data[size-1]._next = _bin_data[_next]._next;
        _bin_data[size-1]._offset = _bin_data[_next]._offset;
        dealloc_bin(_next);
    }
    return _offset;
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
        else if (mod_size == zvec_size_64) {
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
    if (prev_codec == zvec_block_abs && prev_size == zvec_size_64 &&
        (!_dirty || (_dirty && mod_codec == zvec_block_abs && mod_size == zvec_size_64))) {
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

        if (next_size == zvec_size_64) {
            if (a != invalid_offset) {
                dealloc_slab(zvec_size_64, a);
            }
            a = next_offset;
            Trace("switch_page: inplace y1=%zd a=%zd fmt=%s:%zd",
                y1, a, zvec_codec_name(next_codec), zvec_size_bits(next_size));
        } else {
            if (a == invalid_offset) {
                a = alloc_slab(zvec_size_64);
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
