cmake_minimum_required(VERSION 3.10)

project(zvec)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(CheckCXXCompilerFlag)

check_cxx_compiler_flag("-pg" has_gprof
  "int main() { return 0; }")
check_cxx_compiler_flag("-fno-omit-frame-pointer" has_no_omit_fp
  "int main() { return 0; }")
check_cxx_compiler_flag("--coverage" has_coverage
  "int main() { return 0; }")
check_cxx_compiler_flag("-march=sandybridge" has_march_sandybridge
  "int main() { return 0; }")
check_cxx_compiler_flag("-march=skylake-avx512" has_march_skylake_avx512
  "int main() { return 0; }")

if (CMAKE_PROFILE AND has_gprof)
  add_compile_options(-pg)
endif()

if (has_no_omit_fp)
  add_compile_options(-fno-omit-frame-pointer)
endif()

set(HWY_ENABLE_CONTRIB OFF CACHE BOOL "Include contrib/")
set(HWY_ENABLE_EXAMPLES OFF CACHE BOOL "Build examples")
add_subdirectory(third_party/highway)

include_directories(src)
include_directories(third_party/highway)

set(zvec_sources
    src/zvec_logger.cc
    src/zvec_dispatch.cc
    src/zvec_arch_generic.cc)

if (has_march_skylake_avx512)
  list(APPEND zvec_sources src/zvec_arch_x86_avx3.cc)
  set_source_files_properties(src/zvec_arch_x86_avx3.cc
    PROPERTIES COMPILE_FLAGS -march=skylake-avx512)
endif()

add_library(zvec ${zvec_sources})

if (has_march_skylake_avx512)
  target_compile_definitions(zvec PRIVATE ZVEC_HAS_AVX3)
endif()

add_executable(bench-zip-vector tests/bench-zip-vector.cc)
target_link_libraries(bench-zip-vector zvec hwy)

add_executable(bench-zvec-codecs tests/bench-zvec-codecs.cc)
target_link_libraries(bench-zvec-codecs zvec hwy)

foreach(test_num RANGE 0 8)
  add_executable(test-zip-vector-block-${test_num}
    tests/test-zip-vector-block-${test_num}.cc)
  target_link_libraries(test-zip-vector-block-${test_num} zvec hwy)
endforeach()

if (has_coverage)
  target_compile_options(test-zip-vector-block-6 PRIVATE --coverage)
  target_link_options(test-zip-vector-block-6 PRIVATE --coverage)
endif()

add_executable(test-zvec-ll-generic tests/test-zvec-ll-generic.cc)
target_link_libraries(test-zvec-ll-generic zvec hwy)

if (has_march_skylake_avx512)
  add_executable(test-zvec-ll-x86-avx3 tests/test-zvec-ll-x86-avx3.cc)
  target_link_libraries(test-zvec-ll-x86-avx3 zvec hwy)
  target_compile_options(test-zvec-ll-x86-avx3 PRIVATE -march=skylake-avx512)
endif()
