// test_harness.h - Minimal test framework for LabDb9
// Matches existing LabDb9 test pattern - absolute minimalism
#pragma once

#include <iostream>
#include <string>
#include <cassert>
#include <vector>
#include <functional>
#include <sstream>

namespace labdb9_test {

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
// Global test state
//-----------------------------------------------------------------------------
struct TestState {
    std::string current_test;
    bool current_test_passed{true};
    int tests_run{0};
    int tests_passed{0};
    int assertions_run{0};
    int assertions_passed{0};
    std::vector<std::string> failures;
    int verbosity{1}; // 0=minimal, 1=normal, 2=verbose
};

// Global test state instance
inline TestState g_state;

//-----------------------------------------------------------------------------
// Test case structure
//-----------------------------------------------------------------------------
struct TestCase {
    std::string name;
    std::function<void()> func;
};

// Global test registry
inline std::vector<TestCase> g_tests;

//-----------------------------------------------------------------------------
// Core assertion macros (gtest-style but minimal)
//-----------------------------------------------------------------------------
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

// Test registration and execution
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

} // namespace labdb9_test

// Main test runner macro
#define LABDB9_TEST_MAIN() \
    int main() { \
        return labdb9_test::run_all_tests(); \
    }