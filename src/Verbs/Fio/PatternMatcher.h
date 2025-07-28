#pragma once

#include <string>
#include <vector>
#include <memory>

namespace LabDb {

struct MatchResult {
	std::string file_path;
	int line_number;
	int column_position;
	std::string matched_text;
	std::string pattern_used;
	std::string normalization_type; // "original", "case_folded", "ascii_folded"
	std::vector<std::string> context_lines;
};

struct SearchConfig {
	std::vector<std::string> patterns;
	bool case_fold = false;
	bool ascii_fold = false;
	int context_lines = 2;
	size_t max_results = 50;
};

class PatternMatcher {
public:
	PatternMatcher(const SearchConfig& config);
	~PatternMatcher() = default;


	// Directory search with recursive traversal
	std::vector<MatchResult> searchDirectory(const std::string& directory_path, 
																																													int max_depth = 0,
																																													const std::vector<std::string>& extensions = {});

	// Directory search with traversal
	// Core search function
	std::vector<MatchResult> searchFile(const std::string& file_path);

	// Statistics
	size_t getFilesProcessed() const { return files_processed_; }
	size_t getMatchesFound() const { return matches_found_; }

private:
	SearchConfig config_;
	size_t files_processed_ = 0;
	size_t matches_found_ = 0;

	// Core matching logic
	bool matchesPattern(const std::string& line, const std::string& pattern,
												std::string& match_type);
	std::vector<std::string> extractContext(const std::vector<std::string>& lines,
																																			int line_index);
};

} // namespace LabDb