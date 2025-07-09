# LabDb Python C Extension
# Minimal C-based Python bindings for db9 S-expression interface

cmake_minimum_required(VERSION 3.16)

# Only build Python extension if LABDB_MCP option is enabled
if(NOT LABDB_MCP)
    return()
endif()

message(STATUS "Building LabDb Python C Extension...")

# Find Python components
if(LABDB_PYEXE)
    set(Python3_EXECUTABLE ${LABDB_PYEXE})
    message(STATUS "Using specified Python executable: ${LABDB_PYEXE}")
endif()

if(LABDB_PYDIR)
    set(Python3_ROOT_DIR ${LABDB_PYDIR})
    message(STATUS "Using specified Python directory: ${LABDB_PYDIR}")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

message(STATUS "Found Python: ${Python3_EXECUTABLE}")
message(STATUS "Python version: ${Python3_VERSION}")
message(STATUS "Python include dirs: ${Python3_INCLUDE_DIRS}")
message(STATUS "Python libraries: ${Python3_LIBRARIES}")

# Create the Python C extension
add_library(labdb_c MODULE
    labdb_c_bindings.cpp
)

set(CMAKE_BUILD_TYPE Debug)
target_compile_options(labdb_c PRIVATE -g -O0 -fno-strict-aliasing -fno-lto)
set_target_properties(labdb_c PROPERTIES
    LINK_FLAGS "-undefined dynamic_lookup"
)

# Set target properties for Python module
set_target_properties(labdb_c PROPERTIES
    PREFIX ""                               # No 'lib' prefix
    SUFFIX ".so"                           # Simple .so suffix
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "labdb"                   # Import as 'import labdb_c'
)

target_link_options(labdb_c PRIVATE "-undefined dynamic_lookup")

# Include directories
target_include_directories(labdb_c PRIVATE
    ${Python3_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Link against Python and LabDb
target_link_libraries(labdb_c PRIVATE
    #${Python3_LIBRARIES}
    labdb
)



# Ensure LabDb is built first
add_dependencies(labdb_c labdb)

# Define Python installation directory
if(LABDB_PYDIR)
    set(PYTHON_INSTALL_DIR "${LABDB_PYDIR}/labdb")
else()
    # Use Python's site-packages directory
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -c "import site; print(site.getsitepackages()[0])"
        OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(PYTHON_INSTALL_DIR "${PYTHON_SITE_PACKAGES}/labdb")
endif()

message(STATUS "Python extension will be installed to: ${PYTHON_INSTALL_DIR}")

# ==============================================================================
# BUILD-TIME LABDB_PYDIR POPULATION
# ==============================================================================
# Copy Python package structure to LABDB_PYDIR during build for robust iteration
# Uses README.txt as dependency trigger to avoid unnecessary copies

if(LABDB_PYDIR)
    # Create labdb package directory in LABDB_PYDIR
    file(MAKE_DIRECTORY "${PYTHON_INSTALL_DIR}")
    
    # Define source and target paths
    set(SOURCE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    set(SOURCE_INIT "${CMAKE_CURRENT_SOURCE_DIR}/labdb/__init__.py")
    set(TARGET_README "${PYTHON_INSTALL_DIR}/README.txt")
    set(TARGET_INIT "${PYTHON_INSTALL_DIR}/__init__.py")
    
    # MCP server script paths (generated during build)
    set(SOURCE_MCP_PY "${CMAKE_CURRENT_BINARY_DIR}/labdb-mcp-server.py")
    set(SOURCE_MCP_SH "${CMAKE_CURRENT_BINARY_DIR}/labdb-mcp-server.sh")
    set(TARGET_MCP_PY "${LABDB_PYDIR}/labdb-mcp-server.py")
    set(TARGET_MCP_SH "${LABDB_PYDIR}/labdb-mcp-server.sh")
    
    # Custom command to copy static files when README.md changes
    add_custom_command(
        OUTPUT "${TARGET_README}"
        COMMAND ${CMAKE_COMMAND} -E copy "${SOURCE_README}" "${TARGET_README}"
        COMMAND ${CMAKE_COMMAND} -E copy "${SOURCE_INIT}" "${TARGET_INIT}"
        DEPENDS "${SOURCE_README}" "${SOURCE_INIT}"
        COMMENT "Copying Python package files to LABDB_PYDIR"
        VERBATIM
    )
    
    # Custom command to copy the built .so file and MCP scripts after labdb_c is built
    add_custom_command(
        TARGET labdb_c POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:labdb_c>" "${PYTHON_INSTALL_DIR}/labdb.so"
        COMMAND ${CMAKE_COMMAND} -E copy "${SOURCE_MCP_PY}" "${TARGET_MCP_PY}"
        COMMAND ${CMAKE_COMMAND} -E copy "${SOURCE_MCP_SH}" "${TARGET_MCP_SH}"
        COMMENT "Copying labdb.so and MCP server scripts to LABDB_PYDIR: ${LABDB_PYDIR}"
        VERBATIM
    )
    
    # Custom target to ensure static files are copied
    add_custom_target(populate_static_files
        DEPENDS "${TARGET_README}"
        COMMENT "Ensuring Python package files are in LABDB_PYDIR"
    )
    
    # Make sure static files are copied before building the extension
    add_dependencies(labdb_c populate_static_files)
    
    message(STATUS "Build will populate LABDB_PYDIR automatically (triggered by README.md)")
endif()

# ==============================================================================
# INSTALLATION (for formal install step)
# ==============================================================================

# Install the Python module
install(TARGETS labdb_c
    LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}
    COMPONENT python
)

# Install the Python package files
install(FILES 
    labdb/__init__.py
    DESTINATION ${PYTHON_INSTALL_DIR}
    COMPONENT python
)

# Copy the clean Python MCP server script
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/labdb_mcp_server.py
    ${CMAKE_CURRENT_BINARY_DIR}/labdb-mcp-server.py
    COPYONLY
)

# Install the MCP server script
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/labdb-mcp-server.py
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
               GROUP_READ GROUP_EXECUTE 
               WORLD_READ WORLD_EXECUTE
    COMPONENT python
)

# Create a shell script for easy launching
set(LAUNCH_SCRIPT_CONTENT "#!/bin/bash
# LabDb MCP Server Launch Script

# Set PYTHONPATH if LABDB_PYDIR was specified
if [ -n \"${LABDB_PYDIR}\" ]; then
    export PYTHONPATH=\"${LABDB_PYDIR}:\$PYTHONPATH\"
fi

# Launch the MCP server
exec \"${Python3_EXECUTABLE}\" \"${LABDB_PYDIR}/labdb-mcp-server.py\" \"\$@\"
")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/labdb-mcp-server.sh "${LAUNCH_SCRIPT_CONTENT}")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/labdb-mcp-server.sh
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
               GROUP_READ GROUP_EXECUTE 
               WORLD_READ WORLD_EXECUTE
    COMPONENT python
)

message(STATUS "LabDb Python C extension configured successfully")
message(STATUS "After build and install, run: PYTHONPATH=${PYTHON_INSTALL_DIR} ${Python3_EXECUTABLE} labdb-mcp-server.py")
