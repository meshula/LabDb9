#include "test_harness.h"
#include "LabDb/Db9Dispatcher.h"
#include "LabDb/DatabaseVerbs.h"
#include <filesystem>

using namespace labdb9_test;
using namespace LabDb;

// Minimal test to debug AddTripleSemanticVerb only
TEST(debug_add_triple_semantic_verb) {
    std::cout << "🔬 DEBUG: Isolating AddTripleSemanticVerb" << std::endl;
    
    // Setup
    const std::string test_dir = "/tmp/debug_semantic";  
    const std::string test_db = "debug_semantic.db9";
    std::filesystem::create_directories(test_dir);
    std::filesystem::remove(test_dir + "/" + test_db);
    
    // Get dispatcher
    auto& dispatcher = LabDb::getGlobalDb9Dispatcher();
    
    // Create database
    std::string create_cmd = "(create-database :path §" + test_dir + "/" + test_db + "§)";
    std::cout << "📚 Create command: " << create_cmd << std::endl;
    
    auto create_response = dispatcher.executeCommand(create_cmd);
    std::cout << "📊 Create status: " << create_response.status << std::endl;
    std::cout << "📋 Create result: '" << create_response.result << "'" << std::endl;
    
    EXPECT_EQ(Db9Response::Success, create_response.status);
    
    // Extract dbid
    std::string dbid = "db1";  // Default
    if (create_response.result.find("\"dbid\"") != std::string::npos) {
        size_t pos = create_response.result.find("\"dbid\": \"") + 9;
        size_t end = create_response.result.find("\"", pos);
        dbid = create_response.result.substr(pos, end - pos);
    }
    std::cout << "🔑 Using dbid: '" << dbid << "'" << std::endl;
    
    // Test AddTripleSemanticVerb with simple values
    std::string triple_cmd = "(add-triple-semantic :subject §test-subject§ :predicate §test-predicate§ :object §test-object§ :dbid §" + dbid + "§)";
    std::cout << "🔗 Triple command: " << triple_cmd << std::endl;
    
    auto triple_response = dispatcher.executeCommand(triple_cmd);
    std::cout << "📊 Triple status: " << triple_response.status << std::endl;
    std::cout << "📋 Triple result: '" << triple_response.result << "'" << std::endl;
    std::cout << "❌ Error code: '" << triple_response.error_code << "'" << std::endl;  
    std::cout << "📝 Error message: '" << triple_response.error_message << "'" << std::endl;
    
    // This will help us see exactly what's failing
    if (triple_response.status != Db9Response::Success) {
        std::cout << "🚨 FAILED: AddTripleSemanticVerb returned error status!" << std::endl;
        std::cout << "   Status: " << static_cast<int>(triple_response.status) << std::endl;
        std::cout << "   Error Code: '" << triple_response.error_code << "'" << std::endl;
        std::cout << "   Error Message: '" << triple_response.error_message << "'" << std::endl;
    } else {
        std::cout << "✅ SUCCESS: AddTripleSemanticVerb worked!" << std::endl;
    }
    
    // Close database
    std::string close_cmd = "(close-database :dbid §" + dbid + "§)";
    auto close_response = dispatcher.executeCommand(close_cmd);
    std::cout << "📊 Close response: '" << close_response.result << "'" << std::endl;
}

LABDB9_TEST_MAIN()
