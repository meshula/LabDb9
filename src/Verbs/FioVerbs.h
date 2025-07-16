#pragma once

#include "LabDb/Verbs.h"
#include "LabDb/Db9Response.h"
#include "LabDb/LabText.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <memory>

namespace LabDb {

class Db9Dispatcher;

// Forward declarations for line surgery
struct LineRange;

//-----------------------------------------------------------------------------
// File I/O Verb Declarations
//-----------------------------------------------------------------------------

/// Write content to file with atomic operations and backup support
class FioWriteVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "fio-write"; }
    std::string getDescription() const override;
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;

private:
    struct WriteParameters {
        std::string path;
        std::string content;
        std::string encoding = "utf-8";
        bool backup = true;
        bool atomic = true;
        std::string mode = "create"; // create, append, overwrite
    };

    Db9Response performTouchOperation(const std::string& path, std::chrono::steady_clock::time_point start_time);
    WriteParameters extractParameters(const lab::Text::Sexpr& sexpr);
    Db9Response performWrite(const WriteParameters& params);
    Db9Response performFullFileWrite(const std::string& path, const std::string& content, std::chrono::steady_clock::time_point start_time);
    Db9Response performLineSurgery(const std::string& path, const std::string& content, const std::string& lines_param, const std::string& mode, std::chrono::steady_clock::time_point start_time);
    int performLineReplacement(std::vector<std::string>& lines, const std::vector<std::string>& new_content, const LineRange& range);
    int performLineInsertion(std::vector<std::string>& lines, const std::vector<std::string>& new_content, const LineRange& range);
    int performLineAppend(std::vector<std::string>& lines, const std::vector<std::string>& new_content, const LineRange& range);
    std::string createBackupPath(const std::string& original_path);
    bool validatePath(const std::string& path);
    std::string generateTimestamp();
};

/// List directory contents with filtering and metadata
class FioListVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "fio-list"; }
    std::string getDescription() const override;
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;

private:
    struct ListParameters {
        std::string path;
        bool recursive = false;
        std::string filter = "*"; // glob pattern
        bool show_hidden = false;
        bool include_metadata = true;
        std::string sort_by = "name"; // name, size, date
    };

    struct FileInfo {
        std::string name;
        std::string path;
        std::string type; // file, directory, symlink
        size_t size;
        std::string modified;
        std::string permissions;
    };

    ListParameters extractParameters(const lab::Text::Sexpr& sexpr);
    Db9Response performListing(const ListParameters& params);
    std::vector<FileInfo> scanDirectory(const std::string& path, const ListParameters& params);
    bool matchesFilter(const std::string& filename, const std::string& filter);
    std::string formatFileInfo(const FileInfo& info);
    std::string formatFileList(const std::vector<FileInfo>& files);
};

/// Read file content with head/tail/full modes
class FioReadVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "fio-read"; }
    std::string getDescription() const override;
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;

private:
    struct ReadParameters {
        std::string path;
        std::string mode = "full"; // full, head, tail
        size_t lines = 25;
        size_t offset = 0;
        std::string encoding = "utf-8";
        bool show_line_numbers = false;
    };

    ReadParameters extractParameters(const lab::Text::Sexpr& sexpr);
    Db9Response performRead(const ReadParameters& params);
    std::string readFullFile(const std::string& path, const std::string& encoding);
    std::string readHeadLines(const std::string& path, size_t lines, size_t offset, bool line_numbers);
    std::string readTailLines(const std::string& path, size_t lines, size_t offset, bool line_numbers);
};

/// Search content within files with literal/glob/regex modes
class FioSearchVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "fio-search"; }
    std::string getDescription() const override;
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;

private:
    enum SearchMode { Literal, Glob, Regex };
    
    struct SearchMatch {
        int line;
        int column;
        std::string line_content;
        std::string context;
    };
    
    struct SearchParameters {
        std::string path;
        std::string pattern;
        SearchMode mode = Literal;
        std::string lines_param; // Optional line range
        bool case_sensitive = true;
        int context_lines = 0;
        std::vector<std::string> paths; // For multi-file search
    };

    SearchParameters extractParameters(const lab::Text::Sexpr& sexpr);
    Db9Response performSearch(const SearchParameters& params);
    std::vector<SearchMatch> searchLiteral(const std::vector<std::string>& file_lines, const std::string& pattern, bool case_sensitive);
    std::vector<SearchMatch> searchGlob(const std::vector<std::string>& file_lines, const std::string& pattern, bool case_sensitive);
    std::vector<SearchMatch> searchRegex(const std::vector<std::string>& file_lines, const std::string& pattern, bool case_sensitive);
    std::string formatSearchResults(const std::vector<SearchMatch>& matches, const SearchParameters& params);
    std::string createContextWindow(const std::vector<std::string>& file_lines, int line_num, int context_lines);
};

//-----------------------------------------------------------------------------
// Registration Function
//-----------------------------------------------------------------------------

/// Initialize and register all FIO verbs with the dispatcher
void initFioVerbRegistration(Db9Dispatcher& dispatcher);

//-----------------------------------------------------------------------------
// Utility Functions
//-----------------------------------------------------------------------------

namespace FioUtils {
    /// Validate file path for security (prevent directory traversal)
    bool isPathSafe(const std::string& path);
    
    /// Create directories recursively if they don't exist
    bool ensureDirectoryExists(const std::string& path);
    
    /// Generate timestamp string for backups
    std::string getTimestamp();
    
    /// Escape JSON string for response formatting
    std::string escapeJsonString(const std::string& input);
    
    /// Get file permissions as string (cross-platform)
    std::string getFilePermissions(const std::filesystem::path& path);
    
    /// Get human-readable file size
    std::string formatFileSize(size_t bytes);
    
    /// Convert filesystem time to ISO string
    std::string formatFileTime(const std::filesystem::file_time_type& time);

    //-------------------------------------------------------------------------
    // Triadic Consciousness Path Context
    //-------------------------------------------------------------------------
    
    /// Triadic path context for conscious filesystem operations
    struct PathContext {
        std::string requested_path;     // Motion: User's intention
        std::string resolved_path;      // Memory: System understanding  
        std::string working_directory;  // Field: Actual context
        std::string path_type;          // "relative" or "absolute"
        bool exists;                    // Field reality check
        std::string existence_context;  // Context about existence/non-existence
        
        std::string toJsonString() const;
    };
    
    /// Create triadic path context for filesystem operations
    PathContext createPathContext(const std::string& requested_path);
    
    /// Generate context-aware error message for file operations
    std::string createContextualErrorMessage(
        const PathContext& context,
        const std::string& operation,
        const std::string& base_error
    );
    
    /// Generate guidance for empty directory listings
    std::string createDirectoryGuidance(const PathContext& context);
    
    /// Get summary of current working directory contents for context
    std::string getWorkingDirectorySummary(const std::string& working_dir);
    
    /// Add awareness fairy guidance for empty directory listings
    std::string addEmptyDirectoryAwareness(const std::string& json_result, size_t file_count);
}

} // namespace LabDb