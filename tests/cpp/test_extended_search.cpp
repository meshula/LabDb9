#include "test_harness.h"
#include "LabDb/LabText.hpp"
#include "../src/Verbs/FioVerbs.h"
#include <iostream>
#include <filesystem>

using namespace labdb9_test;
using namespace LabDb;

// Helper function to create S-expressions for testing
std::string createSearchSexpr(const std::string& path, const std::vector<std::string>& patterns, int depth = 1) {
    std::string sexpr = "(fio-search-ext :path " + path;
    
    sexpr += " :patterns (";
    for (size_t i = 0; i < patterns.size(); ++i) {
        if (i > 0) sexpr += " ";
        sexpr += patterns[i];
    }
    sexpr += ")";
    
    if (depth > 0) {
        sexpr += " :recursive-depth " + std::to_string(depth);
    }
    
    sexpr += ")";
    return sexpr;
}

TEST(s_expression_parameter_parsing) {
    std::cout << "Testing S-expression parameter parsing..." << std::endl;
    
    // Test basic parameter extraction
    std::string sexpr_text = createSearchSexpr("../tests/cpp/testenv/adventure/holy_grail.md", {"grail"});
    std::cout << "Test S-expression: " << sexpr_text << std::endl;
    
    lab::Text::Sexpr sexpr(sexpr_text);
    
    FioSearchExtVerb verb;
    auto config = verb.extractSearchConfig(sexpr);
    
    std::cout << "Extracted path: " << config.path << std::endl;
    std::cout << "Extracted patterns: ";
    for (const auto& pattern : config.patterns) {
        std::cout << "'" << pattern << "' ";
    }
    std::cout << std::endl;
    
    EXPECT_EQ(config.path, "../tests/cpp/testenv/adventure/holy_grail.md");
    EXPECT_EQ(config.patterns.size(), 1);
    EXPECT_EQ(config.patterns[0], "grail");
}

TEST(multi_pattern_s_expression) {
    std::cout << "Testing multi-pattern S-expression..." << std::endl;
    
    std::string sexpr_text = createSearchSexpr("../tests/cpp/testenv", {"grail", "ring", "staff"}, 3);
    std::cout << "Multi-pattern S-expression: " << sexpr_text << std::endl;
    
    lab::Text::Sexpr sexpr(sexpr_text);
    FioSearchExtVerb verb;
    auto config = verb.extractSearchConfig(sexpr);
    
    EXPECT_EQ(config.patterns.size(), 3);
    EXPECT_EQ(config.recursive_depth, 3);
}

TEST(json_response_format) {
    std::cout << "Testing JSON response format..." << std::endl;
    
    std::string sexpr_text = createSearchSexpr("../tests/cpp/testenv/adventure/holy_grail.md", {"grail"});
    lab::Text::Sexpr sexpr(sexpr_text);
    
    FioSearchExtVerb verb;
    auto response = verb.execute(sexpr);
    
    // Use response.result directly - it's already the JSON string!
    std::cout << "JSON Response length: " << response.result.length() << " characters" << std::endl;
    std::cout << "Response preview: " << response.result.substr(0, 200) << "..." << std::endl;
    
    // Validate JSON structure (basic checks)
    EXPECT_TRUE(response.result.find("\"status\"") != std::string::npos);
    EXPECT_TRUE(response.result.find("\"total_matches\"") != std::string::npos);
    EXPECT_TRUE(response.result.find("\"files_processed\"") != std::string::npos);
    EXPECT_TRUE(response.result.find("\"matches\"") != std::string::npos);
}

TEST(error_handling) {
    std::cout << "Testing error handling..." << std::endl;
    
    // Test missing path
    {
        std::string bad_sexpr = "(fio-search-ext :patterns (grail))";
        lab::Text::Sexpr sexpr(bad_sexpr);
        FioSearchExtVerb verb;
        auto response = verb.execute(sexpr);
        
        std::cout << "Missing path response: " << response.result << std::endl;
        EXPECT_TRUE(response.result.find("error") != std::string::npos);
    }
    
    // Test missing patterns
    {
        std::string bad_sexpr = "(fio-search-ext :path ../tests/cpp/testenv)";
        lab::Text::Sexpr sexpr(bad_sexpr);
        FioSearchExtVerb verb;
        auto response = verb.execute(sexpr);
        
        std::cout << "Missing patterns response: " << response.result << std::endl;
        EXPECT_TRUE(response.result.find("error") != std::string::npos);
    }
}

TEST(unicode_integration) {
    std::cout << "Testing Unicode integration..." << std::endl;
    
    std::string sexpr_text = "(fio-search-ext :path ../tests/cpp/testenv/unicode_test/sanskrit_mcguffins.md :patterns (स्वभाव svabhava) :case-fold true :ascii-fold true)";
    lab::Text::Sexpr sexpr(sexpr_text);
    
    FioSearchExtVerb verb;
    auto response = verb.execute(sexpr);
    
    std::cout << "Unicode response preview: " << response.result.substr(0, 300) << "..." << std::endl;
    
    // Should find both Devanagari and ASCII variants
    EXPECT_TRUE(response.result.find("normalization_type") != std::string::npos);
    EXPECT_TRUE(response.result.find("total_matches") != std::string::npos);
}

TEST(performance_integration) {
    std::cout << "Testing performance integration..." << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::string sexpr_text = createSearchSexpr("../tests/cpp/testenv", {"grail", "ring", "staff", "falcon", "tesseract"}, 3);
    lab::Text::Sexpr sexpr(sexpr_text);
    
    FioSearchExtVerb verb;
    auto response = verb.execute(sexpr);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Full integration test completed in " << duration.count() << " ms" << std::endl;
    
    // Should complete reasonably quickly
    EXPECT_LT(duration.count(), 1000); // Less than 1 second
    EXPECT_TRUE(response.result.find("search_complete") != std::string::npos);
}

int main() {
    std::cout << "=== fio-search-ext Integration Test Suite ===" << std::endl;
    
    // Tests now use relative paths from build directory
    return labdb9_test::run_all_tests();
}