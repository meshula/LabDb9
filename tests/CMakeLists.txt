cmake_minimum_required(VERSION 3.10)

# Test executables

# LMDB wrapper test
add_executable(test_lmdb_wrapper
    test_lmdb_wrapper.cpp
)
target_link_libraries(test_lmdb_wrapper labdb)

add_test(NAME LmdbWrapperTest COMMAND test_lmdb_wrapper)

# NonostoreKeys test
add_executable(test_nonostore_keys
    test_nonostore_keys.cpp
    ../src/NonostoreKeys.cpp
)
target_include_directories(test_nonostore_keys PRIVATE ../include)
add_test(NAME NonostoreKeysTest COMMAND test_nonostore_keys)

# NonoStore core test
add_executable(test_nonostore_core
    test_nonostore_core.cpp
)
target_link_libraries(test_nonostore_core labdb)
add_test(NAME NonostoreCoreTest COMMAND test_nonostore_core)

# Triadic Query Interface test
add_executable(test_triadic_interface
    test_triadic_interface.cpp
)
target_link_libraries(test_triadic_interface labdb)
add_test(NAME TriadicInterfaceTest COMMAND test_triadic_interface)

# Performance Benchmark (not a test, but a benchmark tool)
add_executable(benchmark_performance
    benchmark_performance.cpp
)
target_link_libraries(benchmark_performance labdb)

message(STATUS "Test targets configured")
