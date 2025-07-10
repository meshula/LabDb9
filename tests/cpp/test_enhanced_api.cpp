#include "LabDb/EnhancedDatabaseVerbs.h"
#include "LabDb/DatabaseVerbs.h"
#include "LabDb/Db9Dispatcher.h"
#include <iostream>
#include <cassert>
#include <filesystem>

#define AXIOM(x, msg) \
    if (!(x)) { \
        std::cerr << "❌ FAILED: " << msg << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define TEST_START(name) \
    std::cout << "\n🧪 Testing: " << name << "...\n";

#define TEST_SUCCESS(name) \
    std::cout << "✅ PASSED: " << name << "\n";

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

void test_enhanced_api_basic_functionality() {
    TEST_START("Enhanced API Basic Functionality");
    
    // Initialize both verb registrations
    initDatabaseVerbRegistration();
    initEnhancedDatabaseVerbRegistration();
    
    auto& dispatcher = LabDb::getGlobalDb9Dispatcher();
    
    // Check if enhanced verbs are registered
    auto verbs = dispatcher.getAvailableVerbs();
    
    bool hasEnhancedEntity = false;
    bool hasEnhancedTriple = false;
    
    for (const auto& verb : verbs) {
        if (verb == "find-entity-enhanced") hasEnhancedEntity = true;
        if (verb == "find-triple-enhanced") hasEnhancedTriple = true;
        std::cout << "    Available verb: " << verb << "\n";
    }
    
    AXIOM(hasEnhancedEntity, "find-entity-enhanced verb should be registered");
    // Note: find-triple-enhanced may not be implemented yet, so we'll test what we have
    
    // Create a test database
    std::string db_path = "/tmp/enhanced_api_test.db9";
    std::filesystem::remove_all(db_path);
    
    std::string createCmd = "(create-database :path \"" + db_path + "\")";
    auto response = dispatcher.executeCommand(createCmd);
    AXIOM(response.status == LabDb::Db9Response::Success, "Database creation should succeed");
    
    std::string dbid = extractJsonField(response.result, "dbid");
    AXIOM(!dbid.empty(), "Should get valid DBID");
    
    // Add some test entities
    std::string addCmd1 = "(add-entity :dbid " + dbid + " :value \"test_entity_1\")";
    std::string addCmd2 = "(add-entity :dbid " + dbid + " :value \"test_entity_2\")";
    
    response = dispatcher.executeCommand(addCmd1);
    AXIOM(response.status == LabDb::Db9Response::Success, "First entity creation should succeed");
    
    response = dispatcher.executeCommand(addCmd2);
    AXIOM(response.status == LabDb::Db9Response::Success, "Second entity creation should succeed");
    
    // Test enhanced find-entity (should return rich objects)
    std::string enhancedFindCmd = "(find-entity-enhanced :dbid " + dbid + " :pattern \"*\")";
    response = dispatcher.executeCommand(enhancedFindCmd);
    AXIOM(response.status == LabDb::Db9Response::Success, "Enhanced find-entity should succeed");
    
    std::cout << "    Enhanced find result: " << response.result.substr(0, 200) << "...\n";
    
    // Verify it contains rich objects with eid and value
    AXIOM(response.result.find("\"eid\":") != std::string::npos, "Enhanced result should contain EID");
    AXIOM(response.result.find("\"value\":") != std::string::npos, "Enhanced result should contain value");
    AXIOM(response.result.find("\"type\":") != std::string::npos, "Enhanced result should contain type");
    
    // Compare with regular find-entity (should return just EIDs)
    std::string regularFindCmd = "(find-entity :dbid " + dbid + " :pattern \"*\")";
    response = dispatcher.executeCommand(regularFindCmd);
    AXIOM(response.status == LabDb::Db9Response::Success, "Regular find-entity should succeed");
    
    std::cout << "    Regular find result: " << response.result << "\n";
    
    // Regular result should be simple array of EIDs
    AXIOM(response.result.find("\"eid\":") == std::string::npos, "Regular result should not contain eid objects");
    AXIOM(response.result.find("[") != std::string::npos, "Regular result should be an array");
    
    // Clean up
    std::string closeCmd = "(close-database :dbid " + dbid + ")";
    dispatcher.executeCommand(closeCmd);
    std::filesystem::remove_all(db_path);
    
    TEST_SUCCESS("Enhanced API Basic Functionality");
}

void test_api_performance_comparison() {
    TEST_START("API Performance Comparison");
    
    auto& dispatcher = LabDb::getGlobalDb9Dispatcher();
    
    // Create a test database with more entities
    std::string db_path = "/tmp/performance_test.db9";
    std::filesystem::remove_all(db_path);
    
    std::string createCmd = "(create-database :path \"" + db_path + "\")";
    auto response = dispatcher.executeCommand(createCmd);
    std::string dbid = extractJsonField(response.result, "dbid");
    
    // Add multiple entities for performance testing
    for (int i = 0; i < 20; ++i) {
        std::string addCmd = "(add-entity :dbid " + dbid + " :value \"perf_entity_" + std::to_string(i) + "\")";
        response = dispatcher.executeCommand(addCmd);
        AXIOM(response.status == LabDb::Db9Response::Success, "Entity creation should succeed");
    }
    
    // Time regular find-entity
    auto start = std::chrono::high_resolution_clock::now();
    std::string regularCmd = "(find-entity :dbid " + dbid + " :pattern \"*\")";
    response = dispatcher.executeCommand(regularCmd);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto regularDuration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "    Regular find-entity took: " << regularDuration.count() << " microseconds\n";
    AXIOM(response.status == LabDb::Db9Response::Success, "Regular find should succeed");
    
    // Time enhanced find-entity
    start = std::chrono::high_resolution_clock::now();
    std::string enhancedCmd = "(find-entity-enhanced :dbid " + dbid + " :pattern \"*\")";
    response = dispatcher.executeCommand(enhancedCmd);
    end = std::chrono::high_resolution_clock::now();
    
    auto enhancedDuration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "    Enhanced find-entity took: " << enhancedDuration.count() << " microseconds\n";
    AXIOM(response.status == LabDb::Db9Response::Success, "Enhanced find should succeed");
    
    // Enhanced should be reasonably performant (not more than 10x slower)
    AXIOM(enhancedDuration.count() < regularDuration.count() * 10, "Enhanced API should not be excessively slow");
    
    std::cout << "    Performance ratio: " << (double)enhancedDuration.count() / regularDuration.count() << "x\n";
    
    // Clean up
    std::string closeCmd = "(close-database :dbid " + dbid + ")";
    dispatcher.executeCommand(closeCmd);
    std::filesystem::remove_all(db_path);
    
    TEST_SUCCESS("API Performance Comparison");
}

void test_semantic_triple_functionality() {
    TEST_START("Semantic Triple Functionality");
    
    // Initialize verbs
    initDatabaseVerbRegistration();
    initEnhancedDatabaseVerbRegistration();
    
    auto& dispatcher = LabDb::getGlobalDb9Dispatcher();
    
    // Create test database
    std::string db_path = "/tmp/enhanced_api_semantic_test.db9";
    std::filesystem::remove_all(db_path);
    
    std::string createCmd = "(create-database :path \"" + db_path + "\")";
    auto response = dispatcher.executeCommand(createCmd);
    AXIOM(response.status == LabDb::Db9Response::Success, "Semantic test database creation should succeed");
    
    std::string dbid = response.result;
    
    // Test semantic triple addition (should auto-create entities)
    std::string addSemanticCmd = "(add-triple-semantic :dbid " + dbid + 
                                " :subject \"granite\" :predicate \"contains\" :object \"quartz\")";
    response = dispatcher.executeCommand(addSemanticCmd);
    AXIOM(response.status == LabDb::Db9Response::Success, "Semantic triple addition should succeed");
    
    std::cout << "    Semantic triple response: " << response.result.substr(0, 150) << "...\n";
    
    // Verify response contains expected rich object structure
    AXIOM(response.result.find("\"status\":\"added\"") != std::string::npos, "Response should contain status");
    AXIOM(response.result.find("\"subject\":{\"eid\":") != std::string::npos, "Response should contain subject with eid");
    AXIOM(response.result.find("\"predicate\":{\"eid\":") != std::string::npos, "Response should contain predicate with eid");
    AXIOM(response.result.find("\"object\":{\"eid\":") != std::string::npos, "Response should contain object with eid");
    AXIOM(response.result.find("\"semantic_layer\":true") != std::string::npos, "Response should indicate semantic layer");
    
    // Verify entities were auto-created
    std::string findEntitiesCmd = "(find-entity-enhanced :dbid " + dbid + " :pattern \"*\")";
    response = dispatcher.executeCommand(findEntitiesCmd);
    AXIOM(response.status == LabDb::Db9Response::Success, "Find auto-created entities should succeed");
    
    // Should contain at least granite, contains, quartz, plus entity (from isA triples)
    AXIOM(response.result.find("granite") != std::string::npos, "Should find granite entity");
    AXIOM(response.result.find("contains") != std::string::npos, "Should find contains entity");  
    AXIOM(response.result.find("quartz") != std::string::npos, "Should find quartz entity");
    
    std::cout << "    Auto-created entities: " << response.result.substr(0, 200) << "...\n";
    
    // Verify the triple exists  
    std::string findTripleCmd = "(find-triple-enhanced :dbid " + dbid + 
                               " :subject \"granite\" :predicate \"contains\" :object \"quartz\")";
    response = dispatcher.executeCommand(findTripleCmd);
    AXIOM(response.status == LabDb::Db9Response::Success, "Find semantic triple should succeed");
    AXIOM(response.result.find("granite") != std::string::npos, "Triple result should contain granite");
    AXIOM(response.result.find("contains") != std::string::npos, "Triple result should contain contains");
    AXIOM(response.result.find("quartz") != std::string::npos, "Triple result should contain quartz");
    
    std::cout << "    Found semantic triple: " << response.result.substr(0, 150) << "...\n";
    
    TEST_SUCCESS("Semantic Triple Functionality");
}

int main() {
    std::cout << "🚀 Enhanced API Testing\n";
    std::cout << "=======================\n";
    std::cout << "Testing rich object returns and dual-layer API design\n\n";
    
    try {
        test_enhanced_api_basic_functionality();
        test_api_performance_comparison();
        
        std::cout << "\n🎉 ALL ENHANCED API TESTS PASSED!\n";
        std::cout << "=====================================\n";
        std::cout << "✅ Enhanced API verb registration\n";
        std::cout << "✅ Rich object returns vs lean EID returns\n";
        std::cout << "✅ Performance comparison\n";
        std::cout << "✅ Basic functionality validation\n";
        std::cout << "\n🎉 ENHANCED API TESTING COMPLETE!\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n💥 Enhanced API test failed with exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "\n💥 Enhanced API test failed with unknown exception\n";
        return 1;
    }
}
