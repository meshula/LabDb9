#include "LabDb/NonostoreKeys.h"
#include <algorithm>
#include <sstream>
#include <cassert>

namespace LabDb {

// Static constexpr definition
constexpr const char* NonostoreKeys::INDEX_PREFIXES[];

std::array<std::string, 9> NonostoreKeys::generate_all_keys(
    const std::string& subject,
    const std::string& predicate,
    const std::string& object) {
    
    std::array<std::string, 9> keys;
    
    // Generate all six content indices (traditional hexastore)
    for (int i = 0; i < 6; ++i) {
        keys[i] = generate_key(static_cast<IndexType>(i), subject, predicate, object);
    }
    
    // Generate three vocabulary indices
    keys[6] = generate_vocabulary_key(IndexType::SUBJECTS, subject);
    keys[7] = generate_vocabulary_key(IndexType::PREDICATES, predicate);
    keys[8] = generate_vocabulary_key(IndexType::OBJECTS, object);
    
    return keys;
}

std::string NonostoreKeys::generate_key(
    IndexType index,
    const std::string& subject,
    const std::string& predicate,
    const std::string& object) {
    
    // Escape the input terms to handle special characters
    std::string esc_subj = escape_term(subject);
    std::string esc_pred = escape_term(predicate);
    std::string esc_obj = escape_term(object);
    
    std::string prefix = INDEX_PREFIXES[static_cast<int>(index)];
    
    switch (index) {
        case IndexType::SPO:
            return prefix + esc_subj + SEPARATOR + esc_pred + SEPARATOR + esc_obj;
        case IndexType::SOP:
            return prefix + esc_subj + SEPARATOR + esc_obj + SEPARATOR + esc_pred;
        case IndexType::PSO:
            return prefix + esc_pred + SEPARATOR + esc_subj + SEPARATOR + esc_obj;
        case IndexType::POS:
            return prefix + esc_pred + SEPARATOR + esc_obj + SEPARATOR + esc_subj;
        case IndexType::OSP:
            return prefix + esc_obj + SEPARATOR + esc_subj + SEPARATOR + esc_pred;
        case IndexType::OPS:
            return prefix + esc_obj + SEPARATOR + esc_pred + SEPARATOR + esc_subj;
        default:
            // Vocabulary indices shouldn't use this method
            assert(false && "Use generate_vocabulary_key for vocabulary indices");
            return "";
    }
}

std::string NonostoreKeys::generate_vocabulary_key(IndexType vocab_index, const std::string& term) {
    assert(vocab_index >= IndexType::SUBJECTS && vocab_index <= IndexType::OBJECTS);
    
    std::string prefix = INDEX_PREFIXES[static_cast<int>(vocab_index)];
    std::string esc_term = escape_term(term);
    
    return prefix + esc_term + SEPARATOR + VOCABULARY_VALUE;
}

std::string NonostoreKeys::generate_query_prefix(
    const std::string& subject_pattern,
    const std::string& predicate_pattern,
    const std::string& object_pattern) {
    
    // Determine which index to use based on what's specified vs wildcards
    bool subj_wild = (subject_pattern == "*");
    bool pred_wild = (predicate_pattern == "*");
    bool obj_wild = (object_pattern == "*");
    
    // Choose optimal index based on query pattern
    IndexType optimal_index;
    std::string prefix;
    
    if (!subj_wild && !pred_wild && !obj_wild) {
        // Specific triple: s-p-o -> use SPO index
        optimal_index = IndexType::SPO;
        prefix = INDEX_PREFIXES[static_cast<int>(optimal_index)] + 
                escape_term(subject_pattern) + SEPARATOR + 
                escape_term(predicate_pattern) + SEPARATOR + 
                escape_term(object_pattern);
    }
    else if (!subj_wild && !pred_wild && obj_wild) {
        // s-p-* -> use SPO index
        optimal_index = IndexType::SPO;
        prefix = INDEX_PREFIXES[static_cast<int>(optimal_index)] + 
                escape_term(subject_pattern) + SEPARATOR + 
                escape_term(predicate_pattern) + SEPARATOR;
    }
    else if (!subj_wild && pred_wild && !obj_wild) {
        // s-*-o -> use SOP index
        optimal_index = IndexType::SOP;
        prefix = INDEX_PREFIXES[static_cast<int>(optimal_index)] + 
                escape_term(subject_pattern) + SEPARATOR + 
                escape_term(object_pattern) + SEPARATOR;
    }
    else if (subj_wild && !pred_wild && !obj_wild) {
        // *-p-o -> use POS index
        optimal_index = IndexType::POS;
        prefix = INDEX_PREFIXES[static_cast<int>(optimal_index)] + 
                escape_term(predicate_pattern) + SEPARATOR + 
                escape_term(object_pattern) + SEPARATOR;
    }
    else if (!subj_wild && pred_wild && obj_wild) {
        // s-*-* -> use SPO index
        optimal_index = IndexType::SPO;
        prefix = INDEX_PREFIXES[static_cast<int>(optimal_index)] + 
                escape_term(subject_pattern) + SEPARATOR;
    }
    else if (subj_wild && !pred_wild && obj_wild) {
        // *-p-* -> use PSO index
        optimal_index = IndexType::PSO;
        prefix = INDEX_PREFIXES[static_cast<int>(optimal_index)] + 
                escape_term(predicate_pattern) + SEPARATOR;
    }
    else if (subj_wild && pred_wild && !obj_wild) {
        // *-*-o -> use OSP index
        optimal_index = IndexType::OSP;
        prefix = INDEX_PREFIXES[static_cast<int>(optimal_index)] + 
                escape_term(object_pattern) + SEPARATOR;
    }
    else {
        // *-*-* -> use SPO index (could use any)
        optimal_index = IndexType::SPO;
        prefix = INDEX_PREFIXES[static_cast<int>(optimal_index)];
    }
    
    return prefix;
}

std::string NonostoreKeys::get_vocabulary_prefix(IndexType vocab_index) {
    assert(vocab_index >= IndexType::SUBJECTS && vocab_index <= IndexType::OBJECTS);
    return INDEX_PREFIXES[static_cast<int>(vocab_index)];
}

NonostoreKeys::ParsedKey NonostoreKeys::parse_key(const std::string& key) {
    ParsedKey result;
    result.is_vocabulary = false;
    
    // Find which prefix matches
    for (int i = 0; i < 9; ++i) {
        std::string prefix = INDEX_PREFIXES[i];
        if (key.substr(0, prefix.length()) == prefix) {
            result.index_type = static_cast<IndexType>(i);
            
            // Extract the remainder after prefix
            std::string remainder = key.substr(prefix.length());
            
            if (i >= 6) { // Vocabulary index
                result.is_vocabulary = true;
                // Format: ~subjects~term~1
                size_t sep_pos = remainder.find(SEPARATOR);
                if (sep_pos != std::string::npos) {
                    result.term = unescape_term(remainder.substr(0, sep_pos));
                }
            } else { // Content index
                // Split on unescaped separators to get the three components
                std::vector<std::string> parts;
                std::string current_part;
                
                for (size_t i = 0; i < remainder.length(); ++i) {
                    if (remainder[i] == '~') {
                        if (i + 1 < remainder.length() && (remainder[i + 1] == 'T' || remainder[i + 1] == 'B')) {
                            // This is an escaped character, add it to current part
                            current_part += remainder.substr(i, 2);
                            ++i; // Skip the next character
                        } else {
                            // This is a separator, end current part
                            if (!current_part.empty()) {
                                parts.push_back(unescape_term(current_part));
                                current_part.clear();
                            }
                        }
                    } else {
                        current_part += remainder[i];
                    }
                }
                
                // Add the last part
                if (!current_part.empty()) {
                    parts.push_back(unescape_term(current_part));
                }
                
                if (parts.size() >= 3) {
                    // Map back to S-P-O based on index type
                    switch (result.index_type) {
                        case IndexType::SPO:
                            result.subject = parts[0];
                            result.predicate = parts[1];
                            result.object = parts[2];
                            break;
                        case IndexType::SOP:
                            result.subject = parts[0];
                            result.object = parts[1];
                            result.predicate = parts[2];
                            break;
                        case IndexType::PSO:
                            result.predicate = parts[0];
                            result.subject = parts[1];
                            result.object = parts[2];
                            break;
                        case IndexType::POS:
                            result.predicate = parts[0];
                            result.object = parts[1];
                            result.subject = parts[2];
                            break;
                        case IndexType::OSP:
                            result.object = parts[0];
                            result.subject = parts[1];
                            result.predicate = parts[2];
                            break;
                        case IndexType::OPS:
                            result.object = parts[0];
                            result.predicate = parts[1];
                            result.subject = parts[2];
                            break;
                        default:
                            break;
                    }
                }
            }
            break;
        }
    }
    
    return result;
}

std::string NonostoreKeys::index_name(IndexType index) {
    switch (index) {
        case IndexType::SPO: return "SPO (Subject-Predicate-Object)";
        case IndexType::SOP: return "SOP (Subject-Object-Predicate)";
        case IndexType::PSO: return "PSO (Predicate-Subject-Object)";
        case IndexType::POS: return "POS (Predicate-Object-Subject)";
        case IndexType::OSP: return "OSP (Object-Subject-Predicate)";
        case IndexType::OPS: return "OPS (Object-Predicate-Subject)";
        case IndexType::SUBJECTS: return "SUBJECTS (Motion vocabulary)";
        case IndexType::PREDICATES: return "PREDICATES (Memory vocabulary)";
        case IndexType::OBJECTS: return "OBJECTS (Field vocabulary)";
        default: return "Unknown";
    }
}

// Simple escaping: replace ~ with ~T and \ with ~B
std::string NonostoreKeys::escape_term(const std::string& term) {
    std::string result;
    result.reserve(term.length() * 2); // Worst case
    
    for (char c : term) {
        if (c == '~') {
            result += "~T";
        } else if (c == '\\') {
            result += "~B";
        } else {
            result += c;
        }
    }
    
    return result;
}

std::string NonostoreKeys::unescape_term(const std::string& escaped_term) {
    std::string result;
    result.reserve(escaped_term.length());
    
    for (size_t i = 0; i < escaped_term.length(); ++i) {
        if (escaped_term[i] == '~' && i + 1 < escaped_term.length()) {
            char next = escaped_term[i + 1];
            if (next == 'T') {
                result += '~';
                ++i; // Skip the escape sequence
            } else if (next == 'B') {
                result += '\\';
                ++i; // Skip the escape sequence
            } else {
                result += escaped_term[i]; // Keep the ~
            }
        } else {
            result += escaped_term[i];
        }
    }
    
    return result;
}

} // namespace LabDb
