#pragma once
#include <string>

/*
Example usage:
NormalizedText nt = normalize_text("Śūnya", NormalizeMode::AsciiFold);
std::cout << "Original:      " << nt.original << "\n";
std::cout << "Case Folded:   " << nt.case_folded << "\n";
std::cout << "ASCII Folded:  " << nt.ascii_folded << "\n";
std::cout << "Notes:         " << nt.notes << "\n";
*/

enum class NormalizeMode {
    CaseFoldOnly,
    AsciiFold
};

struct NormalizedText {
    std::string original;
    std::string case_folded;
    std::string ascii_folded;
    std::string notes;
};

NormalizedText normalize_text(const std::string& input, NormalizeMode mode);
