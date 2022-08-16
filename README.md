# Zip Vector

_zip_vector_ is a compressed variable length array that uses vectorized
block codecs to compress and decompress integers using variable bit-width
deltas. The integer block codecs are optimized for vector instruction sets
using Google's Highway C++ library for portable SIMD/vector intrinsics.

> _zip_vector_ reduces memory footprint and can speed up in-order
> traversal of arrays by lowering global memory bandwidth using
> lightening fast vector compression.

![zip-vector-delta-diagram](/images/zip-vector-delta.svg)

## Introduction

The implementation transparently compresses and decompresses fixed
size pages to and from an indexed non-linear slab using simple block
compression codecs. The implementation is optimized for sequential
access using custom iterators but out of order accesses are possible
at the cost of performance. The implementation relies for performance
on the sum of compression latency plus the latency of transferring
compressed blocks to and from global memory being less than the cost
of transferring uncompressed blocks to and from global memory.

The implementation supports 64-bit and 32-bit array containers using
block compression codecs that perform width reduction for absolute
values, signed deltas for relative values, and special blocks for
constant values and sequences.

 - `zip_vector<int32_t>`
   - _{ 8, 16, 24 } bit signed and unsigned fixed-width values._
   - _{ 8, 16, 24 } bit signed deltas with per block initial value._
   - _constants and sequences using per block initial value and delta._
 - `zip_vector<int64_t>`
   - _{ 8, 16, 24, 32, 48 } bit signed and unsigned fixed-width values._
   - _{ 8, 16, 24, 32, 48 } bit signed deltas with per block initial value._
   - _constants and sequences using per block initial value and delta._

The order of compression and decompression of minimally sized blocks
ensures that accesses to uncompressed data happen in L1 and L2 caches
whereas global memory accesses read and write compressed data thus
effectively increasing global memory bandwidth. Nevertheless, the
primary goal is to reduce in-memory footprint.

The _zip_vector_ template is intended to be similar to _std::vector_
although the current prototype implementation does not yet implement
all traits present in _std::vector_, nor is it thread-safe, but it
does support iterators.

```C++
    zip_vector<int64_t> vec;

    vec.resize(8192);
    for (size_t i = 0; i < vec.size(); i++) {
        vec[i] = i;
    }

    int64_t s1 = 0, s2 = 0;
    for (size_t i = 0; i < vec.size(); i++) {
        s1 += vec[i];
    }
    for (auto v : vec) {
        s2 += v;
    }

    assert(s1 == s2);
```

## Implementation Notes

Internally a slab is organised to contain blocks of compressed and
uncompressed page data written in the order the vector is accessed.
Pages have a fixed power of 2 number of elements but blocks in the
slab are variable length due to the use of block compression codecs.
Blocks containing compressed pages in the slab are found using an
offsets array which is indexed by right shifting array indices.

Pages are scanned and compressed using fast but simple block compression
codecs that perform width reduction for small absolute values or encode
values using signed deltas, and special blocks for constant values and
sequences with constant deltas.

 - _8, 16, 24, 32, and 48 bit signed and unsigned absolute values._
 - _8, 16, 24, 32, and 48 bit signed deltas with per block initial value._
 - _constants and sequences using per block initial value and delta._

Compression efficiency ranges from _12.5% (8-bit)_ to _75% (48-bit)_
or worst case 100% _(plus ~ 1% metadata overhead)_. The codecs can
compress sign-extended values thus canonical pointers on _x86_64_ will
use a maximum of 48-bits. Sometimes pages of temporally coherent
pointers can be compressed with 16-bit or 24-bit deltas or even a
constant sequence simply using an initial value and delta. On _x86_64_
the codecs uses _runtime cpuid feature detection_ to select generic
code or AVX-512 optimized code.

There is one active area in the slab per thread to cache the current
page uncompressed. When a page boundary is crossed the active area is
scanned and recompressed to the slab. Blocks can transition from
compressed to uncompressed, uncompressed to compressed and they can
change size when recompressed.

If after scanning, it is found that the active area for the current
page can't be compressed, the offsets array will be updated to point
to uncompressed data which will subsequently be updated in-place.
When scanned again it may later transition back to a compressed state
in which case the uncompressed area will be returned to a binned free
list. If high entropy data is written in-order, the slab should end
up structured equivalently to a regular linear array.

Page dirty status is tracked so that if there are no write accesses to
a block then scanning and compression can be skipped and it is only
necessary to perform decompression when crossing block boundaries.
Please note this initial prototype implementation is not thread safe.

## Build Instructions

It is recommended to build using Ninja:

```
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -- --verbose
```

## Future Work

 - Support additional bit-widths
   - The current scheme uses these widths: _2^(n), 2^(n+1) + 2^(n)_
   - 1, 2, 3, 4, 6, and 12 bit deltas are still to be implemented.
 - Improve bitmap slab allocator
   - The current bitmap allocator uses a naive exhaustive first fit
     algorithm.
   - The slab bitmap could be partitioned based on page index to reduce
     worst case scan performance at the expense of requiring rebalancing
     of the slab partitions when the slab is resized.
   - The slab could be made denser by using a stochastic best fit
     algorithm that records size statistics to guide tactical choices
     about which bitmap chunks are split to match statistical demand
     for frequent block sizes.
 - Add multi-threading support
   - One approach is to add a per page reference count semaphore
     and to simply keep pages that are being concurrently accessed
     as uncompressed, with the last thread recompressing the page.
     There is some complexity for synchronizing slab resizes.

## Codec Support

This table shows vecotorized block codecs that have so far been implemented.

|          | bits | 48 | 32 | 24 | 16 | 12 |  8 |  6 |  4 |  3 |  2 |  1 |
|:--------:|:----:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|
| absoulte | i64  |  X |  X |  X |  X |    |  X |    |    |    |    |    |
|          | u64  |  X |  X |  X |  X |    |  X |    |    |    |    |    |
|          | i32  |    |    |  X |  X |    |  X |    |    |    |    |    |
|          | u32  |    |    |  X |  X |    |  X |    |    |    |    |    |
| relative | i64  |  X |  X |  X |  X |    |  X |    |    |    |    |    |
|          | u64  |  X |  X |  X |  X |    |  X |    |    |    |    |    |
|          | i32  |    |    |  X |  X |    |  X |    |    |    |    |    |
|          | u32  |    |    |  X |  X |    |  X |    |    |    |    |    |

## Benchmarks

Benchmarks are split into high level benchmarks for the array class
and low-level benchmarks for the block compression codecs.

- Zip Vector Array Benchmarks (benchmark program: `bench-zip-vector`)
- Low Level Codec Benchmarks (benchmark program: `bench-zvec-codecs`)

### Zip Vector Array Benchmarks

Benchmark of in-order read-only traversal of a compressed array.

These benchmarks show the throughput for arrays with statistics that
target each of the block compression codecs. _std::vector_ is compared
to _zip_vector_, with 1D iteration, and 2D iteration using a page-sized
block stride to take advantage of LLVM/Clang's auto-vectoriztion.

- Clang 14.0.0, Intel Core i9-7980XE, 4.3GHz, AVX-512
- GCC 11.2.0, Intel Core i9-7980XE, 4.3GHz, AVX-512

#### Clang 14.0.0, Intel Core i9-7980XE, 4.3GHz, AVX-512

_zip_vector<int64_t>_ with 2D iteration

|benchmark                     | size(W)|time(ns)|        word/sec|     MiB/s|
|:-----------------------------|-------:|-------:|---------------:|---------:|
|zip_vector_2D-abs-8           |  16MiW |  0.136 |  7,339,965,481 |  27999.7 |
|zip_vector_2D-rel-8           |  16MiW |  0.253 |  3,945,984,200 |  15052.7 |
|zip_vector_2D-abs-16          |  16MiW |  0.211 |  4,743,869,411 |  18096.4 |
|zip_vector_2D-rel-16          |  16MiW |  0.298 |  3,355,174,115 |  12799.0 |
|zip_vector_2D-abs-24          |  16MiW |  0.372 |  2,687,863,404 |  10253.4 |
|zip_vector_2D-rel-24          |  16MiW |  0.479 |  2,089,221,836 |   7969.7 |
|zip_vector_2D-abs-32          |  16MiW |  0.358 |  2,793,575,309 |  10656.6 |
|zip_vector_2D-rel-32          |  16MiW |  0.399 |  2,504,306,913 |   9553.2 |
|zip_vector_2D-abs-48          |  16MiW |  0.482 |  2,076,496,569 |   7921.2 |
|zip_vector_2D-rel-48          |  16MiW |  0.548 |  1,824,931,526 |   6961.6 |

_zip_vector<int32_t>_ with 2D iteration

|benchmark                     | size(W)|time(ns)|        word/sec|     MiB/s|
|:-----------------------------|-------:|-------:|---------------:|---------:|
|zip_vector_2D-abs-8           |  16MiW |  0.074 | 13,465,854,621 |  51368.2 |
|zip_vector_2D-rel-8           |  16MiW |  0.075 | 13,334,215,010 |  50866.0 |
|zip_vector_2D-abs-16          |  16MiW |  0.149 |  6,723,057,823 |  25646.4 |
|zip_vector_2D-rel-16          |  16MiW |  0.148 |  6,767,145,742 |  25814.6 |
|zip_vector_2D-abs-24          |  16MiW |  0.286 |  3,492,346,682 |  13322.2 |
|zip_vector_2D-rel-24          |  16MiW |  0.287 |  3,481,434,937 |  13280.6 |

_zip_vector<int64_t>_ with 1D iteration

|benchmark                     | size(W)|time(ns)|        word/sec|     MiB/s|
|:-----------------------------|-------:|-------:|---------------:|---------:|
|zip_vector_1D-abs-8           |  16MiW |  1.113 |    898,209,739 |   3426.4 |
|zip_vector_1D-rel-8           |  16MiW |  1.220 |    819,679,680 |   3126.8 |
|zip_vector_1D-abs-16          |  16MiW |  1.171 |    853,726,092 |   3256.7 |
|zip_vector_1D-rel-16          |  16MiW |  1.256 |    795,872,012 |   3036.0 |
|zip_vector_1D-abs-24          |  16MiW |  1.339 |    746,892,358 |   2849.2 |
|zip_vector_1D-rel-24          |  16MiW |  1.454 |    687,776,124 |   2623.7 |
|zip_vector_1D-abs-32          |  16MiW |  1.279 |    782,121,557 |   2983.6 |
|zip_vector_1D-rel-32          |  16MiW |  1.367 |    731,442,921 |   2790.2 |
|zip_vector_1D-abs-48          |  16MiW |  1.458 |    685,831,325 |   2616.2 |
|zip_vector_1D-rel-48          |  16MiW |  1.533 |    652,166,701 |   2487.8 |

_zip_vector<int32_t>_ with 1D iteration

|benchmark                     | size(W)|time(ns)|        word/sec|     MiB/s|
|:-----------------------------|-------:|-------:|---------------:|---------:|
|zip_vector_1D-abs-8           |  16MiW |  1.083 |    923,105,940 |   3521.4 |
|zip_vector_1D-rel-8           |  16MiW |  1.083 |    923,441,839 |   3522.7 |
|zip_vector_1D-abs-16          |  16MiW |  1.155 |    865,819,832 |   3302.8 |
|zip_vector_1D-rel-16          |  16MiW |  1.153 |    867,467,088 |   3309.1 |
|zip_vector_1D-abs-24          |  16MiW |  1.287 |    777,083,587 |   2964.3 |
|zip_vector_1D-rel-24          |  16MiW |  1.288 |    776,600,612 |   2962.5 |

_std::vector<int64_t>_

|benchmark                     | size(W)|time(ns)|        word/sec|     MiB/s|
|:-----------------------------|-------:|-------:|---------------:|---------:|
|std_vector_1D-abs-8           |  16MiW |  0.491 |  2,037,519,162 |   7772.5 |
|std_vector_1D-rel-8           |  16MiW |  0.486 |  2,055,820,499 |   7842.3 |
|std_vector_1D-abs-16          |  16MiW |  0.488 |  2,047,901,254 |   7812.1 |
|std_vector_1D-rel-16          |  16MiW |  0.486 |  2,057,335,612 |   7848.1 |
|std_vector_1D-abs-24          |  16MiW |  0.488 |  2,047,893,255 |   7812.1 |
|std_vector_1D-rel-24          |  16MiW |  0.485 |  2,060,771,399 |   7861.2 |
|std_vector_1D-abs-32          |  16MiW |  0.491 |  2,036,940,546 |   7770.3 |
|std_vector_1D-rel-32          |  16MiW |  0.508 |  1,970,149,975 |   7515.5 |
|std_vector_1D-abs-48          |  16MiW |  0.488 |  2,050,546,412 |   7822.2 |
|std_vector_1D-rel-48          |  16MiW |  0.489 |  2,045,396,563 |   7802.6 |

_std::vector<int32_t>_

|benchmark                     | size(W)|time(ns)|        word/sec|     MiB/s|
|:-----------------------------|-------:|-------:|---------------:|---------:|
|std_vector_1D-abs-8           |  16MiW |  0.227 |  4,399,989,194 |  16784.6 |
|std_vector_1D-rel-8           |  16MiW |  0.224 |  4,456,231,246 |  16999.2 |
|std_vector_1D-abs-16          |  16MiW |  0.224 |  4,460,175,033 |  17014.2 |
|std_vector_1D-rel-16          |  16MiW |  0.233 |  4,293,298,885 |  16377.6 |
|std_vector_1D-abs-24          |  16MiW |  0.229 |  4,370,102,289 |  16670.6 |
|std_vector_1D-rel-24          |  16MiW |  0.232 |  4,307,159,106 |  16430.5 |

#### GCC 11.2.0, Intel Core i9-7980XE, 4.3GHz, AVX-512

_zip_vector<int64_t>_ with 2D iteration

|benchmark                     | size(W)|time(ns)|        word/sec|     MiB/s|
|:-----------------------------|-------:|-------:|---------------:|---------:|
|zip_vector_2D-abs-8           |  16MiW |  0.388 |  2,577,607,383 |   9832.8 |
|zip_vector_2D-rel-8           |  16MiW |  0.505 |  1,978,698,920 |   7548.1 |
|zip_vector_2D-abs-16          |  16MiW |  0.465 |  2,151,404,781 |   8207.0 |
|zip_vector_2D-rel-16          |  16MiW |  0.529 |  1,891,680,714 |   7216.2 |
|zip_vector_2D-abs-24          |  16MiW |  0.682 |  1,467,151,472 |   5596.7 |
|zip_vector_2D-rel-24          |  16MiW |  0.788 |  1,268,457,762 |   4838.8 |
|zip_vector_2D-abs-32          |  16MiW |  0.610 |  1,638,794,975 |   6251.5 |
|zip_vector_2D-rel-32          |  16MiW |  0.668 |  1,497,905,530 |   5714.1 |
|zip_vector_2D-abs-48          |  16MiW |  0.795 |  1,258,082,232 |   4799.2 |
|zip_vector_2D-rel-48          |  16MiW |  0.882 |  1,133,306,070 |   4323.2 |

_zip_vector<int32_t>_ with 2D iteration

|benchmark                     | size(W)|time(ns)|        word/sec|     MiB/s|
|:-----------------------------|-------:|-------:|---------------:|---------:|
|zip_vector_2D-abs-8           |  16MiW |  0.526 |  1,899,431,185 |   7245.8 |
|zip_vector_2D-rel-8           |  16MiW |  0.526 |  1,900,954,056 |   7251.6 |
|zip_vector_2D-abs-16          |  16MiW |  0.596 |  1,677,024,963 |   6397.3 |
|zip_vector_2D-rel-16          |  16MiW |  0.596 |  1,679,239,464 |   6405.8 |
|zip_vector_2D-abs-24          |  16MiW |  0.785 |  1,273,253,906 |   4857.1 |
|zip_vector_2D-rel-24          |  16MiW |  0.783 |  1,276,540,527 |   4869.6 |

_zip_vector<int64_t>_ with 1D iteration

|benchmark                     | size(W)|time(ns)|        word/sec|     MiB/s|
|:-----------------------------|-------:|-------:|---------------:|---------:|
|zip_vector_1D-abs-8           |  16MiW |  1.020 |    980,372,353 |   3739.8 |
|zip_vector_1D-rel-8           |  16MiW |  1.134 |    881,642,641 |   3363.2 |
|zip_vector_1D-abs-16          |  16MiW |  1.091 |    916,833,601 |   3497.4 |
|zip_vector_1D-rel-16          |  16MiW |  1.161 |    861,238,444 |   3285.4 |
|zip_vector_1D-abs-24          |  16MiW |  1.298 |    770,233,637 |   2938.2 |
|zip_vector_1D-rel-24          |  16MiW |  1.423 |    702,710,323 |   2680.6 |
|zip_vector_1D-abs-32          |  16MiW |  1.238 |    807,769,314 |   3081.4 |
|zip_vector_1D-rel-32          |  16MiW |  1.279 |    781,678,405 |   2981.9 |
|zip_vector_1D-abs-48          |  16MiW |  1.407 |    710,610,011 |   2710.8 |
|zip_vector_1D-rel-48          |  16MiW |  1.508 |    663,060,725 |   2529.4 |

_zip_vector<int32_t>_ with 1D iteration

|benchmark                     | size(W)|time(ns)|        word/sec|     MiB/s|
|:-----------------------------|-------:|-------:|---------------:|---------:|
|zip_vector_1D-abs-8           |  16MiW |  0.762 |  1,312,336,224 |   5006.2 |
|zip_vector_1D-rel-8           |  16MiW |  0.761 |  1,313,326,131 |   5009.9 |
|zip_vector_1D-abs-16          |  16MiW |  0.849 |  1,178,253,224 |   4494.7 |
|zip_vector_1D-rel-16          |  16MiW |  0.849 |  1,178,316,529 |   4494.9 |
|zip_vector_1D-abs-24          |  16MiW |  1.030 |    970,811,341 |   3703.4 |
|zip_vector_1D-rel-24          |  16MiW |  1.034 |    967,127,596 |   3689.3 |

_std::vector<int64_t>_

|benchmark                     | size(W)|time(ns)|        word/sec|     MiB/s|
|:-----------------------------|-------:|-------:|---------------:|---------:|
|std_vector_1D-abs-8           |  16MiW |  0.565 |  1,769,713,987 |   6750.9 |
|std_vector_1D-rel-8           |  16MiW |  0.568 |  1,761,752,989 |   6720.6 |
|std_vector_1D-abs-16          |  16MiW |  0.570 |  1,754,592,031 |   6693.2 |
|std_vector_1D-rel-16          |  16MiW |  0.569 |  1,758,711,679 |   6709.0 |
|std_vector_1D-abs-24          |  16MiW |  0.570 |  1,754,429,283 |   6692.6 |
|std_vector_1D-rel-24          |  16MiW |  0.567 |  1,764,868,512 |   6732.4 |
|std_vector_1D-abs-32          |  16MiW |  0.570 |  1,754,771,878 |   6693.9 |
|std_vector_1D-rel-32          |  16MiW |  0.571 |  1,750,488,167 |   6677.6 |
|std_vector_1D-abs-48          |  16MiW |  0.570 |  1,755,106,344 |   6695.2 |
|std_vector_1D-rel-48          |  16MiW |  0.569 |  1,756,333,139 |   6699.9 |

_std::vector<int32_t>_

|benchmark                     | size(W)|time(ns)|        word/sec|     MiB/s|
|:-----------------------------|-------:|-------:|---------------:|---------:|
|std_vector_1D-abs-8           |  16MiW |  0.373 |  2,679,107,795 |  10220.0 |
|std_vector_1D-rel-8           |  16MiW |  0.369 |  2,707,621,476 |  10328.8 |
|std_vector_1D-abs-16          |  16MiW |  0.369 |  2,708,236,438 |  10331.1 |
|std_vector_1D-rel-16          |  16MiW |  0.370 |  2,699,387,340 |  10297.3 |
|std_vector_1D-abs-24          |  16MiW |  0.368 |  2,717,198,314 |  10365.3 |
|std_vector_1D-rel-24          |  16MiW |  0.372 |  2,685,961,839 |  10246.1 |

### Low Level Codec Benchmarks

Benchmarks of the low level integer block compression codecs using AVX-512
for 32-bit and 64-bit datatypes.

#### 64-bit datatype

##### ZVec Scan Block (64-bit)

![bench-zvec-scan-64](/images/bench-zvec-scan-64.png)

_Figure 1: Benchmark ZVec 64-bit Block Encode, Intel Core i9-7980XE, 4.3GHz, AVX-512_

##### ZVec Synthesize Block (64-bit)

![bench-zvec-synth-64](/images/bench-zvec-synth-64.png)

_Figure 2: Benchmark ZVec 64-bit Synthesize Block, Intel Core i9-7980XE, 4.3GHz, AVX-512_

##### ZVec Encode Block (64-bit)

![bench-zvec-encode-64](/images/bench-zvec-encode-64.png)

_Figure 3: Benchmark ZVec 64-bit Encode Block, Intel Core i9-7980XE, 4.3GHz, AVX-512_

##### ZVec Decode Block (64-bit)

![bench-zvec-decode-64](/images/bench-zvec-decode-64.png)

_Figure 4: Benchmark ZVec 64-bit Decode Block, Intel Core i9-7980XE, 4.3GHz, AVX-512_

#### 32-bit datatype

Benchmarks of the ZVec AVX-512 codecs with a 32-bit datatype.

##### ZVec Scan Block (64-bit)

![bench-zvec-scan-32](/images/bench-zvec-scan-32.png)

_Figure 5: Benchmark ZVec 32-bit Block Encode, Intel Core i9-7980XE, 4.3GHz, AVX-512_

##### ZVec Synthesize Block (64-bit)

![bench-zvec-synth-32](/images/bench-zvec-synth-32.png)

_Figure 6: Benchmark ZVec 32-bit Synthesize Block, Intel Core i9-7980XE, 4.3GHz, AVX-512_

##### ZVec Encode Block (64-bit)

![bench-zvec-encode-32](/images/bench-zvec-encode-32.png)

_Figure 7: Benchmark ZVec 32-bit Encode Block, Intel Core i9-7980XE, 4.3GHz, AVX-512_

##### ZVec Decode Block (64-bit)

![bench-zvec-decode-32](/images/bench-zvec-decode-32.png)

_Figure 8: Benchmark ZVec 32-bit Decode Block, Intel Core i9-7980XE, 4.3GHz, AVX-512_

## License

zip_vector and the zvec block codecs are available under _PLEASE LICENSE_,
an ISC derived license using authors' implied copyright as detailed under
The Berne Convention. The primary difference between PLEASE LICENSE and the
ISC license is that _PLEASE LICENSE_ emphasizes implied copyright which is
why _PLEASE LICENSE_ is positioned where the text Copyright would normally
be placed. There is no restriction stating that the copyright notice must be
retained. From this perspective it may seem close to public domain, however
it isn't public domain since copyright is not relinquished, rather it is
explicitly retained in an implied form to defend the free use of the work.

```
All rights to this work are granted for all purposes, with exception of
author's implied right of copyright to defend the free use of this work.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
```
