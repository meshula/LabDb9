    static void test_multiline_content_preservation() {
        std::cout << "🧪 Test 18: Multi-line content preservation" << std::endl;

        std::string test_path = "/tmp/fio_test_multiline.txt";

        // FIXED: Create file with explicit multi-line content using properly escaped strings
        std::string cmd = R"((fio-write :path "/tmp/fio_test_multiline.txt" :content "Line 1: Function header\nLine 2: {\nLine 3:     int x = 42;\nLine 4:     return x;\nLine 5: }"))";

        auto response = db9_execute(cmd);
        assert(response.status == Db9Response::Success);

        // Test fio-read to see if lines are preserved correctly
        std::string read_cmd = "(fio-read :path \"" + test_path + "\")";
        auto read_response = db9_execute(read_cmd);

        std::cout << "📊 Read response: " << read_response.result << std::endl;

        // Parse the JSON response to check line count
        // Look for "total_lines" field in the response
        if (read_response.result.find("\"total_lines\": 5") != std::string::npos) {
            std::cout << "✅ MULTI-LINE PRESERVATION WORKING: 5 lines correctly stored" << std::endl;
        } else if (read_response.result.find("\"total_lines\": 1") != std::string::npos) {
            std::cout << "⚠️ MULTI-LINE COMPRESSION BUG: Content compressed to 1 line" << std::endl;
            assert(false);
        } else {
            std::cout << "❓ UNEXPECTED LINE COUNT: Check response for total_lines" << std::endl;
            std::cout << "DEBUG: Full response = [" << read_response.result << "]" << std::endl;
            std::cout << "DEBUG: Response status = " << (int)read_response.status << std::endl;
            assert(false);
        }

        std::filesystem::remove(test_path);
        std::cout << "📝 Multi-line content preservation test completed\n" << std::endl;
    }

    static void test_complex_content_with_unicode_and_escapes() {
        std::cout << "🧪 Test 19: Complex content with Unicode and escapes" << std::endl;

        std::string test_path = "/tmp/fio_test_complex_content.txt";

        // FIXED: Test content that combines Unicode, escapes, and multi-line (properly escaped)
        std::string cmd = R"((fio-write :path "/tmp/fio_test_complex_content.txt" :content "🔧 Debug function:\nprintf(\"Debug: %s\\n\", message);\n🚀 Status: Complete ✅\nDone."))";

        auto response = db9_execute(cmd);
        assert(response.status == Db9Response::Success);

        // Read back and analyze
        std::string read_cmd = "(fio-read :path \"" + test_path + "\")";
        auto read_response = db9_execute(read_cmd);

        std::cout << "📊 Complex content response: " << read_response.result << std::endl;

        // Check for various issues
        if (read_response.result.find("\"total_lines\": 4") != std::string::npos) {
            std::cout << "✅ COMPLEX CONTENT WORKING: 4 lines preserved with Unicode" << std::endl;
        } else {
            std::cout << "⚠️  COMPLEX CONTENT ISSUES: Check line preservation and Unicode handling" << std::endl;
            std::cout << "DEBUG: Full response = [" << read_response.result << "]" << std::endl;
            std::cout << "DEBUG: Response status = " << (int)read_response.status << std::endl;
            assert(false);
        }

        std::filesystem::remove(test_path);
        std::cout << "📝 Complex content test completed\n" << std::endl;
    }
