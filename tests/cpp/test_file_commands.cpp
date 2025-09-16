#include "LabDb/Db9Dispatcher.h"
#include "LabDb/DatabaseVerbs.h"  // CRITICAL: Needed for database verb auto-registration
#include <iostream>
#include <filesystem>

int main() {
    std::cout << "🧪 Testing File-Based executeCommands with DBID Substitution\n";
    std::cout << "============================================================\n\n";
    
    // Use global dispatcher which has auto-registered verbs
    LabDb::Db9Dispatcher& dispatcher = LabDb::getGlobalDb9Dispatcher();
    
    // Test file path - correct for CTest working directory (build/tests/)
    std::string testFile = "../../tests/cpp/testenv/test_commands.sexpr";
    
    // Verify test file exists
    if (!std::filesystem::exists(testFile)) {
        std::cerr << "❌ Test file not found: " << testFile << "\n";
        return 1;
    }
    
    std::cout << "📂 Test file: " << testFile << "\n";
    
    // Clean up any existing test database files for test isolation
    std::filesystem::remove("/tmp/test_dbid.db9");
    std::filesystem::remove("/tmp/test_file_commands.db9");
    
    // First, create and open a database to get a real dbid
    std::cout << "🔧 Step 1: Creating test database...\n";
    auto createResponse = dispatcher.executeCommand("(create-database :path \"/tmp/test_dbid.db9\")");
    
    if (createResponse.status != LabDb::Db9Response::Success) {
        std::cerr << "❌ Failed to create database: " << createResponse.error_message << "\n";
        return 1;
    }
    
    auto openResponse = dispatcher.executeCommand("(open-database :path \"/tmp/test_dbid.db9\")");
    if (openResponse.status != LabDb::Db9Response::Success) {
        std::cerr << "❌ Failed to open database: " << openResponse.error_message << "\n";
        return 1;
    }
    
    // Extract dbid from response
    std::string dbid;
    auto dbidStart = openResponse.result.find("\"dbid\": \"");
    if (dbidStart != std::string::npos) {
        dbidStart += 10;
        auto dbidEnd = openResponse.result.find("\"", dbidStart);
        if (dbidEnd != std::string::npos) {
            dbid = openResponse.result.substr(dbidStart, dbidEnd - dbidStart);
        }
    }
    
    if (dbid.empty()) {
        std::cerr << "❌ Could not extract dbid from response\n";
        return 1;
    }
    
    std::cout << "✅ Database created with dbid: " << dbid << "\n\n";
    
    // Test the new file-based executeCommands method
    std::cout << "🚀 Step 2: Testing file-based executeCommands...\n";
    auto fileResponse = dispatcher.executeCommands(testFile, dbid);
    
    std::cout << "📊 Response Status: " << static_cast<int>(fileResponse.status) << "\n";
    
    if (fileResponse.status == LabDb::Db9Response::Error) {
        std::cout << "❌ Error Code: " << fileResponse.error_code << "\n";
        std::cout << "❌ Error Message: " << fileResponse.error_message << "\n";
        return 1;
    }
    
    std::cout << "📝 Response Preview (first 500 chars):\n";
    std::string preview = fileResponse.result.substr(0, 500);
    std::cout << preview;
    if (fileResponse.result.length() > 500) {
        std::cout << "...\n";
    } else {
        std::cout << "\n";
    }
    
    std::cout << "\n✅ File-based command execution completed successfully!\n";
    std::cout << "📈 Auto-reflexive metrics:\n";
    std::cout << "   - Operation time: " << fileResponse.auto_reflexive.operation_time_ms.count() << "ms\n";
    std::cout << "   - Items processed: " << fileResponse.auto_reflexive.items_processed << "\n";
    std::cout << "   - Memory usage: " << fileResponse.auto_reflexive.memory_usage_kb << "KB\n";
    
    // Clean up
    dispatcher.executeCommand("(close-database :dbid " + dbid + ")");
    
    std::cout << "\n🎯 Test completed successfully! New bulk loading capability validated.\n";
    
    return 0;
}
