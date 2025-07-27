#pragma once

#include <string>

namespace LabDb {

//-----------------------------------------------------------------------------
// TextEscaping - Unicode Escape System for Db9
//-----------------------------------------------------------------------------

/**
 * Unicode → Backslash Escaping System
 * 
 * Revolutionary visual Unicode characters eliminate JSON escaping complexity forever!
 * This system transforms fio-write from an escape complexity nightmare
 * into a natural, visual code generation tool that LLMs can use effortlessly!
 * 
 * Core Unicode Escapes:
 * - ※ (U+203B) → \ - Universal backslash for regex, file paths, C++ escapes
 * - ″ (U+2033) → " - Clean quotes without JSON conflicts
 * - ↵ (U+21B5) → actual newline - Creates real line breaks in files
 * - ⇥ (U+21E5) → actual tab - Creates real indentation
 */
namespace TextEscaping {

    /**
     * Convert Unicode escape sequences to their target characters
     * 
     * Example transformations:
     * - printf(″Debug: %s※n″, msg); → printf("Debug: %s\n", msg);
     * - std::regex email(″[a-z]+@[a-z]+※.[a-z]+″); → std::regex email("[a-z]+@[a-z]+\.[a-z]+");
     * - ⇥if (validate()) {↵⇥⇥process();↵} → \tif (validate()) {\n\t\tprocess();\n}
     * 
     * Benefits for LLMs and Developers:
     * - Visual Clarity: See exactly what each symbol does
     * - JSON Transparent: Never conflicts with JSON syntax  
     * - Error Proof: Impossible to create malformed escapes
     * - LLM Friendly: Zero cognitive load for code generation
     * - § Delimiter Magic: Works perfectly with § delimiters
     * 
     * @param input String containing Unicode escape sequences
     * @return String with escape sequences converted to target characters
     */
    std::string unescapeDb9String(const std::string& input);

    /**
     * Convert control characters to visible escape sequences for display/debugging
     * 
     * Reverse operation of unescapeDb9String - useful for:
     * - Debugging Unicode escape processing
     * - Displaying file content with visible control characters  
     * - Testing escape sequence behavior
     * - Verification of escape processing results
     * 
     * Example transformations:
     * - printf("Debug: %s\n", msg); → printf(\"Debug: %s\\n\", msg);
     * - std::regex("[a-z]+\.txt"); → std::regex(\"[a-z]+\\.txt\");
     * - \tif (x) {\n\t\treturn;\n} → \\tif (x) {\\n\\t\\treturn;\\n}
     * 
     * @param input String containing control characters
     * @return String with control characters converted to visible escape sequences
     */
    std::string escapeForDisplay(const std::string& input);

} // namespace TextEscaping

} // namespace LabDb