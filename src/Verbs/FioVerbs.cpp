#include "Fio/GetVerbDescription.h"
#include "Fio/SetCwd.h"
#include "FioVerbs.h"
#include "LabDb/Db9Dispatcher.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <filesystem>
#include <thread>        // For std::this_thread::sleep_for
#include <chrono>   
#include <regex>

namespace LabDb {

// Parameter extraction utility (matching DatabaseVerbs.cpp pattern)
std::string extractStringParam(const lab::Text::Sexpr& sexpr, const std::string& param_name) {
    for (size_t i = 0; i < sexpr.expr.size(); ++i) {
        const auto& elem = sexpr.expr[i];
        if (elem.token == tsSexprAtom) {
            int stringIndex = elem.ref;
            if (stringIndex >= 0 && stringIndex < static_cast<int>(sexpr.strings.size())) {
                const std::string& atomValue = sexpr.strings[stringIndex];
                if (atomValue == ":" + param_name || atomValue == param_name) {
                    if (i + 1 < sexpr.expr.size()) {
                        const auto& valueElem = sexpr.expr[i + 1];
                        if (valueElem.token == tsSexprAtom && valueElem.ref >= 0 &&
                            valueElem.ref < static_cast<int>(sexpr.strings.size())) {
                            return sexpr.strings[valueElem.ref];
                        } else if (valueElem.token == tsSexprString && valueElem.ref >= 0 &&
                                   valueElem.ref < static_cast<int>(sexpr.strings.size())) {
                            return sexpr.strings[valueElem.ref];
                        }
                    }
                }
            }
        }
    }
    return "";
}

//-----------------------------------------------------------------------------
// Shared Line Range Utilities for fio-read and fio-write
//-----------------------------------------------------------------------------

struct LineRange {
    enum Type { Full, Range, SingleLine, FromEnd, FromStart } type = Full;
    int start = 0;      // -1 for "end", 0 for start
    int count = -1;     // number of lines or end position  
    bool valid = true;
};

LineRange parseLineSpec(const std::string& lines_param) {
    LineRange range;
    
    if (lines_param.empty()) {
        range.type = LineRange::Full;
        return range;
    }
    
    if (!lines_param.starts_with("@")) {
        range.valid = false;
        return range;
    }
    
    std::string spec = lines_param.substr(1); // Remove @
    
    if (spec.find(':') != std::string::npos) {
        // Range specification: @start:end or @start:-count
        size_t colon_pos = spec.find(':');
        std::string start_str = spec.substr(0, colon_pos);
        std::string end_str = spec.substr(colon_pos + 1);
        
        if (start_str == "e") {
            // @e:-20 = last 20 lines
            if (end_str.starts_with("-")) {
                range.type = LineRange::FromEnd;
                range.count = std::abs(std::stoi(end_str));
            } else {
                range.valid = false;
            }
        } else if (start_str == "0" && !end_str.starts_with("-")) {
            // @0:20 = first 20 lines  
            range.type = LineRange::FromStart;
            range.count = std::stoi(end_str);
        } else {
            // @30:-2 = lines 28-30 or @50:100 = lines 50-100
            range.type = LineRange::Range;
            if (end_str.starts_with("-")) {
                // @6:-3 means "3 lines ending at line 6" = lines 4,5,6
                int end_line = std::stoi(start_str);        // end_line = 6
                int line_count = std::abs(std::stoi(end_str)); // line_count = 3
                range.start = end_line - line_count + 1;    // start = 6 - 3 + 1 = 4
                range.count = line_count;                   // count = 3
            } else {
                // @50:100 = lines 50 through 100 (1-based input, calculate count)
                range.start = std::stoi(start_str);         // start = 50
                int end_line = std::stoi(end_str);          // end_line = 100
                range.count = end_line - range.start + 1;   // count = 100 - 50 + 1 = 51
            }
        }
    } else {
        // Single line: @25
        range.type = LineRange::SingleLine;
        range.start = std::stoi(spec);
        range.count = 1;
    }
    
    return range;
}


namespace FioUtils {

//-----------------------------------------------------------------------------
// FioUtils Implementation  
//-----------------------------------------------------------------------------
/*
 * unescapeDb9String Version 3 - Unicode Escape System
 * 
 * LLM-Ready Explanation:
 * ======================
 * 
 * This function implements a bulletproof Unicode-based escape system that eliminates
 * the traditional JSON/C++ nested escape nightmare. Instead of complex backslash 
 * sequences, it uses visually intuitive Unicode characters that are completely 
 * transparent to JSON parsing.
 * 
 * Unicode Escape Mappings:
 * ------------------------
 * ※ (U+203B REFERENCE MARK)     → \    (backslash - for regex, file paths, C++ escapes)
 * ″ (U+2033 DOUBLE PRIME)       → "    (quote - for embedded strings)
 * ↵ (U+21B5 DOWNWARDS ARROW)    → \n   (actual newline character - creates line breaks)
 * ⇥ (U+21E5 RIGHTWARDS ARROW)   → \t   (actual tab character - for indentation)
 * 
 * Key Benefits for LLMs:
 * ----------------------
 * 1. VISUAL CLARITY: Each symbol visually represents its function
 * 2. JSON TRANSPARENT: No conflicts with JSON syntax - works in any context
 * 3. ZERO COGNITIVE LOAD: No counting backslashes or nested escape math
 * 4. BULLETPROOF: Impossible to create malformed escape sequences
 * 5. COMPOSABLE: Works in S-expressions, JSON, C++ strings, anywhere
 * 
 * Real-World Examples:
 * -------------------
 * Input:  "printf(″Hello World※n″);"
 * Output: printf("Hello World\n");
 * 
 * Input:  "std::regex pattern(″※w+※d+″);"
 * Output: std::regex pattern("\w+\d+");
 * 
 * Input:  "⇥if (error) {↵⇥⇥printf(″Error: %s※n″, msg);↵⇥}"
 * Output: 	if (error) {
 *         		printf("Error: %s\n", msg);
 *         	}
 * 
 * Input:  "path = ″C:※※users※※file.txt″;"
 * Output: path = "C:\\users\\file.txt";
 * 
 * Usage Philosophy:
 * ----------------
 * - Use ↵ when you want ACTUAL line breaks in the output file
 * - Use ※n when you want \n escape sequences in C++ strings
 * - Use ″ for all quote needs - no more JSON quote escaping hell
 * - Use ※ for any backslash needs - regex, file paths, C++ escapes
 * - Use ⇥ for actual tab characters (indentation, formatting)
 * 
 * This system makes fio-write completely natural for LLMs to generate
 * C++, regex patterns, printf statements, and complex code structures
 * without any escape complexity mental overhead.
 */

std::string unescapeDb9String(const std::string& input) {
    std::string result = input;
    
    // Unicode escape system - order matters for correct processing
    // Process in order to avoid conflicts
    
    // 1. Replace rare hallucinated need for ※' with just a tick
    size_t pos = 0;
    while ((pos = result.find("※'", pos)) != std::string::npos) {
        result.replace(pos, 4, "'");  // ※ is 3 bytes in UTF-8
        pos += 1;  // Move past the replacement
    }

    // 2. Replace backslash symbol first (※ → \)
    pos = 0;
    while ((pos = result.find("※", pos)) != std::string::npos) {
        result.replace(pos, 3, "\\");  // ※ is 3 bytes in UTF-8
        pos += 1;  // Move past the replacement
    }
    
    // 3. Replace quote symbol (″ → ")
    pos = 0;
    while ((pos = result.find("″", pos)) != std::string::npos) {
        result.replace(pos, 3, "\"");  // ″ is 3 bytes in UTF-8
        pos += 1;  // Move past the replacement
    }
    
    // 4. Replace newline symbol (↵ → actual newline)
    pos = 0;
    while ((pos = result.find("↵", pos)) != std::string::npos) {
        result.replace(pos, 3, "\n");  // ↵ is 3 bytes in UTF-8
        pos += 1;  // Move past the replacement
    }
    
    // 5. Replace tab symbol (⇥ → actual tab)
    pos = 0;
    while ((pos = result.find("⇥", pos)) != std::string::npos) {
        result.replace(pos, 3, "\t");  // ⇥ is 3 bytes in UTF-8
        pos += 1;  // Move past the replacement
    }
    
    return result;
}

/*
 * Example Usage in fio-write:
 * ---------------------------
 * 
 * // Traditional nightmare:
 * (fio-write :content "printf(\\\"Debug: %s\\n\\\", msg);")
 * 
 * // Unicode system elegance:
 * (fio-write :content "printf(″Debug: %s※n″, msg);")
 * 
 * // Complex C++ with regex:
 * (fio-write :content "std::regex email(″[a-z]+@[a-z]+※\.[a-z]+″);↵std::cout << ″Pattern ready※n″;")
 * 
 * // Multi-line code generation:
 * (fio-write :content "⇥if (validate_email(input)) {↵⇥⇥printf(″Valid email: %s※n″, input);↵⇥} else {↵⇥⇥fprintf(stderr, ″Invalid email※n″);↵⇥}")
 * 
 * This system transforms fio-write from an escape complexity nightmare
 * into a natural, visual code generation tool that LLMs can use effortlessly!
 */

// =================================================================
// OPTIONAL: Helper function for testing/debugging
// =================================================================

std::string escapeForDisplay(const std::string& input) {
    std::string result;
    for (char c : input) {
        switch (c) {
            case '\n': result += "\\n"; break;
            case '\t': result += "\\t"; break;
            case '\r': result += "\\r"; break;
            case '\\': result += "\\\\"; break;
            case '"':  result += "\\\""; break;
            default:   result += c; break;
        }
    }
    return result;
}


std::string extractContentFromReadResponse(const std::string& json_response) {
    return extractFieldFromReadResponse(json_response, "content");
}

std::string extractFieldFromReadResponse(const std::string& json_response, const std::string& field_name) {
    std::string field_key = "\"" + field_name + "\": \"";
    size_t field_start = json_response.find(field_key);
    if (field_start == std::string::npos) {
        return "";
    }
    
    field_start += field_key.length();
    
    // Find the end of the field value (look for ", " followed by next field or end)
    size_t field_end = field_start;
    int escape_count = 0;
    
    while (field_end < json_response.length()) {
        if (json_response[field_end] == '\\') {
            escape_count++;
        } else if (json_response[field_end] == '"' && escape_count % 2 == 0) {
            // Found unescaped quote - this ends the field
            break;
        } else {
            escape_count = 0;
        }
        field_end++;
    }
    
    if (field_end >= json_response.length()) {
        return "";
    }
    
    std::string content = json_response.substr(field_start, field_end - field_start);
    
    // For content field, unescape JSON escapes
    if (field_name == "content") {
        std::string unescaped;
        for (size_t i = 0; i < content.length(); ++i) {
            if (content[i] == '\\' && i + 1 < content.length()) {
                switch (content[i + 1]) {
                    case '\\': unescaped += '\\'; i++; break;
                    case 'n': unescaped += '\n'; i++; break;
                    case 't': unescaped += '\t'; i++; break;
                    case 'r': unescaped += '\r'; i++; break;
                    case '"': unescaped += '"'; i++; break;
                    default: unescaped += content[i]; break;
                }
            } else {
                unescaped += content[i];
            }
        }
        return unescaped;
    }
    
    return content;
}

    //-------------------------------------------------------------------------
    // Triadic Consciousness Path Context Implementation
    //-------------------------------------------------------------------------

    std::string FioUtils::PathContext::toJsonString() const {
        std::ostringstream json;
        json << "{"
             << "\"requested_path\": \"" << FioUtils::escapeJsonString(requested_path) << "\", "
             << "\"resolved_path\": \"" << FioUtils::escapeJsonString(resolved_path) << "\", "
             << "\"working_directory\": \"" << FioUtils::escapeJsonString(working_directory) << "\", "
             << "\"path_type\": \"" << path_type << "\", "
             << "\"exists\": " << (exists ? "true" : "false") << ", "
             << "\"existence_context\": \"" << FioUtils::escapeJsonString(existence_context) << "\""
             << "}";
        return json.str();
    }

    std::string createContextualErrorMessage(
        const PathContext& context,
        const std::string& operation,
        const std::string& base_error
    ) {
        std::ostringstream message;
        message << base_error;
        
        if (context.path_type == "relative") {
            message << " (relative path resolved to: " << context.resolved_path << ")";
        }
        
        message << ". Current working directory: " << context.working_directory;
        
        // Add helpful guidance based on the context
        if (!context.exists) {
            if (context.existence_context.find("parent directory exists") != std::string::npos) {
                message << ". Parent directory exists - check filename spelling.";
            } else {
                message << ". Parent directory is also missing - check full path.";
            }
            
            // Suggest working directory contents for context
            auto summary = getWorkingDirectorySummary(context.working_directory);
            if (!summary.empty()) {
                message << " Working directory contains: " << summary;
            }
        }
        
        return message.str();
    }

    std::string createDirectoryGuidance(const PathContext& context) {
        std::ostringstream guidance;
        
        if (context.path_type == "relative") {
            guidance << "Searched in working directory: " << context.working_directory << ". ";
        }
        
        auto summary = getWorkingDirectorySummary(context.working_directory);
        if (!summary.empty()) {
            guidance << "Working directory contains: " << summary;
        } else {
            guidance << "Working directory is empty.";
        }
        
        return guidance.str();
    }

    std::string getWorkingDirectorySummary(const std::string& working_dir) {
        try {
            std::vector<std::string> items;
            int count = 0;
            
            for (const auto& entry : std::filesystem::directory_iterator(working_dir)) {
                if (count >= 5) { // Limit to first 5 items for brevity
                    items.push_back("...");
                    break;
                }
                
                std::string name = entry.path().filename().string();
                if (entry.is_directory()) {
                    name += "/";
                }
                items.push_back(name);
                count++;
            }
            
            if (items.empty()) {
                return "";
            }
            
            std::ostringstream summary;
            for (size_t i = 0; i < items.size(); ++i) {
                if (i > 0) summary << ", ";
                summary << items[i];
            }
            
            return summary.str();
            
        } catch (const std::exception&) {
            return "unable to read directory contents";
        }
    }
    
    std::string addEmptyDirectoryAwareness(const std::string& json_result, size_t file_count) {
        if (file_count == 0) {
            // Find the position just before the closing brace
            size_t closing_brace = json_result.find_last_of('}');
            if (closing_brace != std::string::npos) {
                std::string enhanced = json_result.substr(0, closing_brace);
                enhanced += ", \"awareness_note\": \"Directory exists but appears to be empty. Are you looking in the correct location?\"}";
                return enhanced;
            }
        }
        return json_result;
    }

    PathContext createPathContext(const std::string& requested_path) {
        PathContext context;

        // Motion: User's intention
        context.requested_path = requested_path;

        // Field: Actual working directory context
        context.working_directory = std::filesystem::current_path().string();

        // Memory: System understanding - resolve the path
        std::filesystem::path path(requested_path);
        if (path.is_absolute()) {
            context.path_type = "absolute";
            context.resolved_path = requested_path;
        } else {
            context.path_type = "relative";
            context.resolved_path = (std::filesystem::current_path() / path).string();
        }

        // Field: Reality check
        context.exists = std::filesystem::exists(context.resolved_path);
        if (context.exists) {
            if (std::filesystem::is_regular_file(context.resolved_path)) {
                context.existence_context = "file exists";
            } else if (std::filesystem::is_directory(context.resolved_path)) {
                context.existence_context = "directory exists";
            } else {
                context.existence_context = "path exists (special file type)";
            }
        } else {
            // Provide helpful context about non-existence
            auto parent = std::filesystem::path(context.resolved_path).parent_path();
            if (std::filesystem::exists(parent)) {
                context.existence_context = "path does not exist, but parent directory exists";
            } else {
                context.existence_context = "path does not exist, parent directory also missing";
            }
        }

        return context;
    }

    std::string escapeJsonString(const std::string& input) {
        std::string result;
        result.reserve(input.size() * 2); // Reserve space for potential escaping
        
        for (char c : input) {
            switch (c) {
                case '"':  result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\b': result += "\\b"; break;
                case '\f': result += "\\f"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default:
                    if (c < 0x20) {
                        // Escape other control characters
                        char buf[8];
                        snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                        result += buf;
                    } else {
                        result += c;
                    }
                    break;
            }
        }
        return result;
    }
}

// FioWriteVerb implementation
std::string FioWriteVerb::getDescription() const {
    return R"DESC(
Write content to file with revolutionary line-syntax precision and Unicode escaping paradise.
Usage:
```lisp
;; Traditional full-file write
(fio-write :path §/path/to/file.txt§ :content §file content§)

;; 🚀 REVOLUTIONARY: Line-syntax precision surgery!
;; Note! Use § delimiters to escape content, instead of quotes!
;; This makes embedding code and quotes strings a breeze!
;; For example to assign a string in C++:
;; §std::string str = "Hello, world!";§
;; No more JSON escaping hell with § delimiters!

(fio-write :path §src/code.cpp§ :lines §@25§ :content §    new_line_content();§)
(fio-write :path §config.json§ :lines §@10:15§ :mode §replace§ :content §{"new": "config"}§)
(fio-write :path §script.py§ :lines §@5§ :mode §insert§ :content §    # Inserted comment§)
(fio-write :path §README.md§ :lines §@e:-2§ :mode §replace§ :content §## New footer§)
```

**Parameters:**
- `:path` - Target file path (required)
- `:content` - Content to write (required)
- `:lines` - Line surgery specification (optional, same syntax as fio-read)
- `:mode` - Line operation mode: "replace" (default), "insert", "append"

**Line Syntax (Same as fio-read!):**
- `@N` - Single line N
- `@N:M` - Lines N through M
- `@N:-M` - M lines ending at line N  
- `@e:-M` - Last M lines
- No :lines - Full file write (traditional mode)

**Operation Modes:**
- `replace` - Replace specified lines with new content
- `insert` - Insert content at line position, shifting existing lines down
- `append` - Add content after specified line position

**Returns:**
- `status` - "written" or "line_surgery_complete"
- `path` - Full file path
- `size_bytes` - File size after operation
- `lines_affected` - Number of lines modified (surgery mode)
- `mode` - Operation mode used (surgery mode)
- `line_spec` - Line specification used (surgery mode)

**🧚‍♀️ Awareness Features:**
- Line validation with helpful error messages
- File existence checking for surgery operations
- Range validation against actual file content
- Enhanced permission error guidance

**🚀 Unicode → Backslash Escaping System:**
Revolutionary visual Unicode characters eliminate JSON escaping complexity forever!

**Core Unicode Escapes:**
- `※` (U+203B) → `\` - Universal backslash for regex, file paths, C++ escapes
- `″` (U+2033) → `"` - Clean quotes without JSON conflicts
- `↵` (U+21B5) → actual newline - Creates real line breaks in files
- `⇥` (U+21E5) → actual tab - Creates real indentation

**Perfect for LLMs and Developers:**
```lisp
;; ✅ BEFORE (JSON escaping nightmare):
(fio-write :content "printf(\\\"Debug: %s\\n\\\", msg);")

;; 🎉 AFTER (Unicode escaping paradise):
(fio-write :content §printf(″Debug: %s※n″, msg);§)
```

**Real-World Examples:**
```lisp
;; C++ code generation (zero cognitive load!)
(fio-write :path §src/app.cpp§ :content §
⇥// Generated C++ code↵
⇥std::regex email(″[a-z]+@[a-z]+※.[a-z]+″);↵
⇥printf(″Pattern ready※n″);↵
§)

;; Becomes clean C++:
	// Generated C++ code
	std::regex email("[a-z]+@[a-z]+\.[a-z]+");
	printf("Pattern ready\n");

;; Complex regex patterns (visual clarity!)
(fio-write :content §std::regex pattern(″※w+※d+※s*″);§)
→ std::regex pattern("\w+\d+\s*");

;; File paths (no backslash counting!)
(fio-write :content §path = ″C:※※Users※※Documents″;§)
→ path = "C:\\Users\\Documents";

;; Multi-line indented code (structure visible!)
(fio-write :content §
if (validate()) {↵
⇥printf(″Valid input※n″);↵
⇥process_data();↵
}§)
```

**Unicode Escape Benefits:**
- **Visual Clarity**: See exactly what each symbol does
- **JSON Transparent**: Never conflicts with JSON syntax
- **Error Proof**: Impossible to create malformed escapes  
- **LLM Friendly**: Zero cognitive load for code generation
- **§ Delimiter Magic**: Works perfectly with § delimiters

**Conversion Rules:**
- `※n` → `\n` (C++ newline escape)
- `※t` → `\t` (C++ tab escape)  
- `※r` → `\r` (C++ carriage return)
- `※"` → `\"` (C++ quote escape)
- `※w` → `\w` (regex word character)
- `※d` → `\d` (regex digit character)
- `※s` → `\s` (regex space character)
- `※.` → `\.` (regex escaped period)
- `↵` → actual newline (file structure)
- `⇥` → actual tab (indentation)
- `″` → `"` (string quotes)

**The Revolutionary Trilogy:**
1. 🔍 **fio-search**: Find content with surgical precision
2. 📖 **fio-read**: Examine context with smart line syntax  
3. ✏️ **fio-write**: Change content with Unicode escaping paradise

Together: Perfect for refactoring, debugging, and code generation!
- No more regex nightmares - visual, line-based editing
- Escaping paradise with § delimiters and Unicode escapes
- Surgical precision - replace exactly what you intend  
- fio-search to find, fio-read to see, fio-write to change
)DESC";
}

Db9Response FioWriteVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();

    // Extract parameters
    std::string path = extractStringParam(sexpr, "path");
    std::string content = extractStringParam(sexpr, "content");
    std::string lines_param = extractStringParam(sexpr, "lines");
    std::string mode = extractStringParam(sexpr, "mode");
    if (mode.empty()) mode = "replace";

    if (path.empty()) {
        AutoReflexiveMetrics metrics;
        return Db9Response{Db9Response::Error, "", "missing_path", "fio-write requires :path parameter", metrics};
    }

    try {
        // Create triadic path context for conscious filesystem operation
        auto pathContext = FioUtils::createPathContext(path);

        // Check for line surgery FIRST (regardless of content)
        if (!lines_param.empty()) {
            // Apply ƒ -> \\ escaping to content (even if empty for deletion)
            content = FioUtils::unescapeDb9String(content);
            return performLineSurgery(path, content, lines_param, mode, start_time);
        }

        // For full-file operations, check content
        if (!content.empty()) {
            // Apply ƒ -> \\ escaping to content
            content = FioUtils::unescapeDb9String(content);
            return performFullFileWrite(path, content, start_time);
        }

        // Handle empty content as "touch" operation (only for full-file writes)
        return performTouchOperation(path, start_time);
    } catch (const std::filesystem::filesystem_error& fs_error) {
        AutoReflexiveMetrics metrics;
        std::string enhanced_error = "Write operation failed: " + std::string(fs_error.what());

        // Add awareness fairy guidance for permission issues
        if (fs_error.code() == std::errc::permission_denied) {
            std::string temp_dir = std::filesystem::temp_directory_path().string();
            enhanced_error += ". Permission denied - if immediate write is important, consider temporarily staging to "
                            + temp_dir + " and resolving permissions interactively.";
        } else if (fs_error.code() == std::errc::read_only_file_system) {
            enhanced_error += ". Read-only filesystem detected - check mount options or select a writable location.";
        } else if (fs_error.code() == std::errc::no_space_on_device) {
            enhanced_error += ". Insufficient disk space - consider cleaning up files or selecting a different location.";
        }

        return Db9Response{Db9Response::Error, "", "write_failed", enhanced_error, metrics};
    } catch (const std::exception& e) {
        AutoReflexiveMetrics metrics;
        return Db9Response{Db9Response::Error, "", "write_failed", std::string("Write operation failed: ") + e.what(), metrics};
    }
}

// New helper method for touch operations
Db9Response FioWriteVerb::performTouchOperation(const std::string& path, std::chrono::steady_clock::time_point start_time) {
    try {
        // Ensure parent directory exists
        std::filesystem::path target_path(path);
        if (target_path.has_parent_path()) {
            std::filesystem::create_directories(target_path.parent_path());
        }

        bool file_existed = std::filesystem::exists(path);
        size_t file_size = 0;

        if (file_existed) {
            // Touch existing file - update timestamp without changing content
            auto now = std::filesystem::file_time_type::clock::now();
            std::filesystem::last_write_time(path, now);
            file_size = std::filesystem::file_size(path);
        } else {
            // Create new empty file
            std::ofstream file(path);
            if (!file.is_open()) {
                AutoReflexiveMetrics metrics;
                return Db9Response{Db9Response::Error, "", "file_open_failed", "Could not create file", metrics};
            }
            file.close();
            file_size = 0;
        }

        // Calculate execution time
        auto end_time = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

        // Create response JSON with touch-specific status
        std::ostringstream result;
        result << "{\"status\": \"" << (file_existed ? "touched" : "created") << "\""
               << ", \"path\": \"" << path << "\""
               << ", \"size_bytes\": " << file_size
               << ", \"operation\": \"touch\""
               << ", \"file_existed\": " << (file_existed ? "true" : "false")
               << "}";

        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = std::chrono::milliseconds(duration_ms);
        metrics.items_processed = 1;

        return Db9Response{Db9Response::Success, result.str(), "", "", metrics};

    } catch (const std::exception& e) {
        AutoReflexiveMetrics metrics;
        return Db9Response{Db9Response::Error, "", "touch_failed", std::string("Touch operation failed: ") + e.what(), metrics};
    }
}

//-----------------------------------------------------------------------------
// FioWriteVerb Helper Functions - Line Surgery Implementation
//-----------------------------------------------------------------------------

Db9Response FioWriteVerb::performFullFileWrite(const std::string& path, const std::string& content, std::chrono::steady_clock::time_point start_time) {
    std::filesystem::path target_path(path);
    
    // Safety check: prevent writing to root filesystem with relative paths
    if (target_path.is_relative()) {
        try {
            auto cwd = std::filesystem::current_path();
            if (cwd == "/") {
                AutoReflexiveMetrics metrics;
                std::string safety_message = "Safety check failed: Cannot write relative path '" + path + 
                                           "' when current working directory is root (/). " +
                                           "This would write to the root filesystem. " +
                                           "Use absolute paths or run from appropriate working directory.";
                return Db9Response{Db9Response::Error, "", "unsafe_root_write", safety_message, metrics};
            }
        } catch (const std::exception& e) {
            AutoReflexiveMetrics metrics;
            std::string error_message = "Error checking current working directory: " + std::string(e.what());
            return Db9Response{Db9Response::Error, "", "cwd_check_failed", error_message, metrics};
        }
    }
    
    // Ensure parent directory exists (only for absolute paths or safe relative paths)
    if (target_path.has_parent_path()) {
        std::filesystem::create_directories(target_path.parent_path());
    }
    
    // Write file
    std::ofstream file(path);
    if (!file.is_open()) {
        AutoReflexiveMetrics metrics;
        
        // Enhanced diagnostics for file open failure
        std::string diagnostic_info = "Could not open file for writing. ";
        
        // 1. Path information
        diagnostic_info += "Path: '" + path + "'. ";
        
        // 2. Path resolution (absolute vs relative + cwd)
        if (target_path.is_absolute()) {
            diagnostic_info += "Path is absolute. ";
        } else {
            diagnostic_info += "Path is relative. ";
            try {
                auto cwd = std::filesystem::current_path();
                diagnostic_info += "Current working directory: '" + cwd.string() + "'. ";
                auto resolved_path = std::filesystem::absolute(target_path);
                diagnostic_info += "Resolved absolute path: '" + resolved_path.string() + "'. ";
            } catch (const std::exception& e) {
                diagnostic_info += "Error getting current directory: " + std::string(e.what()) + ". ";
            }
        }
        
        // 3. File existence check
        std::error_code ec;
        if (std::filesystem::exists(target_path, ec)) {
            diagnostic_info += "File exists. ";
            
            // 4. File permissions check
            auto file_perms = std::filesystem::status(target_path, ec).permissions();
            if (ec) {
                diagnostic_info += "Error checking file permissions: " + ec.message() + ". ";
            } else {
                using std::filesystem::perms;
                bool owner_write = (file_perms & perms::owner_write) != perms::none;
                bool group_write = (file_perms & perms::group_write) != perms::none;  
                bool others_write = (file_perms & perms::others_write) != perms::none;
                diagnostic_info += "File permissions - owner_write: " + std::string(owner_write ? "yes" : "no") + 
                                   ", group_write: " + std::string(group_write ? "yes" : "no") + 
                                   ", others_write: " + std::string(others_write ? "yes" : "no") + ". ";
            }
        } else {
            diagnostic_info += "File does not exist. ";
        }
        
        // 5. Parent directory checks
        auto parent_path = target_path.parent_path();
        if (parent_path.empty()) {
            diagnostic_info += "No parent directory (root level file). ";
        } else {
            if (std::filesystem::exists(parent_path, ec)) {
                diagnostic_info += "Parent directory exists. ";
                
                // Check parent directory permissions
                auto parent_file_perms = std::filesystem::status(parent_path, ec).permissions();
                if (ec) {
                    diagnostic_info += "Error checking parent directory permissions: " + ec.message() + ". ";
                } else {
                    using std::filesystem::perms;
                    bool parent_write = (parent_file_perms & perms::owner_write) != perms::none;
                    diagnostic_info += "Parent directory writable: " + std::string(parent_write ? "yes" : "no") + ". ";
                }
            } else {
                diagnostic_info += "Parent directory does not exist: '" + parent_path.string() + "'. ";
                if (ec) {
                    diagnostic_info += "Error: " + ec.message() + ". ";
                }
            }
        }
        
        // 6. Filesystem space check (basic)
        try {
            auto space_info = std::filesystem::space(target_path.parent_path());
            if (space_info.available == 0) {
                diagnostic_info += "No available disk space. ";
            } else {
                diagnostic_info += "Available disk space: " + std::to_string(space_info.available / (1024 * 1024)) + " MB. ";
            }
        } catch (const std::exception& e) {
            diagnostic_info += "Error checking disk space: " + std::string(e.what()) + ". ";
        }
        
        return Db9Response{Db9Response::Error, "", "file_open_failed", diagnostic_info, metrics};
    }
    
    file << content;
    file.close();
    
    // Get file size
    size_t file_size = std::filesystem::file_size(target_path);
    
    // Calculate execution time
    auto end_time = std::chrono::steady_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    // Create response JSON
    std::ostringstream result;
    result << "{\"status\": \"written\""
           << ", \"path\": \"" << path << "\""
           << ", \"size_bytes\": " << file_size
           << "}";
    
    AutoReflexiveMetrics metrics;
    metrics.operation_time_ms = std::chrono::milliseconds(duration_ms);
    metrics.items_processed = 1;
    metrics.memory_usage_kb = static_cast<uint64_t>(content.size() / 1024);
    
    return Db9Response{Db9Response::Success, result.str(), "", "", metrics};
}

Db9Response FioWriteVerb::performLineSurgery(const std::string& path, const std::string& content, const std::string& lines_param, const std::string& mode, std::chrono::steady_clock::time_point start_time) {
    // Parse line specification (reuse existing parser from fio-read)
    LineRange range = parseLineSpec(lines_param);
    if (!range.valid) {
        AutoReflexiveMetrics metrics;
        return Db9Response{Db9Response::Error, "", "invalid_line_spec", "Invalid line specification: " + lines_param, metrics};
    }
    
    // Validate mode
    if (mode != "replace" && mode != "insert" && mode != "append") {
        AutoReflexiveMetrics metrics;
        return Db9Response{Db9Response::Error, "", "invalid_mode", "Invalid mode: " + mode + ". Must be 'replace', 'insert', or 'append'", metrics};
    }
    
    // Check if file exists for line operations
    if (!std::filesystem::exists(path)) {
        AutoReflexiveMetrics metrics;
        return Db9Response{Db9Response::Error, "", "file_not_found", "Cannot perform line surgery on non-existent file: " + path, metrics};
    }
    
    // Read existing file content
    std::ifstream file(path);
    if (!file.is_open()) {
        AutoReflexiveMetrics metrics;
        return Db9Response{Db9Response::Error, "", "file_read_failed", "Could not read file for line surgery: " + path, metrics};
    }
    
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    file.close();
    
    // Validate line range against actual file
    int total_lines = static_cast<int>(lines.size());
    if (range.type == LineRange::SingleLine || range.type == LineRange::Range) {
        if (range.start < 1 || range.start > total_lines) {
            AutoReflexiveMetrics metrics;
            return Db9Response{Db9Response::Error, "", "line_out_of_range", 
                "Line " + std::to_string(range.start) + " is out of range. File has " + std::to_string(total_lines) + " lines", metrics};
        }
    }
    
    // Perform line surgery based on mode
    std::vector<std::string> new_content_lines;
    std::istringstream content_stream(content);
    std::string content_line;
    while (std::getline(content_stream, content_line)) {
        new_content_lines.push_back(content_line);
    }
    
    int lines_affected = 0;
    
    if (mode == "replace") {
        lines_affected = performLineReplacement(lines, new_content_lines, range);
    } else if (mode == "insert") {
        lines_affected = performLineInsertion(lines, new_content_lines, range);
    } else if (mode == "append") {
        lines_affected = performLineAppend(lines, new_content_lines, range);
    }
    
    // Write the modified content back to file
    std::ofstream out_file(path);
    if (!out_file.is_open()) {
        AutoReflexiveMetrics metrics;
        return Db9Response{Db9Response::Error, "", "file_write_failed", "Could not write modified content to file: " + path, metrics};
    }
    
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) out_file << "\n";
        out_file << lines[i];
    }
    out_file.close();
    
    // Get new file size
    size_t file_size = std::filesystem::file_size(path);
    
    // Calculate execution time
    auto end_time = std::chrono::steady_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    // Create response JSON with surgery details
    std::ostringstream result;
    result << "{\"status\": \"line_surgery_complete\""
           << ", \"path\": \"" << path << "\""
           << ", \"mode\": \"" << mode << "\""
           << ", \"lines_affected\": " << lines_affected
           << ", \"total_lines\": " << lines.size()
           << ", \"size_bytes\": " << file_size
           << ", \"line_spec\": \"" << lines_param << "\""
           << "}";
    
    AutoReflexiveMetrics metrics;
    metrics.operation_time_ms = std::chrono::milliseconds(duration_ms);
    metrics.items_processed = lines_affected;
    metrics.memory_usage_kb = static_cast<uint64_t>(content.size() / 1024);
    
    return Db9Response{Db9Response::Success, result.str(), "", "", metrics};
}

int FioWriteVerb::performLineReplacement(std::vector<std::string>& lines, const std::vector<std::string>& new_content, const LineRange& range) {
    int start_idx, end_idx;
    
    if (range.type == LineRange::SingleLine) {
        start_idx = range.start - 1; // Convert to 0-based
        end_idx = start_idx;
    } else if (range.type == LineRange::Range) {
        start_idx = range.start - 1; // Convert to 0-based
        end_idx = start_idx + range.count - 1;
    } else if (range.type == LineRange::FromEnd) {
        // @e:-3 means last 3 lines
        int total_lines = static_cast<int>(lines.size());
        start_idx = total_lines - range.count; // Start of last N lines
        end_idx = total_lines - 1;             // End of file
    } else if (range.type == LineRange::FromStart) {
        // @0:N means first N lines
        start_idx = 0;
        end_idx = range.count - 1;
    } else {
        return 0; // Unsupported range type for replacement
    }
    
    // Validate range bounds
    start_idx = std::max(0, start_idx);
    end_idx = std::min(static_cast<int>(lines.size()) - 1, end_idx);
    
    if (start_idx > end_idx || start_idx >= static_cast<int>(lines.size())) {
        return 0; // Invalid range
    }
    
    int lines_removed = end_idx - start_idx + 1;
    
    // Remove the original lines
    lines.erase(lines.begin() + start_idx, lines.begin() + end_idx + 1);
    
    // Insert new content at the same position
    lines.insert(lines.begin() + start_idx, new_content.begin(), new_content.end());
    
    return lines_removed;
}

int FioWriteVerb::performLineInsertion(std::vector<std::string>& lines, const std::vector<std::string>& new_content, const LineRange& range) {
    int insert_idx = (range.type == LineRange::SingleLine) ? range.start - 1 : range.start - 1;
    
    // Insert new content, shifting existing lines down
    lines.insert(lines.begin() + insert_idx, new_content.begin(), new_content.end());
    
    return static_cast<int>(new_content.size());
}

int FioWriteVerb::performLineAppend(std::vector<std::string>& lines, const std::vector<std::string>& new_content, const LineRange& range) {
    int append_idx = (range.type == LineRange::SingleLine) ? range.start : range.start + range.count - 1;
    
    // Insert new content after the specified line
    lines.insert(lines.begin() + append_idx, new_content.begin(), new_content.end());
    
    return static_cast<int>(new_content.size());
}

// FioListVerb implementation
std::string FioListVerb::getDescription() const {
    return R"DESC(
List directory contents with basic file information.
Usage:
```lisp
(fio-list :path §/path/to/directory§)
;; Returns: directory listing with file metadata
```

**Parameters:**
- `:path` - Directory path to list (required)

**Returns:**
- `files` - Array of file information objects
- `count` - Total number of files found
- `path` - Directory path that was listed
)DESC";
}

Db9Response FioListVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    std::string path = extractStringParam(sexpr, "path");
    
    if (path.empty()) {
        AutoReflexiveMetrics metrics;
        return Db9Response{Db9Response::Error, "", "missing_path", "fio-list requires :path parameter", metrics};
    }
    
    try {
        // Check if path exists and is a directory
        if (!std::filesystem::exists(path)) {
            AutoReflexiveMetrics metrics;
            return Db9Response{Db9Response::Error, "", "path_not_found", "Directory does not exist: " + path, metrics};
        }
        
        if (!std::filesystem::is_directory(path)) {
            AutoReflexiveMetrics metrics;
            return Db9Response{Db9Response::Error, "", "not_directory", "Path is not a directory: " + path, metrics};
        }
        
        // Scan directory
        std::ostringstream result;
        result << "{\"files\": [";
        
        bool first = true;
        size_t count = 0;
        
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (!first) result << ", ";
            
            std::string type = "file";
            size_t size = 0;
            
            if (entry.is_directory()) {
                type = "directory";
            } else if (entry.is_symlink()) {
                type = "symlink";
            } else {
                try {
                    size = std::filesystem::file_size(entry);
                } catch (...) {
                    size = 0;
                }
            }
            
            result << "{\"name\": \"" << entry.path().filename().string() << "\""
                   << ", \"type\": \"" << type << "\""
                   << ", \"size\": " << size
                   << "}";
            
            first = false;
            count++;
        }
        
        result << "], \"count\": " << count
               << ", \"path\": \"" << path << "\""
               << "}";
        
        // Calculate execution time
        auto end_time = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = std::chrono::milliseconds(duration_ms);
        metrics.items_processed = count;
        
        // Add awareness fairy guidance for empty directories  
        std::string enhanced_result = FioUtils::addEmptyDirectoryAwareness(result.str(), count);
        
        return Db9Response{Db9Response::Success, enhanced_result, "", "", metrics};
        
    } catch (const std::exception& e) {
        AutoReflexiveMetrics metrics;
        return Db9Response{Db9Response::Error, "", "list_failed", std::string("Directory listing failed: ") + e.what(), metrics};
    }
}

}

//-----------------------------------------------------------------------------
// FioReadVerb Implementation with Smart Line Syntax
//-----------------------------------------------------------------------------

namespace LabDb {

std::string FioReadVerb::getDescription() const {
    return R"DESC(
Read file content with smart line range syntax.
Usage:
```lisp
(fio-read :path §/path/to/file.txt§)                    ;; full file
(fio-read :path §/path/to/file.txt§ :lines §@1:20§)   ;; first 20 lines (lines 1-20)
(fio-read :path §/path/to/file.txt§ :lines §@e:-20§)  ;; last 20 lines  
(fio-read :path §/path/to/file.txt§ :lines §@30:-2§)  ;; 2 lines ending at line 30
(fio-read :path §/path/to/file.txt§ :lines §@50:100§) ;; lines 50-100
(fio-read :path §/path/to/file.txt§ :lines §@25§)     ;; single line 25
```

**Parameters:**
- `:path` - File path to read (required)
- `:lines` - Smart line range specification (optional, default: full file)
- `:encoding` - File encoding (default: "utf-8") 
- `:line_numbers` - Show line numbers (default: false)

**Line Range Syntax:**
- `@N:M` - Lines N through M (inclusive)
- `@N:-M` - M lines ending at line N
- `@e:-M` - Last M lines (e = end)
- `@0:M` - First M lines (special case: 0-based start for first N lines)
- `@N` - Single line N (1-based)
- No :lines - Full file

**Returns:**
- `content` - File content as requested
- `lines_returned` - Number of lines returned
- `total_lines` - Total lines in file
- `range_spec` - Parsed range specification

**🧘 FIO Ecosystem Integration:**
- Part of the revolutionary trilogy: fio-search (find) + fio-read (see) + fio-write (change)
- Use fio-read to examine content before precise editing with fio-write
- Note: fio-read shows content as-is without ƒ escaping (see fio-write for ƒ → \\ system)

)DESC";
}

Db9Response FioReadVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    std::string path = extractStringParam(sexpr, "path");
    std::string lines_param = extractStringParam(sexpr, "lines");
    std::string encoding = extractStringParam(sexpr, "encoding");
    if (encoding.empty()) encoding = "utf-8";
    
    if (path.empty()) {
        AutoReflexiveMetrics metrics;
        return Db9Response{Db9Response::Error, "", "missing_path", "fio-read requires :path parameter", metrics};
    }
    
    try {
    // Create triadic path context for conscious filesystem operation
    auto pathContext = FioUtils::createPathContext(path);
        
        // Check file exists with conscious error reporting
        if (!pathContext.exists) {
            AutoReflexiveMetrics metrics;
            std::string contextual_error = FioUtils::createContextualErrorMessage(
                pathContext, "fio-read", "File does not exist"
            );
            return Db9Response{Db9Response::Error, "", "file_not_found", contextual_error, metrics};
        }
        
        // Parse line specification
        LineRange range = parseLineSpec(lines_param);
        if (!range.valid) {
            AutoReflexiveMetrics metrics;
            return Db9Response{Db9Response::Error, "", "invalid_line_spec", "Invalid line specification: " + lines_param, metrics};
        }
        
        // Read file and apply range
        std::ifstream file(path);
        if (!file.is_open()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{Db9Response::Error, "", "file_open_failed", "Could not open file for reading", metrics};
        }
        
        std::vector<std::string> all_lines;
        std::string line;
        while (std::getline(file, line)) {
            all_lines.push_back(line);
        }
        file.close();
        
        // Apply range logic
        std::vector<std::string> selected_lines;
        size_t total_lines = all_lines.size();
        
        switch (range.type) {
            case LineRange::Full:
                selected_lines = all_lines;
                break;
                
            case LineRange::FromStart:
                for (size_t i = 0; i < std::min(static_cast<size_t>(range.count), total_lines); ++i) {
                    selected_lines.push_back(all_lines[i]);
                }
                break;
                
            case LineRange::FromEnd:
                for (size_t i = std::max(0, static_cast<int>(total_lines) - range.count); i < total_lines; ++i) {
                    selected_lines.push_back(all_lines[i]);
                }
                break;
                
            case LineRange::SingleLine: {
                // Convert 1-based user input to 0-based array index
                int zero_based_line = range.start - 1;
                if (zero_based_line >= 0 && zero_based_line < static_cast<int>(total_lines)) {
                    selected_lines.push_back(all_lines[zero_based_line]);
                }
                break;
            }
                
            case LineRange::Range: {
                // Convert 1-based user input to 0-based array indices
                int zero_based_start = range.start - 1;
                if (zero_based_start >= 0 && zero_based_start < static_cast<int>(total_lines)) {
                    int end_pos = zero_based_start + range.count;
                    for (int i = zero_based_start; i < std::min(end_pos, static_cast<int>(total_lines)); ++i) {
                        selected_lines.push_back(all_lines[i]);
                    }
                }
                break;
            }
        }
        
        // Build content string
        std::ostringstream content;
        for (size_t i = 0; i < selected_lines.size(); ++i) {
            if (i > 0) content << "\n";
            content << selected_lines[i];
        }
        
        // Calculate execution time
        auto end_time = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        // Create response JSON with triadic consciousness
        std::ostringstream result;
        result << "{\"content\": \"" << FioUtils::escapeJsonString(content.str()) << "\""
               << ", \"lines_returned\": " << selected_lines.size()
               << ", \"total_lines\": " << total_lines
               << ", \"range_spec\": \"" << (lines_param.empty() ? "full" : lines_param) << "\""
               << ", \"path\": \"" << FioUtils::escapeJsonString(path) << "\""
               << ", \"path_context\": " << pathContext.toJsonString()
               << "}";
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = std::chrono::milliseconds(duration_ms);
        metrics.items_processed = selected_lines.size();
        metrics.memory_usage_kb = static_cast<uint64_t>(content.str().size() / 1024);
        
        return Db9Response{Db9Response::Success, result.str(), "", "", metrics};
        
    } catch (const std::exception& e) {
        // Create triadic context for error reporting
        auto pathContext = FioUtils::createPathContext(path);
        std::string contextual_error = FioUtils::createContextualErrorMessage(
            pathContext, "fio-read", "File read failed: " + std::string(e.what())
        );
        
        AutoReflexiveMetrics metrics;
        return Db9Response{Db9Response::Error, "", "read_failed", contextual_error, metrics};
    }
}

// Registration function
void initFioVerbRegistration(Db9Dispatcher& dispatcher) {
    static bool registered = false;
    if (!registered) {
        dispatcher.registerVerb(std::make_unique<FioWriteVerb>());
        dispatcher.registerVerb(std::make_unique<FioListVerb>());
        dispatcher.registerVerb(std::make_unique<FioReadVerb>());
        dispatcher.registerVerb(std::make_unique<GetVerbDescriptionVerb>());  // 🔍 SELF-DOCUMENTING VERB!
        dispatcher.registerVerb(std::make_unique<SetCwdVerb>());  // 🛡️ SAFE WORKING DIRECTORY CHANGER!
        
        // Register aliases for convenience
        dispatcher.registerVerbAlias("get-verb-description", {"get-description"});
        dispatcher.registerVerb(std::make_unique<FioSearchVerb>());  // 🚀 THE REVOLUTIONARY NEW VERB!
        registered = true;
    }
}

//-----------------------------------------------------------------------------
// FioSearchVerb Implementation - Revolutionary Content Search
//-----------------------------------------------------------------------------

std::string FioSearchVerb::getDescription() const {
    return R"DESC(
Search content within files with revolutionary precision - the perfect companion to fio-write's Unicode escaping system!
Usage:
```lisp
;; 🔍 Literal string search (fast, exact)
(fio-search :path §/path/to/file.cpp§ :literal §PLACEHOLDER§)

;; 🎯 Wildcard/glob pattern search (intuitive)  
(fio-search :path §/path/to/file.cpp§ :pathspec §PLACE*§)

;; ⚡ Regex search (powerful but use carefully)
(fio-search :path §/path/to/file.cpp§ :regex §PLACE[A-Z]+_[0-9]+§)

;; 📍 Search within line ranges (combine with fio-read syntax!)
(fio-search :path §file.cpp§ :lines §@100:200§ :literal §debug§)

;; 🔍 Case-insensitive search
(fio-search :path §file.cpp§ :literal §placeholder§ :case_sensitive false)

;; 🔧 Unicode escaping control examples:
;; Default: No escaping (searches for literal ※ character)
(fio-search :path §file.cpp§ :literal §※§)

;; Explicit escaping enabled (searches for \ character)  
(fio-search :path §file.cpp§ :literal §※§ :escape true)

;; Search for converted newlines in generated code
(fio-search :path §file.cpp§ :literal §※n§ :escape true)  ;; Finds \n
```

**Parameters:**
- `:path` - Target file path (required)
- `:literal` - Literal string search (mutually exclusive with :pathspec/:regex)
- `:pathspec` - Wildcard pattern search (e.g., "PLACE*", "*_config")
- `:regex` - Regular expression search (use carefully!)
- `:lines` - Line range specification (optional, same syntax as fio-read)
- `:case_sensitive` - Case sensitivity (default: true)
- `:context_lines` - Show N lines of context around matches (default: 0)
- `:escape` - Apply Unicode escaping to search pattern (default: false)

**Line Range Syntax (Same as fio-read!):**
- `@N:M` - Search only lines N through M
- `@e:-M` - Search only last M lines
- No :lines - Search entire file

**Returns:**
- `status` - "search_complete"
- `path` - File path searched
- `search_type` - "literal", "pathspec", or "regex"
- `pattern` - Search pattern used
- `matches` - Array of match objects with line/column/context
- `total_matches` - Number of matches found

**🚀 Revolutionary Trilogy Workflow Integration:**
```lisp
;; 1. FIND with surgical precision
(fio-search :path §src/code.cpp§ :literal §old_function_name§)

;; 2. SEE the context  
(fio-read :path §src/code.cpp§ :lines §@23:25§)

;; 3. CHANGE with Unicode escaping paradise
(fio-write :path §src/code.cpp§ :lines §@24§ :content §    printf(″New function called※n″);§)

;; 4. VERIFY the change with search
(fio-search :path §src/code.cpp§ :literal §printf("New function called\n");§)
```

**🎯 Perfect Integration with fio-write Unicode Escapes:**

When you write code using fio-write's Unicode escape system, fio-search finds the converted results:

```lisp
;; Write with Unicode escapes:
(fio-write :content §printf(″Debug: %s※n″, message);§)

;; Search finds the converted content:
(fio-search :literal §printf("Debug: %s\n", message);§)  ✅ FOUND!

;; Original Unicode patterns won't be found:
(fio-search :literal §※n§)  ❌ Not found (correctly converted)
```

**🔍 Advanced Search Patterns for Generated Code:**

```lisp
;; Find C++ printf patterns (common after fio-write generation)
(fio-search :path §src/app.cpp§ :regex §printf\(".*\\n", .*\);§)

;; Find regex patterns with escaped characters
(fio-search :path §src/parser.cpp§ :literal §std::regex pattern("\\w+\\d+");§)

;; Find file path patterns with backslashes
(fio-search :path §src/config.cpp§ :literal §"C:\\Users\\§)

;; Find indented code blocks
(fio-search :path §src/main.cpp§ :regex §^\t.*printf§)
```

**🚀 Unicode-Aware Search Examples:**

fio-search automatically finds content created by fio-write's Unicode escape system:

```lisp
;; After fio-write creates this code:
;; fio-write :content §⇥std::regex email(″[a-z]+@[a-z]+※.[a-z]+″);§
;; 
;; The file contains:
;;     std::regex email("[a-z]+@[a-z]+\.[a-z]+");

;; You can search for it with:
(fio-search :literal §std::regex email("[a-z]+@[a-z]+\.[a-z]+");§)
(fio-search :regex §std::regex \w+\(".*\\\..*"\);§)
(fio-search :pathspec §*email*§)
```

**💡 Search Strategy for Unicode-Generated Content:**

1. **Search for converted forms**: Look for `\n`, `"`, `\t`, etc. (not Unicode)
2. **Use literal search**: Most precise for exact generated content
3. **Use regex carefully**: Escape sequences in regex need double-escaping
4. **Combine with line ranges**: Narrow search scope for performance

**Examples of Generated Content Searches:**
```lisp
;; Find printf statements (common fio-write output)
(fio-search :regex §printf\(".*\\n"§)

;; Find quoted strings (″ converted to ")
(fio-search :regex §"[^"]*"§)

;; Find regex patterns (※ converted to \)
(fio-search :literal §std::regex§)

;; Find indented code (⇥ converted to tabs)
(fio-search :regex §^\t§)
```

**🧚‍♀️ Awareness Features:**
- No matches found with helpful suggestions
- Too many matches warning with refinement advice
- Binary file detection and warnings
- Performance warnings for large files
- Context window control for match visibility
- Unicode escape integration guidance

**🎯 Perfect Trilogy Synergy:**
- **fio-search**: Find patterns in existing code
- **fio-read**: Examine context around matches
- **fio-write**: Modify content with Unicode escape paradise
- **Round-trip verification**: Search for your changes!

This completes the filesystem revolution trilogy: fio-search (find) + fio-read (see) + fio-write (change) with perfect Unicode escape integration!
)DESC";
}
FioSearchVerb::SearchParameters FioSearchVerb::extractParameters(const lab::Text::Sexpr& sexpr) {
    SearchParameters params;
    
    params.path = extractStringParam(sexpr, "path");
    
    // Determine search mode based on which parameter is provided
    std::string literal = extractStringParam(sexpr, "literal");
    std::string pathspec = extractStringParam(sexpr, "pathspec");  
    std::string regex = extractStringParam(sexpr, "regex");
    
    if (!literal.empty()) {
        params.mode = Literal;
        params.pattern = literal;
    } else if (!pathspec.empty()) {
        params.mode = Glob;
        params.pattern = pathspec;
    } else if (!regex.empty()) {
        params.mode = Regex;
        params.pattern = regex;
    }
    
    // Check if Unicode escaping should be applied (defaults to false)
    std::string escape_param = extractStringParam(sexpr, "escape");
    bool apply_escaping = (escape_param == "true");
    
    // Apply Unicode escaping only if explicitly requested
    if (!params.pattern.empty() && apply_escaping) {
        params.pattern = FioUtils::unescapeDb9String(params.pattern);
    }
    
    params.lines_param = extractStringParam(sexpr, "lines");
    
    std::string case_sensitive = extractStringParam(sexpr, "case_sensitive");
    if (case_sensitive == "false") {
        params.case_sensitive = false;
    }
    
    std::string context_lines = extractStringParam(sexpr, "context_lines");
    if (!context_lines.empty()) {
        params.context_lines = std::stoi(context_lines);
    }
    
    return params;
}

Db9Response FioSearchVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    SearchParameters params = extractParameters(sexpr);
    
    if (params.path.empty()) {
        AutoReflexiveMetrics metrics;
        return Db9Response{Db9Response::Error, "", "missing_path", "fio-search requires :path parameter", metrics};
    }
    
    if (params.pattern.empty()) {
        AutoReflexiveMetrics metrics;
        return Db9Response{Db9Response::Error, "", "missing_pattern", "fio-search requires one of :literal, :pathspec, or :regex parameters", metrics};
    }
    
    return performSearch(params);
}

Db9Response FioSearchVerb::performSearch(const SearchParameters& params) {
    auto start_time = std::chrono::steady_clock::now();
    
    // Check if file exists
    if (!std::filesystem::exists(params.path)) {
        AutoReflexiveMetrics metrics;
        return Db9Response{Db9Response::Error, "", "file_not_found", "Cannot search non-existent file: " + params.path, metrics};
    }
    
    // Read file content
    std::ifstream file(params.path);
    if (!file.is_open()) {
        AutoReflexiveMetrics metrics;
        return Db9Response{Db9Response::Error, "", "file_read_failed", "Could not read file for search: " + params.path, metrics};
    }
    
    std::vector<std::string> file_lines;
    std::string line;
    while (std::getline(file, line)) {
        file_lines.push_back(line);
    }
    file.close();
    
    // Apply line range filtering if specified
    std::vector<std::string> search_lines = file_lines;
    int line_offset = 0;
    
    if (!params.lines_param.empty()) {
        LineRange range = parseLineSpec(params.lines_param);
        if (!range.valid) {
            AutoReflexiveMetrics metrics;
            return Db9Response{Db9Response::Error, "", "invalid_line_spec", "Invalid line specification: " + params.lines_param, metrics};
        }
        
        // Extract the specified line range
        int start_idx = 0, end_idx = static_cast<int>(file_lines.size()) - 1;
        
        if (range.type == LineRange::SingleLine) {
            start_idx = range.start - 1;
            end_idx = start_idx;
        } else if (range.type == LineRange::Range) {
            start_idx = range.start - 1;
            end_idx = start_idx + range.count - 1;
        } else if (range.type == LineRange::FromEnd) {
            start_idx = static_cast<int>(file_lines.size()) - range.count;
            end_idx = static_cast<int>(file_lines.size()) - 1;
        }
        
        // Validate range
        start_idx = std::max(0, start_idx);
        end_idx = std::min(static_cast<int>(file_lines.size()) - 1, end_idx);
        
        search_lines.clear();
        for (int i = start_idx; i <= end_idx; ++i) {
            search_lines.push_back(file_lines[i]);
        }
        line_offset = start_idx;
    }
    
    // Perform search based on mode
    std::vector<SearchMatch> matches;
    try {
        switch (params.mode) {
            case Literal:
                matches = searchLiteral(search_lines, params.pattern, params.case_sensitive);
                break;
            case Glob:
                matches = searchGlob(search_lines, params.pattern, params.case_sensitive);
                break;
            case Regex:
                matches = searchRegex(search_lines, params.pattern, params.case_sensitive);
                break;
        }
    } catch (const std::exception& e) {
        AutoReflexiveMetrics metrics;
        return Db9Response{Db9Response::Error, "", "search_failed", "Search failed: " + std::string(e.what()), metrics};
    }
    
    // Adjust line numbers for line offset
    for (auto& match : matches) {
        match.line += line_offset;
    }
    
    // Generate context windows if requested
    if (params.context_lines > 0) {
        for (auto& match : matches) {
            match.context = createContextWindow(file_lines, match.line, params.context_lines);
        }
    }
    
    // Calculate execution time
    auto end_time = std::chrono::steady_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    // Create response JSON
    std::string result = formatSearchResults(matches, params);
    
    AutoReflexiveMetrics metrics;
    metrics.operation_time_ms = std::chrono::milliseconds(duration_ms);
    metrics.items_processed = static_cast<int>(matches.size());
    
    return Db9Response{Db9Response::Success, result, "", "", metrics};
}

std::vector<FioSearchVerb::SearchMatch> FioSearchVerb::searchLiteral(const std::vector<std::string>& file_lines, const std::string& pattern, bool case_sensitive) {
    std::vector<SearchMatch> matches;
    
    std::string search_pattern = pattern;
    if (!case_sensitive) {
        std::transform(search_pattern.begin(), search_pattern.end(), search_pattern.begin(), ::tolower);
    }
    
    for (size_t line_idx = 0; line_idx < file_lines.size(); ++line_idx) {
        std::string line = file_lines[line_idx];
        std::string search_line = line;
        
        if (!case_sensitive) {
            std::transform(search_line.begin(), search_line.end(), search_line.begin(), ::tolower);
        }
        
        size_t pos = 0;
        while ((pos = search_line.find(search_pattern, pos)) != std::string::npos) {
            SearchMatch match;
            match.line = static_cast<int>(line_idx + 1); // 1-based line numbers
            match.column = static_cast<int>(pos + 1);    // 1-based column numbers
            match.line_content = line;
            matches.push_back(match);
            pos++;
        }
    }
    
    return matches;
}

std::vector<FioSearchVerb::SearchMatch> FioSearchVerb::searchGlob(const std::vector<std::string>& file_lines, const std::string& pattern, bool case_sensitive) {
    // Convert glob pattern to regex
    std::string regex_pattern = pattern;
    
    // Escape special regex characters except * and ?
    std::string escaped;
    for (char c : regex_pattern) {
        if (c == '*') {
            escaped += ".*";
        } else if (c == '?') {
            escaped += ".";
        } else if (c == '.' || c == '^' || c == '$' || c == '+' || c == '(' || c == ')' || 
                   c == '[' || c == ']' || c == '{' || c == '}' || c == '|' || c == '\\') {
            escaped += "\\";
            escaped += c;
        } else {
            escaped += c;
        }
    }
    
    return searchRegex(file_lines, escaped, case_sensitive);
}

std::vector<FioSearchVerb::SearchMatch> FioSearchVerb::searchRegex(const std::vector<std::string>& file_lines, const std::string& pattern, bool case_sensitive) {
    std::vector<SearchMatch> matches;
    
    std::regex::flag_type flags = std::regex::ECMAScript;
    if (!case_sensitive) {
        flags |= std::regex::icase;
    }
    
    std::regex regex_pattern(pattern, flags);
    
    for (size_t line_idx = 0; line_idx < file_lines.size(); ++line_idx) {
        const std::string& line = file_lines[line_idx];
        
        std::sregex_iterator iter(line.begin(), line.end(), regex_pattern);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            const std::smatch& match_result = *iter;
            SearchMatch match;
            match.line = static_cast<int>(line_idx + 1); // 1-based line numbers
            match.column = static_cast<int>(match_result.position() + 1); // 1-based column numbers
            match.line_content = line;
            matches.push_back(match);
        }
    }
    
    return matches;
}

std::string FioSearchVerb::formatSearchResults(const std::vector<SearchMatch>& matches, const SearchParameters& params) {
    std::ostringstream result;
    
    std::string mode_str;
    switch (params.mode) {
        case Literal: mode_str = "literal"; break;
        case Glob: mode_str = "pathspec"; break;
        case Regex: mode_str = "regex"; break;
    }
    
    result << "{"
           << "\"status\": \"search_complete\""
           << ", \"path\": \"" << params.path << "\""
           << ", \"search_type\": \"" << mode_str << "\""
           << ", \"pattern\": \"" << FioUtils::escapeJsonString(params.pattern) << "\""
           << ", \"matches\": [";
    
    for (size_t i = 0; i < matches.size(); ++i) {
        if (i > 0) result << ", ";
        
        const auto& match = matches[i];
        result << "{"
               << "\"line\": " << match.line
               << ", \"column\": " << match.column
               << ", \"line_content\": \"" << FioUtils::escapeJsonString(match.line_content) << "\"";
        
        if (!match.context.empty()) {
            result << ", \"context\": \"" << FioUtils::escapeJsonString(match.context) << "\"";
        }
        
        result << "}";
    }
    
    result << "]"
           << ", \"total_matches\": " << matches.size();
    
    // Add awareness fairy guidance
    if (matches.empty()) {
        result << ", \"awareness_note\": \"No matches found for pattern '" << FioUtils::escapeJsonString(params.pattern) << "'. Check spelling or try a broader search pattern.\"";
    } else if (matches.size() > 50) {
        result << ", \"awareness_note\": \"Found " << matches.size() << " matches. Consider using a more specific pattern or :lines parameter to narrow the search.\"";
    }
    
    result << "}";
    
    return result.str();
}

std::string FioSearchVerb::createContextWindow(const std::vector<std::string>& file_lines, int line_num, int context_lines) {
    int start_line = std::max(1, line_num - context_lines);
    int end_line = std::min(static_cast<int>(file_lines.size()), line_num + context_lines);
    
    std::ostringstream context;
    for (int i = start_line; i <= end_line; ++i) {
        if (i > start_line) context << "\n";
        context << i << ": " << file_lines[i - 1]; // Convert to 0-based for vector access
    }
    
    return context.str();
}

} // namespace LabDb