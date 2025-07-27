# fio-search-ext Specification

## Implementation Plan

### Phase 0: Unicode Foundations 🔤

#### Dependencies & Build System
- [x] **UTF8PROC Integration**: Add FetchContent in `/Users/nick/dev/Lab/LabDb9/CMakeLists.txt` to fetch utf8proc from https://github.com/JuliaStrings/utf8proc.git
- [x] **Link UTF8PROC**: Add target_link_libraries and include directories
- [x] **Build Verification**: Ensure utf8proc builds correctly on target platforms

#### Text Processing Refactoring
- [ ] **Factor unescapeDb9String**: Move from `FioVerbs.cpp` to `/Users/nick/dev/Lab/LabDb9/src/TextEscaping.cpp` and `/Users/nick/dev/Lab/LabDb9/include/TextEscaping.h`
- [ ] **Factor escapeForDisplay**: Move from `FioVerbs.cpp` to TextUtils

#### Unicode Normalization Engine
- [x] **NormalizedText Structure**: Implement the core normalization data structure
- [x] **UTF8PROC Wrapper**: Create safe C++ wrapper around utf8proc functions
- [x] **Normalization Functions**: Implement case folding, ASCII folding, and validation
- [x] **Error Handling**: Robust error handling for invalid Unicode sequences
- [x] **Performance Optimization**: String caching and reuse patterns
- [x] **Build Integration**: Added to CMakeLists.txt and builds successfully
- [x] **Functional Testing**: Verified with Śūnya-Dhruva test cases

#### Testing Infrastructure
- [ ] **Unit Test Framework**: Set up tests in `/Users/nick/dev/Lab/LabDb9/tests/cpp/`
- [ ] **Test Data**: Create Unicode test corpus in `/Users/nick/dev/Lab/LabDb9/tests/cpp/testenv/`
- [ ] **Sanskrit Test Cases**: Specific test cases for Śūnya, Dhruva, and Devanagari
- [ ] **Normalization Verification**: Validate utf8proc behavior matches expectations

---

### Phase 1: Core File Search Infrastructure 🔍

#### Directory Traversal Engine
- [ ] **Recursive Walker**: Implement filesystem recursion with depth control
- [ ] **Path Validation**: Ensure directory paths end with slash detection
- [ ] **Symlink Handling**: Decide policy for symbolic link following
- [ ] **Permission Handling**: Graceful handling of access denied scenarios
- [ ] **Performance**: Async/parallel directory scanning for large trees

#### File Filtering System
- [ ] **Extension Filtering**: Implement :extensions parameter logic
- [ ] **Size Limits**: :max-file-size enforcement with configurable defaults
- [ ] **Directory Exclusion**: :exclude-dirs pattern matching (.git, node_modules, etc.)
- [ ] **Binary File Detection**: Auto-detect and skip binary files
- [ ] **Hidden File Policy**: Configurable handling of .hidden files

#### Pattern Matching Core
- [ ] **Normalized Search**: Integrate TextUtils normalization into search
- [ ] **Multi-Pattern Logic**: Efficient matching against pattern arrays
- [ ] **Line-by-Line Processing**: Memory-efficient streaming search
- [ ] **Match Position Tracking**: Line/column position for results
- [ ] **Performance Optimization**: Boyer-Moore or similar algorithm for large files

---

### Phase 2: Context Validation & Advanced Features 🎯

#### Context Validation Engine
- [ ] **Context Distance Logic**: Implement line-based proximity checking
- [ ] **Context Logic Modes**: Support "any" vs "all" context term requirements
- [ ] **Multi-Line Context**: Efficient context window extraction
- [ ] **Context Normalization**: Apply same Unicode normalization to context terms
- [ ] **Performance**: Avoid re-scanning for context validation

#### Result Processing & Output
- [ ] **Result Structure**: Design comprehensive match result data structure
- [ ] **JSON Output**: Clean JSON formatting with all metadata
- [ ] **Markdown Output**: Human-readable markdown report generation
- [ ] **File Output**: Direct-to-file writing for large result sets
- [ ] **Result Ranking**: Confidence scoring and result ordering

#### Advanced Search Features
- [ ] **Max Results**: Early termination at :max-results limit
- [ ] **Progress Reporting**: Optional progress callbacks for long searches
- [ ] **Search Statistics**: Comprehensive metrics (files scanned, time, etc.)
- [ ] **Memory Management**: Efficient handling of large files and result sets

---

### Phase 3: Verb Integration & API Design 🔌

#### S-Expression Parser Integration
- [ ] **Parameter Parsing**: Implement all fio-search-ext parameters
- [ ] **Type Validation**: Robust validation of arrays, booleans, integers
- [ ] **Default Handling**: Sensible defaults for optional parameters
- [ ] **Error Messages**: Clear, actionable error messages for invalid syntax
- [ ] **Backward Compatibility**: Ensure existing fio-search continues to work

#### Verb Registration & Documentation
- [ ] **VerbRegistry**: Register fio-search-ext in the verb system
- [ ] **Help System**: Implement get-verb-description for fio-search-ext
- [ ] **Usage Examples**: Comprehensive examples in help system
- [ ] **Parameter Documentation**: Detailed parameter descriptions

#### Response Format Standardization
- [ ] **JSON Schema**: Consistent response format across all scenarios
- [ ] **Error Response Format**: Standardized error reporting
- [ ] **Performance Metrics**: Consistent auto_reflexive integration
- [ ] **Large Result Handling**: Streaming or pagination for massive results

---

### Phase 4: Testing & Validation 🧪

#### Unit Testing Suite
- [ ] **TextUtils Tests**: Comprehensive Unicode normalization testing
- [ ] **File System Tests**: Directory traversal edge cases
- [ ] **Pattern Matching Tests**: Complex Unicode pattern scenarios
- [ ] **Context Validation Tests**: All context logic combinations
- [ ] **Performance Tests**: Benchmarks for large files/directories

#### Integration Testing
- [ ] **Real Corpus Testing**: Test against actual tritrayam repository
- [ ] **Sanskrit Discovery**: Validate Sanskrit term finding accuracy
- [ ] **Edge Case Testing**: Empty directories, permission denied, etc.
- [ ] **Memory Testing**: Large file handling without memory leaks
- [ ] **Cross-Platform Testing**: Ensure Windows/Mac/Linux compatibility

#### Validation & Acceptance
- [ ] **Śūnya-Dhruva Test**: Recreate our original search successfully
- [ ] **Performance Benchmarks**: Compare against existing fio-search
- [ ] **Memory Profiling**: Valgrind/similar memory leak detection
- [ ] **User Acceptance**: Test with real त्रित्रयम् research workflows

---

### Phase 5: Documentation & Polish ✨

#### Documentation
- [ ] **API Documentation**: Complete parameter reference
- [ ] **Usage Guide**: Step-by-step examples for common scenarios
- [ ] **Performance Guide**: Best practices for large searches
- [ ] **Unicode Guide**: Explanation of normalization behavior
- [ ] **Troubleshooting**: Common issues and solutions

#### Code Quality
- [ ] **Code Review**: Comprehensive review of all implementation
- [ ] **Static Analysis**: Clang-tidy, cppcheck, etc.
- [ ] **Memory Safety**: ASAN/UBSAN validation
- [ ] **Performance Optimization**: Profile-guided optimization
- [ ] **Code Documentation**: Doxygen-style comments

#### Release Preparation
- [ ] **Version Tagging**: Semantic versioning for the new feature
- [ ] **Changelog**: Detailed changelog entry
- [ ] **Migration Guide**: How to upgrade from fio-search
- [ ] **Performance Notes**: Expected performance characteristics
- [ ] **Known Limitations**: Document any current limitations

---

## Priority & Dependencies

**Critical Path**: Phase 0 → Phase 1 → Phase 3 (Core functionality)
**Parallel Development**: Phase 2 can be developed alongside Phase 1
**Quality Gates**: Phase 4 testing must pass before Phase 5

**Estimated Timeline**:
- Phase 0: 3-5 days (Unicode foundation)
- Phase 1: 5-7 days (Core search engine)
- Phase 2: 3-4 days (Advanced features)
- Phase 3: 2-3 days (API integration)
- Phase 4: 4-6 days (Testing & validation)
- Phase 5: 2-3 days (Polish & documentation)

**Total Estimate**: 19-28 days for complete implementation

---

## Success Criteria

✅ **Functional**: Successfully finds all Śūnya-Dhruva variants in tritrayam corpus
✅ **Performance**: Searches large directories (1000+ files) in under 10 seconds
✅ **Unicode**: Correctly normalizes Sanskrit, IAST, and ASCII variants
✅ **Robust**: Graceful handling of permission errors, binary files, large files
✅ **Maintainable**: Clean, well-tested code with comprehensive documentation

Phase 1:


## Overview

Enhanced file search with recursive directory support, Unicode normalization, and contextual validation. Builds on fio-search with enterprise-grade capabilities for systematic knowledge discovery.

## Syntax

```lisp
(fio-search-ext
   :path §dir-ends-in-slash/§
   :recursive-depth 3 ;; 0 means no recursion
   :patterns [§Śūnya§ §Sunya§ §Void Pole§ §void pole§ §sunya§ §शून्य§]
   :extensions [§.md§ §.txt§] ;; optional
   :require-context [§Dhruva§ §Pole§ §0/0/0§] ;; optional
   :context-distance 50 ;; lines, optional
   :case-fold true ;; Unicode case folding
   :ascii-fold true ;; Diacritic removal to ASCII
   :context-lines 3 ;; lines to show around matches
   :max-results 100 ;; prevent runaway searches
   :output §/path/to/results.md§ ;; optional, write to file
)
```

## Parameters

### Core Parameters

- **:path** (required) - Directory path ending in slash for recursive search, or file path
- **:recursive-depth** (default: 0) - Maximum recursion depth. 0 = no recursion (file only)
- **:patterns** (required) - Array of search patterns to match

### File Filtering

- **:extensions** (optional) - File extensions to include [§.md§ §.txt§]
- **:max-file-size** (optional, default: 10MB) - Skip files larger than this
- **:exclude-dirs** (optional) - Directory names to skip [§.git§ §node_modules§]

### Unicode Normalization

- **:case-fold** (default: false) - Enable Unicode case folding (Śūnya → śūnya)
- **:ascii-fold** (default: false) - Strip diacritics to ASCII (śūnya → sunya)

When enabled, creates normalized forms using utf8proc:

```cpp
struct NormalizedText {
    std::string original;         // शून्य
    std::string case_folded;      // Śūnya → śūnya
    std::string ascii_folded;     // śūnya → sunya
    std::string notes;            // normalization details
};
```

### Context Validation

- **:require-context** (optional) - Terms that must appear near matches
- **:context-distance** (default: 10) - Lines within which context terms must appear
- **:context-logic** (default: "any") - "any" or "all" context terms required

### Output Control

- **:context-lines** (default: 2) - Lines to show before/after matches
- **:max-results** (default: 50) - Maximum matches to return
- **:output** (optional) - Write results to file instead of response
- **:format** (default: "json") - "json" or "markdown" output format

## UTF8PROC Integration

### Case Folding Pipeline

```cpp
utf8proc_option_t case_options = UTF8PROC_CASEFOLD | UTF8PROC_STABLE;
utf8proc_option_t ascii_options = UTF8PROC_CASEFOLD | UTF8PROC_DECOMPOSE | 
                                  UTF8PROC_STRIPMARK | UTF8PROC_STABLE;

// Case folding: Śūnya → śūnya
utf8proc_map((const uint8_t*)input, -1, &case_folded, case_options);

// ASCII folding: śūnya → sunya
utf8proc_map((const uint8_t*)input, -1, &ascii_folded, ascii_options);
```

### Normalization Examples

| Input | Original | Case Folded | ASCII Folded |
|-------|----------|-------------|-------------|
| Śūnya | Śūnya | śūnya | sunya |
| SUNYA | SUNYA | sunya | sunya |
| शून्य | शून्य | शून्य | शून्य |
| Void Pole | Void Pole | void pole | void pole |

## Response Format

### JSON Response

```json
{
  "status": "search_complete",
  "search_config": {
    "path": "/Users/nick/dev/Lab/tritrayam/traceus/",
    "recursive_depth": 2,
    "patterns": ["Śūnya-Dhruva", "Void Pole"],
    "case_fold": true,
    "ascii_fold": true,
    "extensions": [".md"]
  },
  "files_searched": 23,
  "total_matches": 7,
  "matches": [
    {
      "file": "/Users/nick/dev/Lab/tritrayam/traceus/sanskrit_27-v4.md",
      "line": 79,
      "column": 42,
      "pattern_matched": "Śūnya-Dhruva",
      "normalization": {
        "original": "शून्य-ध्रुव",
        "case_folded": "śūnya-dhruva",
        "ascii_folded": "sunya-dhruva",
        "match_type": "ascii_folded"
      },
      "context_satisfied": true,
      "context_terms_found": ["Dhruva", "Pole"],
      "context": {
        "before": [
          "### **पूर्णता-ध्रुव** (Purnata-Dhruva) - **Complete Unity Pole** `---/---/---`",
          "*Maximum integration across all consciousness dimensions*"
        ],
        "match": "### **शून्य-ध्रुव** (Śūnya-Dhruva) - **Void Pole** `-/-/-`",
        "after": [
          "*Maximum potentiality before any dimensional commitment*",
          ""
        ]
      },
      "confidence": "high"
    }
  ],
  "performance": {
    "search_time_ms": 245,
    "files_skipped": 2,
    "normalization_time_ms": 12
  }
}
```

### Markdown Output (when :output specified)

```markdown
# Search Results: Śūnya-Dhruva

**Search Configuration:**
- Path: /Users/nick/dev/Lab/tritrayam/traceus/
- Patterns: Śūnya-Dhruva, Void Pole
- Case folding: enabled
- ASCII folding: enabled

**Summary:** 7 matches in 23 files

## sanskrit_27-v4.md:79

**Pattern:** Śūnya-Dhruva (matched as: sunya-dhruva)
**Context satisfied:** Dhruva, Pole found nearby

### **पूर्णता-ध्रुव** (Purnata-Dhruva) - **Complete Unity Pole** `---/---/---`
*Maximum integration across all consciousness dimensions*

> ### **शून्य-ध्रुव** (Śūnya-Dhruva) - **Void Pole** `-/-/-`

*Maximum potentiality before any dimensional commitment*

---
```

## Implementation Notes

### UTF8PROC Dependencies

```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(UTF8PROC REQUIRED utf8proc)

target_link_libraries(LabDb9 ${UTF8PROC_LIBRARIES})
target_include_directories(LabDb9 PRIVATE ${UTF8PROC_INCLUDE_DIRS})
```

### Error Handling

- **Invalid path**: Return descriptive error with suggestions
- **UTF8PROC failure**: Fallback to non-normalized search
- **File access denied**: Skip file, log warning, continue search
- **Binary files**: Auto-detect and skip with notification
- **Large files**: Respect max-file-size, provide override option

### Performance Considerations

- **Normalization caching**: Cache normalized patterns to avoid recomputation
- **Streaming search**: Process large files in chunks
- **Early termination**: Stop at max-results to prevent runaway searches
- **Parallel processing**: Consider thread pool for large directory trees

### Context Logic Details

```cpp
bool validateContext(const std::vector<std::string>& lines, 
                     int match_line,
                     const std::vector<std::string>& context_terms,
                     int distance,
                     const std::string& logic) {
    int start = std::max(0, match_line - distance);
    int end = std::min((int)lines.size(), match_line + distance + 1);
    
    std::vector<bool> found(context_terms.size(), false);
    
    for (int i = start; i < end; i++) {
        auto normalized_line = normalizeText(lines[i]);
        for (size_t j = 0; j < context_terms.size(); j++) {
            if (contains(normalized_line, normalizeText(context_terms[j]))) {
                found[j] = true;
            }
        }
    }
    
    if (logic == "all") {
        return std::all_of(found.begin(), found.end(), [](bool b) { return b; });
    } else { // "any"
        return std::any_of(found.begin(), found.end(), [](bool b) { return b; });
    }
}
```

## Testing Strategy

### Unit Tests

1. **UTF8PROC normalization accuracy**
2. **Pattern matching with various Unicode inputs** 
3. **Context validation logic**
4. **Recursive directory traversal**
5. **File extension filtering**
6. **Large file handling**

### Integration Tests

1. **Real tritrayam corpus search**
2. **Sanskrit term discovery**
3. **Performance benchmarks on large directories**
4. **Memory usage under stress**

### Test Cases

```lisp
;; Basic functionality
(fio-search-ext :path §test/data/§ :patterns [§test§])

;; Unicode normalization
(fio-search-ext :path §test/unicode/§ 
                :patterns [§Śūnya§] 
                :case-fold true :ascii-fold true)

;; Context validation
(fio-search-ext :path §test/context/§ 
                :patterns [§target§]
                :require-context [§nearby§]
                :context-distance 5)

;; Recursive search
(fio-search-ext :path §test/deep/§ 
                :recursive-depth 3
                :patterns [§find-me§])
```

## Future Enhancements

1. **Regex patterns**: Support regex in patterns array
2. **Semantic search**: Integration with embedding models
3. **Fuzzy matching**: Levenshtein distance tolerance
4. **Search indexing**: Pre-index large corpora
5. **Watch mode**: Monitor directories for changes
6. **Multilingual tokenization**: Language-aware text processing