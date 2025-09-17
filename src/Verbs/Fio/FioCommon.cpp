#include "FioCommon.h"
#include <sstream>
#include <cmath>
#include <cctype>

namespace LabDb {

// Parameter extraction utility
std::string extractStringParam(const lab::Text::Sexpr& sexpr, const std::string& param_name) {
	for (size_t i = 0; i < sexpr.expr.size(); ++i) {
		const auto& elem = sexpr.expr[i];
		if (elem.token == tsSexprAtom) {
			int stringIndex = elem.ref;
			if (stringIndex >= 0 && stringIndex < static_cast<int>(sexpr.strings.size())) {
				const std::string& atomValue = sexpr.strings[stringIndex];
				if (atomValue == ":" + param_name || atomValue == param_name) {
					if (i + 1 < sexpr.expr.size()) {
						const auto& valueElem = sexpr.expr[i + 1];
						if (valueElem.token == tsSexprAtom && valueElem.ref >= 0 &&
								valueElem.ref < static_cast<int>(sexpr.strings.size())) {
							return sexpr.strings[valueElem.ref];
						} else if (valueElem.token == tsSexprString && valueElem.ref >= 0 &&
											valueElem.ref < static_cast<int>(sexpr.strings.size())) {
							return sexpr.strings[valueElem.ref];
						}
					}
				}
			}
		}
	}
	return "";
}

// Helper function to safely convert string to integer
bool safeStringToInt(const std::string& str, int& result) {
	if (str.empty()) return false;
	
	// Check if string contains only valid characters for integer
	size_t start = 0;
	if (str[0] == '-' || str[0] == '+') {
		if (str.length() == 1) return false;  // Just a sign is invalid
		start = 1;
	}
	
	for (size_t i = start; i < str.length(); ++i) {
		if (!std::isdigit(str[i])) return false;
	}
	
	try {
		result = std::stoi(str);
		return true;
	} catch (...) {
		return false;
	}
}

LineRange parseLineSpec(const std::string& lines_param) {
	LineRange range;
	if (lines_param.empty()) {
		range.type = LineRange::Full;
		return range;
	}
	if (!lines_param.starts_with("@")) {
		range.valid = false;
		return range;
	}
	std::string spec = lines_param.substr(1);
	if (spec.find(':') != std::string::npos) {
		size_t colon_pos = spec.find(':');
		std::string start_str = spec.substr(0, colon_pos);
		std::string end_str = spec.substr(colon_pos + 1);
		if (start_str == "e") {
			if (end_str.starts_with("-")) {
				range.type = LineRange::FromEnd;
				int temp_count;
				if (!safeStringToInt(end_str, temp_count)) {
					range.valid = false;
					return range;
				}
				range.count = std::abs(temp_count);
			} else if (end_str == "0") {
				range.type = LineRange::AppendAtEnd;
				range.count = 0;
			} else {
				range.valid = false;
			}
		} else if (start_str == "0" && !end_str.starts_with("-")) {
			range.type = LineRange::FromStart;
			if (!safeStringToInt(end_str, range.count)) {
				range.valid = false;
				return range;
			}
		} else {
			range.type = LineRange::Range;
			if (end_str.starts_with("-")) {
				int end_line, line_count;
				if (!safeStringToInt(start_str, end_line) || !safeStringToInt(end_str, line_count)) {
					range.valid = false;
					return range;
				}
				range.start = end_line - std::abs(line_count) + 1;
				range.count = std::abs(line_count);
			} else {
				int end_line;
				if (!safeStringToInt(start_str, range.start) || !safeStringToInt(end_str, end_line)) {
					range.valid = false;
					return range;
				}
				range.count = end_line - range.start + 1;
			}
		}
	} else {
		range.type = LineRange::SingleLine;
		if (!safeStringToInt(spec, range.start)) {
			range.valid = false;
			return range;
		}
		range.count = 1;
	}
	return range;
}

} // namespace LabDb
