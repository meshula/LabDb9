// test_harness.h - Minimal test framework for LabDb9
// Matches existing LabDb9 test pattern - absolute minimalism
#pragma once

#include <iostream>
#include <string>
#include <cassert>

//-----------------------------------------------------------------------------
// Test Framework Macros (matching existing LabDb9 pattern)
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
// Global test state (minimal version)
//-----------------------------------------------------------------------------
struct TestState {
    std::string test_db_path;
    int tests_run{0};
    int tests_passed{0};
    int verbosity{1}; // 0=minimal, 1=normal, 2=verbose
    
    TestState() : test_db_path("/tmp/labdb_rope_test") {}
};

//-----------------------------------------------------------------------------
// Helper Functions
//-----------------------------------------------------------------------------
inline void validateResponse(const std::string& response,
                            bool expectSuccess,
                            const std::string& context) {
    bool isSuccess = response.find("success") != std::string::npos ||
                    response.find(":status \"success\"") != std::string::npos;
    
    if (expectSuccess) {
        AXIOM(isSuccess, context + " - Expected success but got: " + response);
    } else {
        AXIOM(!isSuccess, context + " - Expected failure but got success");
    }
}

// Core assertion macros (gtest-style but minimal)
#define EXPECT_TRUE(condition) \
    do { \
        g_state.assertions_run++; \
        if (condition) { \
            g_state.assertions_passed++; \
        } else { \
            g_state.current_test_passed = false; \
            std::ostringstream oss; \
            oss << g_state.current_test << ": EXPECT_TRUE failed at " << __FILE__ << ":" << __LINE__ << " - " << #condition; \
            g_state.failures.push_back(oss.str()); \
            std::cout << "  ❌ " << #condition << " (line " << __LINE__ << ")" << std::endl; \
        } \
    } while(0)

#define EXPECT_FALSE(condition) \
    do { \
        g_state.assertions_run++; \
        if (!(condition)) { \
            g_state.assertions_passed++; \
        } else { \
            g_state.current_test_passed = false; \
            std::ostringstream oss; \
            oss << g_state.current_test << ": EXPECT_FALSE failed at " << __FILE__ << ":" << __LINE__ << " - " << #condition; \
            g_state.failures.push_back(oss.str()); \
            std::cout << "  ❌ " << #condition << " should be false (line " << __LINE__ << ")" << std::endl; \
        } \
    } while(0)

#define EXPECT_EQ(expected, actual) \
    do { \
        g_state.assertions_run++; \
        if ((expected) == (actual)) { \
            g_state.assertions_passed++; \
        } else { \
            g_state.current_test_passed = false; \
            std::ostringstream oss; \
            oss << g_state.current_test << ": EXPECT_EQ failed at " << __FILE__ << ":" << __LINE__ \
                << " - Expected: " << (expected) << ", Actual: " << (actual); \
            g_state.failures.push_back(oss.str()); \
            std::cout << "  ❌ Expected: " << (expected) << ", Got: " << (actual) << " (line " << __LINE__ << ")" << std::endl; \
        } \
    } while(0)

#define EXPECT_NE(not_expected, actual) \
    do { \
        g_state.assertions_run++; \
        if ((not_expected) != (actual)) { \
            g_state.assertions_passed++; \
        } else { \
            g_state.current_test_passed = false; \
            std::ostringstream oss; \
            oss << g_state.current_test << ": EXPECT_NE failed at " << __FILE__ << ":" << __LINE__ \
                << " - Both values are: " << (actual); \
            g_state.failures.push_back(oss.str()); \
            std::cout << "  ❌ Values should not be equal: " << (actual) << " (line " << __LINE__ << ")" << std::endl; \
        } \
    } while(0)

#define ASSERT_TRUE(condition) \
    do { \
        EXPECT_TRUE(condition); \
        if (!(condition)) return; \
    } while(0)

#define ASSERT_FALSE(condition) \
    do { \
        EXPECT_FALSE(condition); \
        if (condition) return; \
    } while(0)

#define ASSERT_EQ(expected, actual) \
    do { \
        EXPECT_EQ(expected, actual); \
        if ((expected) != (actual)) return; \
    } while(0)

#define ASSERT_NE(not_expected, actual) \
    do { \
        EXPECT_NE(not_expected, actual); \
        if ((not_expected) == (actual)) return; \
    } while(0)

// Test registration macro
#define TEST(test_name) \
    void test_##test_name(); \
    static bool registered_##test_name = []() { \
        g_tests.push_back({#test_name, test_##test_name}); \
        return true; \
    }(); \
    void test_##test_name()

// Test execution functions
inline void run_test(const TestCase& test) {
    g_state.current_test = test.name;
    g_state.current_test_passed = true;
    g_state.tests_run++;
    
    std::cout << "🧪 Running: " << test.name << std::endl;
    
    try {
        test.func();
        if (g_state.current_test_passed) {
            g_state.tests_passed++;
            std::cout << "  ✅ PASSED" << std::endl;
        } else {
            std::cout << "  ❌ FAILED" << std::endl;
        }
    } catch (const std::exception& e) {
        g_state.current_test_passed = false;
        std::ostringstream oss;
        oss << test.name << ": Exception: " << e.what();
        g_state.failures.push_back(oss.str());
        std::cout << "  💥 EXCEPTION: " << e.what() << std::endl;
    } catch (...) {
        g_state.current_test_passed = false;
        std::ostringstream oss;
        oss << test.name << ": Unknown exception";
        g_state.failures.push_back(oss.str());
        std::cout << "  💥 UNKNOWN EXCEPTION" << std::endl;
    }
    
    std::cout << std::endl;
}

inline int run_all_tests() {
    std::cout << "🚀 LabDb9 Test Runner - Minimal Edition" << std::endl;
    std::cout << "=====================================\n" << std::endl;
    
    for (const auto& test : g_tests) {
        run_test(test);
    }
    
    // Final report
    std::cout << "📊 Test Results:" << std::endl;
    std::cout << "  Tests: " << g_state.tests_passed << "/" << g_state.tests_run << " passed" << std::endl;
    std::cout << "  Assertions: " << g_state.assertions_passed << "/" << g_state.assertions_run << " passed" << std::endl;
    
    if (!g_state.failures.empty()) {
        std::cout << "\n❌ Failures:" << std::endl;
        for (const auto& failure : g_state.failures) {
            std::cout << "  " << failure << std::endl;
        }
    }
    
    bool all_passed = (g_state.tests_passed == g_state.tests_run);
    std::cout << "\n" << (all_passed ? "🎉 ALL TESTS PASSED!" : "💔 SOME TESTS FAILED") << std::endl;
    
    return all_passed ? 0 : 1;
}

// Helper macros for setup/teardown without inheritance
#define SETUP_TEST() \
    static void setup_test_environment()

#define TEARDOWN_TEST() \
    static void teardown_test_environment()

#define RUN_SETUP() setup_test_environment()
#define RUN_TEARDOWN() teardown_test_environment()

} // namespace labdb9_test

// Main test runner macro for minimal main() functions
#define LABDB9_TEST_MAIN() \
    int main() { \
        return labdb9_test::run_all_tests(); \
    }