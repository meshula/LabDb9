#include "test_harness.h"
#include "LabDb/TextEscaping.h"
#include <string>

using namespace labdb9_test;
using namespace LabDb::TextEscaping;

TEST(basic_functionality) {
    // Test that regular text passes through unchanged
    std::string input = "hello world";
    std::string result = unescapeDb9String(input);
    EXPECT_EQ("hello world", result);
}

TEST(db9_readme_printf_example) {
    // Example from db9_readme: §printf(″Debug: %s※n″, msg);§
    // Should become: printf("Debug: %s\n", msg);
    
    // UTF-8 bytes: ″ = \xe2\x80\xb3, ※ = \xe2\x80\xbb
    std::string input = "printf(\xe2\x80\xb3" "Debug: %s" "\xe2\x80\xbb" "n" "\xe2\x80\xb3" ", msg);";
    std::string result = unescapeDb9String(input);
    std::string expected = "printf(\"Debug: %s\\n\", msg);";
    
    std::cout << "Printf example:" << std::endl;
    std::cout << "  Input: printf(″Debug: %s※n″, msg);" << std::endl;
    std::cout << "  Expected: " << expected << std::endl;
    std::cout << "  Got:      " << result << std::endl;
    
    EXPECT_EQ(expected, result);
}

TEST(db9_readme_regex_example) {
    // Example: §std::regex email(″[a-z]+@[a-z]+※.[a-z]+″);§
    // Should become: std::regex email("[a-z]+@[a-z]+\.[a-z]+");
    
    std::string input = "std::regex email(\xe2\x80\xb3" "[a-z]+@[a-z]+" "\xe2\x80\xbb" ".[a-z]+" "\xe2\x80\xb3" ");";
    std::string result = unescapeDb9String(input);
    std::string expected = "std::regex email(\"[a-z]+@[a-z]+\\.[a-z]+\");";
    
    std::cout << "Regex example:" << std::endl;
    std::cout << "  Input: std::regex email(″[a-z]+@[a-z]+※.[a-z]+″);" << std::endl;
    std::cout << "  Expected: " << expected << std::endl;
    std::cout << "  Got:      " << result << std::endl;
    
    EXPECT_EQ(expected, result);
}

TEST(db9_readme_file_path_example) {
    // Example: §path = ″C:※※Users※※Documents″;§
    // Should become: path = "C:\\Users\\Documents";
    
    std::string input = "path = \xe2\x80\xb3" "C:" "\xe2\x80\xbb\xe2\x80\xbb" "Users" "\xe2\x80\xbb\xe2\x80\xbb" "Documents" "\xe2\x80\xb3" ";";
    std::string result = unescapeDb9String(input);
    std::string expected = "path = \"C:\\\\Users\\\\Documents\";";
    
    std::cout << "File path example:" << std::endl;
    std::cout << "  Input: path = ″C:※※Users※※Documents″;" << std::endl;
    std::cout << "  Expected: " << expected << std::endl;
    std::cout << "  Got:      " << result << std::endl;
    
    EXPECT_EQ(expected, result);
}

TEST(db9_readme_tab_and_newline_example) {
    // Example: ⇥if (validate()) {↵⇥printf(″Valid input※n″);↵}
    // Should become: \tif (validate()) {\n\tprintf("Valid input\n");\n}
    
    // UTF-8 bytes: ⇥ = \xe2\x87\xa5, ↵ = \xe2\x86\xb5
    std::string input = 
        "\xe2\x87\xa5" "if (validate()) {" "\xe2\x86\xb5"
        "\xe2\x87\xa5" "printf(" "\xe2\x80\xb3" "Valid input" "\xe2\x80\xbb" "n" "\xe2\x80\xb3" ");" "\xe2\x86\xb5"
        "}";
    
    std::string result = unescapeDb9String(input);
    std::string expected = "\tif (validate()) {\n\tprintf(\"Valid input\\n\");\n}";
    
    std::cout << "Tab and newline example:" << std::endl;
    std::cout << "  Input: ⇥if (validate()) {↵⇥printf(″Valid input※n″);↵}" << std::endl;
    std::cout << "  Expected: " << expected << std::endl;
    std::cout << "  Got:      " << result << std::endl;
    
    EXPECT_EQ(expected, result);
}

TEST(db9_readme_complex_multiline_example) {
    // Complex example from db9_readme:
    // §⇥// Generated C++ code↵
    // ⇥std::regex email(″[a-z]+@[a-z]+※.[a-z]+″);↵
    // ⇥printf(″Pattern ready※n″);↵§
    
    std::string input = 
        "\xe2\x87\xa5" "// Generated C++ code" "\xe2\x86\xb5"
        "\xe2\x87\xa5" "std::regex email(" "\xe2\x80\xb3" "[a-z]+@[a-z]+" "\xe2\x80\xbb" ".[a-z]+" "\xe2\x80\xb3" ");" "\xe2\x86\xb5"
        "\xe2\x87\xa5" "printf(" "\xe2\x80\xb3" "Pattern ready" "\xe2\x80\xbb" "n" "\xe2\x80\xb3" ");" "\xe2\x86\xb5";
    
    std::string result = unescapeDb9String(input);
    std::string expected = 
        "\t// Generated C++ code\n"
        "\tstd::regex email(\"[a-z]+@[a-z]+\\.[a-z]+\");\n"
        "\tprintf(\"Pattern ready\\n\");\n";
    
    std::cout << "Complex multiline example:" << std::endl;
    std::cout << "  Input: Complex Unicode code block" << std::endl;
    std::cout << "  Expected:" << std::endl << expected << std::endl;
    std::cout << "  Got:" << std::endl << result << std::endl;
    
    EXPECT_EQ(expected, result);
}

TEST(individual_unicode_character_tests) {
    // Test each Unicode character individually
    
    // Test ※ (U+203B) → backslash character 
    std::string backslash_input = "\xe2\x80\xbb";
    std::string backslash_result = unescapeDb9String(backslash_input);
    std::cout << "※ → '" << backslash_result << "'" << std::endl;
    EXPECT_EQ("\\", backslash_result);
    
    // Test ″ (U+2033) → "
    std::string quote_input = "\xe2\x80\xb3";
    std::string quote_result = unescapeDb9String(quote_input);
    std::cout << "″ → '" << quote_result << "'" << std::endl;
    EXPECT_EQ("\"", quote_result);
    
    // Test ↵ (U+21B5) → \n
    std::string newline_input = "\xe2\x86\xb5";
    std::string newline_result = unescapeDb9String(newline_input);
    std::cout << "↵ → '" << escapeForDisplay(newline_result) << "'" << std::endl;
    EXPECT_EQ("\n", newline_result);
    
    // Test ⇥ (U+21E5) → \t  
    std::string tab_input = "\xe2\x87\xa5";
    std::string tab_result = unescapeDb9String(tab_input);
    std::cout << "⇥ → '" << escapeForDisplay(tab_result) << "'" << std::endl;
    EXPECT_EQ("\t", tab_result);
}

TEST(escapeForDisplay_validation) {
    // Test that escapeForDisplay works correctly with our generated content
    
    std::string content_with_escapes = "printf(\"Debug: %s\\n\", msg);";
    std::string display_result = escapeForDisplay(content_with_escapes);
    
    std::cout << "EscapeForDisplay test:" << std::endl;
    std::cout << "  Input: " << content_with_escapes << std::endl;
    std::cout << "  Output: " << display_result << std::endl;
    
    // Should escape the quotes and backslashes for display
    EXPECT_TRUE(display_result.find("\\\"") != std::string::npos);
    EXPECT_TRUE(display_result.find("\\\\") != std::string::npos);
}

LABDB9_TEST_MAIN()
