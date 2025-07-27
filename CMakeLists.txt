cmake_minimum_required(VERSION 3.16)
project(LabDb VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use FetchContent to get LMDB and UTF8PROC
include(FetchContent)

# UTF8PROC for Unicode normalization
FetchContent_Declare(
    utf8proc
    GIT_REPOSITORY https://github.com/JuliaStrings/utf8proc.git
    GIT_TAG v2.9.0
)
FetchContent_GetProperties(utf8proc)
if(NOT utf8proc_POPULATED)
    FetchContent_Populate(utf8proc)
endif()

# Declare your own target
add_library(utf8proc STATIC
    ${utf8proc_SOURCE_DIR}/utf8proc.c
)

# Header-only, so just include the directory
target_include_directories(utf8proc
    PUBLIC
        $<BUILD_INTERFACE:${utf8proc_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/utf8proc>
)

# Set properties for install
set_target_properties(utf8proc PROPERTIES
    EXPORT_NAME utf8proc
    PUBLIC_HEADER ${utf8proc_SOURCE_DIR}/utf8proc.h
)

# Install target and headers
install(TARGETS utf8proc
    EXPORT LabDbTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/utf8proc
)

# LMDB doesn't have an official CMake build, so we'll build it ourselves
FetchContent_Declare(
    lmdb
    GIT_REPOSITORY https://github.com/LMDB/lmdb.git
    GIT_TAG LMDB_0.9.31
    SOURCE_SUBDIR libraries/liblmdb
)

# Check if LMDB was populated correctly
FetchContent_GetProperties(lmdb)
if(NOT lmdb_POPULATED)
    FetchContent_Populate(lmdb)
endif()

# Create LMDB target manually since it doesn't have CMake support
set(LMDB_SOURCE_DIR ${lmdb_SOURCE_DIR}/libraries/liblmdb)
message(STATUS "LMDB Binary Directory: ${lmdb_BINARY_DIR}")

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

message(STATUS "UTF8PROC configured via FetchContent")
message(STATUS "LMDB configured via FetchContent")

# ==============================================================================
# BUILD OPTIONS
# ==============================================================================

# Primary option for MCP server with C-based Python bindings
option(LABDB_MCP "Build LabDb MCP server with clean C-based Python bindings" OFF)

# Development options
option(BUILD_TESTS "Build test suite" ON)

# Python configuration (used when LABDB_MCP=ON)
# LABDB_PYDIR: Python installation directory (e.g., /opt/conda/envs/myenv)
set(LABDB_PYDIR "" CACHE STRING "Python installation directory for LABDB_MCP")

if(LABDB_MCP)
    # Validate LABDB_PYDIR is defined and writable
    if(NOT LABDB_PYDIR)
        message(FATAL_ERROR "LABDB_MCP=ON requires LABDB_PYDIR to be defined. "
                           "Example: -DLABDB_PYDIR=/opt/conda/envs/myenv")
    endif()
    
    if(NOT IS_DIRECTORY "${LABDB_PYDIR}")
        message(FATAL_ERROR "LABDB_PYDIR directory does not exist: ${LABDB_PYDIR}")
    endif()
    
    # Test write access by attempting to create/remove a temporary file
    set(TEST_WRITE_FILE "${LABDB_PYDIR}/.cmake_write_test_${CMAKE_CURRENT_TIMESTAMP}")
    file(WRITE "${TEST_WRITE_FILE}" "test")
    if(NOT EXISTS "${TEST_WRITE_FILE}")
        message(FATAL_ERROR "LABDB_PYDIR is not writable: ${LABDB_PYDIR}")
    endif()
    file(REMOVE "${TEST_WRITE_FILE}")
    
    message(STATUS "LABDB_MCP: Using Python directory: ${LABDB_PYDIR} [validated]")
    
    # Use whatever python is in PATH to get its executable path
    execute_process(
        COMMAND python -c "import sys; print(sys.executable)"
        OUTPUT_VARIABLE LABDB_PYEXE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE PYTHON_RESULT
    )

    if(NOT PYTHON_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to get Python executable path")
    endif()

    message(STATUS "LABDB_MCP: Using Python executable: ${LABDB_PYEXE}")

    # Validate LABDB_PYEXE is defined and executable exists
    if(NOT LABDB_PYEXE)
        message(FATAL_ERROR "LABDB_MCP=ON requires a Python interpreter to be available.")
    endif()
    
    if(NOT EXISTS "${LABDB_PYEXE}")
        message(FATAL_ERROR "LABDB_PYEXE executable does not exist: ${LABDB_PYEXE}")
    endif()
    
    if(NOT EXISTS "${LABDB_PYEXE}")
        message(FATAL_ERROR "LABDB_PYEXE is not executable: ${LABDB_PYEXE}")
    endif()
    
    # Verify it's actually a Python executable by testing --version
    execute_process(
        COMMAND "${LABDB_PYEXE}" --version
        RESULT_VARIABLE PYTHON_VERSION_RESULT
        OUTPUT_VARIABLE PYTHON_VERSION_OUTPUT
        ERROR_VARIABLE PYTHON_VERSION_ERROR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE
    )
    
    if(NOT PYTHON_VERSION_RESULT EQUAL 0)
        message(FATAL_ERROR "LABDB_PYEXE failed to execute '--version': ${LABDB_PYEXE}\n"
                           "Output: ${PYTHON_VERSION_OUTPUT}\n"
                           "Error: ${PYTHON_VERSION_ERROR}")
    endif()
    
    if(NOT PYTHON_VERSION_OUTPUT MATCHES "Python [0-9]+\\.[0-9]+")
        message(FATAL_ERROR "LABDB_PYEXE does not appear to be a Python executable: ${LABDB_PYEXE}\n"
                           "Version output: ${PYTHON_VERSION_OUTPUT}")
    endif()
    
    message(STATUS "LABDB_MCP: Using Python executable: ${LABDB_PYEXE} [${PYTHON_VERSION_OUTPUT}]")
endif()

# ==============================================================================
# CORE LIBRARY
# ==============================================================================

add_library(labdb
    include/LabDb/AutoReflexiveMetrics.h
    include/LabDb/DatabaseManager.h
    include/LabDb/DatabaseVerbs.h
    include/LabDb/Db9Dispatcher.h
    include/LabDb/Db9Response.h
    include/LabDb/DirectoryTraversal.h
    include/LabDb/EntityId.h
    include/LabDb/InceptionBridge.h
    include/LabDb/LabText.hpp
    include/LabDb/LmdbStore.h
    include/LabDb/NonoStore.h
    include/LabDb/NonostoreKeys.h
    include/LabDb/NormalizeText.h
    include/LabDb/PerformanceBenchmark.h
    include/LabDb/TermDictionary.h
    include/LabDb/TIDSequenceGenerator.h
    include/LabDb/TriadicQuery.h
    include/LabDb/TripleStore.h
    include/LabDb/EnhancedDatabaseVerbs.h
    include/Labdb/Verbs.h
    src/DatabaseManager.cpp
    src/DatabaseVerbs.cpp
    src/DirectoryTraversal.cpp
    src/EnhancedDatabaseVerbs.cpp
    src/Db9Dispatcher.cpp
    src/EntityId.cpp
    src/InceptionBridge.cpp
    src/LmdbStore.cpp
    src/NonoStore.cpp
    src/NonostoreKeys.cpp
    src/NormalizeText.cpp
    src/PerformanceBenchmark.cpp
    src/TermDictionary.cpp
    src/TextEscaping.cpp
    src/TIDSequenceGenerator.cpp
    src/TriadicQuery.cpp
    src/TripleStore.cpp
    src/LabTextODR.cpp
    src/Verbs.cpp
    src/Verbs/VocabularyStatsVerb.h
    src/Verbs/VocabularyStatsVerb.cpp
    src/Verbs/FioVerbs.cpp
    src/Verbs/FioVerbs.h
    src/Verbs/Fio/GetVerbDescription.cpp
    src/Verbs/Fio/GetVerbDescription.h
    src/Verbs/Fio/SetCwd.cpp
    src/Verbs/Fio/SetCwd.h
)

target_include_directories(labdb
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${LMDB_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:include/lmdb>
)

target_link_libraries(labdb 
    PUBLIC 
        ${LMDB_LIBRARIES}  # Make it PUBLIC so it gets exported
        utf8proc           # Unicode normalization library
)

# ==============================================================================
# SUBDIRECTORIES
# ==============================================================================

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples and tools
add_subdirectory(examples)
add_subdirectory(tools)

# ==============================================================================
# PYTHON BINDINGS
# ==============================================================================

# Clean C-based Python extension for MCP server
if(LABDB_MCP)
    message(STATUS "Configuring LabDb Python C Extension for MCP server...")
    add_subdirectory(python)
    message(STATUS "LabDb Python C Extension configured")
endif()

# ==============================================================================
# INSTALLATION
# ==============================================================================

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

# Configure the config file from template
configure_package_config_file(
    cmake/LabDbConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/LabDbConfig.cmake"
    INSTALL_DESTINATION lib/cmake/LabDb
    PATH_VARS CMAKE_INSTALL_PREFIX
)

# Generate version file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/LabDbConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install the targets export
install(EXPORT LabDbTargets
    FILE LabDbTargets.cmake
    NAMESPACE LabDb::
    DESTINATION lib/cmake/LabDb
)

# Install the config files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/LabDbConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/LabDbConfigVersion.cmake"
    DESTINATION lib/cmake/LabDb
)