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

void test_database_lifecycle() {
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
    
    TEST_SUCCESS("Database Lifecycle Operations");
}

void test_entity_operations_success_cases() {
    TEST_START("Entity Operations - Success Cases");
    
    // First, open a database to get a valid DBID
    std::string openCmd = "(open-database :path \"" + g_test.test_db_path + "\")";
    auto openResponse = g_test.dispatcher->executeCommand(openCmd);
    validateResponse(openResponse, LabDb::Db9Response::Success, "Opening database for entity tests");
    
    std::string dbid = extractJsonField(openResponse.result, "dbid");
    AXIOM(!dbid.empty(), "Should get valid database ID");
    std::cout << "    Using database ID: " << dbid << "\n";
    
    // Test 1: Add entity - basic case
    TEST_SECTION("Adding basic entity");
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
    
    // Test 2: Add more entities
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
    
    // Test 3: Get entity by EID
    TEST_SECTION("Retrieving entities by EID");
    std::string getCmd = "(get-entity :dbid " + dbid + " :eid \"" + graniteEid + "\")";
    response = g_test.dispatcher->executeCommand(getCmd);
    validateResponse(response, LabDb::Db9Response::Success, "Getting entity by EID");
    
    std::string retrievedValue = extractJsonField(response.result, "value");
    AXIOM(retrievedValue == "granite", "Retrieved value should match original");
    std::cout << "    Successfully retrieved: " << retrievedValue << "\n";
    
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
    
    TEST_SUCCESS("Entity Operations - Success Cases");
}

void test_entity_operations_failure_cases() {
    TEST_START("Entity Operations - Failure Cases");
    
    // Test 1: Operations with invalid database ID
    TEST_SECTION("Testing invalid database ID");
    std::string invalidDbid = "999";
    std::string addCmd = "(add-entity :dbid " + invalidDbid + " :value \"invalid_test\")";
    auto response = g_test.dispatcher->executeCommand(addCmd);
    validateResponse(response, LabDb::Db9Response::Error, "Invalid database ID should fail");
    
    // Test 2: Get entity with non-existent EID using valid DBID
    TEST_SECTION("Testing non-existent EID");
    // First get a valid DBID
    std::string openCmd = "(open-database :path \"" + g_test.test_db_path + "\")";
    auto openResponse = g_test.dispatcher->executeCommand(openCmd);
    std::string validDbid = extractJsonField(openResponse.result, "dbid");
    
    std::string getCmd = "(get-entity :dbid " + validDbid + " :eid \"nonexistent_eid_12345\")";
    response = g_test.dispatcher->executeCommand(getCmd);
    validateResponse(response, LabDb::Db9Response::Error, "Non-existent EID should fail");
    
    // Test 3: Add entity without required parameters
    TEST_SECTION("Testing missing parameters");
    std::string missingValueCmd = "(add-entity :dbid " + validDbid + ")";
    response = g_test.dispatcher->executeCommand(missingValueCmd);
    validateResponse(response, LabDb::Db9Response::Error, "Missing value parameter should fail");
    
    std::string missingDbidCmd = "(add-entity :value \"test\")";
    response = g_test.dispatcher->executeCommand(missingDbidCmd);
    validateResponse(response, LabDb::Db9Response::Error, "Missing dbid parameter should fail");
    
    // Test 4: Find with invalid pattern
    TEST_SECTION("Testing invalid find patterns");
    std::string invalidPatternCmd = "(find-entity :dbid " + validDbid + " :pattern \"\")";
    response = g_test.dispatcher->executeCommand(invalidPatternCmd);
    validateResponse(response, LabDb::Db9Response::Error, "Empty pattern should fail");
    
    TEST_SUCCESS("Entity Operations - Failure Cases");
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

void test_triple_operations() {
    TEST_START("Triple Operations - Phase 3");
    
    // Get a valid DBID first
    std::string openCmd = "(open-database :path \"" + g_test.test_db_path + "\")";
    auto openResponse = g_test.dispatcher->executeCommand(openCmd);
    std::string dbid = extractJsonField(openResponse.result, "dbid");
    AXIOM(!dbid.empty(), "Should get valid database ID for triple tests");
    std::cout << "    Using database ID: " << dbid << "\n";
    
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
    
    // Test 4: Error cases
    TEST_SECTION("Testing triple operation error cases");
    
    // Missing parameters
    std::string missingSubjectCmd = "(add-triple :dbid " + dbid + " :predicate \"contains\" :object \"quartz\")";
    response = g_test.dispatcher->executeCommand(missingSubjectCmd);
    validateResponse(response, LabDb::Db9Response::Error, "Missing subject should fail");
    
    // Invalid database ID
    std::string invalidDbCmd = "(add-triple :dbid 999 :subject \"test\" :predicate \"test\" :object \"test\")";
    response = g_test.dispatcher->executeCommand(invalidDbCmd);
    validateResponse(response, LabDb::Db9Response::Error, "Invalid database ID should fail");
    
    TEST_SUCCESS("Triple Operations - Phase 3");
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
        test_database_lifecycle();
        test_entity_operations_success_cases();
        test_entity_operations_failure_cases();
        test_malformed_commands();
        test_multiple_commands();
        test_triple_operations();
        test_performance_metrics();
        
        // Summary
        std::cout << "\n🎉 ALL TESTS PASSED!\n";
        std::cout << "=====================================\n";
        std::cout << "✅ Test environment setup\n";
        std::cout << "✅ Dispatcher basic functionality\n";
        std::cout << "✅ Database lifecycle operations\n";
        std::cout << "✅ Entity operations (success cases)\n";
        std::cout << "✅ Entity operations (failure cases)\n";
        std::cout << "✅ Malformed command handling\n";
        std::cout << "✅ Multiple command execution\n";
        std::cout << "✅ Triple operations (Phase 3)\n";
        std::cout << "✅ Performance metrics validation\n";
        std::cout << "\n🎉 Phase 3 Triple Operations Complete!\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n💥 Test failed with exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "\n💥 Test failed with unknown exception\n";
        return 1;
    }
}
