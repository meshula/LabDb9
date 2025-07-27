#pragma once

#include <string>
#include <vector>

namespace LabDb {
namespace FileSearch {

//-----------------------------------------------------------------------------
// Directory Traversal Engine
//-----------------------------------------------------------------------------

/**
 * Configuration for directory traversal operations
 */
struct TraversalConfig {
    int max_depth{0};              // 0 = no recursion, -1 = unlimited
    bool follow_symlinks{false};   // Follow symbolic links
    bool include_hidden{false};    // Include .hidden files
    std::vector<std::string> exclude_dirs; // Directories to skip
    size_t max_file_size{10 * 1024 * 1024}; // 10MB default
};

/**
 * Information about a discovered file
 */
struct FileInfo {
    std::string path;              // Full file path
    std::string relative_path;     // Path relative to search root
    std::string filename;          // Just the filename
    std::string extension;         // File extension (with dot)
    size_t file_size{0};          // File size in bytes
    bool is_directory{false};     // True if this is a directory
    bool is_symlink{false};       // True if this is a symbolic link
    bool is_hidden{false};        // True if filename starts with .
    int depth{0};                 // Depth from search root
};

/**
 * High-performance directory traversal engine
 * Designed for fio-search-ext advanced search capabilities
 */
class DirectoryTraversal {
public:
    explicit DirectoryTraversal(const TraversalConfig& config = {});
    
    // Main traversal interface
    std::vector<FileInfo> traverse(const std::string& root_path);
    
    // Configuration
    void setConfig(const TraversalConfig& config);
    const TraversalConfig& getConfig() const;
    
    // Statistics
    size_t getFilesFound() const { return files_found_; }
    size_t getDirectoriesScanned() const { return dirs_scanned_; }
    size_t getFilesSkipped() const { return files_skipped_; }
    size_t getSymlinksEncountered() const { return symlinks_encountered_; }
    double getTraversalTimeMs() const { return traversal_time_ms_; }

    // Error handling
    const std::vector<std::string>& getErrors() const { return errors_; }
    const std::vector<std::string>& getWarnings() const { return warnings_; }

private:
    // Core traversal implementation
    void traverseRecursive(const std::string& current_path,
                          const std::string& root_path,
                          int current_depth,
                          std::vector<FileInfo>& results);
    
    // Path validation and filtering
    bool shouldSkipDirectory(const std::string& dir_name) const;
    bool shouldIncludeFile(const FileInfo& file_info) const;
    bool isHidden(const std::string& filename) const;
    
    // Error handling
    void addError(const std::string& error);
    void addWarning(const std::string& warning);
    
    // Configuration
    TraversalConfig config_;
    
    // Statistics (mutable for const getters)
    mutable size_t files_found_{0};
    mutable size_t dirs_scanned_{0};
    mutable size_t files_skipped_{0};
    mutable size_t symlinks_encountered_{0};
    mutable double traversal_time_ms_{0.0};
    
    // Error tracking
    std::vector<std::string> errors_;
    std::vector<std::string> warnings_;
};

//-----------------------------------------------------------------------------
// Utility Functions
//-----------------------------------------------------------------------------

/**
 * Check if a path ends with a directory separator
 * Handles both Unix (/) and Windows (\\) separators
 */
bool isDirectoryPath(const std::string& path);

/**
 * Normalize directory path to ensure it ends with separator
 * Returns normalized path for consistent directory handling
 */
std::string normalizeDirectoryPath(const std::string& path);

/**
 * Get file extension including the dot (e.g., ".cpp", ".md")
 * Returns empty string if no extension found
 */
std::string getFileExtension(const std::string& filename);

/**
 * Check if file extension matches any in the provided list
 * Case-insensitive comparison
 */
bool matchesExtensions(const std::string& filename, 
                      const std::vector<std::string>& extensions);

/**
 * Calculate relative path from root to target
 * Handles path normalization and cross-platform compatibility
 */
std::string getRelativePath(const std::string& root_path, 
                           const std::string& target_path);

/**
 * Default exclude directories for common development scenarios
 * Includes .git, node_modules, build, .vscode, etc.
 */
std::vector<std::string> getDefaultExcludeDirs();

} // namespace FileSearch
} // namespace LabDb
