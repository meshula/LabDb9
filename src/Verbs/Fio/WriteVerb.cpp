#include "WriteVerb.h"
#include "FioCommon.h"
#include "LabDb/TextEscaping.h"
#include "LabDb/LabText.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <filesystem>
#include <random>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <iomanip>

namespace LabDb {


//-----------------------------------------------------------------------------
// FioWriteVerb Implementation
//-----------------------------------------------------------------------------

std::string FioWriteVerb::getDescription() const {
	return R"DESC(
Write content to file with revolutionary line-syntax precision and Unicode escaping paradise.

**Parameters:**
- :path - Target file path (required)
- :content - Content to write (required)
- :lines - Line surgery specification (optional)
- :mode - Line operation mode: append (safe default), replace, insert, prepend

**Line Syntax:**
- @N - Single line N
- @N:M - Lines N through M
- @N:-M - M lines ending at line N
- @e:-M - Last M lines
- No :lines - Full file write

**Unicode Escaping System:**
- \ → \\ (backslash for regex, file paths, C++ escapes)
- " → " (quotes without JSON conflicts)
-
 → actual newline (file structure)
- 	 → actual tab (indentation)

Perfect for refactoring, debugging, and code generation!
)DESC";
}

Db9Response FioWriteVerb::execute(const lab::Text::Sexpr& sexpr) {
	auto start_time = std::chrono::steady_clock::now();

	// Extract parameters
	std::string path = extractStringParam(sexpr, "path");
	std::string content = extractStringParam(sexpr, "content");
	std::string lines_param = extractStringParam(sexpr, "lines");
	std::string mode = extractStringParam(sexpr, "mode");

	WriteMode::Type write_mode = WriteMode::fromString(mode);

	if (write_mode == WriteMode::Unrecognized) {
		AutoReflexiveMetrics metrics;
		return Db9Response{Db9Response::Error, "", "invalid_mode",
								"Invalid mode: '" + mode + "'. Must be 'replace', 'insert', 'append', or 'prepend'", metrics};
	}

	if (write_mode == WriteMode::Unspecified) {
		write_mode = WriteMode::Append;
	}

	if (path.empty()) {
		AutoReflexiveMetrics metrics;
		return Db9Response{Db9Response::Error, "", "missing_path", "fio-write requires :path parameter", metrics};
	}

	try {
		bool use_line_surgery = false;

		if (!lines_param.empty()) {
			use_line_surgery = true;
		} else if (write_mode == WriteMode::Insert || write_mode == WriteMode::Prepend) {
			use_line_surgery = true;
		} else if (write_mode == WriteMode::Append && std::filesystem::exists(path)) {
			// Only use line surgery for append if file exists
			use_line_surgery = true;
		}
		// Note: Removed problematic condition that routed to line surgery for simple overwrites
		// Full file write should be the default when no :lines parameter is specified

		if (use_line_surgery) {
			std::string effective_lines = lines_param.empty() ? "@e:0" : lines_param;
			LineRange range = parseLineSpec(effective_lines);
			if (!range.valid) {
				AutoReflexiveMetrics metrics;
				return Db9Response{Db9Response::Error, "", "invalid_line_spec", "Invalid line specification: " + effective_lines, metrics};
			}

			content = LabDb::TextEscaping::unescapeDb9String(content);
			return performLineSurgery(path, content, range, write_mode, start_time);
		}

		if (!content.empty()) {
			content = LabDb::TextEscaping::unescapeDb9String(content);
			// SAFETY FIX: Route full file writes through confirmation system like line surgery
			// Instead of bypassing safety with direct performFullFileWrite() call
			LineRange full_range;
			full_range.type = LineRange::Full;
			full_range.valid = true;
			return performLineSurgery(path, content, full_range, write_mode, start_time);
		}

		return performTouchOperation(path, start_time);
	} catch (const std::filesystem::filesystem_error& fs_error) {
		AutoReflexiveMetrics metrics;
		std::string enhanced_error = "Write operation failed: " + std::string(fs_error.what());
		return Db9Response{Db9Response::Error, "", "write_failed", enhanced_error, metrics};
	} catch (const std::exception& e) {
		AutoReflexiveMetrics metrics;
		return Db9Response{Db9Response::Error, "", "write_failed", std::string("Write operation failed: ") + e.what(), metrics};
	}
}

Db9Response FioWriteVerb::performTouchOperation(const std::string& path, std::chrono::steady_clock::time_point start_time) {
	try {
		std::filesystem::path target_path(path);
		if (target_path.has_parent_path()) {
			std::filesystem::create_directories(target_path.parent_path());
		}
		bool file_existed = std::filesystem::exists(path);
		size_t file_size = 0;
		if (file_existed) {
			auto now = std::filesystem::file_time_type::clock::now();
			std::filesystem::last_write_time(path, now);
			file_size = std::filesystem::file_size(path);
		} else {
			std::ofstream file(path);
			if (!file.is_open()) {
				AutoReflexiveMetrics metrics;
				return Db9Response{Db9Response::Error, "", "file_open_failed", "Could not create file", metrics};
			}
			file.close();
		}
		auto end_time = std::chrono::steady_clock::now();
		auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
		std::ostringstream result;
		result << "{\"status\": \"" << (file_existed ? "touched" : "created") << "\", \"path\": \"" << path << "\", \"size_bytes\": " << file_size << "}";
		AutoReflexiveMetrics metrics;
		return Db9Response{Db9Response::Success, result.str(), "", "", metrics};
	} catch (const std::exception& e) {
		AutoReflexiveMetrics metrics;
		return Db9Response{Db9Response::Error, "", "touch_failed", std::string("Touch operation failed: ") + e.what(), metrics};
	}
}

Db9Response FioWriteVerb::performFullFileWrite(const std::string& path, const std::string& content, std::chrono::steady_clock::time_point start_time) {
	std::filesystem::path target_path(path);
	if (target_path.is_relative()) {
		try {
			auto cwd = std::filesystem::current_path();
			if (cwd == "/") {
				AutoReflexiveMetrics metrics;
				std::string safety_message = "Safety check failed: Cannot write relative path '" + path + "' when current working directory is root (/). Use absolute paths or run from appropriate working directory.";
				return Db9Response{Db9Response::Error, "", "unsafe_root_write", safety_message, metrics};
			}
		} catch (const std::exception& e) {
			AutoReflexiveMetrics metrics;
			return Db9Response{Db9Response::Error, "", "cwd_check_failed", "Error checking current working directory: " + std::string(e.what()), metrics};
		}
	}
	if (target_path.has_parent_path()) {
		std::filesystem::create_directories(target_path.parent_path());
	}
	std::ofstream file(path);
	if (!file.is_open()) {
		AutoReflexiveMetrics metrics;
		return Db9Response{Db9Response::Error, "", "file_open_failed", "Could not open file for writing", metrics};
	}
	file << content;
	file.close();
	size_t file_size = std::filesystem::file_size(target_path);
	auto end_time = std::chrono::steady_clock::now();
	auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
	std::ostringstream result;
	result << "{\"status\": \"written\", \"path\": \"" << path << "\", \"size_bytes\": " << file_size << "}";
	AutoReflexiveMetrics metrics;
	return Db9Response{Db9Response::Success, result.str(), "", "", metrics};
}

// Stub implementations for remaining methods
Db9Response FioWriteVerb::performLineSurgery(const std::string& path, const std::string& content, const LineRange& range, WriteMode::Type write_mode, std::chrono::steady_clock::time_point start_time) {
	// Check if this operation should be previewed
    std::string preview_result;
	if (shouldPreview(path, range, write_mode, content, preview_result)) {
        // Phase C1: Use the preview result generated by shouldPreview()
        // instead of generating a new one that ignores the consciousness-first priority
        std::string token;
        size_t token_pos = preview_result.find("Token: confirm-");
        if (token_pos != std::string::npos) {
            token_pos += 7; // Skip "Token: "
            size_t token_end = preview_result.find("\n", token_pos);
            token = preview_result.substr(token_pos, token_end - token_pos);
        } else {
            // Fallback: generate new token if extraction fails
            token = generatePreviewToken();
        }
        return generatePreviewResponseFromExisting(preview_result, token, start_time);
	}

	// Direct execution (for non-complex operations)
	return executeLineSurgery(path, content, range, write_mode, start_time);
}

Db9Response FioWriteVerb::executeLineSurgery(const std::string& path, const std::string& content, const LineRange& range, WriteMode::Type write_mode, std::chrono::steady_clock::time_point start_time) {
	// For now, handle the trivial case of full file write
	// This enables basic testing while we implement line surgery operations

	if (range.type == LineRange::Full) {
		// Full file write - delegate to existing implementation
		return performFullFileWrite(path, content, start_time);
	}

	// For line surgery operations, we need to:
	// 1. Read existing file content into lines
	// 2. Calculate precise indices for the range
	// 3. Create backup before modification
	// 4. Perform the line operation based on write_mode
	// 5. Write the modified content atomically

	try {
		// Read existing file
		std::vector<std::string> file_lines;
		if (std::filesystem::exists(path)) {
			std::ifstream file(path);
			if (!file.is_open()) {
				AutoReflexiveMetrics metrics;
				return Db9Response{Db9Response::Error, "", "file_open_failed", "Could not open file for reading", metrics};
			}

			std::string line;
			while (std::getline(file, line)) {
				file_lines.push_back(line);
			}
			file.close();
		}

		// Calculate indices for the line range
		IndexRange indices = calculateIndices(range, file_lines.size());
		if (!indices.valid) {
			AutoReflexiveMetrics metrics;
			return Db9Response{Db9Response::Error, "", "invalid_range", "Invalid line range specification", metrics};
		}

		// Create backup before modification
		if (!file_lines.empty()) {
			std::string backup_path = createBackupPath(path);
			std::filesystem::copy_file(path, backup_path, std::filesystem::copy_options::overwrite_existing);
		}

		// Split content into lines for line operations
		std::vector<std::string> new_content_lines;
		
		// For single-line operations, don't split on newlines to prevent duplication
		if (range.type == LineRange::SingleLine) {
			// Single line operation - treat newlines as literal content
			new_content_lines.push_back(content);
		} else {
			// Multi-line operation - split content on newlines as intended
			std::istringstream content_stream(content);
			std::string content_line;
			while (std::getline(content_stream, content_line)) {
				new_content_lines.push_back(content_line);
			}
		}

		// Perform the line operation based on write_mode
		int lines_changed = 0;
		switch (write_mode) {
			case WriteMode::Replace:
				lines_changed = performLineReplacement(file_lines, new_content_lines, range);
				break;
			case WriteMode::Insert:
				lines_changed = performLineInsertion(file_lines, new_content_lines, range);
				break;
			case WriteMode::Append:
				lines_changed = performLineAppend(file_lines, new_content_lines, range);
				break;
			case WriteMode::Prepend:
				lines_changed = performLinePrepend(file_lines, new_content_lines);
				break;
			default:
				AutoReflexiveMetrics metrics;
				return Db9Response{Db9Response::Error, "", "unsupported_mode", "Unsupported write mode for line surgery", metrics};
		}

		// Write the modified content atomically
		std::ofstream output_file(path);
		if (!output_file.is_open()) {
			AutoReflexiveMetrics metrics;
			return Db9Response{Db9Response::Error, "", "file_write_failed", "Could not open file for writing", metrics};
		}

		for (size_t i = 0; i < file_lines.size(); ++i) {
			output_file << file_lines[i];
			if (i < file_lines.size() - 1) {
				output_file << "\n";
			}
		}
		output_file.close();

		// Generate success response
		auto end_time = std::chrono::steady_clock::now();
		auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
		size_t file_size = std::filesystem::file_size(path);

		std::ostringstream result;
		result << "{\"status\": \"line_surgery_complete\", \"path\": \"" << path << "\", \"lines_changed\": " << lines_changed << ", \"total_lines\": " << file_lines.size() << ", \"size_bytes\": " << file_size << "}";

		AutoReflexiveMetrics metrics;
		return Db9Response{Db9Response::Success, result.str(), "", "", metrics};

	} catch (const std::exception& e) {
		AutoReflexiveMetrics metrics;
		return Db9Response{Db9Response::Error, "", "line_surgery_failed", "Line surgery operation failed: " + std::string(e.what()), metrics};
	}
}

FioWriteVerb::IndexRange FioWriteVerb::calculateIndices(const LineRange& range, int file_line_count) {
	// Convert LineRange (1-based line numbers) to IndexRange (0-based array indices)

	switch (range.type) {
		case LineRange::Full:
			return IndexRange(0, file_line_count - 1, true);

		case LineRange::SingleLine: {
			// @3 -> line 3 (1-based) -> index 2 (0-based)
			int zero_based_line = range.start - 1;
			if (zero_based_line < 0 || zero_based_line >= file_line_count) {
				return IndexRange(0, 0, false); // Out of bounds
			}
			return IndexRange(zero_based_line, zero_based_line, true);
		}

		case LineRange::Range: {
			// @3:5 -> lines 3-5 (1-based) -> indices 2-4 (0-based)
			int start_zero_based = range.start - 1;
			int end_zero_based = start_zero_based + range.count - 1;

			if (start_zero_based < 0 || start_zero_based >= file_line_count) {
				return IndexRange(0, 0, false); // Start out of bounds
			}

			// Clamp end to file bounds
			if (end_zero_based >= file_line_count) {
				end_zero_based = file_line_count - 1;
			}

			return IndexRange(start_zero_based, end_zero_based, true);
		}

		case LineRange::FromEnd: {
			// @e:-3 -> last 3 lines
			if (range.count <= 0 || range.count > file_line_count) {
				return IndexRange(0, 0, false); // Invalid count
			}

			int start_zero_based = file_line_count - range.count;
			int end_zero_based = file_line_count - 1;
			return IndexRange(start_zero_based, end_zero_based, true);
		}

		case LineRange::FromStart: {
			// @0:5 -> first 5 lines
			if (range.count <= 0) {
				return IndexRange(0, 0, false); // Invalid count
			}

			int end_zero_based = std::min(range.count - 1, file_line_count - 1);
			return IndexRange(0, end_zero_based, true);
		}

		case LineRange::AppendAtEnd: {
			// @e:0 -> append at end (special case for append operations)
			// Return indices that represent "after the last line"
			return IndexRange(file_line_count, file_line_count, true);
		}

		default:
			return IndexRange(0, 0, false); // Unknown type
	}
}

int FioWriteVerb::performLineReplacement(std::vector<std::string>& lines, const std::vector<std::string>& new_content, const LineRange& range) {
	// Replace specified line range with new content

	// Calculate the indices for the range to replace
	IndexRange indices = calculateIndices(range, lines.size());
	if (!indices.valid) {
		return 0; // Invalid range, no changes
	}

	// Ensure indices are within bounds
	if (indices.start_idx < 0 || indices.start_idx >= static_cast<int>(lines.size())) {
		return 0; // Start index out of bounds
	}

	// Clamp end index to valid range
	int actual_end_idx = std::min(indices.end_idx, static_cast<int>(lines.size()) - 1);
	if (actual_end_idx < indices.start_idx) {
		return 0; // Invalid range
	}

	// Calculate how many lines we're replacing
	int lines_to_replace = actual_end_idx - indices.start_idx + 1;

	// Remove the old lines from the range
	auto start_iter = lines.begin() + indices.start_idx;
	auto end_iter = lines.begin() + actual_end_idx + 1;
	lines.erase(start_iter, end_iter);

	// Insert the new content at the same position
	if (!new_content.empty()) {
		lines.insert(lines.begin() + indices.start_idx, new_content.begin(), new_content.end());
	}

	// Calculate net change in line count
	int new_lines_added = static_cast<int>(new_content.size());
	int net_change = new_lines_added - lines_to_replace;

	return new_lines_added; // Return number of new lines added (consistent with other methods)
}

int FioWriteVerb::performLineInsertion(std::vector<std::string>& lines, const std::vector<std::string>& new_content, const LineRange& range) {
	// Insert new content at the specified position, shifting existing lines down

	if (new_content.empty()) {
		return 0; // Nothing to insert
	}

	// Calculate insertion position based on range type
	size_t insert_pos = 0;

	switch (range.type) {
		case LineRange::SingleLine: {
			// @2 - insert AT line 2 (1-based), shifting line 2+ down
			int zero_based_line = range.start - 1;
			if (zero_based_line < 0) {
				insert_pos = 0; // Insert at beginning if invalid
			} else if (zero_based_line > static_cast<int>(lines.size())) {
				insert_pos = lines.size(); // Insert at end if beyond file
			} else {
				insert_pos = zero_based_line; // Insert AT the specified line
			}
			break;
		}

		case LineRange::Range: {
			// @3:5 - insert AT line 3 (start of range), shifting line 3+ down
			int zero_based_start = range.start - 1;
			if (zero_based_start < 0) {
				insert_pos = 0;
			} else if (zero_based_start > static_cast<int>(lines.size())) {
				insert_pos = lines.size();
			} else {
				insert_pos = zero_based_start; // Insert AT the range start
			}
			break;
		}

		case LineRange::FromEnd: {
			// @e:-3 - insert at position (file_size - 3)
			if (range.count <= 0 || range.count > static_cast<int>(lines.size())) {
				insert_pos = lines.size(); // Insert at end if invalid
			} else {
				insert_pos = lines.size() - range.count;
			}
			break;
		}

		case LineRange::FromStart: {
			// @0:5 - insert at position 5 (after first 5 lines)
			if (range.count <= 0) {
				insert_pos = 0;
			} else {
				insert_pos = std::min(range.count, static_cast<int>(lines.size()));
			}
			break;
		}

		default:
			// For other types (Full, AppendAtEnd), insert at end as safe fallback
			insert_pos = lines.size();
			break;
	}

	// Insert the new content at the calculated position
	// This automatically shifts all existing lines at and after insert_pos down
	lines.insert(lines.begin() + insert_pos, new_content.begin(), new_content.end());

	return static_cast<int>(new_content.size());
}

int FioWriteVerb::performLineAppend(std::vector<std::string>& lines, const std::vector<std::string>& new_content, const LineRange& range) {
	// Append new content after the specified line/range

	if (new_content.empty()) {
		return 0; // Nothing to append
	}

	// Calculate insertion point based on range type
	size_t insert_pos = 0;

	switch (range.type) {
		case LineRange::AppendAtEnd:
			// @e:0 - append at end of file
			insert_pos = lines.size();
			break;

		case LineRange::SingleLine: {
			// @5 - append after line 5 (1-based)
			int zero_based_line = range.start - 1;
			if (zero_based_line < 0) {
				zero_based_line = 0; // Clamp to beginning
			}
			if (zero_based_line >= static_cast<int>(lines.size())) {
				insert_pos = lines.size(); // Append at end if beyond file
			} else {
				insert_pos = zero_based_line + 1; // After the specified line
			}
			break;
		}

		case LineRange::Range: {
			// @3:5 - append after the range (after line 5)
			int end_zero_based = range.start + range.count - 2; // Convert to 0-based end
			if (end_zero_based < 0) {
				insert_pos = 0;
			} else if (end_zero_based >= static_cast<int>(lines.size())) {
				insert_pos = lines.size();
			} else {
				insert_pos = end_zero_based + 1; // After the range
			}
			break;
		}

		case LineRange::FromEnd: {
			// @e:-1 - insert before the last 1 line
			// @e:-3 - insert before the last 3 lines
			int from_end_pos = static_cast<int>(lines.size()) - range.count;
			insert_pos = std::max(0, from_end_pos);
			break;
		}

		case LineRange::FromStart: {
			// @0:5 - append after first 5 lines
			int end_pos = std::min(range.count, static_cast<int>(lines.size()));
			insert_pos = end_pos;
			break;
		}

		default:
			// For other types, append at end as safe fallback
			insert_pos = lines.size();
			break;
	}

	// Insert the new content at the calculated position
	lines.insert(lines.begin() + insert_pos, new_content.begin(), new_content.end());

	return static_cast<int>(new_content.size());
}

int FioWriteVerb::performLinePrepend(std::vector<std::string>& lines, const std::vector<std::string>& new_content) {
	// Prepend new content at the beginning of the file, shifting all existing lines down

	if (new_content.empty()) {
		return 0; // Nothing to prepend
	}

	// Insert at position 0 (beginning of file)
	// This automatically shifts all existing lines down
	lines.insert(lines.begin(), new_content.begin(), new_content.end());

	return static_cast<int>(new_content.size());
}

std::string FioWriteVerb::createBackupPath(const std::string& original_path) {
	return original_path + ".bak";
}

bool FioWriteVerb::validatePath(const std::string& path) {
	return !path.empty();
}

std::string FioWriteVerb::generateTimestamp() {
	auto now = std::chrono::system_clock::now();
	auto time_t = std::chrono::system_clock::to_time_t(now);
	std::stringstream ss;
	ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
	return ss.str();
}

FioWriteVerb::WriteParameters FioWriteVerb::extractParameters(const lab::Text::Sexpr& sexpr) {
	WriteParameters params;
	params.path = extractStringParam(sexpr, "path");
	params.content = extractStringParam(sexpr, "content");
	return params;
}

Db9Response FioWriteVerb::performWrite(const WriteParameters& params) {
	auto start_time = std::chrono::steady_clock::now();
	return performFullFileWrite(params.path, params.content, start_time);
}

//-----------------------------------------------------------------------------
// Preview and Confirmation System
//-----------------------------------------------------------------------------

// Static cache initialization
std::unordered_map<std::string, FioWriteVerb::PreviewContext> FioWriteVerb::preview_cache;
std::mutex FioWriteVerb::cache_mutex;

bool FioWriteVerb::shouldPreview(const std::string& path, const LineRange& range, WriteMode::Type write_mode,
	const std::string& content, std::string& result) {

    // Phase C1: Intelligent Preview Decision Making
    bool should_preview = false;  // Default to direct execution for simple operations
    std::string preview_reason = "";

    // 1. Unicode escaping detection (existing logic)
    std::string preview_escape = LabDb::TextEscaping::unescapeDb9String(content);
    if (preview_escape != content) {
        should_preview = true;
        if (!preview_reason.empty()) preview_reason += ",";
        preview_reason += "unicode_substitution";
    }
    
    // 2. Complex line surgery operations should always preview
    if (range.type != LineRange::Full) {
        should_preview = true;
        if (!preview_reason.empty()) preview_reason += ",";
        preview_reason += "complex_line_surgery";
    }
    
    // 3. Multi-line content operations (NEW: Phase C1)
    size_t newline_count = std::count(content.begin(), content.end(), '\n');
    if (newline_count > 10) {
        should_preview = true;
        if (!preview_reason.empty()) preview_reason += ",";
        preview_reason += "multi_line_content";
    }

    // 4. Destructive operations (NEW: Phase C1)
    if (write_mode == WriteMode::Replace && range.type != LineRange::SingleLine) {
        should_preview = true;
        if (!preview_reason.empty()) preview_reason += ",";
        preview_reason += "destructive_operation";
    }
    
    // 5. Full file operations should ALWAYS preview for AI safety
    // Prevents LLM confusion: "I'll append this method" -> accidentally overwrites entire file
    if (range.type == LineRange::Full) {
        should_preview = true;
        if (!preview_reason.empty()) preview_reason += ",";
        if (std::filesystem::exists(path)) {
            preview_reason += "full_file_overwrite";
        } else {
            preview_reason += "creating_new_file";
        }
    }

    // Only truly simple operations execute directly:
    // - Line-specific operations without special characters
    // - Operations that can't accidentally destroy file content
    if (!should_preview) {
        return false;  // Execute directly
    }

    // Generate preview context and token
    std::string token = generatePreviewToken();
    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();

    // Load current file content for preview generation
    std::vector<std::string> current_lines;
    if (std::filesystem::exists(path)) {
        std::ifstream file(path);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                current_lines.push_back(line);
            }
            file.close();
        }
    }

    PreviewContext context{token, path, content, range, write_mode, start_time, current_lines, preview_reason};
    {
        std::lock_guard<std::mutex> lock(cache_mutex);
        preview_cache[token] = context;
    }

    // Phase C1: Consciousness-First Preview Priority System
    // 🧠 LINE SURGERY AWARENESS takes precedence over Unicode escaping concerns

    // 1. PRIORITY: Line Surgery Preview when surgical operations detected
    if (preview_reason.find("complex_line_surgery") != std::string::npos ||
        preview_reason.find("destructive_operation") != std::string::npos ||
        preview_reason.find("large_file") != std::string::npos ||
        preview_reason.find("multi_line_content") != std::string::npos) {
        // Generate line surgery awareness preview (Phase C1)
        std::string response = generateLineSurgeryPreview(context, preview_reason);
        result = response;
        return true;
    }

    // 2. FALLBACK: Unicode escaping preview only when no surgical awareness needed
    if (preview_reason.find("unicode_escaping") != std::string::npos) {
        // Handle Unicode escaping previews (existing logic)
        if (preview_escape != content) {
            // Check for specific Unicode characters and generate appropriate messages
            size_t pos = content.find("※");
            if (pos != std::string::npos) {
                size_t start = (pos > 20) ? pos - 20 : 0;
                size_t end = (pos + 20 < content.size()) ? pos + 20 : content.size();
                std::string surrounding = content.substr(start, end - start);
                std::string response = generatePreviewResponse(context);
                result = "At least one ※ has been escaped to \\\\ in the content: '" + surrounding + "' " + response;
                return true;
            }

            pos = content.find("↵");
            if (pos != std::string::npos) {
                size_t start = (pos > 20) ? pos - 20 : 0;
                size_t end = (pos + 20 < content.size()) ? pos + 20 : content.size();
                std::string surrounding = content.substr(start, end - start);
                std::string response = generatePreviewResponse(context);
                result = "At least one ↵ has been escaped to \\\\n in the content: '" + surrounding + "' " + response;
                return true;
            }

            pos = content.find("→");
            if (pos != std::string::npos) {
                size_t start = (pos > 20) ? pos - 20 : 0;
                size_t end = (pos + 20 < content.size()) ? pos + 20 : content.size();
                std::string surrounding = content.substr(start, end - start);
                std::string response = generatePreviewResponse(context);
                result = "At least one → has been escaped to \\\\t in the content: '" + surrounding + "' " + response;
                return true;
            }
        }
    }

    // 3. DEFAULT: Generate line surgery awareness preview for any other case
    std::string response = generateLineSurgeryPreview(context, preview_reason);
    result = response;
    return true;
  }

std::string FioWriteVerb::generatePreviewToken() {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_int_distribution<> dis(0, 15);
	std::string token = "confirm-";
	for (int i = 0; i < 8; ++i) {
		token += "0123456789abcdef"[dis(gen)];
	}
	return token;
}

//-----------------------------------------------------------------------------
// Phase C1: Line Surgery Preview and Awareness System
//-----------------------------------------------------------------------------

std::string FioWriteVerb::generateLineSurgeryPreview(const PreviewContext& context, const std::string& reason) {
	std::ostringstream result;

	// Generate intelligent preview based on operation complexity
	result << "🧠 LINE SURGERY AWARENESS PREVIEW\n\n";
	result << "Target: " << context.path << "\n";
	result << "Operation: " << WriteMode::toString(context.write_mode_type) << "\n";
	result << "Range: ";

	// Describe the range in human terms
	switch (context.range.type) {
		case LineRange::SingleLine:
			result << "Line " << context.range.start << " (single line)";
			break;
		case LineRange::Range:
			result << "Lines " << context.range.start << "-" << (context.range.start + context.range.count - 1) << " (" << context.range.count << " lines)";
			break;
		case LineRange::FromEnd:
			result << "Last " << context.range.count << " lines";
			break;
		case LineRange::FromStart:
			result << "First " << context.range.count << " lines";
			break;
		case LineRange::AppendAtEnd:
			result << "End of file (append)";
			break;
		case LineRange::Full:
			result << "Entire file";
			break;
	}
	result << "\n\n";

	// Show impact analysis
	result << "📊 IMPACT ANALYSIS:\n";

	// File size analysis
	if (std::filesystem::exists(context.path)) {
		auto file_size = std::filesystem::file_size(context.path);
		result << "• File size: " << file_size << " bytes";
		if (file_size > 50000) {
			result << " ⚠️ LARGE FILE";
		}
		result << "\n";
	}

	// Current file line count
	result << "• Current lines: " << context.current_lines.size() << "\n";

	// Content analysis
	size_t new_line_count = std::count(context.content.begin(), context.content.end(), '\n') + 1;
	result << "• New content lines: " << new_line_count << "\n";

	// Predict impact
	int predicted_final_lines = context.current_lines.size();
	switch (context.write_mode_type) {
		case WriteMode::Append:
		case WriteMode::Prepend:
		case WriteMode::Insert:
			predicted_final_lines += new_line_count;
			result << "• Predicted final lines: " << predicted_final_lines << " (+" << new_line_count << ")\n";
			break;
		case WriteMode::Replace: {
			if (context.range.type == LineRange::SingleLine) {
				predicted_final_lines = predicted_final_lines - 1 + new_line_count;
			} else if (context.range.type == LineRange::Range) {
				predicted_final_lines = predicted_final_lines - context.range.count + new_line_count;
			}
			result << "• Predicted final lines: " << predicted_final_lines;
			int net_change = predicted_final_lines - static_cast<int>(context.current_lines.size());
			if (net_change > 0) result << " (+" << net_change << ")";
			else if (net_change < 0) result << " (" << net_change << ")";
			result << "\n";
			break;
		}
		case WriteMode::Unspecified:
		case WriteMode::Unrecognized:
			result << "• Predicted final lines: Unknown (unspecified operation)\n";
			break;
	}

	// Show preview reasons
	result << "\n🔍 PREVIEW TRIGGERED BY:\n";
	if (reason.find("complex_line_surgery") != std::string::npos) {
		result << "• Complex line surgery operation detected\n";
	}
	if (reason.find("large_file") != std::string::npos) {
		result << "• Large file operation (>50KB)\n";
	}
	if (reason.find("multi_line_content") != std::string::npos) {
		result << "• Multi-line content insertion (>10 lines)\n";
	}
	if (reason.find("destructive_operation") != std::string::npos) {
		result << "• Destructive operation (multi-line replace)\n";
	}
	if (reason.find("unicode_escaping") != std::string::npos) {
		result << "• Unicode character escaping detected\n";
	}

	// Show content preview with intelligent truncation
	result << "\n📝 CONTENT PREVIEW:\n";
	result << "┌─ New content to " << WriteMode::toString(context.write_mode_type) << " ─┐\n";

	std::string preview_content = context.content;
	if (preview_content.length() > 200) {
		preview_content = preview_content.substr(0, 200) + "... [+" + std::to_string(context.content.length() - 200) + " more chars]";
	}

	// Show content with line indicators
	std::istringstream content_stream(preview_content);
	std::string line;
	int line_num = 1;
	while (std::getline(content_stream, line) && line_num <= 5) {
		result << "│ " << std::setw(2) << line_num << ": " << line << "\n";
		line_num++;
	}
	if (new_line_count > 5) {
		result << "│ ... [+" << (new_line_count - 5) << " more lines]\n";
	}
	result << "└─────────────────────────────────┘\n\n";

	// Show affected area of current file
	if (!context.current_lines.empty() && context.range.type != LineRange::Full) {
		result << "📄 AFFECTED AREA (current file):\n";
		result << "┌─ Current content in target range ─┐\n";

		IndexRange indices = calculateIndices(context.range, context.current_lines.size());
		if (indices.valid) {
			int start_line = std::max(0, indices.start_idx - 1);
			int end_line = std::min(static_cast<int>(context.current_lines.size()) - 1, indices.end_idx + 1);

			for (int i = start_line; i <= end_line && i < static_cast<int>(context.current_lines.size()); i++) {
				bool in_range = (i >= indices.start_idx && i <= indices.end_idx);
				std::string marker = in_range ? "→" : " ";
				result << "│ " << marker << std::setw(2) << (i + 1) << ": " << context.current_lines[i] << "\n";
			}
		}
		result << "└─────────────────────────────────┘\n\n";
	}

	// Confirmation and rollback information
	result << "⚡ SAFETY FEATURES:\n";
	result << "• Automatic backup created before modification\n";
	result << "• Atomic operation (all-or-nothing)\n";
	result << "• Rollback available via backup restoration\n\n";

	result << "🎯 CONFIRMATION:\n";
	result << "Token: " << context.token << "\n";
	result << "Use (fio-confirm :token §" << context.token << "§) to proceed\n";
	result << "Use (fio-cancel :token §" << context.token << "§) to cancel\n\n";

	return result.str();
}

// Phase C1: Generate preview response from existing preview content
Db9Response FioWriteVerb::generatePreviewResponseFromExisting(const std::string& preview_content,
		const std::string& token, std::chrono::steady_clock::time_point start_time) {

	// Calculate metrics
	auto end_time = std::chrono::steady_clock::now();
	auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

	// Create JSON response in the expected format
	std::ostringstream json_result;
	json_result << "{"
				<< "\"status\": \"preview_generated\", "
				<< "\"token\": \"" << token << "\", "
				<< "\"preview\": \"" << preview_content << "\""
				<< "}";

	AutoReflexiveMetrics metrics;
	return Db9Response{Db9Response::Success, json_result.str(), "", "", metrics};
}

Db9Response FioWriteVerb::generatePreviewResponse(const std::string& path,
		const std::string& token, const std::string& content,
		const LineRange& range, WriteMode::Type write_mode,
		std::chrono::steady_clock::time_point start_time) {
	std::ostringstream result;

	// Generate rich preview response based on fio-preview-spec.md
	result << "🔍 UNICODE ESCAPING PREVIEW\n\n";
	result << "Target: " << path << "\n\n";

	// Analyze Unicode transformations in content
	bool has_backslash = content.find("※") != std::string::npos;
	bool has_quotes = content.find("″") != std::string::npos;
	bool has_newlines = content.find("↵") != std::string::npos;
	bool has_tabs = content.find("⇥") != std::string::npos;

	if (has_backslash || has_quotes || has_newlines || has_tabs) {
		result << "Unicode transformations detected:\n";
		if (has_quotes) result << "• ″ → \" (quotation marks)\n";
		if (has_backslash) result << "• ※ → \\\\ (backslash escapes)\n";
		if (has_tabs) result << "• ⇥ → [TAB] (indentation)\n";
		if (has_newlines) result << "• ↵ → [NEWLINE] (line break)\n";
		result << "\n";
	}

	// Show content preview with box formatting
	result << "Content preview:\n";
	result << "┌─ Transformed output ─┐\n";

	// Show preview of content (truncated if too long)
	std::string preview_content = content;
	if (preview_content.length() > 100) {
		preview_content = preview_content.substr(0, 100) + "...";
	}
	result << "│ " << preview_content << " │\n";
	result << "└──────────────────────┘\n\n";

	result << "Raw escapes preserved:\n";
	result << "• Emoji characters: 🧚 🐚 🌊 (remain as-is)\n";
	result << "• Special symbols: Keep visual representation\n\n";

	result << "Confirmation token: " << token << "\n";
	result << "Use (fio-confirm :token §" << token << "§) to apply\n";

	// Generate structured JSON response
	std::ostringstream json_result;
	json_result << "{\"status\": \"preview_generated\", \"token\": \"" << token << "\", ";
	json_result << "\"preview\": \"" << result.str() << "\"}";

	AutoReflexiveMetrics metrics;
	return Db9Response{Db9Response::Success, json_result.str(), "", "", metrics};
}

// Single-parameter overload for PreviewContext
std::string FioWriteVerb::generatePreviewResponse(const PreviewContext& context) {
	std::ostringstream result;

	// Generate rich preview response for context
	result << "🔍 UNICODE ESCAPING PREVIEW\n\n";
	result << "Target: " << context.path << "\n\n";

	// Analyze Unicode transformations in content
	bool has_backslash = context.content.find("※") != std::string::npos;
	bool has_quotes = context.content.find("″") != std::string::npos;
	bool has_newlines = context.content.find("↵") != std::string::npos;
	bool has_tabs = context.content.find("⇥") != std::string::npos;

	if (has_backslash || has_quotes || has_newlines || has_tabs) {
		result << "Unicode transformations detected:\n";
		if (has_quotes) result << "• ″ → \" (quotation marks)\n";
		if (has_backslash) result << "• ※ → \\\\ (backslash escapes)\n";
		if (has_tabs) result << "• ⇥ → [TAB] (indentation)\n";
		if (has_newlines) result << "• ↵ → [NEWLINE] (line break)\n";
		result << "\n";
	}

	// Show content preview with box formatting
	result << "Content preview:\n";
	result << "┌─ Transformed output ─┐\n";

	// Show preview of content (truncated if too long)
	std::string preview_content = context.content;
	if (preview_content.length() > 100) {
		preview_content = preview_content.substr(0, 100) + "...";
	}
	result << "│ " << preview_content << " │\n";
	result << "└──────────────────────┘\n\n";

	result << "Confirmation token: " << context.token << "\n";
	result << "Use (fio-confirm :token §" << context.token << "§) to apply\n";

	return result.str();
}

//-----------------------------------------------------------------------------
// Public Accessors for Confirmation System
//-----------------------------------------------------------------------------

bool FioWriteVerb::hasPreviewToken(const std::string& token) {
	std::lock_guard<std::mutex> lock(cache_mutex);
	return preview_cache.find(token) != preview_cache.end();
}

FioWriteVerb::PreviewContext FioWriteVerb::retrievePreviewContext(const std::string& token) {
	std::lock_guard<std::mutex> lock(cache_mutex);
	auto it = preview_cache.find(token);
	if (it == preview_cache.end()) {
		throw std::runtime_error("Preview token not found: " + token);
	}
	return it->second;
}

void FioWriteVerb::removePreviewToken(const std::string& token) {
	std::lock_guard<std::mutex> lock(cache_mutex);
	preview_cache.erase(token);
}

Db9Response FioWriteVerb::executeConfirmedOperation(const std::string& token) {
	PreviewContext context = retrievePreviewContext(token);

	// Execute the line surgery with proper start time
	auto execution_start = std::chrono::steady_clock::now();
	Db9Response result = executeLineSurgery(
		context.path,
		context.content,
		context.range,
		context.write_mode_type,
		execution_start
	);

	// Clean up token on successful execution
	if (result.status == Db9Response::Success) {
		removePreviewToken(token);
	}

	return result;
}

} // namespace LabDb
