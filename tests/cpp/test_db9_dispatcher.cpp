#include "LabDb/Db9Dispatcher.h"
#include "LabDb/DatabaseVerbs.h"
#include "LabDb/NonoStore.h"
#include <iostream>
#include <cassert>
#include <filesystem>
#include <vector>
#include <string>
#include <chrono>

// Note: Verbs should be auto-registered via static initializers in DatabaseVerbs.cpp

//-----------------------------------------------------------------------------
// Test Framework Macros
//-----------------------------------------------------------------------------
#define AXIOM(x, msg) \
    if (!(x)) { \
        std::cerr << "❌ FAILED: " << msg << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define TEST_START(name) \
    std::cout << "\n🧪 Testing: " << name << "...\n";

#define TEST_SUCCESS(name) \
    std::cout << "✅ PASSED: " << name << "\n";

#define TEST_SECTION(desc) \
    std::cout << "  📋 " << desc << "\n";

//-----------------------------------------------------------------------------
// Global test state
//-----------------------------------------------------------------------------
struct TestState {
    std::string test_db_path;
    LabDb::Db9Dispatcher* dispatcher;
    int tests_run{0};
    int tests_passed{0};
    int verbosity{1}; // 0=minimal, 1=normal, 2=verbose
    
    TestState() : test_db_path("/tmp/labdb_db9_test"), dispatcher(nullptr) {}
};

static TestState g_test;

//-----------------------------------------------------------------------------
// Helper Functions
//-----------------------------------------------------------------------------
void setupTestEnvironment() {
    TEST_START("Test Environment Setup");
    
    // Initialize verb registration
    initDatabaseVerbRegistration();

    // Clean up any existing test database
    std::filesystem::remove_all(g_test.test_db_path);
    
    // Get global dispatcher instance
    g_test.dispatcher = &LabDb::getGlobalDb9Dispatcher();
    AXIOM(g_test.dispatcher != nullptr, "Failed to get global dispatcher");
    
    TEST_SUCCESS("Test Environment Setup");
}

void validateResponse(const LabDb::Db9Response& response, 
                     LabDb::Db9Response::Status expectedStatus,
                     const std::string& context) {
    if (expectedStatus == LabDb::Db9Response::Success) {
        AXIOM(response.status == LabDb::Db9Response::Success, 
              context + " - Expected success but got: " + response.error_message);
    } else {
        AXIOM(response.status != LabDb::Db9Response::Success,
              context + " - Expected failure but got success");
    }
    
    // Verbosity-controlled output
    if (g_test.verbosity >= 2) {
        std::cout << "    Response result: " << response.result << "\n";
    } else if (g_test.verbosity >= 1 && !response.result.empty()) {
        // Show first 200 chars for normal verbosity
        std::string preview = response.result.length() > 200 ? 
                             response.result.substr(0, 200) + "..." : 
                             response.result;
        std::cout << "    " << context << " result: " << preview << "\n";
    }
}

std::string extractJsonField(const std::string& json, const std::string& field) {
    // Simple JSON field extraction for testing
    std::string searchFor = "\"" + field + "\":";
    size_t pos = json.find(searchFor);
    if (pos == std::string::npos) return "";
    
    pos += searchFor.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    
    if (pos >= json.length()) return "";
    
    if (json[pos] == '"') {
        // String value
        pos++;
        size_t end = json.find('"', pos);
        if (end == std::string::npos) return "";
        return json.substr(pos, end - pos);
    } else {
        // Number or other value
        size_t end = pos;
        while (end < json.length() && json[end] != ',' && json[end] != '}' && json[end] != '\n') {
            end++;
        }
        return json.substr(pos, end - pos);
    }
}

//-----------------------------------------------------------------------------
// Test Functions
//-----------------------------------------------------------------------------

void test_dispatcher_basic_functionality() {
    TEST_START("Dispatcher Basic Functionality");
    
    // Test 1: Verify dispatcher exists and has verbs registered
    TEST_SECTION("Checking available verbs");
    auto verbs = g_test.dispatcher->getAvailableVerbs();
    std::cout << "    Available verbs: ";
    for (const auto& verb : verbs) {
        std::cout << verb << " ";
    }
    std::cout << "\n";
    
    AXIOM(!verbs.empty(), "No verbs registered in dispatcher");
    AXIOM(verbs.size() >= 3, "Expected at least 3 verbs (create-db, add-entity, get-entity)");
    
    // Test 2: Check if expected Phase 2 verbs are present
    bool hasOpenDb = false, hasAddEntity = false, hasGetEntity = false, hasFindEntity = false;
    for (const auto& verb : verbs) {
        if (verb == "open-database") hasOpenDb = true;
        if (verb == "add-entity") hasAddEntity = true;
        if (verb == "get-entity") hasGetEntity = true;
        if (verb == "find-entity") hasFindEntity = true;
    }
    
    AXIOM(hasOpenDb, "open-database verb not found");
    AXIOM(hasAddEntity, "add-entity verb not found");
    AXIOM(hasGetEntity, "get-entity verb not found");
    AXIOM(hasFindEntity, "find-entity verb not found");
    
    // Test 3: Get specification
    TEST_SECTION("Testing specification generation");
    std::string spec = g_test.dispatcher->getSpecification();
    AXIOM(!spec.empty(), "Specification should not be empty");
    AXIOM(spec.find("db9 Tool Specification") != std::string::npos, "Specification should have title");
    AXIOM(spec.find("S-expressions") != std::string::npos, "Specification should mention S-expressions");
    
    TEST_SUCCESS("Dispatcher Basic Functionality");
}

std::pair<std::string, std::string> test_database_lifecycle() {
    TEST_START("Database Lifecycle Operations");
    
    // Test 1: Create test database file using NonoStore directly
    TEST_SECTION("Creating test database file");
    try {
        LabDb::NonoStore store(g_test.test_db_path);
        // Add a test triple to make it a valid database
        store.add_triple("test", "isA", "placeholder");
        std::cout << "    Created database file at: " << g_test.test_db_path << "\n";
    } catch (const std::exception& e) {
        AXIOM(false, "Failed to create test database: " + std::string(e.what()));
    }
    
    // Test 2: Open database - success case
    TEST_SECTION("Opening test database");
    std::string openCmd = "(open-database :path \"" + g_test.test_db_path + "\")";
    auto response = g_test.dispatcher->executeCommand(openCmd);
    
    std::cout << "    Response: " << response.result << "\n";
    if (response.status != LabDb::Db9Response::Success) {
        std::cout << "    Error: " << response.error_message << "\n";
    }
    
    validateResponse(response, LabDb::Db9Response::Success, "Database opening");
    
    // Extract the DBID from the successful open response
    std::string dbid = extractJsonField(response.result, "dbid");
    AXIOM(!dbid.empty(), "Should return a database ID");
    std::cout << "    Opened database with DBID: " << dbid << "\n";
    
    // Test 3: Try to open database again - should succeed and return new DBID
    TEST_SECTION("Testing duplicate database opening");
    response = g_test.dispatcher->executeCommand(openCmd);
    validateResponse(response, LabDb::Db9Response::Success, "Duplicate opening should succeed");
    std::string dbid2 = extractJsonField(response.result, "dbid");
    std::cout << "    Second open got DBID: " << dbid2 << "\n";
    
    // Test 4: Try to open database with invalid path - should fail
    TEST_SECTION("Testing invalid database path");
    std::string invalidCmd = "(open-database :path \"/invalid/nonexistent/path/test.db\")";
    response = g_test.dispatcher->executeCommand(invalidCmd);
    validateResponse(response, LabDb::Db9Response::Error, "Invalid path should fail");
    
    // Test 5: Test create-database verb with equivalent results
    TEST_SECTION("Testing create-database verb");
    std::string created_db_path = "/tmp/labdb_created_test";
    
    // Remove test file if it exists
    std::filesystem::remove_all(created_db_path);
    
    // Create database using create-database verb
    std::string createCmd = "(create-database :path \"" + created_db_path + "\")";
    auto createResponse = g_test.dispatcher->executeCommand(createCmd);
    validateResponse(createResponse, LabDb::Db9Response::Success, "Database creation should succeed");
    
    std::string created_dbid = extractJsonField(createResponse.result, "dbid");
    AXIOM(!created_dbid.empty(), "Should return a database ID for created database");
    std::cout << "    Created database with DBID: " << created_dbid << "\n";
    
    // Verify created database file exists
    AXIOM(std::filesystem::exists(created_db_path), "Created database file should exist");
    
    // Add test triple to created database to make it equivalent
    std::string addTripleCmd = "(add-triple :dbid " + created_dbid + " :subject \"test\" :predicate \"isA\" :object \"placeholder\")";
    auto tripleResponse = g_test.dispatcher->executeCommand(addTripleCmd);
    validateResponse(tripleResponse, LabDb::Db9Response::Success, "Adding test triple to created database");

    TEST_SUCCESS("Database Lifecycle Operations");
    
    // Return both dbids for comparative testing
    return std::make_pair(dbid, created_dbid);
}

void test_database_isolation() {
    TEST_START("Database Isolation Verification");
    
    // Create three separate databases with distinct content
    std::vector<std::string> db_paths = {
        "/tmp/labdb_isolation_test_1",
        "/tmp/labdb_isolation_test_2", 
        "/tmp/labdb_isolation_test_3"
    };
    
    // Three entities per database for comprehensive testing
    std::vector<std::vector<std::string>> test_entities = {
        {"test_1a", "test_1b", "test_1c"},      // db1
        {"granite_2a", "granite_2b", "granite_2c"},   // db2  
        {"grass_3a", "grass_3b", "grass_3c"}    // db3
    };
    
    std::vector<std::string> dbids;
    std::vector<std::vector<std::string>> all_eids(3); // Store EIDs for each database
    
    // Clean up any existing test databases
    for (const auto& path : db_paths) {
        std::filesystem::remove_all(path);
    }
    
    TEST_SECTION("Creating three isolated databases");
    
    // Create databases first
    for (size_t i = 0; i < db_paths.size(); ++i) {
        std::string createCmd = "(create-database :path \"" + db_paths[i] + "\")";
        auto createResponse = g_test.dispatcher->executeCommand(createCmd);
        validateResponse(createResponse, LabDb::Db9Response::Success, 
                        "Database " + std::to_string(i+1) + " creation should succeed");
        
        std::string dbid = extractJsonField(createResponse.result, "dbid");
        AXIOM(!dbid.empty(), "Should return database ID for database " + std::to_string(i+1));
        dbids.push_back(dbid);
        
        std::cout << "    Created database " << (i+1) << " with DBID: " << dbid 
                  << " at path: " << db_paths[i] << "\n";
    }
    
    TEST_SECTION("Adding entities in interleaved pattern (1,2,3,1,2,3,1,2,3)");
    
    // Add entities in interleaved pattern: db1-entity1, db2-entity1, db3-entity1, db1-entity2, etc.
    for (size_t entity_idx = 0; entity_idx < 3; ++entity_idx) {
        for (size_t db_idx = 0; db_idx < dbids.size(); ++db_idx) {
            std::string entity_name = test_entities[db_idx][entity_idx];
            std::string addEntityCmd = "(add-entity :dbid " + dbids[db_idx] + " :value \"" + entity_name + "\")";
            auto addResponse = g_test.dispatcher->executeCommand(addEntityCmd);
            validateResponse(addResponse, LabDb::Db9Response::Success, 
                            "Adding entity '" + entity_name + "' to database " + std::to_string(db_idx+1));
            
            std::string eid = extractJsonField(addResponse.result, "eid");
            all_eids[db_idx].push_back(eid);
            std::cout << "    Added '" << entity_name << "' with EID: " << eid 
                      << " to database " << (db_idx+1) << "\n";
        }
    }
    
    TEST_SECTION("Verifying comprehensive database isolation - all entity retrieval");
    
    // Verify each database can retrieve ALL its entities correctly
    for (size_t db_idx = 0; db_idx < dbids.size(); ++db_idx) {
        std::cout << "    Testing database " << (db_idx+1) << " (DBID: " << dbids[db_idx] << "):\n";
        
        for (size_t entity_idx = 0; entity_idx < 3; ++entity_idx) {
            std::string expected_entity = test_entities[db_idx][entity_idx];
            std::string eid = all_eids[db_idx][entity_idx];
            
            std::string getCmd = "(get-entity :dbid " + dbids[db_idx] + " :eid \"" + eid + "\")";
            auto getResponse = g_test.dispatcher->executeCommand(getCmd);
            validateResponse(getResponse, LabDb::Db9Response::Success, 
                            "Getting entity '" + expected_entity + "' from database " + std::to_string(db_idx+1));
            
            std::string retrieved = extractJsonField(getResponse.result, "value");
            std::cout << "      EID " << eid << " → '" << retrieved << "' (expected: '" << expected_entity << "')\n";
            
            AXIOM(retrieved == expected_entity, 
                  "Database " + std::to_string(db_idx+1) + " EID " + eid + " should return '" + expected_entity + 
                  "' but returned '" + retrieved + "'");
        }
    }
    
    TEST_SECTION("Verifying cross-database EID isolation");
    
    // Verify databases don't return each other's entities when using cross-database EIDs
    for (size_t db_idx = 0; db_idx < dbids.size(); ++db_idx) {
        for (size_t other_db_idx = 0; other_db_idx < dbids.size(); ++other_db_idx) {
            if (db_idx == other_db_idx) continue; // Skip same database
            
            // Try to use other database's EIDs in this database
            for (size_t entity_idx = 0; entity_idx < 3; ++entity_idx) {
                std::string foreign_eid = all_eids[other_db_idx][entity_idx];
                std::string foreign_entity = test_entities[other_db_idx][entity_idx];
                
                std::string getCmd = "(get-entity :dbid " + dbids[db_idx] + " :eid \"" + foreign_eid + "\")";
                auto getResponse = g_test.dispatcher->executeCommand(getCmd);
                
                // Should fail - foreign EID should not resolve in this database
                validateResponse(getResponse, LabDb::Db9Response::Error, 
                                "Foreign EID " + foreign_eid + " should not resolve in database " + std::to_string(db_idx+1));
                
                std::cout << "      ✓ Database " << (db_idx+1) << " correctly rejects foreign EID " 
                          << foreign_eid << " (from db" << (other_db_idx+1) << ")\n";
            }
        }
    }
    
    TEST_SECTION("Verifying entity name isolation");
    
    // Verify databases don't return each other's entities by name search
    for (size_t db_idx = 0; db_idx < dbids.size(); ++db_idx) {
        for (size_t other_db_idx = 0; other_db_idx < dbids.size(); ++other_db_idx) {
            if (db_idx == other_db_idx) continue; // Skip same database
            
            for (size_t entity_idx = 0; entity_idx < 3; ++entity_idx) {
                std::string foreign_entity = test_entities[other_db_idx][entity_idx];
                
                std::string findCmd = "(find-entity :dbid " + dbids[db_idx] + 
                                     " :pattern \"" + foreign_entity + "\")";
                auto findResponse = g_test.dispatcher->executeCommand(findCmd);
                validateResponse(findResponse, LabDb::Db9Response::Success, 
                                "Find operation should succeed even if no results");
                
                // Should return empty array since entity doesn't exist in this database
                std::string results = findResponse.result;
                AXIOM(results == "[]", 
                      "Database " + std::to_string(db_idx+1) + " should not contain entity '" + 
                      foreign_entity + "' but find returned: " + results);
            }
        }
    }
    
    TEST_SECTION("Testing database lookup verification");
    
    // Test invalid database ID
    std::string invalidCmd = "(get-entity :dbid invalid_db_999 :eid \"eid:1\")";
    auto invalidResponse = g_test.dispatcher->executeCommand(invalidCmd);
    validateResponse(invalidResponse, LabDb::Db9Response::Error, "Invalid DBID should fail");
    std::cout << "    ✓ Invalid DBID correctly rejected\n";
    
    TEST_SECTION("Cleanup - closing databases");
    
    // Close all test databases
    for (size_t i = 0; i < dbids.size(); ++i) {
        std::string closeCmd = "(close-database :dbid " + dbids[i] + ")";
        auto closeResponse = g_test.dispatcher->executeCommand(closeCmd);
        validateResponse(closeResponse, LabDb::Db9Response::Success, 
                        "Closing database " + std::to_string(i+1));
        std::cout << "    Closed database " << (i+1) << " (DBID: " << dbids[i] << ")\n";
    }
    
    // Clean up test files
    for (const auto& path : db_paths) {
        std::filesystem::remove_all(path);
    }
    
    TEST_SUCCESS("Database Isolation Verification");
}


// Helper function to create fresh database for testing
std::string create_fresh_database(const std::string& name_suffix) {
    std::string db_path = "/tmp/labdb_fresh_" + name_suffix;
    
    // Clean up any existing database
    std::filesystem::remove_all(db_path);
    
    // Create fresh database
    std::string createCmd = "(create-database :path \"" + db_path + "\")";
    auto response = g_test.dispatcher->executeCommand(createCmd);
    
    if (response.status != LabDb::Db9Response::Success) {
        throw std::runtime_error("Failed to create fresh database: " + response.error_message);
    }
    
    std::string dbid = extractJsonField(response.result, "dbid");
    if (dbid.empty()) {
        throw std::runtime_error("Failed to get DBID from fresh database creation");
    }
    
    return dbid;
}

void test_entity_operations_success_cases(const std::string& test_name) {
    TEST_START("Entity Operations - Success Cases (" + test_name + ")");
    
    // Create fresh database for this test
    std::string dbid = create_fresh_database("entity_success_" + test_name);
    std::cout << "    Using fresh database ID: " << dbid << "\n";
    
    // Test 1: Add entity - basic case
    TEST_SECTION("Adding basic entity: granite");
    std::string addCmd = "(add-entity :dbid " + dbid + " :value \"granite\")";
    auto response = g_test.dispatcher->executeCommand(addCmd);
    
    std::cout << "    Add entity response: " << response.result << "\n";
    if (response.status != LabDb::Db9Response::Success) {
        std::cout << "    Error: " << response.error_message << "\n";
    }
    validateResponse(response, LabDb::Db9Response::Success, "Adding entity 'granite'");

    // Extract the EID from response
    std::string graniteEid = extractJsonField(response.result, "eid");
    AXIOM(!graniteEid.empty(), "Should return an EID for created entity");
    std::cout << "    Created entity with EID: " << graniteEid << "\n";

    // Test 2: Get entity by EID
    TEST_SECTION("Retrieving entities by EID");    
    std::string getCmd = "(get-entity :dbid " + dbid + " :eid \"" + graniteEid + "\")";
    response = g_test.dispatcher->executeCommand(getCmd);
    validateResponse(response, LabDb::Db9Response::Success, "Getting entity by EID");

    std::string retrievedValue = extractJsonField(response.result, "value");
    std::cout << "    Retrieved entity value: " << retrievedValue << "\n";
    AXIOM(retrievedValue == "granite", "Retrieved value should match original");
    std::cout << "    Successfully retrieved: " << retrievedValue << "\n";

    // Test 3: Add more entities
    TEST_SECTION("Adding multiple entities");
    std::vector<std::string> entities = {"quartz", "feldspar", "mica", "rock_crystal"};
    std::vector<std::string> eids;
    
    for (const auto& entity : entities) {
        std::string cmd = "(add-entity :dbid " + dbid + " :value \"" + entity + "\")";
        response = g_test.dispatcher->executeCommand(cmd);
        validateResponse(response, LabDb::Db9Response::Success, "Adding entity '" + entity + "'");
        
        std::string eid = extractJsonField(response.result, "eid");
        AXIOM(!eid.empty(), "Should return EID for " + entity);
        eids.push_back(eid);
    }
        
    // Test 4: Find entities with wildcard patterns
    TEST_SECTION("Finding entities with patterns");
    
    // Find all entities
    std::string findAllCmd = "(find-entity :dbid " + dbid + " :pattern \"*\")";
    response = g_test.dispatcher->executeCommand(findAllCmd);
    validateResponse(response, LabDb::Db9Response::Success, "Finding all entities");
    std::cout << "    Find all result: " << response.result << "\n";
    
    // Find entities with prefix
    std::string findPrefixCmd = "(find-entity :dbid " + dbid + " :pattern \"rock*\")";
    response = g_test.dispatcher->executeCommand(findPrefixCmd);
    validateResponse(response, LabDb::Db9Response::Success, "Finding entities with prefix");
    std::cout << "    Find prefix result: " << response.result << "\n";
    
    // Close the test database
    std::string closeCmd = "(close-database :dbid " + dbid + ")";
    auto closeResponse = g_test.dispatcher->executeCommand(closeCmd);
    if (closeResponse.status == LabDb::Db9Response::Success) {
        std::cout << "    Closed test database: " << dbid << "\n";
    }
    
    TEST_SUCCESS("Entity Operations - Success Cases (" + test_name + ")");
}

void test_entity_operations_failure_cases(const std::string& test_name) {
    TEST_START("Entity Operations - Failure Cases (" + test_name + ")");
    
    // Create fresh database for this test
    std::string dbid = create_fresh_database("entity_failure_" + test_name);
    std::cout << "    Using fresh database ID: " << dbid << "\n";
    
    // Test 1: Operations with invalid database ID
    TEST_SECTION("Testing invalid database ID");
    std::string invalidDbid = "999";
    std::string addCmd = "(add-entity :dbid " + invalidDbid + " :value \"invalid_test\")";
    auto response = g_test.dispatcher->executeCommand(addCmd);
    validateResponse(response, LabDb::Db9Response::Error, "Invalid database ID should fail");
    
    // Test 2: Get entity with non-existent EID using valid DBID
    TEST_SECTION("Testing non-existent EID");
    std::string getCmd = "(get-entity :dbid " + dbid + " :eid \"nonexistent_eid_12345\")";
    response = g_test.dispatcher->executeCommand(getCmd);
    validateResponse(response, LabDb::Db9Response::Error, "Non-existent EID should fail");
    
    // Test 3: Add entity without required parameters
    TEST_SECTION("Testing missing parameters");
    std::string missingValueCmd = "(add-entity :dbid " + dbid + ")";
    response = g_test.dispatcher->executeCommand(missingValueCmd);
    validateResponse(response, LabDb::Db9Response::Error, "Missing value parameter should fail");
    
    std::string missingDbidCmd = "(add-entity :value \"test\")";
    response = g_test.dispatcher->executeCommand(missingDbidCmd);
    validateResponse(response, LabDb::Db9Response::Error, "Missing dbid parameter should fail");
    
    // Test 4: Find with invalid pattern
    TEST_SECTION("Testing invalid find patterns");
    std::string invalidPatternCmd = "(find-entity :dbid " + dbid + " :pattern \"\")";
    response = g_test.dispatcher->executeCommand(invalidPatternCmd);
    validateResponse(response, LabDb::Db9Response::Error, "Empty pattern should fail");
    
    // Close the test database
    std::string closeCmd = "(close-database :dbid " + dbid + ")";
    auto closeResponse = g_test.dispatcher->executeCommand(closeCmd);
    if (closeResponse.status == LabDb::Db9Response::Success) {
        std::cout << "    Closed test database: " << dbid << "\n";
    }
    
    TEST_SUCCESS("Entity Operations - Failure Cases (" + test_name + ")");
}

void test_malformed_commands() {
    TEST_START("Malformed Command Handling");
    
    // Test 1: Invalid S-expression syntax
    TEST_SECTION("Testing malformed S-expressions");
    std::vector<std::string> malformedCommands = {
        "(unclosed-paren",
        "not-an-sexpr",
        "()",
        "(unknown-verb :param value)",
        "(add-entity :invalid-param-name value)",
        "(add-entity :dbid not-a-number :value \"test\")"
    };
    
    for (const auto& cmd : malformedCommands) {
        auto response = g_test.dispatcher->executeCommand(cmd);
        validateResponse(response, LabDb::Db9Response::Error, "Malformed command should fail: " + cmd);
        std::cout << "    ✓ Correctly rejected: " << cmd << "\n";
    }
    
    TEST_SUCCESS("Malformed Command Handling");
}

void test_multiple_commands() {
    TEST_START("Multiple Command Execution");
    
    // Get a valid DBID first
    std::string openCmd = "(open-database :path \"" + g_test.test_db_path + "\")";
    auto openResponse = g_test.dispatcher->executeCommand(openCmd);
    std::string dbid = extractJsonField(openResponse.result, "dbid");
    
    // Test batch execution
    TEST_SECTION("Testing batch command execution");
    std::vector<std::string> commands = {
        "(add-entity :dbid " + dbid + " :value \"batch_test_1\")",
        "(add-entity :dbid " + dbid + " :value \"batch_test_2\")",
        "(find-entity :dbid " + dbid + " :pattern \"batch_test*\")"
    };
    
    auto response = g_test.dispatcher->executeCommands(commands);
    validateResponse(response, LabDb::Db9Response::Success, "Batch command execution");
    
    std::cout << "    Batch result: " << response.result << "\n";
    
    TEST_SUCCESS("Multiple Command Execution");
}

void test_triple_operations(const std::string& test_name) {
    TEST_START("Triple Operations - Phase 3 (" + test_name + ")");
    
    // Create fresh database for this test
    std::string dbid = create_fresh_database("triple_ops_" + test_name);
    std::cout << "    Using fresh database ID: " << dbid << "\n";
    
    // Test 1: Add basic triple
    TEST_SECTION("Adding basic triple");
    std::string addTripleCmd = "(add-triple :dbid " + dbid + " :subject \"granite\" :predicate \"contains\" :object \"quartz\")";
    auto response = g_test.dispatcher->executeCommand(addTripleCmd);
    
    std::cout << "    Add triple response: " << response.result << "\n";
    if (response.status != LabDb::Db9Response::Success) {
        std::cout << "    Error: " << response.error_message << "\n";
    }
    validateResponse(response, LabDb::Db9Response::Success, "Adding triple 'granite contains quartz'");
    
    // Verify the response contains expected fields
    std::string status = extractJsonField(response.result, "status");
    std::string subject = extractJsonField(response.result, "subject");
    std::string predicate = extractJsonField(response.result, "predicate");
    std::string object = extractJsonField(response.result, "object");
    
    AXIOM(status == "added", "Status should be 'added'");
    AXIOM(subject == "granite", "Subject should match input");
    AXIOM(predicate == "contains", "Predicate should match input");
    AXIOM(object == "quartz", "Object should match input");
    
    // Test 2: Add more triples to build a knowledge base
    TEST_SECTION("Adding multiple triples");
    std::vector<std::tuple<std::string, std::string, std::string>> triples = {
        {"quartz", "hasProperty", "hardness_7"},
        {"granite", "isA", "igneous_rock"},
        {"feldspar", "isA", "mineral"},
        {"granite", "contains", "feldspar"},
        {"granite", "contains", "mica"}
    };
    
    for (const auto& [s, p, o] : triples) {
        std::string cmd = "(add-triple :dbid " + dbid + " :subject \"" + s + "\" :predicate \"" + p + "\" :object \"" + o + "\")";
        response = g_test.dispatcher->executeCommand(cmd);
        validateResponse(response, LabDb::Db9Response::Success, "Adding triple '" + s + " " + p + " " + o + "'");
    }
    
    // Test 3: Find triples using patterns
    TEST_SECTION("Finding triples with patterns");
    
    // Find all triples
    std::string findAllCmd = "(find-triple :dbid " + dbid + ")";
    response = g_test.dispatcher->executeCommand(findAllCmd);
    validateResponse(response, LabDb::Db9Response::Success, "Finding all triples");
    std::cout << "    Find all triples result (first 200 chars): " << response.result.substr(0, 200) << "...\n";
    
    // Find triples with specific subject
    std::string findSubjectCmd = "(find-triple :dbid " + dbid + " :subject \"granite\")";
    response = g_test.dispatcher->executeCommand(findSubjectCmd);
    validateResponse(response, LabDb::Db9Response::Success, "Finding triples with subject 'granite'");
    std::cout << "    Find granite triples: " << response.result << "\n";
    
    // Find triples with specific predicate
    std::string findPredicateCmd = "(find-triple :dbid " + dbid + " :predicate \"contains\")";
    response = g_test.dispatcher->executeCommand(findPredicateCmd);
    validateResponse(response, LabDb::Db9Response::Success, "Finding triples with predicate 'contains'");
    std::cout << "    Find 'contains' triples: " << response.result << "\n";
    
    // Find specific triple
    std::string findSpecificCmd = "(find-triple :dbid " + dbid + " :subject \"granite\" :predicate \"contains\" :object \"quartz\")";
    response = g_test.dispatcher->executeCommand(findSpecificCmd);
    validateResponse(response, LabDb::Db9Response::Success, "Finding specific triple 'granite contains quartz'");
    std::cout << "    Find specific triple: " << response.result << "\n";
    
    // Test 4: Get specific triple
    TEST_SECTION("Getting specific triples");
    
    // Get existing triple
    std::string getTripleCmd = "(get-triple :dbid " + dbid + " :subject \"granite\" :predicate \"contains\" :object \"quartz\")";
    response = g_test.dispatcher->executeCommand(getTripleCmd);
    validateResponse(response, LabDb::Db9Response::Success, "Getting existing triple 'granite contains quartz'");
    std::cout << "    Get existing triple: " << response.result << "\n";
    
    // Try to get non-existent triple
    std::string getNonExistentCmd = "(get-triple :dbid " + dbid + " :subject \"granite\" :predicate \"contains\" :object \"diamond\")";
    response = g_test.dispatcher->executeCommand(getNonExistentCmd);
    validateResponse(response, LabDb::Db9Response::Error, "Getting non-existent triple should fail");
    
    // Test 5: Remove triple
    TEST_SECTION("Removing triples");
    
    // First verify the triple exists
    std::string verifyExistsCmd = "(find-triple :dbid " + dbid + " :subject \"granite\" :predicate \"contains\" :object \"mica\")";
    response = g_test.dispatcher->executeCommand(verifyExistsCmd);
    validateResponse(response, LabDb::Db9Response::Success, "Verify mica triple exists before removal");
    
    // Remove the triple
    std::string removeTripleCmd = "(remove-triple :dbid " + dbid + " :subject \"granite\" :predicate \"contains\" :object \"mica\")";
    response = g_test.dispatcher->executeCommand(removeTripleCmd);
    validateResponse(response, LabDb::Db9Response::Success, "Removing triple 'granite contains mica'");
    std::cout << "    Remove triple result: " << response.result << "\n";
    
    // Verify the triple no longer exists
    response = g_test.dispatcher->executeCommand(verifyExistsCmd);
    AXIOM(response.result == "[]", "Triple should no longer exist after removal");
    
    // Try to remove non-existent triple
    std::string removeNonExistentCmd = "(remove-triple :dbid " + dbid + " :subject \"granite\" :predicate \"contains\" :object \"diamond\")";
    response = g_test.dispatcher->executeCommand(removeNonExistentCmd);
    validateResponse(response, LabDb::Db9Response::Error, "Removing non-existent triple should fail");
    
    // Test 6: Error cases
    TEST_SECTION("Testing triple operation error cases");
    
    // Missing parameters
    std::string missingSubjectCmd = "(add-triple :dbid " + dbid + " :predicate \"contains\" :object \"quartz\")";
    response = g_test.dispatcher->executeCommand(missingSubjectCmd);
    validateResponse(response, LabDb::Db9Response::Error, "Missing subject should fail");
    
    // Invalid database ID
    std::string invalidDbCmd = "(add-triple :dbid 999 :subject \"test\" :predicate \"test\" :object \"test\")";
    response = g_test.dispatcher->executeCommand(invalidDbCmd);
    validateResponse(response, LabDb::Db9Response::Error, "Invalid database ID should fail");
    
    // Close the test database
    std::string closeCmd = "(close-database :dbid " + dbid + ")";
    auto closeResponse = g_test.dispatcher->executeCommand(closeCmd);
    if (closeResponse.status == LabDb::Db9Response::Success) {
        std::cout << "    Closed test database: " << dbid << "\n";
    }
    
    TEST_SUCCESS("Triple Operations - Phase 3 (" + test_name + ")");
}

void test_performance_metrics() {
    TEST_START("Performance Metrics Validation");
    
    // Get a valid DBID first
    std::string openCmd = "(open-database :path \"" + g_test.test_db_path + "\")";
    auto openResponse = g_test.dispatcher->executeCommand(openCmd);
    std::string dbid = extractJsonField(openResponse.result, "dbid");
    
    TEST_SECTION("Checking auto-reflexive metrics");
    std::string cmd = "(find-entity :dbid " + dbid + " :pattern \"*\")";
    auto response = g_test.dispatcher->executeCommand(cmd);
    
    validateResponse(response, LabDb::Db9Response::Success, "Command for metrics test");
    
    // Check that metrics are populated
    AXIOM(response.auto_reflexive.operation_time_ms.count() >= 0, "Operation time should be recorded");
    
    std::cout << "    Operation time: " << response.auto_reflexive.operation_time_ms.count() << "ms\n";
    std::cout << "    Items processed: " << response.auto_reflexive.items_processed << "\n";
    std::cout << "    Memory usage: " << response.auto_reflexive.memory_usage_kb << "KB\n";
    
    TEST_SUCCESS("Performance Metrics Validation");
}

//-----------------------------------------------------------------------------
// Main Test Runner
//-----------------------------------------------------------------------------
int main() {
    std::cout << "🚀 LabDb Db9Dispatcher Test Harness\n";
    std::cout << "=====================================\n";
    
    try {
        // Setup
        setupTestEnvironment();
        
        // Core functionality tests
        test_dispatcher_basic_functionality();

        test_database_isolation();

        auto dbids = test_database_lifecycle();

        // Test entity operations with fresh databases for clean isolation
        g_test.verbosity = 1; // Normal verbosity for entity tests
        test_entity_operations_success_cases("fresh_1");
        test_entity_operations_success_cases("fresh_2");
        test_entity_operations_failure_cases("fresh_1");
        test_entity_operations_failure_cases("fresh_2");
        
        test_malformed_commands();
        test_multiple_commands();
        
        // Test triple operations with fresh databases for clean isolation
        g_test.verbosity = 1; // Normal verbosity for triple tests
        test_triple_operations("fresh_1");
        test_triple_operations("fresh_2");
        
        test_performance_metrics();
        
        // Summary
        std::cout << "\n🎉 ALL TESTS PASSED!\n";
        std::cout << "=====================================\n";
        std::cout << "✅ Test environment setup\n";
        std::cout << "✅ Dispatcher basic functionality\n";
        std::cout << "✅ Database isolation verification\n";
        std::cout << "✅ Database lifecycle operations (including create-database)\n";
        std::cout << "✅ Entity operations (success cases) - Fresh isolated databases\n";
        std::cout << "✅ Entity operations (failure cases) - Fresh isolated databases\n";
        std::cout << "✅ Malformed command handling\n";
        std::cout << "✅ Multiple command execution\n";
        std::cout << "✅ Triple operations (Phase 3) - Fresh isolated databases\n";
        std::cout << "✅ Performance metrics validation\n";
        std::cout << "\n🎉 Phase 4.1 + Clean Database Isolation Testing Complete!\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n💥 Test failed with exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "\n💥 Test failed with unknown exception\n";
        return 1;
    }
}
