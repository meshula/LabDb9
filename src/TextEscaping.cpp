#include "LabDb/TextEscaping.h"

namespace LabDb {
namespace TextEscaping {

std::string unescapeDb9String(const std::string& input) {
    std::string result = input;
    size_t pos = 0;

    // Replace backslash symbol (U+203B → backslash)
    while ((pos = result.find("※", pos)) != std::string::npos) {
        result.replace(pos, 3, "\\");
        pos += 1;
    }

    // Replace quote symbol (U+2033 → quote)
    pos = 0;
    while ((pos = result.find("″", pos)) != std::string::npos) {
        result.replace(pos, 3, "\"");
        pos += 1;
    }

    // Replace newline symbol (U+21B5 → \n)
    pos = 0;
    while ((pos = result.find("↵", pos)) != std::string::npos) {
        result.replace(pos, 3, "\n");
        pos += 1;
    }

    // Replace tab symbol (U+21E5 → \t)
    pos = 0;
    while ((pos = result.find("⇥", pos)) != std::string::npos) {
        result.replace(pos, 3, "\t");
        pos += 1;
    }
    return result;
}

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

} // namespace TextEscaping
} // namespace LabDb
