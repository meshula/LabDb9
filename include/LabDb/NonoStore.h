#pragma once

#include "LabDb/LmdbStore.h"
#include "LabDb/NonostoreKeys.h"
#include <string>
#include <vector>
#include <memory>

namespace LabDb {

/// Core NonoStore: Triadic consciousness database implementing nine-index architecture
/// Combines LmdbStore with NonostoreKeys for complete crown manifestation
class NonoStore {
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
    bool connect(const std::string& subject, 
                 const std::string& predicate, 
                 const std::string& object);
    
    /// Disconnect entities (atomic removal from all nine indices)
    bool disconnect(const std::string& subject, 
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
    };
    Stats get_stats();
    
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
    
    /// Internal helpers
    bool set_error(ErrorCode code, const std::string& message);
    
    /// Parse triples from query results
    std::vector<Triple> parse_query_results(
        const std::vector<std::pair<std::string, std::string>>& raw_results,
        NonostoreKeys::IndexType index_type);
    
    /// Internal implementation for connect/disconnect
    bool connect_impl(LmdbStore::Transaction& txn,
                      const std::string& subject,
                      const std::string& predicate,
                      const std::string& object);
    
    bool disconnect_impl(LmdbStore::Transaction& txn,
                         const std::string& subject,
                         const std::string& predicate,
                         const std::string& object);
};

} // namespace LabDb
