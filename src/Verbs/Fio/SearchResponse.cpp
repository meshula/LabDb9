#include "SearchResponse.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <iostream>

namespace LabDb {
namespace SearchEngine {

std::string getCurrentTimestamp() {
	auto now = std::chrono::system_clock::now();
	auto time_t = std::chrono::system_clock::to_time_t(now);
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		now.time_since_epoch()) % 1000;
	std::stringstream ss;
	ss << std::put_time(std::localtime(&time_t), "%Y-%m-%dT%H:%M:%S");
	ss << "." << std::setfill('0') << std::setw(3) << ms.count() << "Z";
	return ss.str();
}

void SearchResponse::addError(const std::string& message, 
																										const std::optional<std::string>& file,
																										const std::optional<int>& line) {
	SearchIssue issue;
	issue.type = SearchIssue::Type::ERROR;
	issue.message = message;
	issue.file_path = file;
	issue.line_number = line;
	issue.timestamp = getCurrentTimestamp();
	errors.push_back(issue);
}

void SearchResponse::addWarning(const std::string& message,
																											const std::optional<std::string>& file,
																											const std::optional<int>& line) {
	SearchIssue issue;
	issue.type = SearchIssue::Type::WARNING;
	issue.message = message;
	issue.file_path = file;
	issue.line_number = line;
	issue.timestamp = getCurrentTimestamp();
	warnings.push_back(issue);
}

void SearchResponse::addInfo(const std::string& message) {
	SearchIssue issue;
	issue.type = SearchIssue::Type::INFO;
	issue.message = message;
	issue.timestamp = getCurrentTimestamp();
	info_messages.push_back(issue);
}


std::string escapeJsonString(const std::string& input) {
	std::string escaped;
	escaped.reserve(input.length() + 10);
	
	for (char c : input) {
		switch (c) {
			case '"': escaped += "\\\""; break;
			case '\\': escaped += "\\\\"; break;
			case '\b': escaped += "\\b"; break;
			case '\f': escaped += "\\f"; break;
			case '\n': escaped += "\\n"; break;
			case '\r': escaped += "\\r"; break;
			case '\t': escaped += "\\t"; break;
			default: escaped += c; break;
		}
	}
	return escaped;
}

std::string formatJsonArray(const std::vector<std::string>& items) {
	std::string result = "[";
	for (size_t i = 0; i < items.size(); ++i) {
		if (i > 0) result += ", ";
		result += "\"" + escapeJsonString(items[i]) + "\"";
	}
	result += "]";
	return result;
}

std::string formatMatchResultJson(const EnhancedMatchResult& match) {
	std::stringstream ss;
	ss << "{";
	ss << "\"file_path\": \"" << escapeJsonString(match.file_path) << "\",";
	ss << "\"line_number\": " << match.line_number << ",";
	ss << "\"column_start\": " << match.column_start << ",";
	ss << "\"pattern_used\": \"" << escapeJsonString(match.pattern_used) << "\",";
	ss << "\"matched_text\": \"" << escapeJsonString(match.matched_text) << "\",";
	ss << "\"normalization_type\": \"" << escapeJsonString(match.normalization_type) << "\",";
	ss << "\"confidence_score\": " << match.confidence_score;
	ss << "}";
	return ss.str();
}

std::string formatSearchResponseJson(const SearchResponse& response) {
	std::stringstream ss;
	ss << "{";
	ss << "\"status\": \"" << escapeJsonString(response.status) << "\",";
	ss << "\"total_matches\": " << response.matches.size() << ",";
	ss << "\"files_processed\": " << response.stats.files_processed << ",";
	ss << "\"processing_time_ms\": " << response.stats.total_duration.count() << ",";
	ss << "\"results_truncated\": " << (response.results_truncated ? "true" : "false") << ",";
	
	// Format matches array
	ss << "\"matches\": [";
	for (size_t i = 0; i < response.matches.size(); ++i) {
		if (i > 0) ss << ", ";
		ss << formatMatchResultJson(response.matches[i]);
	}
	ss << "]";
	
	// Add any error messages
	if (!response.errors.empty()) {
		ss << ",\"errors\": [";
		for (size_t i = 0; i < response.errors.size(); ++i) {
			if (i > 0) ss << ", ";
			ss << "\"" << escapeJsonString(response.errors[i].message) << "\"";
		}
		ss << "]";
	}
	
	// Add any warnings
	if (!response.warnings.empty()) {
		ss << ",\"warnings\": [";
		for (size_t i = 0; i < response.warnings.size(); ++i) {
			if (i > 0) ss << ", ";
			ss << "\"" << escapeJsonString(response.warnings[i].message) << "\"";
		}
		ss << "]";
	}
	
	ss << "}";
	return ss.str();
}


} // namespace SearchEngine
} // namespace LabDb