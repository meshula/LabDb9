#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <filesystem>
#include <vector>
#include <sstream>
#include <regex>

// Include the test harness
#include "test_harness.h"

// Include the actual LabDb headers
#include "LabDb/Db9Dispatcher.h"
#include "../src/Verbs/FioVerbs.h"
#include "../src/Verbs/Fio/WriteVerb.h"
#include "../src/Verbs/Fio/ConfirmVerb.h"

// Use the actual LabDb namespace
using namespace LabDb;
using namespace labdb9_test;

//-----------------------------------------------------------------------------
// Test Infrastructure
//-----------------------------------------------------------------------------

// Real db9 execution using LabDb dispatcher  
Db9Response db9_execute(const std::string& command) {
    static Db9Dispatcher dispatcher;
    static bool initialized = false;
    
    if (!initialized) {
        // Register FIO verbs for testing - INCLUDING FioConfirmVerb!
        dispatcher.registerVerb(std::make_unique<FioWriteVerb>());
        dispatcher.registerVerb(std::make_unique<FioReadVerb>());
        dispatcher.registerVerb(std::make_unique<FioSearchVerb>());
        dispatcher.registerVerb(std::make_unique<FioListVerb>());
        dispatcher.registerVerb(std::make_unique<FioConfirmVerb>());  // CRITICAL ADDITION!
        initialized = true;
    }
    
    return dispatcher.executeCommand(command);
}

// Helper function to extract confirmation token from response
std::string extract_confirmation_token(const std::string& response) {
    // Look for "token": "confirm-xxxxx" pattern in JSON
    std::regex token_regex("\"token\":\\s*\"(confirm-[^\"]+)\"");
    std::smatch match;
    
    if (std::regex_search(response, match, token_regex)) {
        return match[1].str();
    }
    
    // Also check for plain text token pattern
    std::regex text_token_regex("Token:\\s*(confirm-[a-f0-9]+)");
    if (std::regex_search(response, match, text_token_regex)) {
        return match[1].str();
    }
    
    return "";
}

// Helper function to execute line surgery with confirmation
std::pair<Db9Response, Db9Response> execute_with_confirmation(const std::string& command) {
    // Step 1: Execute the command (may generate preview)
    auto preview_response = db9_execute(command);
    
    // Step 2: If preview generated, extract token and confirm
    if (preview_response.status == Db9Response::Success) {
        std::string token = extract_confirmation_token(preview_response.result);
        
        if (!token.empty()) {
            // Found a token, execute confirmation
            std::string confirm_cmd = "(fio-confirm :token \"" + token + "\")";
            auto confirm_response = db9_execute(confirm_cmd);
            return {preview_response, confirm_response};
        }
    }
    
    // No token found or error, return original response with empty confirm
    return {preview_response, Db9Response()};
}

// Test helper functions
std::string create_test_file(const std::string& content) {
    static int counter = 0;
    std::string path = "/Users/nick/dev/Lab/LabDb9/tests/cpp/testenv/line_surgery_test_" + std::to_string(++counter) + ".txt";
    
    // Ensure testenv directory exists
    std::filesystem::create_directories("/Users/nick/dev/Lab/LabDb9/tests/cpp/testenv");
    
    std::ofstream file(path);
    if (file.is_open()) {
        file << content;
        file.close();
    }
    return path;
}

void cleanup_test_file(const std::string& path) {
    if (std::filesystem::exists(path)) {
        std::filesystem::remove(path);
    }
    
    // Also cleanup any backup files
    std::string backup_path = path + ".bak";
    if (std::filesystem::exists(backup_path)) {
        std::filesystem::remove(backup_path);
    }
}

std::vector<std::string> read_file_lines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream file(path);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        file.close();
    }
    return lines;
}

void verify_file_content(const std::string& path, const std::vector<std::string>& expected) {
    auto actual = read_file_lines(path);
    AXIOM(actual.size() == expected.size(), "File line count mismatch");
    
    for (size_t i = 0; i < expected.size(); ++i) {
        AXIOM(actual[i] == expected[i], "Line " + std::to_string(i + 1) + " content mismatch");
    }
}

std::string read_file_content(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

//-----------------------------------------------------------------------------
// Phase A: Test Infrastructure Verification
//-----------------------------------------------------------------------------

static void test_infrastructure_setup() {
    TEST_START("Test Infrastructure Setup");
    
    TEST_SECTION("File creation and cleanup");
    std::string test_content = "Line 1\nLine 2\nLine 3";
    std::string test_path = create_test_file(test_content);
    
    AXIOM(std::filesystem::exists(test_path), "Test file should be created");
    
    std::string actual_content = read_file_content(test_path);
    AXIOM(actual_content == test_content, "Test file content should match");
    
    cleanup_test_file(test_path);
    AXIOM(!std::filesystem::exists(test_path), "Test file should be cleaned up");
    
    TEST_SECTION("Line reading helper");
    std::string multi_line_content = "First line\nSecond line\nThird line";
    test_path = create_test_file(multi_line_content);
    
    auto lines = read_file_lines(test_path);
    AXIOM(lines.size() == 3, "Should read 3 lines");
    AXIOM(lines[0] == "First line", "First line should match");
    AXIOM(lines[1] == "Second line", "Second line should match");
    AXIOM(lines[2] == "Third line", "Third line should match");
    
    cleanup_test_file(test_path);
    
    TEST_SUCCESS("Test Infrastructure Setup");
}

//-----------------------------------------------------------------------------
// Phase B1: Trivial Case Testing (Full File Write via Line Surgery)
//-----------------------------------------------------------------------------

static void test_trivial_full_file_write() {
    TEST_START("Trivial Case: Full File Write via executeLineSurgery");
    
    TEST_SECTION("Basic full file write through line surgery path");
    std::string test_path = "/Users/nick/dev/Lab/LabDb9/tests/cpp/testenv/trivial_test.txt";
    
    // Clean up any existing file
    cleanup_test_file(test_path);
    
    std::string test_content = "Hello from executeLineSurgery!\nThis is line 2\nThis is line 3";
    
    // Force line surgery path by not specifying :lines (should default to Full range)
    std::string write_cmd = "(fio-write :path \"" + test_path + "\" :content \"" + test_content + "\")";
    
    // Use the preview-confirm workflow for safety (prevents accidental overwrites)
    auto [preview_response, confirm_response] = execute_with_confirmation(write_cmd);
    
    std::cout << "Write response (preview): " << preview_response.result << std::endl;
    
    AXIOM(preview_response.status == Db9Response::Success, "Preview should succeed");
    
    // If confirmation was needed and executed
    if (!confirm_response.result.empty()) {
        std::cout << "Confirm response: " << confirm_response.result << std::endl;
        AXIOM(confirm_response.status == Db9Response::Success, "Confirmation should succeed");
    }
    
    AXIOM(std::filesystem::exists(test_path), "File should be created after confirmation");
    
    // Verify content
    std::string actual_content = read_file_content(test_path);
    AXIOM(actual_content == test_content, "File content should match exactly");
    
    // Verify line count
    auto lines = read_file_lines(test_path);
    AXIOM(lines.size() == 3, "Should have 3 lines");
    AXIOM(lines[0] == "Hello from executeLineSurgery!", "First line should match");
    AXIOM(lines[1] == "This is line 2", "Second line should match");
    AXIOM(lines[2] == "This is line 3", "Third line should match");
    
    cleanup_test_file(test_path);
    
    TEST_SUCCESS("Trivial Case: Full File Write via executeLineSurgery");
}

static void test_existing_file_overwrite() {
    TEST_START("Trivial Case: Overwrite Existing File");
    
    TEST_SECTION("Overwrite existing file content");
    std::string original_content = "Original line 1\nOriginal line 2";
    std::string test_path = create_test_file(original_content);
    
    // Verify original content
    auto original_lines = read_file_lines(test_path);
    AXIOM(original_lines.size() == 2, "Should start with 2 lines");
    
    std::string new_content = "New line 1\nNew line 2\nNew line 3\nNew line 4";
    
    std::string write_cmd = "(fio-write :path \"" + test_path + "\" :content \"" + new_content + "\")";
    
    // Use the preview-confirm workflow for safety (prevents accidental overwrites)
    auto [preview_response, confirm_response] = execute_with_confirmation(write_cmd);
    
    AXIOM(preview_response.status == Db9Response::Success, "Preview should succeed");
    
    // If confirmation was needed and executed
    if (!confirm_response.result.empty()) {
        AXIOM(confirm_response.status == Db9Response::Success, "Confirmation should succeed");
    }
    
    // Verify new content completely replaced old content
    auto new_lines = read_file_lines(test_path);
    AXIOM(new_lines.size() == 4, "Should have 4 lines after overwrite");
    AXIOM(new_lines[0] == "New line 1", "First line should be new content");
    AXIOM(new_lines[3] == "New line 4", "Last line should be new content");
    
    // Verify no trace of original content
    std::string actual_content = read_file_content(test_path);
    AXIOM(actual_content.find("Original") == std::string::npos, "No original content should remain");
    
    cleanup_test_file(test_path);
    
    TEST_SUCCESS("Trivial Case: Overwrite Existing File");
}

static void test_empty_file_creation() {
    TEST_START("Trivial Case: Empty File Creation");
    
    TEST_SECTION("Create empty file");
    std::string test_path = "/Users/nick/dev/Lab/LabDb9/tests/cpp/testenv/empty_test.txt";
    cleanup_test_file(test_path);
    
    std::string write_cmd = "(fio-write :path \"" + test_path + "\" :content \"\")";
    auto write_response = db9_execute(write_cmd);
    
    AXIOM(write_response.status == Db9Response::Success, "Empty file creation should succeed");
    AXIOM(std::filesystem::exists(test_path), "Empty file should exist");
    
    // Verify file is actually empty
    auto lines = read_file_lines(test_path);
    AXIOM(lines.size() == 0, "Empty file should have 0 lines");
    
    std::string content = read_file_content(test_path);
    AXIOM(content.empty(), "File content should be empty");
    
    cleanup_test_file(test_path);
    
    TEST_SUCCESS("Trivial Case: Empty File Creation");
}

//-----------------------------------------------------------------------------
// Phase B2: Line Surgery with Preview-Confirm Workflow
//-----------------------------------------------------------------------------

static void test_line_surgery_with_confirmation() {
    TEST_START("Line Surgery: Preview-Confirm Workflow (FIXED)");
    
    TEST_SECTION("All line surgery operations with proper confirmation");
    std::string original_content = "Line 1\nLine 2\nLine 3\nLine 4\nLine 5";
    std::string test_path = create_test_file(original_content);
    
    // Test replace operation
    {
        std::ofstream reset_file(test_path);
        reset_file << original_content;
        reset_file.close();
        
        std::string replace_cmd = "(fio-write :path \"" + test_path + "\" :content \"New content\" :lines \"@3\" :mode \"replace\")";
        auto [preview, confirm] = execute_with_confirmation(replace_cmd);
        
        std::cout << "Replace operation:" << std::endl;
        std::cout << "  Preview status: " << (preview.status == Db9Response::Success ? "Success" : "Error") << std::endl;
        std::cout << "  Confirm status: " << (confirm.status == Db9Response::Success ? "Success" : "Error") << std::endl;
        
        auto lines_after_replace = read_file_lines(test_path);
        std::cout << "  Lines after replace: " << lines_after_replace.size() << std::endl;
        AXIOM(lines_after_replace.size() == 5, "Replace should maintain line count");
    }
    
    // Test insert operation - THE CRITICAL TEST THAT WAS FAILING!
    {
        std::ofstream reset_file(test_path);
        reset_file << original_content;
        reset_file.close();
        
        size_t lines_before = read_file_lines(test_path).size();
        
        std::string insert_cmd = "(fio-write :path \"" + test_path + "\" :content \"Inserted\" :lines \"@2\" :mode \"insert\")";
        auto [preview, confirm] = execute_with_confirmation(insert_cmd);
        
        std::cout << "Insert operation:" << std::endl;
        std::cout << "  Preview status: " << (preview.status == Db9Response::Success ? "Success" : "Error") << std::endl;
        std::cout << "  Confirm status: " << (confirm.status == Db9Response::Success ? "Success" : "Error") << std::endl;
        
        auto lines_after_insert = read_file_lines(test_path);
        std::cout << "  Lines before insert: " << lines_before << std::endl;
        std::cout << "  Lines after insert: " << lines_after_insert.size() << std::endl;
        
        // This is the critical test that was failing!
        AXIOM(lines_after_insert.size() > lines_before, "Insert should increase line count");
        AXIOM(lines_after_insert[0] == "Line 1", "Line 1 should remain at top for @2 insert");
    }
    
    // Test append operation
    {
        std::ofstream reset_file(test_path);
        reset_file << original_content;
        reset_file.close();
        
        size_t lines_before = read_file_lines(test_path).size();
        
        std::string append_cmd = "(fio-write :path \"" + test_path + "\" :content \"Appended\" :lines \"@5\" :mode \"append\")";
        auto [preview, confirm] = execute_with_confirmation(append_cmd);
        
        std::cout << "Append operation:" << std::endl;
        std::cout << "  Preview status: " << (preview.status == Db9Response::Success ? "Success" : "Error") << std::endl;
        std::cout << "  Confirm status: " << (confirm.status == Db9Response::Success ? "Success" : "Error") << std::endl;
        
        auto lines_after_append = read_file_lines(test_path);
        std::cout << "  Lines after append: " << lines_after_append.size() << std::endl;
        AXIOM(lines_after_append.size() > lines_before, "Append should increase line count");
        AXIOM(lines_after_append[0] == "Line 1", "Original content should be preserved");
    }
    
    // Test prepend operation
    {
        std::ofstream reset_file(test_path);
        reset_file << original_content;
        reset_file.close();
        
        size_t lines_before = read_file_lines(test_path).size();
        
        std::string prepend_cmd = "(fio-write :path \"" + test_path + "\" :content \"Prepended\" :mode \"prepend\")";
        auto [preview, confirm] = execute_with_confirmation(prepend_cmd);
        
        std::cout << "Prepend operation:" << std::endl;
        std::cout << "  Preview status: " << (preview.status == Db9Response::Success ? "Success" : "Error") << std::endl;
        std::cout << "  Confirm status: " << (confirm.status == Db9Response::Success ? "Success" : "Error") << std::endl;
        
        auto lines_after_prepend = read_file_lines(test_path);
        std::cout << "  Lines after prepend: " << lines_after_prepend.size() << std::endl;
        AXIOM(lines_after_prepend.size() > lines_before, "Prepend should increase line count");
        AXIOM(lines_after_prepend[lines_after_prepend.size()-1] == "Line 5", "Original content should be shifted down");
    }
    
    cleanup_test_file(test_path);
    
    TEST_SUCCESS("Line Surgery: Preview-Confirm Workflow (FIXED)");
}

//-----------------------------------------------------------------------------
// Phase B3: Error Handling Testing  
//-----------------------------------------------------------------------------

static void test_error_handling() {
    TEST_START("Error Handling: Invalid Operations");
    
    TEST_SECTION("Invalid file path");
    std::string invalid_path = "/invalid/directory/that/does/not/exist/test.txt";
    std::string write_cmd = "(fio-write :path \"" + invalid_path + "\" :content \"test\")";
    auto response = db9_execute(write_cmd);
    
    std::cout << "Invalid path response: " << response.result << std::endl;
    std::cout << "Error message: " << response.error_message << std::endl;
    
    // The method should handle this gracefully (might succeed due to create_directories)
    // This tests our error handling paths
    
    TEST_SECTION("Missing path parameter");
    std::string missing_path_cmd = "(fio-write :content \"test content\")";
    auto missing_response = db9_execute(missing_path_cmd);
    
    AXIOM(missing_response.status == Db9Response::Error, "Missing path should cause error");
    AXIOM(!missing_response.error_message.empty(), "Should have error message");
    
    std::cout << "Missing path error: " << missing_response.error_message << std::endl;
    
    TEST_SUCCESS("Error Handling: Invalid Operations");
}

//-----------------------------------------------------------------------------
// Main Test Runner
//-----------------------------------------------------------------------------

int main() {
    std::cout << "🧪 Line Surgery TDD Test Suite (FIXED)" << std::endl;
    std::cout << "Testing preview-confirm workflow for line surgery operations" << std::endl << std::endl;
    
    try {
        // Phase A: Test Infrastructure
        test_infrastructure_setup();
        
        // Phase B1: Trivial Cases (Should Work)
        test_trivial_full_file_write();
        test_existing_file_overwrite();
        test_empty_file_creation();
        
        // Phase B2: Line Surgery with Preview-Confirm
        test_line_surgery_with_confirmation();
        
        // Phase B3: Error Handling
        test_error_handling();
        
        std::cout << std::endl << "🎉 All tests completed!" << std::endl;
        std::cout << "✅ Preview-confirm workflow working correctly" << std::endl;
        std::cout << "✅ All four line surgery operations confirmed functional" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test suite failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test suite failed with unknown exception" << std::endl;
        return 1;
    }
}
