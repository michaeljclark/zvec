# Zip Vector

_zip_vector_ is a compressed variable length array that uses block codecs
to compress and decompress blocks of integers using variable bit-width
deltas. The integer block codecs are optimized for vector instruction sets
using Google's Highway C++ library for portable SIMD/vector intrinsics.

> _zip_vector_ reduces memory footprint and can speed up in-order
> traversal of arrays by lowering global memory bandwidth using
> its lightening fast vector compression codecs.

![zip-vector-delta-diagram](/images/zip-vector-delta.png)

## Introduction

The implementation transparently compresses and decompresses fixed
size pages to and from an indexed non-linear slab using simple block
compression codecs. The implementation is optimized for sequential
access using custom iterators but out of order accesses are possible
at the cost of performance. The implementation relies for performance
on the sum of compression latency plus the latency of transferring
compressed blocks to and from global memory being less than the cost
of transferring uncompressed blocks to and from global memory.

The order of compression and decompression of appropriately sized
blocks ensures that accesses to uncompressed data happen in the L1
and L2 caches whereas global memory accesses will read and write
compressed data thus effectively increasing global memory bandwidth.
Nevertheless, the primary goal is to reduce the in-memory footprint.

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

    int64_t sum1 = 0, sum2 = 0;
    for (size_t i = 0; i < vec.size(); i++) {
        sum1 += vec[i];
    }
    for (auto v : vec) {
        sum2 += v;
    }

    assert(sum1 == sum2);
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
values using signed deltas and initial value, or special blocks for
constant values and sequences with constant deltas.

 - _8, 16, 24, 32, and 48 bit signed and unsigned absolute values._
 - _8, 16, 24, 32, and 48 bit signed deltas with per block initial value._
 - _constants and sequences using per block initial value and delta._

Compression efficiency ranges from _12.5% (8-bit)_ to _75% (48-bit)_
or worst case 100% _(plus ~ 1% metadata overhead)_. The codecs can
compress sign-extended values thus canonical pointers on _x86_64_ will
use a maximum of 48-bits entropy but often a page of temporally
coherent pointers can be compressed with 16-bit or 24-bit deltas.

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

## Zip Vector Benchmarks

Benchmark of in-order read-only traversal of a compressed array.

These benchmarks show the throughput for arrays with statistics that
target each of the block compression codecs. _std::vector_ is compared
to _zip_vector_, with 1D iteration, and 2D iteration using a page-sized
block stride to take advantage of LLVM/Clang's auto-vectoriztion.

- Clang 14.0.0, Intel Core i9-7980XE, 4.3GHz, AVX-512
- GCC 11.2.0, Intel Core i9-7980XE, 4.3GHz, AVX-512

#### Clang 14.0.0, Intel Core i9-7980XE, 4.3GHz, AVX-512

_std::vector_

|benchmark                     | size(W)|time(ns)|        word/sec|     MiB/s|
|:-----------------------------|-------:|-------:|---------------:|---------:|
|std_vector_1D-abs-8           | 128MiW |  0.529 |  1,891,879,364 | 7216.947 |
|std_vector_1D-rel-8           | 128MiW |  0.530 |  1,887,875,087 | 7201.672 |
|std_vector_1D-abs-16          | 128MiW |  0.528 |  1,892,884,744 | 7220.782 |
|std_vector_1D-rel-16          | 128MiW |  0.529 |  1,889,013,391 | 7206.014 |
|std_vector_1D-abs-24          | 128MiW |  0.529 |  1,891,753,316 | 7216.466 |
|std_vector_1D-rel-24          | 128MiW |  0.530 |  1,887,662,411 | 7200.861 |
|std_vector_1D-abs-32          | 128MiW |  0.530 |  1,887,820,706 | 7201.464 |
|std_vector_1D-rel-32          | 128MiW |  0.531 |  1,884,319,681 | 7188.109 |
|std_vector_1D-abs-48          | 128MiW |  0.530 |  1,887,256,970 | 7199.314 |
|std_vector_1D-rel-48          | 128MiW |  0.529 |  1,889,144,232 | 7206.513 |

_zip_vector_ with 1D iteration

|benchmark                     | size(W)|time(ns)|        word/sec|     MiB/s|
|:-----------------------------|-------:|-------:|---------------:|---------:|
|zip_vector_1D-abs-8           | 128MiW |  1.125 |    888,653,417 | 3389.944 |
|zip_vector_1D-rel-8           | 128MiW |  1.237 |    808,660,226 | 3084.794 |
|zip_vector_1D-abs-16          | 128MiW |  1.193 |    838,248,093 | 3197.663 |
|zip_vector_1D-rel-16          | 128MiW |  1.279 |    782,044,413 | 2983.263 |
|zip_vector_1D-abs-24          | 128MiW |  1.359 |    735,859,932 | 2807.083 |
|zip_vector_1D-rel-24          | 128MiW |  1.466 |    682,128,029 | 2602.112 |
|zip_vector_1D-abs-32          | 128MiW |  1.285 |    778,149,841 | 2968.406 |
|zip_vector_1D-rel-32          | 128MiW |  1.386 |    721,330,995 | 2751.659 |
|zip_vector_1D-abs-48          | 128MiW |  1.474 |    678,340,116 | 2587.662 |
|zip_vector_1D-rel-48          | 128MiW |  1.566 |    638,719,513 | 2436.522 |

_zip_vector_ with 2D iteration

|benchmark                     | size(W)|time(ns)|        word/sec|     MiB/s|
|:-----------------------------|-------:|-------:|---------------:|---------:|
|zip_vector_2D-abs-8           | 128MiW |  0.159 |  6,278,942,558 |23952.265 |
|zip_vector_2D-rel-8           | 128MiW |  0.262 |  3,812,147,744 |14542.190 |
|zip_vector_2D-abs-16          | 128MiW |  0.232 |  4,305,196,718 |16423.022 |
|zip_vector_2D-rel-16          | 128MiW |  0.309 |  3,236,402,881 |12345.897 |
|zip_vector_2D-abs-24          | 128MiW |  0.393 |  2,546,376,549 | 9713.656 |
|zip_vector_2D-rel-24          | 128MiW |  0.486 |  2,055,610,740 | 7841.533 |
|zip_vector_2D-abs-32          | 128MiW |  0.370 |  2,700,407,564 |10301.237 |
|zip_vector_2D-rel-32          | 128MiW |  0.412 |  2,424,960,335 | 9250.490 |
|zip_vector_2D-abs-48          | 128MiW |  0.495 |  2,019,027,922 | 7701.980 |
|zip_vector_2D-rel-48          | 128MiW |  0.578 |  1,729,890,781 | 6599.010 |

#### GCC 11.2.0, Intel Core i9-7980XE, 4.3GHz, AVX-512

_std::vector_

|benchmark                     | size(W)|time(ns)|        word/sec|     MiB/s|
|:-----------------------------|-------:|-------:|---------------:|---------:|
|std_vector_1D-abs-8           | 128MiW |  0.565 |  1,770,115,009 | 6752.453 |
|std_vector_1D-rel-8           | 128MiW |  0.564 |  1,771,856,644 | 6759.097 |
|std_vector_1D-abs-16          | 128MiW |  0.564 |  1,772,953,257 | 6763.280 |
|std_vector_1D-rel-16          | 128MiW |  0.565 |  1,768,512,313 | 6746.339 |
|std_vector_1D-abs-24          | 128MiW |  0.566 |  1,767,232,942 | 6741.459 |
|std_vector_1D-rel-24          | 128MiW |  0.566 |  1,765,706,117 | 6735.634 |
|std_vector_1D-abs-32          | 128MiW |  0.565 |  1,769,965,661 | 6751.883 |
|std_vector_1D-rel-32          | 128MiW |  0.565 |  1,771,409,990 | 6757.393 |
|std_vector_1D-abs-48          | 128MiW |  0.567 |  1,764,744,806 | 6731.967 |
|std_vector_1D-rel-48          | 128MiW |  0.566 |  1,767,714,905 | 6743.297 |

_zip_vector_ with 1D iteration

|benchmark                     | size(W)|time(ns)|        word/sec|     MiB/s|
|:-----------------------------|-------:|-------:|---------------:|---------:|
|zip_vector_1D-abs-8           | 128MiW |  0.581 |  1,720,850,965 | 6564.525 |
|zip_vector_1D-rel-8           | 128MiW |  0.690 |  1,448,881,349 | 5527.044 |
|zip_vector_1D-abs-16          | 128MiW |  0.657 |  1,522,438,729 | 5807.643 |
|zip_vector_1D-rel-16          | 128MiW |  0.718 |  1,392,686,322 | 5312.677 |
|zip_vector_1D-abs-24          | 128MiW |  0.873 |  1,145,200,581 | 4368.594 |
|zip_vector_1D-rel-24          | 128MiW |  0.975 |  1,025,475,302 | 3911.878 |
|zip_vector_1D-abs-32          | 128MiW |  0.787 |  1,270,031,285 | 4844.785 |
|zip_vector_1D-rel-32          | 128MiW |  0.841 |  1,189,268,382 | 4536.699 |
|zip_vector_1D-abs-48          | 128MiW |  1.005 |    995,359,515 | 3796.995 |
|zip_vector_1D-rel-48          | 128MiW |  1.085 |    921,817,723 | 3516.456 |

_zip_vector_ with 2D iteration

|benchmark                     | size(W)|time(ns)|        word/sec|     MiB/s|
|:-----------------------------|-------:|-------:|---------------:|---------:|
|zip_vector_2D-abs-8           | 128MiW |  0.402 |  2,489,949,372 | 9498.403 |
|zip_vector_2D-rel-8           | 128MiW |  0.503 |  1,986,747,862 | 7578.842 |
|zip_vector_2D-abs-16          | 128MiW |  0.477 |  2,095,220,795 | 7992.633 |
|zip_vector_2D-rel-16          | 128MiW |  0.533 |  1,875,659,187 | 7155.072 |
|zip_vector_2D-abs-24          | 128MiW |  0.702 |  1,425,105,130 | 5436.345 |
|zip_vector_2D-rel-24          | 128MiW |  0.793 |  1,261,083,234 | 4810.651 |
|zip_vector_2D-abs-32          | 128MiW |  0.600 |  1,667,928,918 | 6362.644 |
|zip_vector_2D-rel-32          | 128MiW |  0.648 |  1,542,334,051 | 5883.537 |
|zip_vector_2D-abs-48          | 128MiW |  0.816 |  1,225,588,347 | 4675.249 |
|zip_vector_2D-rel-48          | 128MiW |  0.894 |  1,118,325,013 | 4266.071 |

## ZVec Codec Benchmarks

Benchmarks for the _ZVec_ integer compression codecs.

#### ZVec Scan Block

![bench-zvec-scan](/images/bench-zvec-scan.png)

_Figure 1: Benchmark ZVec Block Encode, Intel Core i9-7980XE, 4.3GHz, AVX-512_

#### ZVec Synthesize Block

![bench-zvec-synth](/images/bench-zvec-synth.png)

_Figure 2: Benchmark ZVec Synthesize Block, Intel Core i9-7980XE, 4.3GHz, AVX-512_

#### ZVec Encode Block

![bench-zvec-encode](/images/bench-zvec-encode.png)

_Figure 3: Benchmark ZVec Encode Block, Intel Core i9-7980XE, 4.3GHz, AVX-512_

#### ZVec Decode Block

![bench-zvec-decode](/images/bench-zvec-decode.png)

_Figure 4: Benchmark ZVec Decode Block, Intel Core i9-7980XE, 4.3GHz, AVX-512_

## Build Instructions

It is recommended to build using Ninja:

```
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -- --verbose
```
