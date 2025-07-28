#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <optional>

namespace LabDb {
namespace SearchEngine {

//-----------------------------------------------------------------------------
// Comprehensive Search Response Architecture
//-----------------------------------------------------------------------------

struct EnhancedMatchResult {
	std::string file_path;
	std::string relative_path;
	int line_number;
	int column_start;
	int column_end;
	std::string pattern_used;
	std::string matched_text;
	std::string full_line;
	std::string normalization_type;
	std::vector<std::string> context_before;
	std::vector<std::string> context_after;
	int context_line_start;
	double confidence_score = 1.0;
	int pattern_priority = 0;
	size_t file_size_bytes = 0;
	std::string file_extension;
	std::chrono::system_clock::time_point file_modified_time;
};

struct SearchStatistics {
	std::chrono::milliseconds total_duration{0};
	std::chrono::milliseconds traversal_duration{0};
	std::chrono::milliseconds search_duration{0};
	std::chrono::milliseconds normalization_duration{0};
	size_t files_found = 0;
	size_t files_processed = 0;
	size_t files_skipped = 0;
	size_t directories_scanned = 0;
	size_t symlinks_encountered = 0;
	size_t total_matches = 0;
	size_t unique_files_with_matches = 0;
	size_t patterns_matched = 0;
	size_t lines_searched = 0;
	size_t bytes_processed = 0;
	size_t unicode_normalizations = 0;
	double files_per_second = 0.0;
	double bytes_per_second = 0.0;
	double matches_per_file = 0.0;
};

struct SearchConfigEcho {
	std::string root_path;
	std::vector<std::string> patterns;
	std::vector<std::string> extensions;
	std::vector<std::string> context_terms;
	int max_depth = 0;
	int context_lines = 2;
	int context_distance = 10;
	size_t max_results = 50;
	size_t max_file_size = 10 * 1024 * 1024;
	bool case_fold = false;
	bool ascii_fold = false;
	bool recursive = false;
	std::string context_logic = "any";
	std::string output_format = "json";
	std::optional<std::string> output_file;
};

struct SearchIssue {
	enum class Type { ERROR, WARNING, INFO };
	Type type;
	std::string message;
	std::optional<std::string> file_path;
	std::optional<int> line_number;
	std::string timestamp;
};

struct SearchResponse {
	std::string status;
	std::string search_id;
	std::string timestamp_start;
	std::string timestamp_end;
	SearchConfigEcho config;
	std::vector<EnhancedMatchResult> matches;
	bool results_truncated = false;
	std::string truncation_reason;
	SearchStatistics stats;
	std::vector<SearchIssue> errors;
	std::vector<SearchIssue> warnings;
	std::vector<SearchIssue> info_messages;
	std::vector<std::string> patterns_not_found;
	std::vector<std::string> files_with_errors;
	std::optional<std::string> context_validation_summary;
	std::string format_version = "1.0";
	std::optional<std::string> output_file_written;
	
	bool hasErrors() const { return !errors.empty(); }
	bool hasWarnings() const { return !warnings.empty(); }
	bool wasSuccessful() const { return status == "success" && !hasErrors(); }
	size_t getTotalIssues() const { return errors.size() + warnings.size(); }
	
	void addError(const std::string& message, 
												const std::optional<std::string>& file = {},
												const std::optional<int>& line = {});
	void addWarning(const std::string& message,
													const std::optional<std::string>& file = {},
													const std::optional<int>& line = {});
	void addInfo(const std::string& message);
};

//-----------------------------------------------------------------------------
// JSON Formatting Functions
//-----------------------------------------------------------------------------

std::string getCurrentTimestamp();
std::string escapeJsonString(const std::string& input);
std::string formatJsonArray(const std::vector<std::string>& items);
std::string formatMatchResultJson(const EnhancedMatchResult& match);
std::string formatSearchResponseJson(const SearchResponse& response);

} // namespace SearchEngine
} // namespace LabDb