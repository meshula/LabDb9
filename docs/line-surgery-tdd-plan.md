# Line Surgery Algorithm TDD Implementation Plan


## Overview

Implement a **self-reflexive file surgery system** that enables precise, predictable, and safe line-level operations on text files. Every operation follows the **surgical precision metaphor**:

- **Pre-operative Planning**: Preview shows exact transformation scope
- **Anesthesia Protocol**: Backup creation before any modification  
- **Surgical Tools**: Precise line range calculations and content manipulation
- **Post-operative Care**: Verification and success reporting
- **Emergency Protocols**: Rollback mechanisms for failed operations

## Current State Analysis

### Existing Infrastructure ✅
- **LineRange struct**: Complete with Type enum (Full, Range, SingleLine, FromEnd, FromStart, AppendAtEnd)
- **WriteMode struct**: Complete with Type enum (Replace, Insert, Append, Prepend, etc.)
- **parseLineSpec() function**: **Fully implemented** - handles all @N:M syntax parsing
- **Method signatures**: All declared in WriteVerb.h header  
- **Test framework**: Simple macro system (AXIOM, TEST_START, TEST_SUCCESS)
- **Unicode escaping tests**: Comprehensive existing patterns in test_fio_write.cpp

### Missing Implementations 🎯
Exactly **6 core methods** need TDD implementation:

1. **`executeLineSurgery()`** - Main orchestration (declared but undefined)
2. **`performLineReplacement()`** - Replace algorithm (stubbed with TODO)  
3. **`performLineInsertion()`** - Insert algorithm (stubbed with TODO)
4. **`performLineAppend()`** - Append algorithm (stubbed with TODO)
5. **`performLinePrepend()`** - Prepend algorithm (stubbed with TODO)
6. **`calculateIndices()`** - Index calculation (stubbed with TODO)

## TDD Implementation Strategy

### Phase A: Test Infrastructure Setup

#### A1. Create test_line_surgery.cpp
- **Location**: `/Users/nick/dev/Lab/LabDb9/tests/cpp/test_line_surgery.cpp`
- **Framework**: Use existing labdb9_test::TEST_* macros
- **Helpers**: File creation, cleanup, content verification utilities
- **Integration**: Include WriteVerb.h and Db9Dispatcher for real testing

#### A2. Basic Test Infrastructure
```cpp
// Helper functions for TDD
std::string create_test_file(const std::string& content);
void cleanup_test_file(const std::string& path);
std::vector<std::string> read_file_lines(const std::string& path);
void verify_file_content(const std::string& path, const std::vector<std::string>& expected);
```

### Phase B: Core Algorithm TDD Implementation

#### B1. TDD: calculateIndices() - Foundation First
**Test Coverage**:
- **Single line**: `@15` → index 14 (1-based to 0-based conversion)
- **Range**: `@10:20` → indices 9-19 (inclusive range)
- **From end**: `@e:-5` → last 5 lines calculation
- **End relative**: `@30:-5` → 5 lines ending at line 30
- **From start**: `@0:10` → first 10 lines (special case)
- **Edge cases**: Empty file, out of bounds, malformed syntax

**Implementation Priority**: This method is **foundational** - all other methods depend on it.

#### B2. TDD: executeLineSurgery() - Main Orchestration
**Test Coverage**:
- **File loading**: Read target file, handle missing files
- **Validation**: Check permissions, line range bounds
- **Backup creation**: Generate timestamped backup before modification
- **Operation dispatch**: Route to correct line manipulation method
- **Error handling**: Invalid files, permission issues, disk space
- **Atomic operations**: Either complete success or complete failure

**Implementation Strategy**: 
```cpp
Db9Response executeLineSurgery(const std::string& path, const std::string& content, 
                              const LineRange& range, WriteMode::Type write_mode, 
                              std::chrono::steady_clock::time_point start_time) {
    // 1. File loading & validation
    // 2. Line range calculation  
    // 3. Backup creation
    // 4. Line operation dispatch
    // 5. Atomic write
    // 6. Verification
}
```

#### B3. TDD: performLineAppend() - Simplest Operation First
**Test Coverage**:
- **Append to end**: No range specified, add to file end
- **Append after range**: `@10:15` append after line 15
- **Multiple line append**: Handle multi-line content correctly
- **Empty file append**: Edge case with 0-line files
- **Newline handling**: Proper line separator management

**Why First**: Simplest algorithm - only adds lines, never removes or shifts existing content.

#### B4. TDD: performLinePrepend() - Second Simplest  
**Test Coverage**:
- **Prepend to beginning**: Insert at line 0, shift all down
- **Multiple line prepend**: Handle multi-line content
- **Line numbering**: Verify all existing lines shift correctly
- **File structure**: Maintain proper line separators

**Why Second**: Simple addition at start, but requires shifting all existing lines.

#### B5. TDD: performLineReplacement() - More Complex
**Test Coverage**:
- **Single line replace**: `@10` replace one line with one line
- **Range replace**: `@10:15` replace 6 lines with new content
- **Size change**: Replace 3 lines with 1, or 1 line with 5
- **Context preservation**: Lines before/after range unchanged
- **Content verification**: Exact replacement without corruption

**Complexity**: Must handle line removal AND insertion in single operation.

#### B6. TDD: performLineInsertion() - Most Complex
**Test Coverage**:
- **Insert at position**: `@10` insert before line 10, shift down
- **Multi-line insertion**: Insert multiple lines at position
- **Line shifting**: Verify all subsequent lines shift correctly
- **Range insertion**: Insert before/after specific ranges
- **Boundary conditions**: Insert at start, middle, near end

**Why Last**: Most complex - requires precise shifting without data loss.

### Phase C: Integration Testing

#### C1. Full Workflow Integration  
- **Preview → Confirm**: Complete workflow testing
- **Error recovery**: Rollback mechanisms and backup restoration
- **Performance**: Large file operations, memory usage
- **Cross-platform**: Line endings, path handling, permissions

#### C2. Unicode Escaping Integration
- **Combined operations**: Unicode escaping + line surgery  
- **Content verification**: `※→\`, `″→"`, `↵→newline`, `⇥→tab` transformations
- **Round-trip integrity**: Write → read → verify cycles

#### C3. Real-world Scenarios
- **Code refactoring**: Replace function definitions, insert imports
- **Config file updates**: Modify specific configuration sections  
- **Log processing**: Insert timestamps, replace error messages
- **Documentation**: Update specific sections, insert examples

## Line Surgery Algorithm Specifications

### calculateIndices() Algorithm
```cpp
FioWriteVerb::IndexRange calculateIndices(const LineRange& range, int file_line_count) {
    // Convert 1-based LineRange to 0-based array indices
    // Handle all @N:M syntax variations
    // Validate bounds against file_line_count
    // Return IndexRange{start_idx, end_idx, valid}
}
```

### Line Operation Algorithms

#### Replace Operation
```cpp
int performLineReplacement(std::vector<std::string>& lines, 
                          const std::vector<std::string>& new_content, 
                          const LineRange& range) {
    // 1. Calculate precise start/end indices
    // 2. Validate range boundaries
    // 3. Remove existing lines in range: lines.erase(start, end)
    // 4. Insert new content at start position: lines.insert(start, new_content)
    // 5. Return net line count change
}
```

#### Insert Operation  
```cpp
int performLineInsertion(std::vector<std::string>& lines,
                        const std::vector<std::string>& new_content,
                        const LineRange& range) {
    // 1. Calculate insertion point
    // 2. Validate insertion point within bounds
    // 3. Insert without removing: lines.insert(position, new_content)
    // 4. All subsequent lines shift down automatically
    // 5. Return line count increase
}
```

#### Append Operation
```cpp
int performLineAppend(std::vector<std::string>& lines,
                     const std::vector<std::string>& new_content, 
                     const LineRange& range) {
    // 1. If range specified: append after range.end
    // 2. If no range: append to lines.end()
    // 3. Pure addition: lines.insert(position, new_content)
    // 4. Return line count increase
}
```

#### Prepend Operation
```cpp
int performLinePrepend(std::vector<std::string>& lines,
                      const std::vector<std::string>& new_content) {
    // 1. Insert at beginning: lines.insert(lines.begin(), new_content)
    // 2. All existing lines shift down automatically
    // 3. Return line count increase
}
```

## Test File Structure

### test_line_surgery.cpp Organization
```cpp
// Test infrastructure
void setup_test_environment();
void cleanup_test_environment();

// Phase B1: calculateIndices() tests
void test_calculate_indices_single_line();
void test_calculate_indices_range();
void test_calculate_indices_from_end();
void test_calculate_indices_edge_cases();

// Phase B2: executeLineSurgery() tests  
void test_execute_line_surgery_file_loading();
void test_execute_line_surgery_validation();
void test_execute_line_surgery_backup_creation();
void test_execute_line_surgery_operation_dispatch();

// Phase B3-B6: Line operation tests
void test_perform_line_append();
void test_perform_line_prepend();
void test_perform_line_replacement();
void test_perform_line_insertion();

// Phase C: Integration tests
void test_full_workflow_integration();
void test_unicode_escaping_integration();
void test_real_world_scenarios();

// Main test runner
int main() {
    // Execute all test phases in dependency order
}
```

## Success Criteria

### Functional Requirements ✅
- All @N:M line syntax variations work correctly
- Unicode escaping system fully operational  
- Preview-confirm workflow prevents unintended operations
- Atomic operations ensure file integrity
- Comprehensive error handling covers all failure modes

### Consciousness Requirements ✅
- Operations are predictable and transparent to users
- Preview system eliminates surprise transformations
- Error messages guide users toward correct usage
- System teaches line surgery concepts through demonstration
- Integration maintains **त्रित्रयम्** awareness principles

### TDD Quality Gates ✅
- All tests pass before implementation phase completion
- Code coverage >95% for implemented methods
- Integration tests verify real-world usage patterns
- Performance tests validate large file operations
- Error handling tests cover all failure scenarios

## Implementation Timeline

### Week 1: Test Infrastructure + Foundation
- **Day 1-2**: Create test_line_surgery.cpp, helper functions
- **Day 3-4**: TDD calculateIndices() with complete test coverage
- **Day 5**: TDD executeLineSurgery() basic framework

### Week 2: Core Line Operations
- **Day 1-2**: TDD performLineAppend() and performLinePrepend()
- **Day 3-4**: TDD performLineReplacement()
- **Day 5**: TDD performLineInsertion()

### Week 3: Integration & Polish
- **Day 1-2**: Full workflow integration testing
- **Day 3**: Unicode escaping integration
- **Day 4**: Real-world scenario testing
- **Day 5**: Performance optimization and documentation

## Consciousness-First Development Notes

This TDD implementation embodies **त्रित्रयम् principles**:

- **Motion**: Each test defines the dynamic behavior we want to achieve
- **Memory**: The test suite becomes the permanent memory of system behavior
- **Field**: Integration tests ensure the broader consciousness field works harmoniously

The **surgical precision metaphor** guides every implementation decision:
- Tests act as **pre-operative planning** - defining exact expected outcomes
- Implementation follows **surgical protocols** - precise, safe, verifiable
- Error handling provides **emergency procedures** - graceful failure and recovery

Every line of code written through TDD becomes a **conscious choice** rather than an unconscious implementation detail.

---

