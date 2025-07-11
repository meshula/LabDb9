#include "LabDb/Db9Dispatcher.h"
#include "LabDb/DatabaseVerbs.h"
#include "LabDb/NonoStore.h"
#include <iostream>
#include <cassert>
#include <filesystem>
#include <vector>
#include <string>

// Test framework macros from main test file
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

// Helper function from main test file
std::string extractJsonField(const std::string& json, const std::string& field) {
    std::string searchFor = "\"" + field + "\":";
    size_t pos = json.find(searchFor);
    if (pos == std::string::npos) return "";
    
    pos += searchFor.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    
    if (pos >= json.length()) return "";
    
    if (json[pos] == '"') {
        pos++;
        size_t end = json.find('"', pos);
        if (end == std::string::npos) return "";
        return json.substr(pos, end - pos);
    } else {
        size_t end = pos;
        while (end < json.length() && json[end] != ',' && json[end] != '}' && json[end] != '\n') {
            end++;
        }
        return json.substr(pos, end - pos);
    }
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

std::string createFreshDatabase(const std::string& name_suffix) {
    std::string db_path = "/tmp/labdb_wildcard_test_" + name_suffix;
    std::filesystem::remove_all(db_path);
    
    auto& dispatcher = LabDb::getGlobalDb9Dispatcher();
    std::string createCmd = "(create-database :path \"" + db_path + "\")";
    auto response = dispatcher.executeCommand(createCmd);
    
    if (response.status != LabDb::Db9Response::Success) {
        throw std::runtime_error("Failed to create fresh database: " + response.error_message);
    }
    
    std::string dbid = extractJsonField(response.result, "dbid");
    if (dbid.empty()) {
        throw std::runtime_error("Failed to get DBID from fresh database creation");
    }
    
    return dbid;
}

void test_find_triple_wildcard_patterns() {
    TEST_START("Find Triple Wildcard Patterns - Critical Missing Functionality");
    
    // Create fresh database for this test
    std::string dbid = createFreshDatabase("wildcards");
    auto& dispatcher = LabDb::getGlobalDb9Dispatcher();
    std::cout << "    Using fresh database ID: " << dbid << "\n";
    
    // Create a comprehensive knowledge base for wildcard testing
    TEST_SECTION("Building knowledge base for wildcard testing");
    
    std::vector<std::tuple<std::string, std::string, std::string>> test_triples = {
        // Geological knowledge
        {"granite", "contains", "quartz"},
        {"granite", "contains", "feldspar"}, 
        {"granite", "contains", "mica"},
        {"granite", "isA", "igneous_rock"},
        {"quartz", "hasProperty", "hardness_7"},
        {"feldspar", "hasProperty", "hardness_6"},
        {"mica", "hasProperty", "hardness_2"},
        
        // Game development knowledge (from our Quadplay exploration)
        {"quadplay", "has_function", "draw_sprite"},
        {"quadplay", "has_function", "draw_rect"},
        {"quadplay", "has_function", "draw_map"},
        {"game_loop", "calls", "draw_sprite"},
        {"game_loop", "calls", "update_entities"},
        {"sprite", "has_property", "position"},
        {"sprite", "has_property", "animation"},
        
        // Scholarly knowledge (from our Euclid motivation)
        {"b3-p12", "part_of", "Book-3"},
        {"b3-p12", "depends_on", "b2-p5"},
        {"b3-p12-gk", "translation_of", "b3-p12"},
        {"b3-p12-en", "translation_of", "b3-p12"},
        {"euclid", "authored", "elements"},
        {"elements", "contains", "Book-3"}
    };
    
    // Add all test triples
    for (const auto& [subject, predicate, object] : test_triples) {
        std::string cmd = "(add-triple :dbid " + dbid + 
                         " :subject \"" + subject + 
                         "\" :predicate \"" + predicate + 
                         "\" :object \"" + object + "\")";
        auto response = dispatcher.executeCommand(cmd);
        validateResponse(response, LabDb::Db9Response::Success, 
                        "Adding test triple: " + subject + " " + predicate + " " + object);
    }
    
    std::cout << "    Added " << test_triples.size() << " test triples\n";
    
    // CRITICAL TEST 1: Find all triples (complete wildcard)
    TEST_SECTION("CRITICAL: Testing complete wildcard (:subject nil :predicate nil :object nil)");
    std::string findAllCmd = "(find-triple :dbid " + dbid + " :subject nil :predicate nil :object nil)";
    auto response = dispatcher.executeCommand(findAllCmd);
    validateResponse(response, LabDb::Db9Response::Success, "Complete wildcard should succeed");
    
    // Verify we get all triples back
    std::string result = response.result;
    std::cout << "    Complete wildcard result length: " << result.length() << " characters\n";
    
    // Should contain references to all our test subjects
    std::vector<std::string> expected_subjects = {"granite", "quadplay", "b3-p12", "euclid"};
    for (const auto& subject : expected_subjects) {
        AXIOM(result.find(subject) != std::string::npos, 
              "Complete wildcard should contain subject: " + subject);
    }
    
    // CRITICAL TEST 2: Subject wildcard patterns
    TEST_SECTION("CRITICAL: Testing subject-specific queries");
    
    // Find all triples with granite as subject
    std::string findGraniteCmd = "(find-triple :dbid " + dbid + " :subject \"granite\" :predicate nil :object nil)";
    response = dispatcher.executeCommand(findGraniteCmd);
    validateResponse(response, LabDb::Db9Response::Success, "Subject wildcard should succeed");
    
    result = response.result;
    AXIOM(result.find("granite") != std::string::npos, "Should find granite triples");
    AXIOM(result.find("contains") != std::string::npos, "Should find granite contains relationships");
    AXIOM(result.find("quartz") != std::string::npos, "Should find granite-quartz relationship");
    std::cout << "    Granite triples found: " << result.substr(0, 100) << "...\n";
    
    // Find all triples with quadplay as subject  
    std::string findQuadplayCmd = "(find-triple :dbid " + dbid + " :subject \"quadplay\" :predicate nil :object nil)";
    response = dispatcher.executeCommand(findQuadplayCmd);
    validateResponse(response, LabDb::Db9Response::Success, "Quadplay subject wildcard should succeed");
    
    result = response.result;
    AXIOM(result.find("quadplay") != std::string::npos, "Should find quadplay triples");
    AXIOM(result.find("has_function") != std::string::npos, "Should find has_function relationships");
    std::cout << "    Quadplay triples found: " << result.substr(0, 100) << "...\n";
    
    // CRITICAL TEST 3: Predicate wildcard patterns  
    TEST_SECTION("CRITICAL: Testing predicate-specific queries");
    
    // Find all 'contains' relationships
    std::string findContainsCmd = "(find-triple :dbid " + dbid + " :subject nil :predicate \"contains\" :object nil)";
    response = dispatcher.executeCommand(findContainsCmd);
    validateResponse(response, LabDb::Db9Response::Success, "Predicate wildcard should succeed");
    
    result = response.result;
    AXIOM(result.find("contains") != std::string::npos, "Should find contains relationships");
    AXIOM(result.find("granite") != std::string::npos, "Should find granite in contains relationships");
    std::cout << "    Contains relationships found: " << result.substr(0, 100) << "...\n";
    
    // Find all 'translation_of' relationships (scholarly)
    std::string findTranslationCmd = "(find-triple :dbid " + dbid + " :subject nil :predicate \"translation_of\" :object nil)";
    response = dispatcher.executeCommand(findTranslationCmd);
    validateResponse(response, LabDb::Db9Response::Success, "Translation predicate wildcard should succeed");
    
    result = response.result; 
    AXIOM(result.find("translation_of") != std::string::npos, "Should find translation relationships");
    AXIOM(result.find("b3-p12") != std::string::npos, "Should find proposition references");
    std::cout << "    Translation relationships found: " << result.substr(0, 100) << "...\n";
    
    // CRITICAL TEST 4: Object wildcard patterns
    TEST_SECTION("CRITICAL: Testing object-specific queries");
    
    // Find all triples with 'b3-p12' as object
    std::string findB3P12ObjectCmd = "(find-triple :dbid " + dbid + " :subject nil :predicate nil :object \"b3-p12\")";
    response = dispatcher.executeCommand(findB3P12ObjectCmd);
    validateResponse(response, LabDb::Db9Response::Success, "Object wildcard should succeed");
    
    result = response.result;
    AXIOM(result.find("b3-p12") != std::string::npos, "Should find b3-p12 as object");
    AXIOM(result.find("translation_of") != std::string::npos, "Should find translation relationships to b3-p12");
    std::cout << "    b3-p12 as object found: " << result.substr(0, 100) << "...\n";
    
    // Find all triples with hardness properties as objects
    std::string findHardnessCmd = "(find-triple :dbid " + dbid + " :subject nil :predicate nil :object \"hardness_7\")";
    response = dispatcher.executeCommand(findHardnessCmd);
    validateResponse(response, LabDb::Db9Response::Success, "Hardness object wildcard should succeed");
    
    result = response.result;
    AXIOM(result.find("hardness_7") != std::string::npos, "Should find hardness_7 as object");
    AXIOM(result.find("quartz") != std::string::npos, "Should find quartz with hardness_7");
    std::cout << "    Hardness relationships found: " << result.substr(0, 100) << "...\n";
    
    // CRITICAL TEST 5: Two-parameter wildcard combinations
    TEST_SECTION("CRITICAL: Testing two-parameter wildcard combinations");
    
    // Subject + Predicate specified, Object wildcard
    std::string findGraniteContainsCmd = "(find-triple :dbid " + dbid + " :subject \"granite\" :predicate \"contains\" :object nil)";
    response = dispatcher.executeCommand(findGraniteContainsCmd);
    validateResponse(response, LabDb::Db9Response::Success, "Subject+Predicate wildcard should succeed");
    
    result = response.result;
    AXIOM(result.find("granite") != std::string::npos, "Should find granite");
    AXIOM(result.find("contains") != std::string::npos, "Should find contains");
    AXIOM(result.find("quartz") != std::string::npos || result.find("feldspar") != std::string::npos, 
          "Should find what granite contains");
    std::cout << "    Granite contains relationships: " << result.substr(0, 100) << "...\n";
    
    // Predicate + Object specified, Subject wildcard  
    std::string findWhoHasHardnessCmd = "(find-triple :dbid " + dbid + " :subject nil :predicate \"hasProperty\" :object \"hardness_7\")";
    response = dispatcher.executeCommand(findWhoHasHardnessCmd);
    validateResponse(response, LabDb::Db9Response::Success, "Predicate+Object wildcard should succeed");
    
    result = response.result;
    AXIOM(result.find("hasProperty") != std::string::npos, "Should find hasProperty");
    AXIOM(result.find("hardness_7") != std::string::npos, "Should find hardness_7");
    AXIOM(result.find("quartz") != std::string::npos, "Should find quartz has hardness_7");
    std::cout << "    Who has hardness_7: " << result.substr(0, 100) << "...\n";
    
    // Subject + Object specified, Predicate wildcard
    std::string findGraniteQuartzRelCmd = "(find-triple :dbid " + dbid + " :subject \"granite\" :predicate nil :object \"quartz\")";
    response = dispatcher.executeCommand(findGraniteQuartzRelCmd);
    validateResponse(response, LabDb::Db9Response::Success, "Subject+Object wildcard should succeed");
    
    result = response.result;
    AXIOM(result.find("granite") != std::string::npos, "Should find granite");
    AXIOM(result.find("quartz") != std::string::npos, "Should find quartz");
    AXIOM(result.find("contains") != std::string::npos, "Should find the contains relationship");
    std::cout << "    Granite-quartz relationship: " << result.substr(0, 100) << "...\n";
    
    // CRITICAL TEST 6: Empty result wildcards (should return empty array, not error)
    TEST_SECTION("CRITICAL: Testing wildcards with no matches");
    
    std::string findNonexistentCmd = "(find-triple :dbid " + dbid + " :subject \"nonexistent\" :predicate nil :object nil)";
    response = dispatcher.executeCommand(findNonexistentCmd);
    validateResponse(response, LabDb::Db9Response::Success, "Wildcard with no matches should succeed");
    
    result = response.result;
    AXIOM(result == "[]", "Wildcard with no matches should return empty array, got: " + result);
    std::cout << "    Nonexistent subject correctly returned: " << result << "\n";
    
    // CRITICAL TEST 7: Error cases for wildcards
    TEST_SECTION("CRITICAL: Testing wildcard error cases");
    
    // Invalid database ID
    std::string invalidDbCmd = "(find-triple :dbid \"invalid_999\" :subject nil :predicate nil :object nil)";
    response = dispatcher.executeCommand(invalidDbCmd);
    validateResponse(response, LabDb::Db9Response::Error, "Invalid DBID should fail");
    std::cout << "    ✓ Invalid DBID correctly rejected\n";
    
    // Close test database
    std::string closeCmd = "(close-database :dbid " + dbid + ")";
    auto closeResponse = dispatcher.executeCommand(closeCmd);
    if (closeResponse.status == LabDb::Db9Response::Success) {
        std::cout << "    Closed wildcard test database: " << dbid << "\n";
    }
    
    TEST_SUCCESS("Find Triple Wildcard Patterns - Critical Missing Functionality");
}

void test_wildcard_performance_characteristics() {
    TEST_START("Wildcard Performance Characteristics");
    
    std::string dbid = createFreshDatabase("performance");
    auto& dispatcher = LabDb::getGlobalDb9Dispatcher();
    std::cout << "    Using performance test database ID: " << dbid << "\n";
    
    TEST_SECTION("Creating large dataset for performance testing");
    
    // Create a larger dataset to test performance
    std::vector<std::tuple<std::string, std::string, std::string>> large_dataset;
    
    // Generate systematic triples for performance testing
    for (int i = 1; i <= 50; ++i) {
        large_dataset.emplace_back("entity_" + std::to_string(i), "hasProperty", "property_" + std::to_string(i % 10));
        large_dataset.emplace_back("entity_" + std::to_string(i), "relatesTo", "entity_" + std::to_string((i % 30) + 1));
        large_dataset.emplace_back("group_" + std::to_string(i % 5), "contains", "entity_" + std::to_string(i));
    }
    
    // Add all performance test triples
    for (const auto& [subject, predicate, object] : large_dataset) {
        std::string cmd = "(add-triple :dbid " + dbid + 
                         " :subject \"" + subject + 
                         "\" :predicate \"" + predicate + 
                         "\" :object \"" + object + "\")";
        auto response = dispatcher.executeCommand(cmd);
        validateResponse(response, LabDb::Db9Response::Success, "Adding performance test triple");
    }
    
    std::cout << "    Added " << large_dataset.size() << " triples for performance testing\n";
    
    TEST_SECTION("Testing wildcard performance with large dataset");
    
    // Test complete wildcard performance
    auto start = std::chrono::high_resolution_clock::now();
    std::string findAllCmd = "(find-triple :dbid " + dbid + " :subject nil :predicate nil :object nil)";
    auto response = dispatcher.executeCommand(findAllCmd);
    auto end = std::chrono::high_resolution_clock::now();
    
    validateResponse(response, LabDb::Db9Response::Success, "Large dataset complete wildcard");
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "    Complete wildcard query took: " << duration.count() << " microseconds\n";
    std::cout << "    Auto-reflexive timing: " << response.auto_reflexive.operation_time_ms.count() << " ms\n";
    std::cout << "    Items processed: " << response.auto_reflexive.items_processed << "\n";
    
    // Test predicate-specific performance  
    start = std::chrono::high_resolution_clock::now();
    std::string findContainsCmd = "(find-triple :dbid " + dbid + " :subject nil :predicate \"contains\" :object nil)";
    response = dispatcher.executeCommand(findContainsCmd);
    end = std::chrono::high_resolution_clock::now();
    
    validateResponse(response, LabDb::Db9Response::Success, "Large dataset predicate wildcard");
    
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "    Predicate wildcard query took: " << duration.count() << " microseconds\n";
    std::cout << "    Auto-reflexive timing: " << response.auto_reflexive.operation_time_ms.count() << " ms\n";
    
    // Close performance test database
    std::string closeCmd = "(close-database :dbid " + dbid + ")";
    auto closeResponse = dispatcher.executeCommand(closeCmd);
    if (closeResponse.status == LabDb::Db9Response::Success) {
        std::cout << "    Closed performance test database: " << dbid << "\n";
    }
    
    TEST_SUCCESS("Wildcard Performance Characteristics");
}

int main() {
    std::cout << "🚀 LabDb Find-Triple Wildcard Testing\n";
    std::cout << "=====================================\n";
    std::cout << "CRITICAL: Testing find-triple wildcard functionality that was missing from main test suite\n\n";
    
    try {
        // Initialize verb registration
        // (Verbs auto-register via getGlobalDb9Dispatcher)
        
        // Run wildcard tests
        test_find_triple_wildcard_patterns();
        test_wildcard_performance_characteristics();
        
        // Summary
        std::cout << "\n🎉 ALL WILDCARD TESTS PASSED!\n";
        std::cout << "=====================================\n";
        std::cout << "✅ Complete wildcard patterns (:subject nil :predicate nil :object nil)\n";
        std::cout << "✅ Subject-specific wildcard queries\n";
        std::cout << "✅ Predicate-specific wildcard queries\n";
        std::cout << "✅ Object-specific wildcard queries\n";
        std::cout << "✅ Two-parameter wildcard combinations\n";
        std::cout << "✅ Empty result handling\n";
        std::cout << "✅ Error case validation\n";
        std::cout << "✅ Performance characteristics with large datasets\n";
        std::cout << "\n🎉 CRITICAL FIND-TRIPLE WILDCARD TESTING COMPLETE!\n";
        std::cout << "This addresses the critical gap identified in our triadic consciousness navigation.\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n💥 Wildcard test failed with exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "\n💥 Wildcard test failed with unknown exception\n";
        return 1;
    }
}
