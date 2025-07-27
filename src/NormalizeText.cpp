
#include "LabDb/NormalizeText.h"

#include <string>
#include <utf8proc.h>

NormalizedText normalize_text(const std::string& input, NormalizeMode mode) {
    NormalizedText result;
    result.original = input;

    // Common: Case fold
    utf8proc_option_t case_options = static_cast<utf8proc_option_t>(UTF8PROC_CASEFOLD | UTF8PROC_STABLE);
    utf8proc_uint8_t* folded = nullptr;
    utf8proc_ssize_t len = utf8proc_map(
        reinterpret_cast<const utf8proc_uint8_t*>(input.c_str()),
        input.length(),  // Use explicit length
        &folded,
        case_options
    );

    if (len < 0 || !folded) {
        result.notes = "Error during case folding.";
        return result;
    }

    result.case_folded = std::string(reinterpret_cast<char*>(folded), len);
    free(folded);

    // Optional: ASCII fold
    if (mode == NormalizeMode::AsciiFold) {
        utf8proc_option_t ascii_options = static_cast<utf8proc_option_t>(
            UTF8PROC_CASEFOLD |
            UTF8PROC_DECOMPOSE |
            UTF8PROC_STRIPMARK |
            UTF8PROC_STABLE
        );

        utf8proc_uint8_t* ascii = nullptr;
        utf8proc_ssize_t ascii_len = utf8proc_map(
            reinterpret_cast<const utf8proc_uint8_t*>(input.c_str()),
            input.length(),  // Use explicit length
            &ascii,
            ascii_options
        );

        if (ascii_len < 0 || !ascii) {
            result.notes += " Error during ASCII folding.";
        } else {
            result.ascii_folded = std::string(reinterpret_cast<char*>(ascii), ascii_len);
            free(ascii);
        }
    } else {
        // For CaseFoldOnly mode, ASCII folded is same as case folded
        result.ascii_folded = result.case_folded;
    }

    // Add informative notes
    result.notes += "Normalization applied: ";
    result.notes += (mode == NormalizeMode::AsciiFold) ? "Case + ASCII fold." : "Case fold only.";

    return result;
}
