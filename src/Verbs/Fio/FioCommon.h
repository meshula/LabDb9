#pragma once

#include "LabDb/LabText.hpp"
#include <string>
#include <algorithm>

namespace LabDb {

// Parameter extraction utility
std::string extractStringParam(const lab::Text::Sexpr& sexpr, const std::string& param_name);

// Line range parsing for fio-read and fio-write
struct LineRange {
	enum Type { Full, Range, SingleLine, FromEnd, FromStart, AppendAtEnd } type = Full;
	int start = 0;
	int count = -1;
	bool valid = true;
};

LineRange parseLineSpec(const std::string& lines_param);

// Write mode enum for fio-write safety and functionality
struct WriteMode {
	enum Type { 
		Unspecified, Replace, Insert, Append, Prepend, Unrecognized
	};
	
	static Type fromString(const std::string& mode_str) {
		if (mode_str.empty()) return Unspecified;
		std::string mode_lower = mode_str;
		std::transform(mode_lower.begin(), mode_lower.end(), mode_lower.begin(), ::tolower);
		if (mode_lower == "replace") return Replace;
		if (mode_lower == "insert") return Insert;
		if (mode_lower == "append") return Append;
		if (mode_lower == "prepend") return Prepend;
		return Unrecognized;
	}
	
	static std::string toString(Type type) {
		switch (type) {
			case Unspecified: return "unspecified";
			case Replace: return "replace";
			case Insert: return "insert";
			case Append: return "append";
			case Prepend: return "prepend";
			case Unrecognized: return "unrecognized";
		}
		return "unknown";
	}
};

} // namespace LabDb
