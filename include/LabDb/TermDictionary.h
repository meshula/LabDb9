#pragma once

#include "LabDb/LmdbStore.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <lmdb.h>

namespace LabDb {

/// TermDictionary: Bidirectional string↔u64 mapping for compact storage
/// Implements the foundation for TID-based architecture transformation
/// 
/// Core capability: Convert arbitrary UTF-8 strings to compact 64-bit IDs
/// and vice versa, with persistent storage in LMDB 'dict' DBI.
/// This enables crown indices to store 8-byte TIDs instead of full strings,
/// reducing storage by orders of magnitude.
class TermDictionary {
public:
    using TermID = uint64_t;
    
    /// Constructor - opens/creates dictionary in existing LMDB environment
    /// Uses separate 'dict' and 'dict_reverse' DBIs for bidirectional mapping
    explicit TermDictionary(MDB_env* env);
    
    /// Destructor - ensures clean shutdown
    ~TermDictionary();
    
    /// No copy/move for now - keep it simple and safe
    TermDictionary(const TermDictionary&) = delete;
    TermDictionary& operator=(const TermDictionary&) = delete;
    TermDictionary(TermDictionary&&) = delete;
    TermDictionary& operator=(TermDictionary&&) = delete;

    /// Core dictionary operations
    
    /// Intern a string: convert to TermID, creating new ID if needed
    /// This is the primary method for string→ID conversion
    /// Returns existing ID if string already exists, creates new ID otherwise
    TermID intern(MDB_txn* txn, const std::string& term);
    
    /// Lookup TermID for existing string (read-only, no creation)
    /// Returns std::nullopt if string not found in dictionary
    std::optional<TermID> lookup(MDB_txn* txn, const std::string& term) const;
    
    /// Reverse lookup: TermID → string
    /// Returns std::nullopt if TermID not found in dictionary
    std::optional<std::string> resolve(MDB_txn* txn, TermID term_id) const;
    
    /// Check if string exists in dictionary
    bool exists(MDB_txn* txn, const std::string& term) const;
    
    /// Check if TermID exists in dictionary
    bool exists(MDB_txn* txn, TermID term_id) const;
    
    /// Dictionary statistics and management
    
    /// Get total number of terms in dictionary
    size_t count(MDB_txn* txn) const;
    
    /// Get next available TermID (for sequence management)
    TermID next_available_id(MDB_txn* txn) const;
    
    /// Dictionary iteration and vocabulary discovery
    
    /// Iterator for vocabulary discovery - enables "what terms exist?" queries
    class Iterator {
    public:
        Iterator(MDB_txn* txn, TermDictionary& dict, bool reverse_order = false);
        ~Iterator();
        
        // Make Iterator move-only (no copying)
        Iterator(const Iterator&) = delete;
        Iterator& operator=(const Iterator&) = delete;
        Iterator(Iterator&&) noexcept;
        Iterator& operator=(Iterator&&) noexcept;
        
        /// Move to first term
        bool first();
        
        /// Move to next term
        bool next();
        
        /// Move to previous term (if reverse iteration supported)
        bool prev();
        
        /// Get current term string
        std::string current_term() const;
        
        /// Get current TermID
        TermID current_id() const;
        
        /// Check if iterator is valid
        bool valid() const { return _valid; }
        
        /// Seek to specific term (for prefix-based iteration)
        bool seek(const std::string& term);
        
        /// Seek to specific TermID
        bool seek(TermID term_id);
        
    private:
        MDB_txn* _txn;
        TermDictionary& _dict;
        MDB_cursor* _cursor;
        bool _reverse_order;
        bool _valid;
        MDB_val _key, _value;
        
        void update_validity();
    };
    
    /// Convenience methods for bulk operations
    
    /// Get all terms (for vocabulary discovery)
    /// Warning: This can be large! Consider using Iterator for production
    std::vector<std::string> all_terms(MDB_txn* txn, size_t limit = 0) const;
    
    /// Get all term IDs
    std::vector<TermID> all_term_ids(MDB_txn* txn, size_t limit = 0) const;
    
    /// Batch intern multiple terms (more efficient than individual calls)
    std::vector<TermID> intern_batch(MDB_txn* txn, const std::vector<std::string>& terms);
    
    /// Batch resolve multiple IDs
    std::vector<std::optional<std::string>> resolve_batch(MDB_txn* txn, const std::vector<TermID>& term_ids) const;
    
    /// Debugging and diagnostics
    
    /// Get dictionary statistics
    struct Stats {
        size_t total_terms;
        size_t next_available_id;
        size_t dict_size_bytes;
        size_t reverse_dict_size_bytes;
        TermID min_term_id;
        TermID max_term_id;
    };
    Stats get_stats(MDB_txn* txn) const;
    
    /// Validate dictionary consistency (forward and reverse mappings match)
    bool validate_consistency(MDB_txn* txn) const;
    
    /// Get the underlying LMDB DBIs (for advanced operations)
    MDB_dbi dict_dbi() const { return _dict_dbi; }
    MDB_dbi reverse_dict_dbi() const { return _reverse_dict_dbi; }

    /// Constants
    static constexpr TermID INVALID_TERM_ID = 0;     ///< Reserved invalid ID
    static constexpr TermID FIRST_VALID_ID = 1;      ///< First valid TermID
    static constexpr const char* SEQUENCE_KEY = "__next_term_id__";  ///< Sequence counter key
    
    /// Utility methods for binary encoding (static for use throughout codebase)
    static std::string encode_term_id_for_storage(TermID term_id);
    static TermID decode_term_id_from_storage(const std::string& encoded);

private:
    MDB_env* _env;              ///< LMDB environment (not owned)
    MDB_dbi _dict_dbi;          ///< string → TermID mapping
    MDB_dbi _reverse_dict_dbi;  ///< TermID → string mapping  
    MDB_dbi _sequence_dbi;      ///< sequence counter for next TermID
    
    /// Sequence management for TermID generation
    TermID next_sequence(MDB_txn* txn);
    
    /// Internal initialization
    void init();
    
    /// Key encoding/decoding utilities
    static std::string encode_term_id(TermID id);
    static TermID decode_term_id(const std::string& encoded);
    
    /// Validation helpers
    bool validate_term(const std::string& term) const;
};

/// Exception class for TermDictionary-specific errors
class TermDictionaryException : public std::exception {
public:
    explicit TermDictionaryException(const std::string& message, int mdb_error = 0)
        : _message(message), _mdb_error(mdb_error) {
        if (mdb_error != 0) {
            _message += " (LMDB error: " + std::string(mdb_strerror(mdb_error)) + ")";
        }
    }
    
    const char* what() const noexcept override {
        return _message.c_str();
    }
    
    int mdb_error() const { return _mdb_error; }
    
private:
    std::string _message;
    int _mdb_error;
};

} // namespace LabDb
