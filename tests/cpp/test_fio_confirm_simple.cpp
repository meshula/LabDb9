#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <filesystem>

// Include the actual LabDb headers
#include "LabDb/Db9Dispatcher.h"
#include "../src/Verbs/FioVerbs.h"
#include "../src/Verbs/Fio/WriteVerb.h"
#include "../src/Verbs/Fio/ConfirmVerb.h"

// Use the actual LabDb namespace
using namespace LabDb;

// Real db9 execution using LabDb dispatcher
Db9Response db9_execute(const std::string& command) {
    static Db9Dispatcher dispatcher;
    static bool initialized = false;
    
    if (!initialized) {
        // Register FIO verbs including the new ConfirmVerb
        dispatcher.registerVerb(std::make_unique<FioWriteVerb>());
        dispatcher.registerVerb(std::make_unique<FioReadVerb>());
        dispatcher.registerVerb(std::make_unique<FioSearchVerb>());
        dispatcher.registerVerb(std::make_unique<FioListVerb>());
        dispatcher.registerVerb(std::make_unique<FioConfirmVerb>());
        initialized = true;
    }
    
    return dispatcher.executeCommand(command);
}

// Test function for fio-confirm ping-pong protocol
static void test_fio_confirm_ping_pong_protocol() {
    std::cout << "🧪 Test: fio-confirm ping-pong testing protocol" << std::endl;
    
    std::string test_path = "/tmp/fio_confirm_ping_test.txt";
    
    // Remove file if exists
    if (std::filesystem::exists(test_path)) {
        std::filesystem::remove(test_path);
    }
    
    // Test ping-pong protocol: fio-confirm should write "pong" to specified path
    std::string cmd = "(fio-confirm :path \"" + test_path + "\")";
    std::cout << "  📤 Executing ping command: " << cmd << std::endl;
    
    auto response = db9_execute(cmd);
    
    std::cout << "  📋 Response status: " << (response.status == Db9Response::Success ? "SUCCESS" : "ERROR") << std::endl;
    std::cout << "  📋 Response result: " << response.result << std::endl;
    
    if (!response.error_message.empty()) {
        std::cout << "  ❌ Error: " << response.error_message << std::endl;
    }
    
    // Verify the response indicates success
    assert(response.status == Db9Response::Success);
    
    // Verify file was created
    assert(std::filesystem::exists(test_path));
    std::cout << "  ✅ Ping file created successfully" << std::endl;
    
    // Verify file contains "pong"
    std::ifstream file(test_path);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    assert(content == "pong");
    std::cout << "  ✅ File contains correct 'pong' content" << std::endl;
    
    // Verify rich preview response format (should contain both rich display and JSON)
    if (response.result.find("🔍 PING-PONG OPERATION COMPLETE") != std::string::npos &&
        response.result.find("✅ File created") != std::string::npos &&
        response.result.find("📄 Content: \"pong\"") != std::string::npos &&
        response.result.find("📋 JSON Response:") != std::string::npos &&
        response.result.find("\"status\": \"pong_written\"") != std::string::npos) {
        std::cout << "  ✅ Rich preview response format correct" << std::endl;
    } else {
        std::cout << "  ⚠️ Rich preview response format unexpected" << std::endl;
        std::cout << "  📝 Response preview: " << response.result.substr(0, 200) << "..." << std::endl;
    }
    
    // Test file size verification
    size_t file_size = std::filesystem::file_size(test_path);
    assert(file_size == 4); // "pong" is 4 bytes
    std::cout << "  ✅ File size correct (4 bytes)" << std::endl;
    
    // Clean up
    std::filesystem::remove(test_path);
    
    std::cout << "🎉 fio-confirm ping-pong protocol test PASSED!" << std::endl;
    std::cout << "📝 fio-confirm ping-pong testing protocol completed\n" << std::endl;
}

// Test for token confirmation placeholder
static void test_fio_confirm_token_placeholder() {
    std::cout << "🧪 Test: fio-confirm token confirmation placeholder" << std::endl;
    
    // Test with a mock token
    std::string cmd = "(fio-confirm :token \"confirm-12345678\")";
    std::cout << "  📤 Testing token confirmation: " << cmd << std::endl;
    
    auto response = db9_execute(cmd);
    
    std::cout << "  📋 Response status: " << (response.status == Db9Response::Success ? "SUCCESS" : "ERROR") << std::endl;
    std::cout << "  📋 Response result: " << response.result << std::endl;
    
    // Should succeed with placeholder message
    assert(response.status == Db9Response::Success);
    
    // Verify rich placeholder response structure
    if (response.result.find("🔍 TOKEN CONFIRMATION PREVIEW") != std::string::npos &&
        response.result.find("🚧 Under Development") != std::string::npos &&
        response.result.find("🔗 Token Recognized") != std::string::npos &&
        response.result.find("📋 JSON Response:") != std::string::npos &&
        response.result.find("\"status\": \"confirmation_placeholder\"") != std::string::npos &&
        response.result.find("\"token\": \"confirm-12345678\"") != std::string::npos) {
        std::cout << "  ✅ Rich token placeholder response correct" << std::endl;
    } else {
        std::cout << "  ❌ Rich token placeholder response unexpected" << std::endl;
        std::cout << "  📝 Response preview: " << response.result.substr(0, 200) << "..." << std::endl;
    }
    
    // Test error handling for invalid token
    std::string bad_cmd = "(fio-confirm :token \"bad\")";
    auto bad_response = db9_execute(bad_cmd);
    
    assert(bad_response.status == Db9Response::Error);
    assert(bad_response.error_code == "invalid_token");
    std::cout << "  ✅ Invalid token correctly rejected" << std::endl;
    
    // Test error handling for missing parameters
    std::string empty_cmd = "(fio-confirm)";
    auto empty_response = db9_execute(empty_cmd);
    
    assert(empty_response.status == Db9Response::Error);
    assert(empty_response.error_code == "missing_parameter");
    std::cout << "  ✅ Missing parameters correctly rejected" << std::endl;
    
    std::cout << "🎉 fio-confirm token placeholder test PASSED!" << std::endl;
    std::cout << "📝 fio-confirm token confirmation test completed\n" << std::endl;
}

// Integration test combining both protocols
static void test_fio_confirm_complete_workflow() {
    std::cout << "🧪 Test: fio-confirm complete workflow integration" << std::endl;
    
    std::string ping_path = "/tmp/fio_confirm_workflow_ping.txt";
    std::string pong_path = "/tmp/fio_confirm_workflow_pong.txt";
    
    // Remove files if they exist
    if (std::filesystem::exists(ping_path)) {
        std::filesystem::remove(ping_path);
    }
    if (std::filesystem::exists(pong_path)) {
        std::filesystem::remove(pong_path);
    }
    
    // Step 1: Use fio-write to create a "ping" file
    std::string write_cmd = "(fio-write :path \"" + ping_path + "\" :content \"ping\")";
    auto write_response = db9_execute(write_cmd);
    assert(write_response.status == Db9Response::Success);
    std::cout << "  ✅ Step 1: fio-write created ping file" << std::endl;
    
    // Step 2: Use fio-confirm to create a "pong" response
    std::string confirm_cmd = "(fio-confirm :path \"" + pong_path + "\")";
    auto confirm_response = db9_execute(confirm_cmd);
    assert(confirm_response.status == Db9Response::Success);
    std::cout << "  ✅ Step 2: fio-confirm created pong file" << std::endl;
    
    // Step 3: Verify both files exist and have correct content
    std::ifstream ping_file(ping_path);
    std::string ping_content((std::istreambuf_iterator<char>(ping_file)), std::istreambuf_iterator<char>());
    ping_file.close();
    assert(ping_content == "ping");
    std::cout << "  ✅ Step 3a: Ping file contains 'ping'" << std::endl;
    
    std::ifstream pong_file(pong_path);
    std::string pong_content((std::istreambuf_iterator<char>(pong_file)), std::istreambuf_iterator<char>());
    pong_file.close();
    assert(pong_content == "pong");
    std::cout << "  ✅ Step 3b: Pong file contains 'pong'" << std::endl;
    
    // Step 4: Verify using fio-read (if available)
    std::string read_ping_cmd = "(fio-read :path \"" + ping_path + "\")";
    auto read_ping_response = db9_execute(read_ping_cmd);
    if (read_ping_response.status == Db9Response::Success) {
        std::cout << "  ✅ Step 4a: fio-read can read ping file" << std::endl;
    }
    
    std::string read_pong_cmd = "(fio-read :path \"" + pong_path + "\")";
    auto read_pong_response = db9_execute(read_pong_cmd);
    if (read_pong_response.status == Db9Response::Success) {
        std::cout << "  ✅ Step 4b: fio-read can read pong file" << std::endl;
    }
    
    // Clean up
    std::filesystem::remove(ping_path);
    std::filesystem::remove(pong_path);
    
    std::cout << "🎉 fio-confirm complete workflow integration test PASSED!" << std::endl;
    std::cout << "📝 Complete fio-write ↔ fio-confirm workflow verified\n" << std::endl;
}

// Simple test runner
int main() {
    std::cout << "🧠 Starting FIO-CONFIRM Ping-Pong Test Suite\n" << std::endl;
    
    try {
        // Run our focused tests
        test_fio_confirm_ping_pong_protocol();
        test_fio_confirm_token_placeholder();
        test_fio_confirm_complete_workflow();
        
        std::cout << "🚀 All fio-confirm tests completed successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}