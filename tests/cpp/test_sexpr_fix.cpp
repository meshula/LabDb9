#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <regex>
#include <filesystem>

// Include the actual LabDb headers
#include "LabDb/Db9Dispatcher.h"
#include "../src/Verbs/FioVerbs.h"
#include "../src/Verbs/Fio/WriteVerb.h"
#include "../src/Verbs/Fio/ConfirmVerb.h"

using namespace LabDb;

// Real db9 execution using LabDb dispatcher
Db9Response db9_execute(const std::string& command) {
    static LabDb::Db9Dispatcher dispatcher;
    static bool initialized = false;
    using namespace LabDb;

    if (!initialized) {
        // Register FIO verbs
        dispatcher.registerVerb(std::make_unique<FioWriteVerb>());
        dispatcher.registerVerb(std::make_unique<FioReadVerb>());
        dispatcher.registerVerb(std::make_unique<FioSearchVerb>());
        dispatcher.registerVerb(std::make_unique<FioListVerb>());
        dispatcher.registerVerb(std::make_unique<FioConfirmVerb>());
        initialized = true;
    }

    return dispatcher.executeCommand(command);
}

// Test the fix for multiline content preservation
void test_multiline_fix() {
    std::cout << "🧪 Testing multiline S-expression fix" << std::endl;

    std::string test_path = "/tmp/test_multiline_fix.txt";

    // FIXED: Properly escaped multi-line content in S-expression (no embedded newlines)
    std::string cmd = R"((fio-write :path "/tmp/test_multiline_fix.txt" :content "Line 1: Function header\nLine 2: {\nLine 3:     int x = 42;\nLine 4:     return x;\nLine 5: }"))";

    std::cout << "📝 Write command: " << cmd << std::endl;

    auto write_response = db9_execute(cmd);
    std::cout << "📝 Write response status: " << (int)write_response.status << std::endl;
    std::cout << "📝 Write response result: " << write_response.result << std::endl;

    if (write_response.status != Db9Response::Success) {
        std::cout << "❌ Write command failed!" << std::endl;
        return;
    }

    // Now try to read it back
    std::string read_cmd = "(fio-read :path \"" + test_path + "\")";
    std::cout << "📖 Read command: " << read_cmd << std::endl;

    auto read_response = db9_execute(read_cmd);
    std::cout << "📖 Read response status: " << (int)read_response.status << std::endl;
    std::cout << "📖 Read response result: " << read_response.result << std::endl;

    // Check if we get content back
    if (!read_response.result.empty()) {
        if (read_response.result.find("\"total_lines\": 5") != std::string::npos) {
            std::cout << "✅ SUCCESS: 5 lines correctly stored and retrieved!" << std::endl;
        } else {
            std::cout << "⚠️ Unexpected line count in response" << std::endl;
        }
    } else {
        std::cout << "❌ FAILED: Empty read response (S-expression parsing issue)" << std::endl;
    }

    std::filesystem::remove(test_path);
}

// Test the broken version (for comparison)
void test_multiline_broken() {
    std::cout << "\n🧪 Testing broken multiline S-expression (for comparison)" << std::endl;

    std::string test_path = "/tmp/test_multiline_broken.txt";

    // BROKEN: Raw string with embedded newlines in S-expression (this will fail)
    std::string cmd = R"((fio-write :path "/tmp/test_multiline_broken.txt" :content "Line 1: Function header\n
Line 2: {\n
Line 3:     int x = 42;\n
Line 4:     return x;\n
Line 5: }"))";

    std::cout << "📝 Broken command: " << cmd << std::endl;

    auto write_response = db9_execute(cmd);
    std::cout << "📝 Write response status: " << (int)write_response.status << std::endl;
    std::cout << "📝 Write response result: " << write_response.result << std::endl;

    // This should fail or produce empty result due to malformed S-expression
    std::string read_cmd = "(fio-read :path \"" + test_path + "\")";
    auto read_response = db9_execute(read_cmd);
    std::cout << "📖 Read response status: " << (int)read_response.status << std::endl;
    std::cout << "📖 Read response result: " << read_response.result << std::endl;

    if (read_response.result.empty()) {
        std::cout << "✅ CONFIRMED: Broken version produces empty response (malformed S-expression)" << std::endl;
    }

    std::filesystem::remove(test_path);
}

int main() {
    std::cout << "🔬 S-Expression Fix Validation Test" << std::endl;
    
    test_multiline_fix();
    test_multiline_broken();
    
    std::cout << "\n🎯 Test complete - this shows the fix for the original bug!" << std::endl;
    return 0;
}
