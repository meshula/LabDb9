cmake_minimum_required(VERSION 3.16)
project(db9_cli VERSION 1.0.0 LANGUAGES C CXX)

if(APPLE)
    enable_language(OBJCXX)
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Platform detection
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(LINUX TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(MACOS TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(WINDOWS TRUE)
endif()

# Set up paths
set(EXTERN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern)
set(LABDB_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# Try to find the built LMDB from the parent project
find_path(LMDB_INCLUDE_DIR 
    NAMES lmdb.h
    PATHS 
        ${LABDB_ROOT}/build/_deps/lmdb-src/libraries/liblmdb
        ${LABDB_ROOT}/_deps/lmdb-src/libraries/liblmdb
)

find_library(LMDB_LIBRARY
    NAMES lmdb_lib lmdb
    PATHS
        ${LABDB_ROOT}/build
        ${LABDB_ROOT}/build/_deps/lmdb-build
)

if(NOT LMDB_INCLUDE_DIR OR NOT LMDB_LIBRARY)
    message(STATUS "LMDB not found in expected locations, trying package manager...")
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LMDB QUIET lmdb)
    endif()
    
    if(NOT LMDB_FOUND)
        message(FATAL_ERROR "LMDB not found. Please install LMDB or build the parent LabDb project first.")
    endif()
endif()

# Find LabDb library
if(EXISTS ${LABDB_ROOT}/build/liblabdb.a)
    set(LABDB_LIBRARY ${LABDB_ROOT}/build/liblabdb.a)
    message(STATUS "Found LabDb library: ${LABDB_LIBRARY}")
else()
    message(FATAL_ERROR "LabDb library not found. Build LabDb first with 'cmake --build build' from project root.")
endif()

# Include directories
include_directories(
    ${LABDB_ROOT}/include
    ${LMDB_INCLUDE_DIR}
    src/
)

# Platform-specific language override for macOS
if(APPLE)
    set_source_files_properties(src/main.cpp PROPERTIES LANGUAGE OBJCXX)
endif()

# Minimal source files + required ImGui sources for sokol_imgui
set(SOURCES
    src/main.cpp
)

# Create executable
add_executable(db9_cli ${SOURCES} ${IMGUI_SOURCES})

# Platform-specific configurations
if(MACOS)
    # macOS build
    
    find_library(COCOA_LIBRARY Cocoa)
    
    target_link_libraries(db9_cli
        ${LABDB_LIBRARY}
        ${LMDB_LIBRARY}
        ${COCOA_LIBRARY}
    )
    
elseif(LINUX)
    # Linux build
        
    target_link_libraries(db9_cli ${LABDB_LIBRARY} ${LMDB_LIBRARY} dl m pthread)
    
elseif(WINDOWS)
    
    target_link_libraries(db9_cli ${LABDB_LIBRARY} ${LMDB_LIBRARY})
endif()

# Common compile definitions
target_compile_definitions(db9_cli PRIVATE
)

# Compiler flags
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(db9_cli PRIVATE
        -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers
    )
endif()

# Installation
install(TARGETS db9_cli
    RUNTIME DESTINATION bin
)

message(STATUS "")
message(STATUS "Triadic Consciousness Explorer Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  LabDb library: ${LABDB_LIBRARY}")
message(STATUS "  LMDB include: ${LMDB_INCLUDE_DIR}")
message(STATUS "  LMDB library: ${LMDB_LIBRARY}")
message(STATUS "")
