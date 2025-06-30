# LabDb Migration Tool
cmake_minimum_required(VERSION 3.20)

# Add the migration tool executable
add_executable(labdb-migrate
    labdb-migrate.cpp
    # Add MigrationTool.cpp when implemented
    # MigrationTool.cpp
)

# Include directories
target_include_directories(labdb-migrate 
    PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

# Link against LabDb library
target_link_libraries(labdb-migrate 
    PRIVATE 
    labdb
    ${LMDB_LIBRARIES}
)

# Compiler options
target_compile_features(labdb-migrate PRIVATE cxx_std_20)
target_compile_options(labdb-migrate PRIVATE -Wall -Wextra)

# Add to install
install(TARGETS labdb-migrate
    DESTINATION bin
    COMPONENT tools
)

# Add custom target for testing migration tool
add_custom_target(test_migration_tool
    COMMAND labdb-migrate --help
    DEPENDS labdb-migrate
    COMMENT "Testing migration tool help output"
)
