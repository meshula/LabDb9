#include "PatternMatcher.h"
#include "LabDb/NormalizeText.h"
#include "LabDb/DirectoryTraversal.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace LabDb {

PatternMatcher::PatternMatcher(const SearchConfig& config) : config_(config) {
	if (config_.patterns.empty()) {
		throw std::invalid_argument("At least one search pattern required");
	}
}

std::vector<MatchResult> PatternMatcher::searchFile(const std::string& file_path) {
	std::vector<MatchResult> results;
	std::ifstream file(file_path);
	if (!file.is_open()) {
		std::cerr << "Warning: Could not open file: " << file_path << std::endl;
		return results;
	}
	std::vector<std::string> lines;
	std::string line;
	while (std::getline(file, line)) {
		lines.push_back(line);
	}
	file.close();
	files_processed_++;
	for (size_t line_idx = 0; line_idx < lines.size(); ++line_idx) {
		const std::string& current_line = lines[line_idx];
		for (const auto& pattern : config_.patterns) {
			std::string match_type;
			if (matchesPattern(current_line, pattern, match_type)) {
				MatchResult result;
				result.file_path = file_path;
				result.line_number = static_cast<int>(line_idx + 1);
				result.column_position = 0;
				result.matched_text = current_line;
				result.pattern_used = pattern;
				result.normalization_type = match_type;
				result.context_lines = extractContext(lines, static_cast<int>(line_idx));
				results.push_back(result);
				matches_found_++;
				if (results.size() >= config_.max_results) {
					return results;
				}
			}
		}
	}
	return results;
}

bool PatternMatcher::matchesPattern(const std::string& line, const std::string& pattern, std::string& match_type) {
	if (line.find(pattern) != std::string::npos) {
		match_type = "original";
		return true;
	}
	if (config_.case_fold) {
		auto normalized_line = normalize_text(line, NormalizeMode::CaseFoldOnly);
		auto normalized_pattern = normalize_text(pattern, NormalizeMode::CaseFoldOnly);
		if (normalized_line.case_folded.find(normalized_pattern.case_folded) != std::string::npos) {
			match_type = "case_folded";
			return true;
		}
	}
	if (config_.ascii_fold) {
		auto normalized_line = normalize_text(line, NormalizeMode::AsciiFold);
		auto normalized_pattern = normalize_text(pattern, NormalizeMode::AsciiFold);
		if (normalized_line.ascii_folded.find(normalized_pattern.ascii_folded) != std::string::npos) {
			match_type = "ascii_folded";
			return true;
		}
	}
	match_type = "none";
	return false;
}

std::vector<std::string> PatternMatcher::extractContext(const std::vector<std::string>& lines, int line_index) {
	std::vector<std::string> context;
	int start = std::max(0, line_index - config_.context_lines);
	int end = std::min(static_cast<int>(lines.size()), line_index + config_.context_lines + 1);
	for (int i = start; i < end; ++i) {
		if (i == line_index) {
			context.push_back(">>> " + lines[i]);
		} else {
			context.push_back("    " + lines[i]);
		}
	}
	return context;
}

std::vector<MatchResult> PatternMatcher::searchDirectory(const std::string& directory_path,
														int max_depth,
														const std::vector<std::string>& extensions) {
	std::vector<MatchResult> all_results;
	FileSearch::TraversalConfig traversal_config;
	traversal_config.max_depth = max_depth;
	FileSearch::DirectoryTraversal traversal(traversal_config);
	auto files = traversal.traverse(directory_path);
	//std::cout << "Searching " << files.size() << " files in directory tree..." << std::endl;
	for (const auto& file : files) {
		if (!extensions.empty()) {
			bool extension_matches = false;
			for (const auto& ext : extensions) {
				if (file.extension == ext) {
					extension_matches = true;
					break;
				}
			}
			if (!extension_matches) {
				continue;
			}
		}
		auto file_results = searchFile(file.path);
		all_results.insert(all_results.end(), file_results.begin(), file_results.end());
		if (all_results.size() >= config_.max_results) {
			//std::cout << "Reached max results limit (" << config_.max_results << ")" << std::endl;
			break;
		}
	}
	//std::cout << "Traversal completed: " << traversal.getFilesFound() << " files found, " 
	//							 << traversal.getFilesSkipped() << " skipped, " 
	//							 << traversal.getTraversalTimeMs() << "ms" << std::endl;
	return all_results;
}

} // namespace LabDb