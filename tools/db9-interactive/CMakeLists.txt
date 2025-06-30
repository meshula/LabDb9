cmake_minimum_required(VERSION 3.16)
project(db9_interactive_explorer VERSION 1.0.0 LANGUAGES C CXX)

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
        /usr/local/include
        /opt/homebrew/include
        /usr/include
)

find_library(LMDB_LIBRARY
    NAMES lmdb_lib lmdb
    PATHS
        ${LABDB_ROOT}/build
        ${LABDB_ROOT}/build/_deps/lmdb-build
        /usr/local/lib
        /opt/homebrew/lib
        /usr/lib
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
    ${EXTERN_DIR}/sokol
    ${EXTERN_DIR}/cimgui
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

# ImGui sources required by sokol_imgui
set(IMGUI_SOURCES
    ${EXTERN_DIR}/cimgui/imgui.cpp
    ${EXTERN_DIR}/cimgui/imgui_demo.cpp
    ${EXTERN_DIR}/cimgui/imgui_draw.cpp
    ${EXTERN_DIR}/cimgui/imgui_tables.cpp
    ${EXTERN_DIR}/cimgui/imgui_widgets.cpp
)

# Create executable
add_executable(triadic_explorer ${SOURCES} ${IMGUI_SOURCES})

# Platform-specific configurations
if(MACOS)
    # macOS build
    target_compile_definitions(triadic_explorer PRIVATE 
        SOKOL_METAL
        SOKOL_NO_DEPRECATED
    )
    
    find_library(COCOA_LIBRARY Cocoa)
    find_library(METAL_LIBRARY Metal)
    find_library(METALKIT_LIBRARY MetalKit)
    find_library(QUARTZ_CORE_LIBRARY QuartzCore)
    
    target_link_libraries(triadic_explorer
        ${LABDB_LIBRARY}
        ${LMDB_LIBRARY}
        ${COCOA_LIBRARY}
        ${METAL_LIBRARY}
        ${METALKIT_LIBRARY}
        ${QUARTZ_CORE_LIBRARY}
    )
    
elseif(LINUX)
    # Linux build
    target_compile_definitions(triadic_explorer PRIVATE 
        SOKOL_GLCORE33
        SOKOL_NO_DEPRECATED
    )
    
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(X11 REQUIRED x11 xi xcursor)
    
    target_include_directories(triadic_explorer PRIVATE ${X11_INCLUDE_DIRS})
    target_link_libraries(triadic_explorer ${LABDB_LIBRARY} ${LMDB_LIBRARY} ${X11_LIBRARIES} GL dl m pthread)
    
elseif(WINDOWS)
    # Windows build
    target_compile_definitions(triadic_explorer PRIVATE 
        SOKOL_D3D11
        SOKOL_NO_DEPRECATED
    )
    
    target_link_libraries(triadic_explorer ${LABDB_LIBRARY} ${LMDB_LIBRARY})
endif()

# Common compile definitions
target_compile_definitions(triadic_explorer PRIVATE
    CIMGUI_DEFINE_ENUMS_AND_STRUCTS=1
    IMGUI_DISABLE_OBSOLETE_FUNCTIONS=1
)

# Compiler flags
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(triadic_explorer PRIVATE
        -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers
    )
endif()

# Installation
install(TARGETS triadic_explorer
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
