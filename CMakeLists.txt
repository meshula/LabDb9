cmake_minimum_required(VERSION 3.16)
project(LabDb VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use FetchContent to get LMDB
include(FetchContent)

# LMDB doesn't have an official CMake build, so we'll build it ourselves
FetchContent_Declare(
    lmdb
    GIT_REPOSITORY https://github.com/LMDB/lmdb.git
    GIT_TAG LMDB_0.9.31
    SOURCE_SUBDIR libraries/liblmdb
)

# Check if we can fetch LMDB
FetchContent_MakeAvailable(lmdb)

# Create LMDB target manually since it doesn't have CMake support
set(LMDB_SOURCE_DIR ${lmdb_SOURCE_DIR}/libraries/liblmdb)

add_library(lmdb_lib STATIC
    ${LMDB_SOURCE_DIR}/mdb.c
    ${LMDB_SOURCE_DIR}/midl.c
)

target_include_directories(lmdb_lib PUBLIC
    $<BUILD_INTERFACE:${LMDB_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/lmdb>
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(lmdb_lib PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_link_libraries(lmdb_lib PRIVATE pthread)
endif()

# Set LMDB variables for consistency
set(LMDB_FOUND TRUE)
set(LMDB_INCLUDE_DIRS ${LMDB_SOURCE_DIR})
set(LMDB_LIBRARIES lmdb_lib)

message(STATUS "LMDB configured via FetchContent")

# Option for Python bindings
option(BUILD_PYTHON_BINDINGS "Build Python bindings" ON)

# Core library
add_library(labdb
    src/LmdbStore.cpp
    src/NonostoreKeys.cpp
    src/NonoStore.cpp
    src/TriadicQuery.cpp
    src/InceptionBridge.cpp
    src/PerformanceBenchmark.cpp
)

target_include_directories(labdb
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${LMDB_INCLUDE_DIRS}
)

target_link_libraries(labdb 
    PUBLIC ${LMDB_LIBRARIES}  # Make it PUBLIC so it gets exported
)

# Tests
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Python bindings
if(BUILD_PYTHON_BINDINGS)
    find_package(pybind11 QUIET)
    if(pybind11_FOUND)
        pybind11_add_module(pylabdb python/bindings.cpp)
        target_link_libraries(pylabdb PRIVATE labdb)
        
        # Set properties for proper Python module
        set_target_properties(pylabdb PROPERTIES
            CXX_VISIBILITY_PRESET "hidden"
            VISIBILITY_INLINES_HIDDEN YES
        )
        
        # Install Python module to python/labdb/ directory
        install(TARGETS pylabdb DESTINATION python/labdb)
        
        message(STATUS "Python bindings enabled")
    else()
        message(WARNING "pybind11 not found - skipping Python bindings")
    endif()
else()
    message(STATUS "Python bindings disabled")
endif()

# Examples
add_subdirectory(examples)

# Installation
install(TARGETS labdb lmdb_lib
    EXPORT LabDbTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/LabDb DESTINATION include)

# Install LMDB headers
install(FILES 
    ${LMDB_SOURCE_DIR}/lmdb.h
    ${LMDB_SOURCE_DIR}/midl.h
    DESTINATION include/lmdb
)

# Package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    LabDbConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(EXPORT LabDbTargets
    FILE LabDbTargets.cmake
    DESTINATION lib/cmake/LabDb
)

install(FILES
    cmake/LabDbConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/LabDbConfigVersion.cmake
    DESTINATION lib/cmake/LabDb
)
