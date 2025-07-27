#include "LabDb/DirectoryTraversal.h"
#include <filesystem>
#include <chrono>
#include <algorithm>
#include <iostream>

namespace fs = std::filesystem;
namespace LabDb {
namespace FileSearch {

//-----------------------------------------------------------------------------
// DirectoryTraversal Implementation
//-----------------------------------------------------------------------------

DirectoryTraversal::DirectoryTraversal(const TraversalConfig& config) 
    : config_(config) {
    // Initialize with default exclude directories if none provided
    if (config_.exclude_dirs.empty()) {
        config_.exclude_dirs = getDefaultExcludeDirs();
    }
}

std::vector<FileInfo> DirectoryTraversal::traverse(const std::string& root_path) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Reset statistics
    files_found_ = 0;
    dirs_scanned_ = 0;
    files_skipped_ = 0;
    symlinks_encountered_ = 0;
    errors_.clear();
    warnings_.clear();
    
    std::vector<FileInfo> results;
    
    try {
        // Validate root path exists
        if (!fs::exists(root_path)) {
            addError("Root path does not exist: " + root_path);
            return results;
        }
        
        // Handle single file vs directory
        if (fs::is_regular_file(root_path)) {
            // Single file - create FileInfo and return
            FileInfo file_info;
            file_info.path = fs::absolute(root_path).string();
            file_info.relative_path = fs::path(root_path).filename().string();
            file_info.filename = fs::path(root_path).filename().string();
            file_info.extension = getFileExtension(file_info.filename);
            file_info.file_size = fs::file_size(root_path);
            file_info.is_directory = false;
            file_info.is_symlink = fs::is_symlink(root_path);
            file_info.is_hidden = isHidden(file_info.filename);
            file_info.depth = 0;
            
            if (shouldIncludeFile(file_info)) {
                results.push_back(file_info);
                files_found_++;
            } else {
                files_skipped_++;
            }
        } else if (fs::is_directory(root_path)) {
            // Directory traversal
            traverseRecursive(root_path, root_path, 0, results);
        } else {
            addError("Path is neither file nor directory: " + root_path);
        }
        
    } catch (const fs::filesystem_error& e) {
        addError("Filesystem error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        addError("Unexpected error: " + std::string(e.what()));
    }
    
    // Calculate traversal time
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    traversal_time_ms_ = duration.count() / 1000.0;
    
    return results;
}

void DirectoryTraversal::setConfig(const TraversalConfig& config) {
    config_ = config;
}

const TraversalConfig& DirectoryTraversal::getConfig() const {
    return config_;
}

// Private Implementation
//-----------------------------------------------------------------------------
// Private Implementation
//-----------------------------------------------------------------------------

void DirectoryTraversal::traverseRecursive(const std::string& current_path,
                                         const std::string& root_path,
                                         int current_depth,
                                         std::vector<FileInfo>& results) {
    try {
        // Check depth limit
        if (config_.max_depth >= 0 && current_depth > config_.max_depth) {
            return;
        }
        
        dirs_scanned_++;
        
        // Iterate through directory entries
        for (const auto& entry : fs::directory_iterator(current_path)) {
            try {
                const std::string entry_path = entry.path().string();
                const std::string entry_name = entry.path().filename().string();
                
                // Handle symbolic links
                if (entry.is_symlink()) {
                    symlinks_encountered_++;
                    if (!config_.follow_symlinks) {
                        addWarning("Skipping symlink: " + entry_path);
                        continue;
                    }
                }
                
                // Handle directories
                if (entry.is_directory()) {
                    // Check if we should skip this directory
                    if (shouldSkipDirectory(entry_name)) {
                        addWarning("Skipping excluded directory: " + entry_path);
                        continue;
                    }
                    
                    // Check hidden directory policy
                    if (!config_.include_hidden && isHidden(entry_name)) {
                        addWarning("Skipping hidden directory: " + entry_path);
                        continue;
                    }
                    
                    // Recurse into subdirectory
                    traverseRecursive(entry_path, root_path, current_depth + 1, results);
                }
                // Handle regular files
                else if (entry.is_regular_file()) {
                    FileInfo file_info;
                    file_info.path = fs::absolute(entry_path).string();
                    file_info.relative_path = getRelativePath(root_path, file_info.path);
                    file_info.filename = entry_name;
                    file_info.extension = getFileExtension(entry_name);
                    file_info.file_size = entry.file_size();
                    file_info.is_directory = false;
                    file_info.is_symlink = entry.is_symlink();
                    file_info.is_hidden = isHidden(entry_name);
                    file_info.depth = current_depth;
                    
                    // Apply file filtering
                    if (shouldIncludeFile(file_info)) {
                        results.push_back(file_info);
                        files_found_++;
                    } else {
                        files_skipped_++;
                    }
                }
                
            } catch (const fs::filesystem_error& e) {
                addError("Error processing entry " + entry.path().string() + ": " + e.what());
                continue;
            }
        }
        
    } catch (const fs::filesystem_error& e) {
        addError("Error traversing directory " + current_path + ": " + e.what());
    }
}

bool DirectoryTraversal::shouldSkipDirectory(const std::string& dir_name) const {
    return std::find(config_.exclude_dirs.begin(), config_.exclude_dirs.end(), dir_name) 
           != config_.exclude_dirs.end();
}

bool DirectoryTraversal::shouldIncludeFile(const FileInfo& file_info) const {
    // Check file size limit
    if (file_info.file_size > config_.max_file_size) {
        return false;
    }
    
    // Check hidden file policy
    if (!config_.include_hidden && file_info.is_hidden) {
        return false;
    }
    
    return true;
}

bool DirectoryTraversal::isHidden(const std::string& filename) const {
    return !filename.empty() && filename[0] == '.';
}

void DirectoryTraversal::addError(const std::string& error) {
    errors_.push_back(error);
}

void DirectoryTraversal::addWarning(const std::string& warning) {
    warnings_.push_back(warning);
}

//-----------------------------------------------------------------------------
// Utility Functions
//-----------------------------------------------------------------------------

bool isDirectoryPath(const std::string& path) {
    if (path.empty()) return false;
    char last_char = path.back();
    return last_char == '/' || last_char == '\\';
}

std::string normalizeDirectoryPath(const std::string& path) {
    if (path.empty()) return path;
    
    std::string normalized = path;
    
    // Convert backslashes to forward slashes for consistency
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    
    // Ensure path ends with separator
    if (normalized.back() != '/') {
        normalized += '/';
    }
    
    return normalized;
}

std::string getFileExtension(const std::string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos || dot_pos == 0 || dot_pos == filename.length() - 1) {
        return "";
    }
    return filename.substr(dot_pos);
}

bool matchesExtensions(const std::string& filename, 
                      const std::vector<std::string>& extensions) {
    if (extensions.empty()) return true; // No filter means all extensions
    
    std::string file_ext = getFileExtension(filename);
    
    // Case-insensitive comparison
    std::string file_ext_lower = file_ext;
    std::transform(file_ext_lower.begin(), file_ext_lower.end(), 
                   file_ext_lower.begin(), ::tolower);
    
    for (const auto& ext : extensions) {
        std::string ext_lower = ext;
        std::transform(ext_lower.begin(), ext_lower.end(), 
                       ext_lower.begin(), ::tolower);
        
        if (file_ext_lower == ext_lower) {
            return true;
        }
    }
    
    return false;
}

std::string getRelativePath(const std::string& root_path, 
                           const std::string& target_path) {
    try {
        fs::path root(root_path);
        fs::path target(target_path);
        
        // Convert to absolute paths for consistency
        root = fs::absolute(root);
        target = fs::absolute(target);
        
        // Calculate relative path
        fs::path relative = fs::relative(target, root);
        return relative.string();
        
    } catch (const fs::filesystem_error&) {
        // Fallback: try simple string manipulation
        std::string abs_root = fs::absolute(root_path).string();
        std::string abs_target = fs::absolute(target_path).string();
        
        if (abs_target.find(abs_root) == 0) {
            return abs_target.substr(abs_root.length());
        }
        
        return target_path; // Return original if can't calculate relative
    }
}

std::vector<std::string> getDefaultExcludeDirs() {
    return {
        ".git",
        ".svn",
        ".hg",
        "node_modules",
        "build",
        "dist",
        ".vscode",
        ".idea",
        ".vs",
        "bin",
        "obj",
        "target",
        ".gradle",
        ".cache",
        "__pycache__",
        ".pytest_cache",
        ".mypy_cache",
        "coverage",
        ".coverage",
        ".nyc_output",
        ".DS_Store"
    };
}

} // namespace FileSearch
} // namespace LabDb