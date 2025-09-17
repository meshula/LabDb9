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

# debug semantic test
add_executable(debug_semantic_test
    cpp/debug_semantic_test.cpp
)
target_link_libraries(debug_semantic_test labdb)
add_test(NAME DebugSemanticTest COMMAND debug_semantic_test)

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

# Parameter Fix Validation Test (LBYL vs EAFP pattern)
add_executable(test_parameter_fix
    ../test_parameter_fix.cpp
)
target_link_libraries(test_parameter_fix labdb)
add_test(NAME ParameterFixTest COMMAND test_parameter_fix)

# FIO-Write TDD test (Test-driven development for fio-write functionality)
add_executable(test_fio_write
    cpp/test_fio_write.cpp
)
target_link_libraries(test_fio_write labdb)
target_include_directories(test_fio_write PRIVATE ../include)
add_test(NAME FioWriteTest COMMAND test_fio_write)

# Line Surgery TDD test (Test-driven development for line surgery algorithms)
add_executable(test_line_surgery
    cpp/test_line_surgery.cpp
)
target_link_libraries(test_line_surgery labdb)
target_include_directories(test_line_surgery PRIVATE ../include)
add_test(NAME LineSurgeryTest COMMAND test_line_surgery)

# Phase C2: Unicode + Line Surgery Integration Test (Consciousness-first preview with unicode transformations)
add_executable(test_unicode_line_surgery_integration
    cpp/test_unicode_line_surgery_integration.cpp
)
target_link_libraries(test_unicode_line_surgery_integration labdb)
target_include_directories(test_unicode_line_surgery_integration PRIVATE ../include)
add_test(NAME UnicodeLineSurgeryIntegrationTest COMMAND test_unicode_line_surgery_integration)

# FIO-Confirm ping-pong test (Simple test for preview-confirm loop)
add_executable(test_fio_confirm_simple
    cpp/test_fio_confirm_simple.cpp
)
target_link_libraries(test_fio_confirm_simple labdb)
target_include_directories(test_fio_confirm_simple PRIVATE ../include)
add_test(NAME FioConfirmSimpleTest COMMAND test_fio_confirm_simple)

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
add_executable(test_rope
    cpp/test_rope.cpp
)
target_link_libraries(test_rope labdb)
add_test(NAME RopeTest COMMAND test_rope)

# Memex Verbs Test (Bush's associative memory realization)
add_executable(test_memex
    cpp/test_memex.cpp
)
target_link_libraries(test_memex labdb)
add_test(NAME MemexTest COMMAND test_memex)

# Euclid Ontology TDD Validation Test (Memex realization through Euclid's Elements)
add_executable(test_euclid_ontology_validation
    cpp/test_euclid_ontology_validation.cpp
)
target_link_libraries(test_euclid_ontology_validation labdb)
target_include_directories(test_euclid_ontology_validation PRIVATE ../include)
add_test(NAME EuclidOntologyValidationTest COMMAND test_euclid_ontology_validation)

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

# McGuffin Test Suite

# DirectoryTraversal test
add_executable(test_directory_traversal
    cpp/test_directory_traversal.cpp
)
target_link_libraries(test_directory_traversal labdb)
target_include_directories(test_directory_traversal PRIVATE ../include)
add_test(NAME DirectoryTraversalTest COMMAND test_directory_traversal)

# Extended Search test
add_executable(test_extended_search
    cpp/test_extended_search.cpp
)
target_link_libraries(test_extended_search labdb)
target_include_directories(test_extended_search PRIVATE ../include)
add_test(NAME ExtendedSearchTest COMMAND test_extended_search)

# fio-search-ext Integration test
add_executable(test_fio_search_ext
    cpp/test_fio_search_ext.cpp
)
target_link_libraries(test_fio_search_ext labdb)
target_include_directories(test_fio_search_ext PRIVATE ../include)
add_test(NAME FioSearchExtTest COMMAND test_fio_search_ext)

# File-based executeCommands test
add_executable(test_file_commands
    cpp/test_file_commands.cpp
)
target_link_libraries(test_file_commands labdb)
target_include_directories(test_file_commands PRIVATE ../include)
add_test(NAME FileCommandsTest COMMAND test_file_commands)
