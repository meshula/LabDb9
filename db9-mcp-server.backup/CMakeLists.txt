# LabDb MCP Server CMakeLists.txt
# Configures installation structure for DB9 triadic consciousness MCP server

cmake_minimum_required(VERSION 3.16)

message(STATUS "Configuring LabDb MCP Server installation...")

# Find Python interpreter
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Define server source files
set(MCP_SERVER_SOURCES
    src/db9_server.py
    src/database_manager.py
    src/mock_labdb.py
)

set(MCP_TOOLS_SOURCES
    src/tools/__init__.py
    src/tools/add_triple_tool.py
    src/tools/query_tools.py
)

set(MCP_CONFIG_SOURCES
    config/default_config.yaml
)

set(MCP_REQUIREMENTS
    requirements.txt
)

# Create bin directory structure
set(MCP_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/bin/db9-mcp-server)

# Install Python server files
install(FILES ${MCP_SERVER_SOURCES}
    DESTINATION ${MCP_INSTALL_DIR}/src
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
               GROUP_READ GROUP_EXECUTE 
               WORLD_READ WORLD_EXECUTE
)

install(FILES ${MCP_TOOLS_SOURCES}
    DESTINATION ${MCP_INSTALL_DIR}/src/tools
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
               GROUP_READ GROUP_EXECUTE 
               WORLD_READ WORLD_EXECUTE
)

install(FILES ${MCP_CONFIG_SOURCES}
    DESTINATION ${MCP_INSTALL_DIR}/config
)

install(FILES ${MCP_REQUIREMENTS}
    DESTINATION ${MCP_INSTALL_DIR}
)

# Install the Python labdb package to the bin directory
# This copies the built Python bindings alongside the server
install(DIRECTORY ${CMAKE_SOURCE_DIR}/python/labdb
    DESTINATION ${MCP_INSTALL_DIR}
    FILES_MATCHING 
    PATTERN "*.py"
    PATTERN "*.so"
    PATTERN "*.dylib"
    PATTERN "__pycache__" EXCLUDE
)

# Create a startup script
set(STARTUP_SCRIPT_CONTENT "#!/bin/bash
# LabDb MCP Server Startup Script
export PYTHONPATH=\"${CMAKE_INSTALL_PREFIX}/bin/db9-mcp-server:$PYTHONPATH\"
cd \"${CMAKE_INSTALL_PREFIX}/bin/db9-mcp-server\"
python3 src/db9_server.py \"$@\"
")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/labdb-mcp-server ${STARTUP_SCRIPT_CONTENT})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/labdb-mcp-server
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
               GROUP_READ GROUP_EXECUTE 
               WORLD_READ WORLD_EXECUTE
)

# Create a Python launcher script for direct execution
set(PYTHON_LAUNCHER_CONTENT "#!/usr/bin/env python3
\"\"\"
LabDb MCP Server Python Launcher
Ensures proper PYTHONPATH and working directory setup
\"\"\"
import sys
import os
from pathlib import Path

# Set up paths
server_dir = Path(__file__).parent / 'db9-mcp-server'
sys.path.insert(0, str(server_dir))
sys.path.insert(0, str(server_dir / 'labdb'))

# Change to server directory
os.chdir(server_dir)

# Import and run the server
try:
    from src.db9_server import main
    main()
except ImportError as e:
    print(f\"Error importing server: {e}\", file=sys.stderr)
    print(f\"PYTHONPATH: {sys.path}\", file=sys.stderr)
    print(f\"Working directory: {os.getcwd()}\", file=sys.stderr)
    sys.exit(1)
")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/labdb-mcp-server.py ${PYTHON_LAUNCHER_CONTENT})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/labdb-mcp-server.py
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
               GROUP_READ GROUP_EXECUTE 
               WORLD_READ WORLD_EXECUTE
)

message(STATUS "LabDb MCP Server will be installed to: ${MCP_INSTALL_DIR}")
message(STATUS "Startup scripts will be installed to: ${CMAKE_INSTALL_PREFIX}/bin")
