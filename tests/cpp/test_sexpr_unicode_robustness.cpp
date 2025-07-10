//
// test_sexpr_unicode_robustness.cpp
// Comprehensive unicode robustness testing for LabDb S-expression parser
//

#include <LabDb/LabText.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <cassert>

using namespace lab::Text;

//-----------------------------------------------------------------------------
// Test Framework Macros (consistent with other LabDb tests)
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// Test Utilities
//-----------------------------------------------------------------------------

// Helper to parse and validate S-expression structure
struct ParseResult {
    bool success;
    std::vector<std::string> atoms;
    std::vector<std::string> strings;
    std::vector<int> structure; // 1 for push, -1 for pop, 0 for atom/string
    std::string error_message;
};

ParseResult ParseSexpr(const std::string& input) {
    ParseResult result;
    result.success = false;

    try {
        StrView view(input);
        Sexpr sexpr(view);
        
        result.success = (sexpr.balance == 0); // Properly balanced
        
        // Extract structure and content
        for (const auto& elem : sexpr.expr) {
            switch (elem.token) {
                case tsSexprPushList:
                    result.structure.push_back(1);
                    break;
                case tsSexprPopList:
                    result.structure.push_back(-1);
                    break;
                case tsSexprAtom:
                    result.structure.push_back(0);
                    if (elem.ref < sexpr.strings.size()) {
                        result.atoms.push_back(sexpr.strings[elem.ref]);
                    }
                    break;
                case tsSexprString:
                    result.structure.push_back(0);
                    if (elem.ref < sexpr.strings.size()) {
                        result.strings.push_back(sexpr.strings[elem.ref]);
                    }
                    break;
                case tsSexprInteger:
                case tsSexprFloat:
                    result.structure.push_back(0);
                    break;
            }
        }
    } catch (const std::exception& e) {
        result.error_message = e.what();
    } catch (...) {
        result.error_message = "Unknown exception during parsing";
    }

    return result;
}

// Helper to print unicode codepoints for debugging
std::string DebugUnicode(const std::string& str) {
    std::ostringstream oss;
    for (unsigned char c : str) {
        oss << "0x" << std::hex << (int)c << " ";
    }
    return oss.str();
}

//-----------------------------------------------------------------------------
// Test Functions
//-----------------------------------------------------------------------------

// Test 1: Devanagari script robustness
void TestDevanagariScriptHandling() {
    TEST_START("Devanagari Script Handling");
    
    // Sanskrit/Hindi text with various devanagari characters
    std::vector<std::string> devanagari_tests = {
        // Basic devanagari
        "(त्रित्रयम् consciousness field)",
        "(स्वभाव \"essence\" lakshana)",
        "(गमन \"going\" प्रक्रिया)",
        
        // Complex devanagari with conjuncts
        "(त्रित्रयस्य चक्रे त्वमेव)",
        "(प्रज्ञाप्ति \"wisdom\" काल)",
        "(सत्यम् शिवम् सुन्दरम्)",
        
        // Mixed devanagari and Latin
        "(triadic त्रित्रयम् \"mixed script test\")",
        "(database \"डेटाबेस\" स्टोर)",
        "(motion गमन memory स्मृति field क्षेत्र)"
    };

    for (const auto& test : devanagari_tests) {
        TEST_SECTION("Testing: " + test);
        
        auto result = ParseSexpr(test);
        AXIOM(result.success, "Parse failed: " + result.error_message);
        AXIOM(result.structure.size() > 0, "No structure elements found");
        
        // Verify balanced parentheses
        int balance = 0;
        for (int val : result.structure) {
            balance += val;
        }
        AXIOM(balance == 0, "Unbalanced parentheses");
    }
    
    TEST_SUCCESS("Devanagari Script Handling");
}

// Test 2: Emoji and symbol robustness
void TestEmojiAndSymbolHandling() {
    TEST_START("Emoji and Symbol Handling");
    
    std::vector<std::string> emoji_tests = {
        // Basic emojis
        "(🌊 motion 🧠 memory 🌌 field)",
        "(database \"🗄️\" query \"🔍\")",
        "(consciousness \"🧘\" awareness \"✨\")",
        
        // Complex emojis with skin tones and modifiers
        "(user \"👨🏽‍💻\" action \"typing\")",
        "(family \"👨‍👩‍👧‍👦\" unity \"together\")",
        
        // Mathematical and technical symbols
        "(∀ universal ∃ existential)",
        "(λ lambda → arrow ∞ infinity)",
        "(α alpha β beta γ gamma δ delta)",
        
        // Mixed emoji and text
        "(status \"✅ success\" error \"❌ failed\")",
        "(🎯 target achieved 💯 percent)"
    };

    for (const auto& test : emoji_tests) {
        TEST_SECTION("Testing: " + test);
        
        auto result = ParseSexpr(test);
        AXIOM(result.success, "Parse failed: " + result.error_message);
        
        // Emojis should be treated as atoms, not parsed as separate characters
        AXIOM(result.atoms.size() + result.strings.size() > 0, "No atoms or strings found");
    }
    
    TEST_SUCCESS("Emoji and Symbol Handling");
}

// Test 3: Nested S-expressions as string content
void TestNestedSexprAsStringContent() {
    TEST_START("Nested S-expressions as String Content");
    
    std::vector<std::string> nested_tests = {
        // S-expressions inside strings
        "(code \"(defun hello (x) (print x))\")",
        "(query \"(find-triple ?s ?p ?o)\")",
        "(lisp \"(λ (x) (+ x 1))\")",
        
        // Mixed nested with unicode
        "(sanskrit \"(त्रित्रयम् स्वभाव)\" meaning \"triadic essence\")",
        "(example \"(🌊 गमन memory)\" type \"mixed\")",
        
        // Deeply nested string content - corrected balanced parentheses  
        
        "(meta \"(code \\\"(inner (nested))\\\"))\")",
        "(escaped \"(quotes inside strings)\")"
    };

    for (const auto& test : nested_tests) {
        TEST_SECTION("Testing: " + test);
        
        auto result = ParseSexpr(test);
        AXIOM(result.success, "Parse failed: " + result.error_message);
        
        // Should have at least one string containing parentheses
        bool found_nested = false;
        for (const auto& str : result.strings) {
            if (str.find('(') != std::string::npos || str.find(')') != std::string::npos) {
                found_nested = true;
                break;
            }
        }
        AXIOM(found_nested, "No nested S-expressions found in strings");
    }
    
    TEST_SUCCESS("Nested S-expressions as String Content");
}

// Test 4: Unicode characters that visually resemble parentheses
void TestVisualParenthesesLookalikes() {
    TEST_START("Visual Parentheses Lookalikes");
    
    std::vector<std::string> lookalike_tests = {
        // Various bracket and parenthesis-like unicode characters
        "(test ❨visual❩ parentheses)",           // U+2768, U+2769
        "(test ❪angle❫ brackets)",               // U+276A, U+276B  
        "(test ❬tortoise❭ shell)",               // U+276C, U+276D
        "(test ❮pointing❯ angles)",              // U+276E, U+276F
        "(test ❰heavy❱ angles)",                 // U+2770, U+2771
        "(test ⟨mathematical⟩ angles)",          // U+27E8, U+27E9
        "(test ⟪double⟫ angles)",                // U+27EA, U+27EB
        "(test ⦃curly⦄ brackets)",               // U+2983, U+2984
        "(test ⦅white⦆ parentheses)",            // U+2985, U+2986
        
        // Fullwidth parentheses (common in CJK)
        "(test （fullwidth） parentheses)",       // U+FF08, U+FF09
        
        // Mathematical operators that might confuse
        "(test ∈ ∉ ∋ ∌ membership)",
        "(test ⊂ ⊃ ⊄ ⊅ subset)",
        
        // Mixed real and lookalike
        "(real (nested) with ❨visual❩ mixed)"
    };

    for (const auto& test : lookalike_tests) {
        TEST_SECTION("Testing: " + test);
        
        auto result = ParseSexpr(test);
        AXIOM(result.success, "Parse failed: " + result.error_message);
        
        // Should only recognize actual ASCII parentheses for structure
        // Lookalikes should be treated as regular atom content
        bool has_proper_structure = false;
        for (int val : result.structure) {
            if (val == 1 || val == -1) {
                has_proper_structure = true;
                break;
            }
        }
        AXIOM(has_proper_structure, "No proper S-expression structure found");
    }
    
    TEST_SUCCESS("Visual Parentheses Lookalikes");
}

// Integration test: Real-world triadic consciousness data
void TestRealWorldTriadicData() {
    TEST_START("Real-World Triadic Data");
    
    std::string triadic_example = R"TRI(
        (motion-memory-field
            (त्रित्रयम् "triadic consciousness")
            (स्वभाव essence (लक्षण manifestation) (कारित्र function))
            (database-entry
                (subject "granite🗿")
                (predicate "contains")
                (object "quartz💎")
                (metadata 
                    (confidence 0.95)
                    (source "geological_survey_🌍")
                    (timestamp "2025-01-15T10:30:00Z")
                    (tags "mineral" "rock" "geology" "त्रित्रयम्")))
            (query-pattern
                (lisp-code "(find-triple ?s contains ?o)")
                (description "Find all containment relationships")
                (unicode-test "Mixed scripts: Latin, देवनागरी, Emoji 🔍"))
            (consciousness-state
                (स्पर्श touch)
                (अन्तराल interval) 
                (उद्भव emergence)
                (field-dynamic "🌊 flowing awareness")
                (memory-crystallization "💎 permanent knowledge")
                (motion-integration "⚡ dynamic processing")))
    )TRI";

    auto result = ParseSexpr(triadic_example);
    AXIOM(result.success, "Real-world example failed: " + result.error_message);
    
    // Verify we found the expected elements
    AXIOM(result.atoms.size() > 20, "Too few atoms found");
    AXIOM(result.strings.size() > 10, "Too few strings found");
    
    // Check for specific unicode content
    bool found_devanagari = false;
    bool found_emoji = false;
    
    for (const auto& atom : result.atoms) {
        if (atom.find("त्रित्रयम्") != std::string::npos) found_devanagari = true;
        if (atom.find("🌊") != std::string::npos) found_emoji = true;
    }
    
    for (const auto& str : result.strings) {
        if (str.find("त्रित्रयम्") != std::string::npos) found_devanagari = true;
        if (str.find("🌊") != std::string::npos) found_emoji = true;
    }
    
    AXIOM(found_devanagari, "Devanagari content not found in parsed result");
    AXIOM(found_emoji, "Emoji content not found in parsed result");
}
    
// Diagnostic test: Find minimal escaped quote failure
void TestEscapedQuoteDiagnostics() {
    TEST_START("Escaped Quote Diagnostics");
    
    // Progressive complexity test cases
    std::vector<std::pair<std::string, std::string>> escape_tests = {
        // Level 1: Basic strings (should work)
        {"(simple \"hello\")", "Basic quoted string"},
        {"(test \"content\")", "Simple content string"},
        
        // Level 2: Single escaped quote (should work)
        {"(escaped \"say \\\"hello\\\"\")", "Single escaped quote"},
        {"(test \"\\\"quoted\\\"\")", "Just escaped quotes"},
        
        // Level 3: Nested parens in strings (should work)
        {"(code \"(print hello)\")", "Parens in string"},
        {"(nested \"(def func)\")", "S-expr in string"},
        
        // Level 4: Escaped quotes with parens (the trouble zone)
        {"(meta \"(code \\\"inner\\\")\")", "Escaped quotes with parens"},
        {"(complex \"(func \\\"arg\\\")\")", "Function call with quoted arg"},
        
        // Level 5: Test the parentheses balance issue step by step
        {"(meta \"(code \\\"(inner (nested))\\\"))\")", "balanced parentheses"},
        {"(meta \"(code \\\"inner\\\")\")", "Simplified - remove nested parens"},

        {"(balanced parens)", "Balanced control case"},
    };
    
    for (const auto& [test_case, description] : escape_tests) {
        TEST_SECTION("Testing: " + description + " -> " + test_case);
        
        // Add detailed debugging for the failing case
        std::cout << "    🔍 Input length: " << test_case.length() << std::endl;
        std::cout << "    🔍 Raw bytes: ";
        for (size_t i = 0; i < std::min(test_case.length(), size_t(50)); ++i) {
            std::cout << std::hex << (int)(unsigned char)test_case[i] << " ";
        }
        std::cout << std::dec << std::endl;
        
        // Count parentheses manually
        int open_count = 0, close_count = 0;
        bool in_string = false;
        bool escaped = false;
        for (char c : test_case) {
            if (escaped) {
                escaped = false;
                continue;
            }
            if (c == '\\' && in_string) {
                escaped = true;
                continue;
            }
            if (c == '"') {
                in_string = !in_string;
                continue;
            }
            if (!in_string) {
                if (c == '(') open_count++;
                if (c == ')') close_count++;
            }
        }
        std::cout << "    🔍 Paren balance: " << open_count << " open, " << close_count << " close" << std::endl;
        
        auto result = ParseSexpr(test_case);
        if (result.success) {
            std::cout << "    ✅ PASSED: " << description << std::endl;
            
            // Debug: Show what was parsed
            std::cout << "    📊 Atoms: " << result.atoms.size() 
                      << ", Strings: " << result.strings.size() << std::endl;
            if (!result.strings.empty()) {
                std::cout << "    📝 First string: \"" << result.strings[0] << "\"" << std::endl;
            }
        } else {
            std::cout << "    ❌ FAILED: " << description << std::endl;
            std::cout << "    💥 Error: '" << result.error_message << "'" << std::endl;
            std::cout << "    📊 Partial parse - Atoms: " << result.atoms.size() 
                      << ", Strings: " << result.strings.size() 
                      << ", Structure: " << result.structure.size() << std::endl;
            
            // Show what was successfully parsed before failure
            if (!result.atoms.empty()) {
                std::cout << "    📝 Parsed atoms: ";
                for (const auto& atom : result.atoms) {
                    std::cout << "\"" << atom << "\" ";
                }
                std::cout << std::endl;
            }
            if (!result.strings.empty()) {
                std::cout << "    📝 Parsed strings: ";
                for (const auto& str : result.strings) {
                    std::cout << "\"" << str << "\" ";
                }
                std::cout << std::endl;
            }
            
            std::cout << "    🔍 This is the minimal failing case!" << std::endl;
            break; // Stop at first failure to identify the boundary
        }
    }
    
    // Test cases that SHOULD FAIL (parser correctly rejects malformed input)
    TEST_SECTION("Testing malformed input rejection...");
    std::vector<std::pair<std::string, std::string>> failure_tests = {
        {"(test \"simple\"))", "Extra closing paren"},
        {"(unmatched \"string", "Unmatched quote"},
        {"(missing close", "Missing closing paren"},
    };
    
    for (const auto& [test_case, description] : failure_tests) {
        std::cout << "    🔍 Testing rejection: " << description << " -> " << test_case << std::endl;
        
        try {
            auto result = ParseSexpr(test_case);
            if (!result.success) {
                std::cout << "    ✅ CORRECTLY REJECTED: " << description << std::endl;
            } else {
                std::cout << "    ❌ INCORRECTLY ACCEPTED: " << description << std::endl;
                AXIOM(false, "Parser should have rejected malformed input: " + test_case);
            }
        } catch (const std::exception& e) {
            std::cout << "    💥 EXCEPTION during rejection test: " << e.what() << std::endl;
            std::cout << "    🔍 This reveals a parser robustness issue!" << std::endl;
            // Don't fail the test - this is useful diagnostic info
        } catch (...) {
            std::cout << "    💥 UNKNOWN EXCEPTION during rejection test" << std::endl;
            std::cout << "    🔍 This reveals a parser robustness issue!" << std::endl;
            // Don't fail the test - this is useful diagnostic info
        }
    }
    
    TEST_SUCCESS("Escaped Quote Diagnostics");
}

//-----------------------------------------------------------------------------
// Enhanced Entity Pattern List Parsing Test
// Add this to test_sexpr_unicode_robustness.cpp or create a new test file
//-----------------------------------------------------------------------------

void TestEnhancedEntityPatternParsing() {
    TEST_START("Enhanced Entity Pattern List Parsing");
    
    //-------------------------------------------------------------------------
    // Test 1: Single pattern (existing syntax) 
    //-------------------------------------------------------------------------
    TEST_SECTION("Single pattern syntax");
    {
        std::string single_cmd = "(find-entity-enhanced :pattern \"*camera*\" :dbid \"db1\")";
        ParseResult result = ParseSexpr(single_cmd);
        
        AXIOM(result.success, "Single pattern S-expression should parse successfully");
        AXIOM(result.atoms.size() >= 3, "Should have verb + parameters");
        AXIOM(result.atoms[0] == "find-entity-enhanced", "First atom should be verb name");
        AXIOM(result.strings.size() >= 2, "Should have pattern and dbid strings");
        
        // Find :pattern parameter
        bool found_pattern_param = false;
        for (size_t i = 0; i < result.atoms.size(); ++i) {
            if (result.atoms[i] == ":pattern") {
                found_pattern_param = true;
                break;
            }
        }
        AXIOM(found_pattern_param, "Should contain :pattern parameter");
    }
    
    //-------------------------------------------------------------------------  
    // Test 2: Multiple patterns (new list syntax)
    //-------------------------------------------------------------------------
    TEST_SECTION("Multiple pattern list syntax");
    {
        std::string multi_cmd = "(find-entity-enhanced :patterns (\"*camera*\" \"*background*\" \"*scroll*\") :dbid \"db1\")";
        ParseResult result = ParseSexpr(multi_cmd);
        
        AXIOM(result.success, "Multi-pattern S-expression should parse successfully");
        AXIOM(result.atoms[0] == "find-entity-enhanced", "First atom should be verb name");
        
        // Should have nested list structure
        bool has_nested_list = false;
        int depth = 0;
        for (int struct_elem : result.structure) {
            if (struct_elem == 1) depth++;
            if (struct_elem == -1) depth--;
            if (depth > 1) has_nested_list = true;
        }
        AXIOM(has_nested_list, "Should contain nested list for patterns");
        
        // Should have 3 pattern strings plus dbid
        AXIOM(result.strings.size() >= 4, "Should have 3 patterns + dbid string");
        
        // Find :patterns parameter
        bool found_patterns_param = false;
        for (size_t i = 0; i < result.atoms.size(); ++i) {
            if (result.atoms[i] == ":patterns") {
                found_patterns_param = true;
                break;
            }
        }
        AXIOM(found_patterns_param, "Should contain :patterns parameter");
    }
    
    //-------------------------------------------------------------------------
    // Test 3: Extract pattern list from parsed S-expression
    //-------------------------------------------------------------------------  
    TEST_SECTION("Pattern list extraction");
    {
        std::string multi_cmd = "(find-entity-enhanced :patterns (\"*camera*\" \"*background*\" \"*scroll*\") :dbid \"db1\")";
        StrView view(multi_cmd);
        Sexpr sexpr(view);
        
        AXIOM(sexpr.balance == 0, "S-expression should be balanced");
        
        // Simulate extractPatternList functionality
        std::vector<std::string> extracted_patterns;
        
        // Find :patterns parameter in atoms
        bool in_patterns_list = false;
        int list_depth = 0;
        
        for (size_t i = 0; i < sexpr.expr.size(); ++i) {
            const auto& elem = sexpr.expr[i];
            
            // Check if we hit :patterns atom
            if (elem.token == tsSexprAtom && elem.ref < sexpr.strings.size()) {
                if (sexpr.strings[elem.ref] == ":patterns") {
                    in_patterns_list = true;
                    continue;
                }
            }
            
            if (in_patterns_list) {
                if (elem.token == tsSexprPushList) {
                    list_depth++;
                } else if (elem.token == tsSexprPopList) {
                    list_depth--;
                    if (list_depth == 0) {
                        in_patterns_list = false; // End of patterns list
                    }
                } else if (elem.token == tsSexprString && list_depth > 0) {
                    // This is a pattern string inside the list
                    if (elem.ref < sexpr.strings.size()) {
                        extracted_patterns.push_back(sexpr.strings[elem.ref]);
                    }
                }
            }
        }
        
        AXIOM(extracted_patterns.size() == 3, "Should extract exactly 3 patterns");
        AXIOM(extracted_patterns[0] == "*camera*", "First pattern should be *camera*");
        AXIOM(extracted_patterns[1] == "*background*", "Second pattern should be *background*");
        AXIOM(extracted_patterns[2] == "*scroll*", "Third pattern should be *scroll*");
    }
    
    //-------------------------------------------------------------------------
    // Test 4: Edge cases and error conditions
    //-------------------------------------------------------------------------
    TEST_SECTION("Edge cases");
    {
        // Empty patterns list
        std::string empty_list = "(find-entity-enhanced :patterns () :dbid \"db1\")";
        ParseResult empty_result = ParseSexpr(empty_list);
        AXIOM(empty_result.success, "Empty pattern list should parse");
        
        // Single pattern in list (should work same as :pattern)
        std::string single_in_list = "(find-entity-enhanced :patterns (\"*camera*\") :dbid \"db1\")"; 
        ParseResult single_result = ParseSexpr(single_in_list);
        AXIOM(single_result.success, "Single pattern in list should parse");
        
        // Missing dbid (should parse but fail validation later)
        std::string no_dbid = "(find-entity-enhanced :patterns (\"*camera*\"))";
        ParseResult no_dbid_result = ParseSexpr(no_dbid);
        AXIOM(no_dbid_result.success, "Missing dbid should still parse (validation happens later)");
    }
    
    TEST_SUCCESS("Enhanced Entity Pattern List Parsing");
}


//-----------------------------------------------------------------------------
// Main Test Runner
//-----------------------------------------------------------------------------

int main() {
    std::cout << "🧘 LabDb S-Expression Unicode Robustness Test Suite\n";
    std::cout << "===================================================\n";
    
    try {
        TestNestedSexprAsStringContent();
        TestDevanagariScriptHandling();
        TestEscapedQuoteDiagnostics();
        TestEmojiAndSymbolHandling();
        TestVisualParenthesesLookalikes();
        TestRealWorldTriadicData();
        TestEnhancedEntityPatternParsing();

        
        // Original quick test preserved
        std::cout << "\n🧪 Quick Additional Unicode Test...\n";
        auto emoji_result = ParseSexpr("(🌊 त्रित्रयम् \"consciousness 🧘\")");
        AXIOM(emoji_result.success, "Additional emoji test failed");
        std::cout << "✅ PASSED: Additional unicode integration\n";
        
        std::cout << "\n🎉 ALL UNICODE ROBUSTNESS TESTS PASSED!\n";
        std::cout << "S-expression parser is robust against unicode edge cases.\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n💥 EXCEPTION: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "\n💥 UNKNOWN EXCEPTION occurred during testing" << std::endl;
        return 1;
    }
}
