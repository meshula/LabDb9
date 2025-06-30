# LabDb Tools
cmake_minimum_required(VERSION 3.16)

message(STATUS "Configuring LabDb tools...")

# Command-line utilities
add_executable(labdb-explore labdb_explorer.cpp)
target_link_libraries(labdb-explore PRIVATE labdb)
target_include_directories(labdb-explore PRIVATE ../include)

add_executable(create-test-db create_test_db.cpp)
target_link_libraries(create-test-db PRIVATE labdb)
target_include_directories(create-test-db PRIVATE ../include)

# Interactive tools
#if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/db9-interactive/CMakeLists.txt)
#    message(STATUS "Adding db9-interactive triadic consciousness explorer...")
#    add_subdirectory(db9-interactive)
#else()
#    message(STATUS "db9-interactive not found, skipping")
#endif()

# Install all tools
install(TARGETS labdb-explore create-test-db DESTINATION bin)

message(STATUS "LabDb tools configured")
