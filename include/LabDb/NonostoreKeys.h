#pragma once

#include <string>
#include <array>
#include <vector>

namespace LabDb {

/// Nine-index key generation for nonostore (hexastore + vocabulary discovery)
class NonostoreKeys {
public:
    /// The nine index types in our nonostore
    enum class IndexType {
        SPO = 0,  // Subject-Predicate-Object
        SOP = 1,  // Subject-Object-Predicate  
        PSO = 2,  // Predicate-Subject-Object
        POS = 3,  // Predicate-Object-Subject
        OSP = 4,  // Object-Subject-Predicate
        OPS = 5,  // Object-Predicate-Subject
        SUBJECTS = 6,  // Vocabulary: all subjects
        PREDICATES = 7,  // Vocabulary: all predicates
        OBJECTS = 8   // Vocabulary: all objects
    };
    
    /// Generate all nine keys for a subject-predicate-object triple
    static std::array<std::string, 9> generate_all_keys(
        const std::string& subject,
        const std::string& predicate, 
        const std::string& object
    );
    
    /// Generate a specific index key
    static std::string generate_key(
        IndexType index,
        const std::string& subject,
        const std::string& predicate,
        const std::string& object
    );
    
    /// Generate vocabulary key for a specific term
    static std::string generate_vocabulary_key(IndexType vocab_index, const std::string& term);
    
    /// Generate prefix for queries like "granite-*-*" or "*-isA-*"
    static std::string generate_query_prefix(
        const std::string& subject_pattern,    // "granite" or "*"
        const std::string& predicate_pattern,  // "isA" or "*"
        const std::string& object_pattern      // "rock" or "*"
    );
    
    /// Get prefix for vocabulary discovery queries
    static std::string get_vocabulary_prefix(IndexType vocab_index);
    
    /// Parse a key back into its components (for debugging/analysis)
    struct ParsedKey {
        IndexType index_type;
        std::string subject;
        std::string predicate;
        std::string object;
        bool is_vocabulary;
        std::string term; // for vocabulary keys
    };
    static ParsedKey parse_key(const std::string& key);
    
    /// Get human-readable name for index type
    static std::string index_name(IndexType index);
    
private:
    /// Internal escaping for safe key generation
    static std::string escape_term(const std::string& term);
    static std::string unescape_term(const std::string& escaped_term);
    
    /// Index prefixes
    static constexpr const char* INDEX_PREFIXES[] = {
        "~spo~",     // SPO
        "~sop~",     // SOP
        "~pso~",     // PSO  
        "~pos~",     // POS
        "~osp~",     // OSP
        "~ops~",     // OPS
        "~subjects~", // SUBJECTS
        "~predicates~", // PREDICATES
        "~objects~"  // OBJECTS
    };
    
    static constexpr const char* SEPARATOR = "~";
    static constexpr const char* VOCABULARY_VALUE = "1";
};

} // namespace LabDb
