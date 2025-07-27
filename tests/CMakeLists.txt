cmake_minimum_required(VERSION 3.10)

# Copy test environment to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cpp/testenv 
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Test executables

# LMDB wrapper test
add_executable(test_lmdb_wrapper
    cpp/test_lmdb_wrapper.cpp
)
target_link_libraries(test_lmdb_wrapper labdb)

add_test(NAME LmdbWrapperTest COMMAND test_lmdb_wrapper)

# NonostoreKeys test
add_executable(test_nonostore_keys
    cpp/test_nonostore_keys.cpp
    ../src/NonostoreKeys.cpp
)
target_link_libraries(test_nonostore_keys labdb)
target_include_directories(test_nonostore_keys PRIVATE ../include)
add_test(NAME NonostoreKeysTest COMMAND test_nonostore_keys)

# NonoStore core test
add_executable(test_nonostore_core
    cpp/test_nonostore_core.cpp
)
target_link_libraries(test_nonostore_core labdb)
add_test(NAME NonostoreCoreTest COMMAND test_nonostore_core)

# Triadic Query Interface test
add_executable(test_triadic_interface
    cpp/test_triadic_interface.cpp
)
target_link_libraries(test_triadic_interface labdb)
add_test(NAME TriadicInterfaceTest COMMAND test_triadic_interface)

# TermDictionary test
add_executable(test_term_dictionary
    test_term_dictionary.cpp
)
target_link_libraries(test_term_dictionary labdb)
add_test(NAME TermDictionaryTest COMMAND test_term_dictionary)

# TripleStore test
add_executable(test_triple_store
    test_triple_store.cpp
)
target_link_libraries(test_triple_store labdb)
add_test(NAME TripleStoreTest COMMAND test_triple_store)

# S-Expression Unicode Robustness test
add_executable(test_sexpr_unicode_robustness
    cpp/test_sexpr_unicode_robustness.cpp
)
target_link_libraries(test_sexpr_unicode_robustness labdb)
target_include_directories(test_sexpr_unicode_robustness PRIVATE ../include)
add_test(NAME SexprUnicodeRobustnessTest COMMAND test_sexpr_unicode_robustness)

# Db9Dispatcher test
add_executable(test_db9_dispatcher
    cpp/test_db9_dispatcher.cpp
)
target_link_libraries(test_db9_dispatcher labdb)
add_test(NAME Db9DispatcherTest COMMAND test_db9_dispatcher)

# Find-Triple Wildcard test (Critical missing functionality)
add_executable(test_find_triple_wildcards
    cpp/test_find_triple_wildcards.cpp
)
target_link_libraries(test_find_triple_wildcards labdb)
add_test(NAME FindTripleWildcardsTest COMMAND test_find_triple_wildcards)

# Enhanced API test (Rich object returns and dual-layer design)
add_executable(test_enhanced_api
    cpp/test_enhanced_api.cpp
)
target_link_libraries(test_enhanced_api labdb)
add_test(NAME EnhancedApiTest COMMAND test_enhanced_api)

# FIO-Write TDD test (Test-driven development for fio-write functionality)
add_executable(test_fio_write
    cpp/test_fio_write.cpp
)
target_link_libraries(test_fio_write labdb)
target_include_directories(test_fio_write PRIVATE ../include)
add_test(NAME FioWriteTest COMMAND test_fio_write)

# EID Chain Debug test (Debug EID resolution chains)
add_executable(eid_debug_test
    cpp/eid_debug_test.cpp
)
target_link_libraries(eid_debug_test labdb)
# Note: This is a debug test, not added to CTest suite

# Performance Benchmark (not a test, but a benchmark tool)
add_executable(benchmark_performance
    cpp/benchmark_performance.cpp
)
target_link_libraries(benchmark_performance labdb)

# Rope functionality test
add_executable(test_rope_minimal
    cpp/test_rope_minimal.cpp
)
target_link_libraries(test_rope_minimal labdb)
add_test(NAME RopeMinimalTest COMMAND test_rope_minimal)

# Python tests (if Python bindings are available)
if(BUILD_PYTHON_BINDINGS)
    # Add Python streaming interface test
    add_test(
        NAME StreamingInterfaceTest
        COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/python/test_streaming_interface.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    # Add comprehensive Python test  
    add_test(
        NAME ComprehensivePythonTest
        COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/python/test_comprehensive_python.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    # Add Python integration test
    add_test(
        NAME PythonIntegrationTest
        COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/python/test_python_integration.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    message(STATUS "Python test targets configured")
else()
    message(STATUS "Python tests skipped - BUILD_PYTHON_BINDINGS=OFF")
endif()


# TextEscaping test
add_executable(test_text_escaping
    cpp/test_text_escaping.cpp
)
target_link_libraries(test_text_escaping labdb)
target_include_directories(test_text_escaping PRIVATE ../include)
add_test(NAME TextEscapingTest COMMAND test_text_escaping)
message(STATUS "Test targets configured")