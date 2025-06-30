# LabDb Performance Benchmarking Suite
cmake_minimum_required(VERSION 3.20)

project(LabDbBenchmarks 
    VERSION 1.0
    DESCRIPTION "Performance benchmarking suite for LabDb TID-based architecture"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(LMDB REQUIRED lmdb)

# Include LabDb headers
include_directories(${CMAKE_SOURCE_DIR}/../../include)

# Link LabDb library (assuming it's been built)
link_directories(${CMAKE_SOURCE_DIR}/../../build)

# Create benchmark executable
add_executable(labdb_benchmarks
    benchmark_suite.cpp
)

# Link libraries
target_link_libraries(labdb_benchmarks
    PRIVATE
    ${LMDB_LIBRARIES}
    # Add LabDb library when available
    # labdb
)

# Compiler flags for optimization
target_compile_options(labdb_benchmarks PRIVATE
    -O3
    -march=native
    -DNDEBUG
)

# Add compile definitions
target_compile_definitions(labdb_benchmarks PRIVATE
    BENCHMARK_MODE=1
    TID_ARCHITECTURE=1
)

# Installation
install(TARGETS labdb_benchmarks
    DESTINATION bin
)

# Custom targets for different benchmark runs
add_custom_target(benchmark_quick
    COMMAND labdb_benchmarks --quick
    DEPENDS labdb_benchmarks
    COMMENT "Running quick benchmarks"
)

add_custom_target(benchmark_full
    COMMAND labdb_benchmarks --full
    DEPENDS labdb_benchmarks  
    COMMENT "Running comprehensive benchmarks"
)

add_custom_target(benchmark_euclid
    COMMAND labdb_benchmarks --euclid-scale
    DEPENDS labdb_benchmarks
    COMMENT "Running Euclid-scale benchmarks"
)
