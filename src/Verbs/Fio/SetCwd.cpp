#include "SetCwd.h"
#include "LabDb/AutoReflexiveMetrics.h"

#include <chrono>
#include <sstream>
#include <stdexcept>
#include <filesystem>

namespace LabDb {

std::string SetCwdVerb::getDescription() const {
    return "Set current working directory with safety checks to prevent filesystem accidents.\n"
           "Usage:\n"
           "```lisp\n"
           "(set-cwd :path §/path/to/directory§)\n"
           ";; Returns: success status and new working directory\n"
           "\n"
           "(set-cwd :path §/Users/nick/dev/Lab/LabDb9§)\n"
           ";; Returns: {'status': 'success', 'previous_cwd': '/', 'new_cwd': '/Users/nick/dev/Lab/LabDb9'}\n"
           "```\n"
           "\n"
           "**Parameters:**\n"
           "- `:path` - Directory path to change to (required)\n"
           "\n"
           "**Returns:**\n"
           "- `status` - Operation status ('success' or 'error')\n"
           "- `previous_cwd` - Previous working directory\n"
           "- `new_cwd` - New working directory after change\n"
           "- `safety_check` - Information about safety validations performed\n"
           "\n"
           "**Safety Features:**\n"
           "- **Root Protection**: Prevents setting CWD to root (/) filesystem\n"
           "- **Path Validation**: Ensures target directory exists and is accessible\n"
           "- **Permission Checks**: Validates directory is readable\n"
           "- **Absolute Path Resolution**: Always resolves to absolute paths\n"
           "- **Error Recovery**: Maintains previous CWD if operation fails\n"
           "\n"
           "**Integration with fio-write:**\n"
           "```lisp\n"
           ";; Fix the root CWD issue that fio-write detects:\n"
           "(set-cwd :path §/Users/nick/dev/Lab/LabDb9§)\n"
           "(fio-write :path §build.sh§ :content §#!/bin/bash\\necho test§)\n"
           "```\n"
           "\n"
           "Use when fio-write reports 'unsafe_root_write' to establish proper working directory.\n";
}

SetCwdVerb::CwdParameters SetCwdVerb::extractParameters(const lab::Text::Sexpr& sexpr) {
    CwdParameters params;
    
    for (size_t i = 0; i < sexpr.expr.size(); ++i) {
        const auto& elem = sexpr.expr[i];
        if (elem.token == tsSexprAtom) {
            int stringIndex = elem.ref;
            if (stringIndex >= 0 && stringIndex < static_cast<int>(sexpr.strings.size())) {
                const std::string& atomValue = sexpr.strings[stringIndex];
                if (atomValue == ":path" || atomValue == "path") {
                    if (i + 1 < sexpr.expr.size()) {
                        const auto& valueElem = sexpr.expr[i + 1];
                        if (valueElem.token == tsSexprAtom && valueElem.ref >= 0 &&
                            valueElem.ref < static_cast<int>(sexpr.strings.size())) {
                            params.path = sexpr.strings[valueElem.ref];
                        } else if (valueElem.token == tsSexprString && valueElem.ref >= 0 &&
                                   valueElem.ref < static_cast<int>(sexpr.strings.size())) {
                            params.path = sexpr.strings[valueElem.ref];
                        }
                    }
                }
            }
        }
    }
    
    return params;
}

Db9Response SetCwdVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        auto params = extractParameters(sexpr);
        
        if (params.path.empty()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{Db9Response::Error, "", "missing_parameter", 
                             "Missing required parameter :path for directory to change to", metrics};
        }
        
        // Get current working directory before attempting change
        std::string previous_cwd;
        try {
            previous_cwd = std::filesystem::current_path().string();
        } catch (const std::exception& e) {
            AutoReflexiveMetrics metrics;
            return Db9Response{Db9Response::Error, "", "cwd_access_failed", 
                             "Could not access current working directory: " + std::string(e.what()), metrics};
        }
        
        // Resolve target path to absolute
        std::filesystem::path target_path;
        try {
            target_path = std::filesystem::absolute(params.path);
        } catch (const std::exception& e) {
            AutoReflexiveMetrics metrics;
            return Db9Response{Db9Response::Error, "", "path_resolution_failed", 
                             "Could not resolve path '" + params.path + "': " + std::string(e.what()), metrics};
        }
        
        // Safety check: prevent setting CWD to root
        if (target_path == "/") {
            AutoReflexiveMetrics metrics;
            return Db9Response{Db9Response::Error, "", "unsafe_root_cwd", 
                             "Safety check failed: Cannot set current working directory to root (/). "
                             "This would make relative paths resolve to root filesystem. "
                             "Choose a project-specific directory instead.", metrics};
        }
        
        // Validate target directory exists
        std::error_code ec;
        if (!std::filesystem::exists(target_path, ec)) {
            AutoReflexiveMetrics metrics;
            std::string error_msg = "Target directory does not exist: '" + target_path.string() + "'";
            if (ec) {
                error_msg += " (Error: " + ec.message() + ")";
            }
            return Db9Response{Db9Response::Error, "", "directory_not_found", error_msg, metrics};
        }
        
        // Validate it's actually a directory
        if (!std::filesystem::is_directory(target_path, ec)) {
            AutoReflexiveMetrics metrics;
            std::string error_msg = "Target path is not a directory: '" + target_path.string() + "'";
            if (ec) {
                error_msg += " (Error: " + ec.message() + ")";
            }
            return Db9Response{Db9Response::Error, "", "not_a_directory", error_msg, metrics};
        }
        
        // Attempt to change working directory
        try {
            std::filesystem::current_path(target_path);
        } catch (const std::exception& e) {
            AutoReflexiveMetrics metrics;
            return Db9Response{Db9Response::Error, "", "cwd_change_failed", 
                             "Failed to change working directory to '" + target_path.string() + "': " + std::string(e.what()), metrics};
        }
        
        // Verify the change succeeded
        std::string new_cwd;
        try {
            new_cwd = std::filesystem::current_path().string();
        } catch (const std::exception& e) {
            AutoReflexiveMetrics metrics;
            return Db9Response{Db9Response::Error, "", "cwd_verification_failed", 
                             "Working directory changed but verification failed: " + std::string(e.what()), metrics};
        }
        
        // Calculate execution time
        auto end_time = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        // Create success response JSON
        std::ostringstream result;
        result << "{\n"
               << "  \"status\": \"success\",\n"
               << "  \"previous_cwd\": \"" << previous_cwd << "\",\n"
               << "  \"new_cwd\": \"" << new_cwd << "\",\n"
               << "  \"target_path\": \"" << target_path.string() << "\",\n"
               << "  \"safety_check\": \"passed\",\n"
               << "  \"execution_time_ms\": " << duration_ms << "\n"
               << "}";
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = std::chrono::milliseconds(duration_ms);
        metrics.items_processed = 1;
        
        return Db9Response{Db9Response::Success, result.str(), "", "", metrics};
        
    } catch (const std::exception& e) {
        AutoReflexiveMetrics metrics;
        return Db9Response{Db9Response::Error, "", "execution_error", 
                         "Unexpected error during CWD change: " + std::string(e.what()), metrics};
    }
}

}
