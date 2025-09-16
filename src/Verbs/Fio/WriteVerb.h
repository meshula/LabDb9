#pragma once

#include "FioCommon.h"
#include "LabDb/Verbs.h"
#include "LabDb/Db9Response.h"
#include "LabDb/LabText.hpp"
#include <string>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <mutex>

namespace LabDb {

//-----------------------------------------------------------------------------
// FioWriteVerb - File Write Operations with Line Surgery
//-----------------------------------------------------------------------------

/// Write content to file with atomic operations and backup support
class FioWriteVerb : public IDb9Verb {
public:
	std::string getVerbName() const override { return "fio-write"; }
	std::string getDescription() const override;
	Db9Response execute(const lab::Text::Sexpr& sexpr) override;

	// Preview and confirmation system
	struct PreviewContext {
		std::string token;
		std::string path;
		std::string content;
		LineRange range;
		WriteMode::Type write_mode_type;
		std::chrono::steady_clock::time_point created;
		std::vector<std::string> current_lines;
		std::string preview_reason; // NEW: Phase C1 - Track why preview was triggered
		
		bool isExpired() const {
			auto now = std::chrono::steady_clock::now();
			auto age = std::chrono::duration_cast<std::chrono::minutes>(now - created);
			return age.count() > 15;
		}
	};

	struct IndexRange {
		int start_idx;  // 0-based start index
		int end_idx;    // 0-based end index
		bool valid;
		
		IndexRange(int start, int end, bool v = true) : start_idx(start), end_idx(end), valid(v) {}
	};

	// Public accessors for confirmation system
	static bool hasPreviewToken(const std::string& token);
	static PreviewContext retrievePreviewContext(const std::string& token);
	static void removePreviewToken(const std::string& token);
	Db9Response executeConfirmedOperation(const std::string& token);

private:
    // Preview and cache management - static members
    static std::unordered_map<std::string, PreviewContext> preview_cache;
    static std::mutex cache_mutex;

    struct WriteParameters {
		std::string path;
		std::string content;

        enum class WriteMode {
            None,
            Insert,
            Replace,
            Append,
            Prepend,
            Touch
        };

        WriteMode write_mode = WriteMode::None; // Default to not writing
        std::string lines_param; // Line specification for surgery
        std::string preview_token; // Token for preview operations
        std::chrono::steady_clock::time_point start_time; // Start time for operation
	};

    Db9Response generatePreviewResponse(const std::string& path, const std::string& token, const std::string& content, const LineRange& range, WriteMode::Type, std::chrono::steady_clock::time_point start_time);
    Db9Response generatePreviewResponseFromExisting(const std::string& preview_content, const std::string& token, std::chrono::steady_clock::time_point start_time); // Phase C1
    Db9Response generatePreviewDisplay(const PreviewContext& context);
    Db9Response executeLineSurgery(const std::string& path, const std::string& content, const LineRange& range, WriteMode::Type, std::chrono::steady_clock::time_point start_time);
    std::string generatePreviewToken();
    void cleanupExpiredTokens();
    PreviewContext* getPreviewContext(const std::string& token);
    std::string generatePreviewResponse(const PreviewContext& context);
    bool shouldPreview(const std::string& path, const LineRange& range, WriteMode::Type, const std::string& content, std::string& result);
    
    // Phase C1: Line Surgery Awareness System
    std::string generateLineSurgeryPreview(const PreviewContext& context, const std::string& reason);

	Db9Response performTouchOperation(const std::string& path, std::chrono::steady_clock::time_point start_time);
	WriteParameters extractParameters(const lab::Text::Sexpr& sexpr);
	Db9Response performWrite(const WriteParameters& params);
	Db9Response performFullFileWrite(const std::string& path, const std::string& content, std::chrono::steady_clock::time_point start_time);
	Db9Response performLineSurgery(const std::string& path, const std::string& content, const LineRange& range, WriteMode::Type, std::chrono::steady_clock::time_point start_time);
	int performLineReplacement(std::vector<std::string>& lines, const std::vector<std::string>& new_content, const LineRange& range);
	int performLineInsertion(std::vector<std::string>& lines, const std::vector<std::string>& new_content, const LineRange& range);
	int performLineAppend(std::vector<std::string>& lines, const std::vector<std::string>& new_content, const LineRange& range);
	int performLinePrepend(std::vector<std::string>& lines, const std::vector<std::string>& new_content);
	std::string createBackupPath(const std::string& original_path);
	bool validatePath(const std::string& path);
	std::string generateTimestamp();
	IndexRange calculateIndices(const LineRange& range, int file_line_count);
};

} // namespace LabDb