#pragma once

#include "LabDb/LmdbStore.h"
#include "LabDb/NonostoreKeys.h"
#include <string>
#include <vector>
#include <memory>

// Forward declarations for TID architecture
namespace LabDb {
    class TermDictionary;
    class TIDSequenceGenerator;
    class TripleStore;
    class TriadicQuery; // Forward declaration for factory method
}

namespace LabDb {

/// Core NonoStore: Triadic consciousness database implementing nine-index architecture
/// Combines LmdbStore with NonostoreKeys for complete crown manifestation
class NonoStore : public std::enable_shared_from_this<NonoStore> {
public:
    /// Constructor - creates/opens database at specified path
    explicit NonoStore(const std::string& database_path, 
                       size_t map_size = 1UL * 1024 * 1024 * 1024); // 1GB default
    
    /// Destructor - ensures clean shutdown
    ~NonoStore();
    
    /// No copy/move for now - keep it simple and safe
    NonoStore(const NonoStore&) = delete;
    NonoStore& operator=(const NonoStore&) = delete;
    NonoStore(NonoStore&&) = delete;
    NonoStore& operator=(NonoStore&&) = delete;
    
    /// Core triadic operations
    
    /// Connect entities through relationship (atomic across all nine indices)
    bool add_triple(const std::string& subject, 
                    const std::string& predicate, 
                    const std::string& object);
    
    /// Disconnect entities (atomic removal from all nine indices)
    bool remove_triple(const std::string& subject, 
                       const std::string& predicate, 
                       const std::string& object);
    
    /// Query result structure
    struct Triple {
        std::string subject;
        std::string predicate;
        std::string object;
        
        Triple(const std::string& s, const std::string& p, const std::string& o)
            : subject(s), predicate(p), object(o) {}
    };
    
    /// Core query patterns (hexastore-style with optimal index selection)
    std::vector<Triple> query(const std::string& subject_pattern,    // "granite" or "*"
                              const std::string& predicate_pattern,  // "isA" or "*"
                              const std::string& object_pattern);    // "rock" or "*"
    
    /// Motion vocabulary discovery (all entities that act/exist)
    std::vector<std::string> all_subjects();
    
    /// Memory vocabulary discovery (all relationship types)
    std::vector<std::string> all_predicates();
    
    /// Field vocabulary discovery (all contexts/properties)
    std::vector<std::string> all_objects();
    
    /// Factory pattern for TriadicQuery creation
    /// Creates TriadicQuery with weak_ptr to avoid pybind11 holder type issues
    /// This enables safe lifecycle management and prevents circular references
    std::unique_ptr<TriadicQuery> create_triadic_query();
    
    /// Convenience query methods for common patterns
    
    /// What properties does this entity have? (subject-*-*)
    std::vector<Triple> properties_of(const std::string& subject) {
        return query(subject, "*", "*");
    }
    
    /// What entities have this relationship? (*-predicate-*)
    std::vector<Triple> entities_with_relation(const std::string& predicate) {
        return query("*", predicate, "*");
    }
    
    /// What connects to this context? (*-*-object)
    std::vector<Triple> connections_to(const std::string& object) {
        return query("*", "*", object);
    }
    
    /// Database introspection
    
    /// Get database statistics
    struct Stats {
        size_t total_triples;
        size_t unique_subjects;
        size_t unique_predicates;
        size_t unique_objects;
        LmdbStore::Stats lmdb_stats;
        
        // TID architecture statistics
        size_t term_dictionary_size;     // Number of terms in TID dictionary
        size_t subject_vocabulary_size;  // Number of subject terms
        size_t predicate_vocabulary_size; // Number of predicate terms
        size_t object_vocabulary_size;   // Number of object terms
        size_t hexastore_indices_size;   // Total size of hexastore indices
        size_t crown_indices_size;       // Total size of crown indices (if implemented)
    };
    Stats get_stats();
    
    /// Get detailed TID architecture metrics
    struct TIDMetrics {
        size_t term_dict_entries;        // Total entries in term dictionary
        size_t subject_tid_range;        // Highest subject TID allocated
        size_t predicate_tid_range;      // Highest predicate TID allocated
        size_t object_tid_range;         // Highest object TID allocated
        size_t total_tids_allocated;     // Total TIDs allocated across all types
        double storage_efficiency;      // Ratio of logical triples to storage entries
    };
    TIDMetrics get_tid_metrics();
    
    /// Check if a specific triple exists
    bool exists(const std::string& subject, 
                const std::string& predicate, 
                const std::string& object);
    
    /// Count results for a query pattern (without returning them)
    size_t count(const std::string& subject_pattern,
                 const std::string& predicate_pattern,
                 const std::string& object_pattern);
    
    /// Advanced operations
    
    /// Batch operations for efficiency
    class BatchTransaction {
    public:
        BatchTransaction(NonoStore& store);
        ~BatchTransaction();
        
        /// Add operations to batch
        void connect(const std::string& subject, 
                     const std::string& predicate, 
                     const std::string& object);
        void disconnect(const std::string& subject, 
                        const std::string& predicate, 
                        const std::string& object);
        
        /// Execute all operations atomically
        bool commit();
        
        /// Cancel all operations
        void abort();
        
    private:
        NonoStore& _store;
        std::unique_ptr<LmdbStore::Transaction> _txn;
        std::vector<std::array<std::string, 9>> _connect_keys;
        std::vector<std::array<std::string, 9>> _disconnect_keys;
        bool _active;
    };
    
    /// Create a batch transaction for multiple operations
    std::unique_ptr<BatchTransaction> begin_batch();
    
    /// Error handling
    enum class ErrorCode {
        Success,
        DatabaseError,
        TransactionError,
        InvalidQuery,
        KeyGenerationError
    };
    
    struct Result {
        ErrorCode code;
        std::string message;
        
        bool success() const { return code == ErrorCode::Success; }
        operator bool() const { return success(); }
    };
    
    /// Get last error (for debugging)
    Result get_last_error() const { return _last_error; }
    
private:
    std::unique_ptr<LmdbStore> _store;
    mutable Result _last_error;
    
    // TID-based architecture components
    std::unique_ptr<TermDictionary> _term_dict;
    std::unique_ptr<TIDSequenceGenerator> _tid_gen;
    std::unique_ptr<TripleStore> _triple_store;
    
    /// Internal helpers
    bool set_error(ErrorCode code, const std::string& message);
    
    /// Parse triples from query results
    std::vector<Triple> parse_query_results(
        const std::vector<std::pair<std::string, std::string>>& raw_results,
        NonostoreKeys::IndexType index_type);
    
    /// Internal implementation for add_triple/remove_triple
    bool add_triple_impl(LmdbStore::Transaction& txn,
                      const std::string& subject,
                      const std::string& predicate,
                      const std::string& object);
    
    bool remove_triple_impl(LmdbStore::Transaction& txn,
                         const std::string& subject,
                         const std::string& predicate,
                         const std::string& object);
    
    /// TID-based architecture helpers
    std::string encode_tid_for_storage(uint64_t tid);
    uint64_t decode_tid_from_storage(const std::string& stored);
    std::vector<std::string> generate_tid_based_crown_keys(uint64_t subject_id, uint64_t predicate_id, uint64_t object_id);
    std::string generate_vocabulary_key_for_term_id(NonostoreKeys::IndexType vocab_type, uint64_t term_id);
};

} // namespace LabDb
