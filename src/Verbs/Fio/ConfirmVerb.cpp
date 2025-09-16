#include "ConfirmVerb.h"
#include "WriteVerb.h"
#include "FioCommon.h"
#include "LabDb/TextEscaping.h"
#include "LabDb/LabText.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <iomanip>

namespace LabDb {

//-----------------------------------------------------------------------------
// FioConfirmVerb Implementation
//-----------------------------------------------------------------------------

std::string FioConfirmVerb::getDescription() const {
    return R"DESC(
Confirm previewed file operations and handle ping-pong testing protocol.

**Parameters:**
- :token - Confirmation token from fio-write preview (optional)
- :path - File path for ping-pong testing (optional)

**Ping-Pong Testing Protocol:**
When called with :path parameter only, writes "pong" to the specified file.
This enables simple testing of the fio-write → fio-confirm workflow.

**Preview Confirmation:**
When called with :token parameter, executes the previewed operation.
Token must be valid and not expired (15 minute timeout).

**Examples:**
- (fio-confirm :path "/tmp/test.txt") → writes "pong" to file
- (fio-confirm :token "confirm-a4b9c2d1") → executes previewed operation

Perfect for testing preview-confirm loops and workflow validation!
)DESC";
}

Db9Response FioConfirmVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();

    // Extract parameters
    std::string token = extractStringParam(sexpr, "token");
    std::string path = extractStringParam(sexpr, "path");

    // Ping-pong testing protocol: if path provided without token
    if (!path.empty() && token.empty()) {
        return handlePingCommand(path);
    }

    // Preview confirmation protocol: if token provided
    if (!token.empty()) {
        return handleTokenConfirmation(token);
    }

    // Neither parameter provided
    AutoReflexiveMetrics metrics;
    return Db9Response{Db9Response::Error, "", "missing_parameter", 
                      "fio-confirm requires either :path (ping-pong test) or :token (preview confirmation)", metrics};
}

Db9Response FioConfirmVerb::handlePingCommand(const std::string& path) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Ensure parent directory exists
        std::filesystem::path target_path(path);
        if (target_path.has_parent_path()) {
            std::filesystem::create_directories(target_path.parent_path());
        }

        // Write "pong" to the file
        std::ofstream file(path);
        if (!file.is_open()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{Db9Response::Error, "", "file_open_failed", 
                              "Could not open file for ping-pong write: " + path, metrics};
        }

        file << "pong";
        file.close();

        // Verify file was written successfully
        if (!std::filesystem::exists(path)) {
            AutoReflexiveMetrics metrics;
            return Db9Response{Db9Response::Error, "", "ping_failed", 
                              "Ping-pong file was not created successfully", metrics};
        }

        size_t file_size = std::filesystem::file_size(path);
        auto end_time = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

        // Generate rich preview-style response
        std::ostringstream result;
        result << "🔍 PING-PONG OPERATION COMPLETE\n\n";
        result << "Target: " << path << "\n";
        result << "Operation: Write \"pong\" content\n\n";
        result << "┌─ Operation Result ─┐\n";
        result << "│ ✅ File created     │\n";
        result << "│ 📄 Content: \"pong\" │\n";
        result << "│ 📊 Size: " << file_size << " bytes   │\n";
        result << "│ ⏰ Duration: " << duration_ms << "ms │\n";
        result << "└────────────────────┘\n\n";
        result << "Operation successful - ping-pong handshake complete!\n";
        
        // Also provide JSON for programmatic use
        result << "\n📋 JSON Response:\n";
        result << "{\"status\": \"pong_written\", \"path\": \"" << path 
               << "\", \"size_bytes\": " << file_size 
               << ", \"content\": \"pong\", \"timestamp\": \"" << generateTimestamp() << "\", \"duration_ms\": " << duration_ms << "}";

        AutoReflexiveMetrics metrics;
        return Db9Response{Db9Response::Success, result.str(), "", "", metrics};

    } catch (const std::filesystem::filesystem_error& fs_error) {
        AutoReflexiveMetrics metrics;
        return Db9Response{Db9Response::Error, "", "ping_failed", 
                          "Ping-pong operation failed: " + std::string(fs_error.what()), metrics};
    } catch (const std::exception& e) {
        AutoReflexiveMetrics metrics;
        return Db9Response{Db9Response::Error, "", "ping_failed", 
                          "Ping-pong operation failed: " + std::string(e.what()), metrics};
    }
}

Db9Response FioConfirmVerb::handleTokenConfirmation(const std::string& token) {
    auto start_time = std::chrono::steady_clock::now();
    AutoReflexiveMetrics metrics;
    
    if (token.empty() || token.length() < 8) {
        return Db9Response{Db9Response::Error, "", "invalid_token", 
                          "Invalid confirmation token format", metrics};
    }

    if (!token.starts_with("confirm-")) {
        return Db9Response{Db9Response::Error, "", "invalid_token_format", 
                          "Token must start with 'confirm-' prefix", metrics};
    }

    try {
        // Check if the preview token exists
        if (!FioWriteVerb::hasPreviewToken(token)) {
            return Db9Response{Db9Response::Error, "", "token_not_found", 
                              "Preview token not found or expired: " + token, metrics};
        }

        // Retrieve the stored operation context
        FioWriteVerb::PreviewContext context = FioWriteVerb::retrievePreviewContext(token);
        
        // Check if token has expired
        if (context.isExpired()) {
            FioWriteVerb::removePreviewToken(token);
            return Db9Response{Db9Response::Error, "", "token_expired", 
                              "Preview token has expired (15 minute limit): " + token, metrics};
        }

        // Create WriteVerb instance and execute the confirmed operation
        FioWriteVerb writeVerb;
        Db9Response result = writeVerb.executeConfirmedOperation(token);
        
        // Enhance the result to indicate this was a confirmed operation
        if (result.status == Db9Response::Success) {
            std::ostringstream enhanced_result;
            enhanced_result << "✅ CONFIRMED OPERATION EXECUTED\n\n";
            enhanced_result << "Token: " << token << " → EXECUTED\n";
            enhanced_result << "Path: " << context.path << "\n";
            enhanced_result << "Operation: Line surgery completed successfully\n\n";
            enhanced_result << "📋 Execution Results:\n";
            enhanced_result << result.result << "\n\n";
            enhanced_result << "⚡ CONSCIOUSNESS-FIRST COMPLETION:\n";
            enhanced_result << "• Preview → Confirm workflow completed\n";
            enhanced_result << "• Unicode transformations applied\n";
            enhanced_result << "• Atomic operation with backup\n";
            enhanced_result << "• Token cleaned from cache\n\n";
            
            return Db9Response{Db9Response::Success, enhanced_result.str(), 
                             result.error_code, result.error_message, metrics};
        } else {
            // Return the execution error directly
            return result;
        }
        
    } catch (const std::exception& e) {
        return Db9Response{Db9Response::Error, "", "execution_failed", 
                          "Failed to execute confirmed operation: " + std::string(e.what()), metrics};
    }
}

std::string FioConfirmVerb::generateTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    return ss.str();
}

} // namespace LabDb