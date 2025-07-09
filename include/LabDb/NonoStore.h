#pragma once

#include "LabDb/LmdbStore.h"
#include "LabDb/NonostoreKeys.h"
#include "LabDb/EntityId.h"
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <optional>

// Forward declarations for TID architecture
namespace LabDb {
    class TermDictionary;
    class TIDSequenceGenerator;
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
    
    /// add entities through relationship (atomic across all nine indices)
    bool add_triple(const std::string& subject, 
                    const std::string& predicate, 
                    const std::string& object);
    
    /// remove entities (atomic removal from all nine indices)
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
    std::vector<Triple> entities_with_subject(const std::string& subject) {
        return query(subject, "*", "*");
    }
    
    /// What entities have this relationship? (*-predicate-*)
    std::vector<Triple> entities_with_predicate(const std::string& predicate) {
        return query("*", predicate, "*");
    }
    
    /// What connects to this context? (*-*-object)
    std::vector<Triple> entities_with_object(const std::string& object) {
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
    
    /// Synchronous transaction for EntityId operations
    /// Encapsulates transaction management without leaking MDB_txn* details
    class SynchronousTransaction {
    public:
        SynchronousTransaction(NonoStore& store);
        ~SynchronousTransaction();
        
        /// Intern a term and return EntityId (get-or-create)
        EntityId intern_eid(const std::string& term);
        
        /// Resolve EID to EntityId (lookup existing)
        EntityId resolve_eid(const std::string& eid);
        
        /// Resolve TID to EntityId (lookup existing)
        EntityId resolve_tid(const TID& tid);
        
    private:
        NonoStore& _store;
    };
    
    /// Create a synchronous transaction for EntityId operations
    std::unique_ptr<SynchronousTransaction> begin_sync();
    
    /// Batch operations for efficiency
    class BatchTransaction {
    public:
        BatchTransaction(NonoStore& store);
        ~BatchTransaction();
        
        /// Add operations to batch
        void add_triple(const std::string& subject, 
                        const std::string& predicate, 
                        const std::string& object);
        void remove_triple(const std::string& subject, 
                           const std::string& predicate, 
                           const std::string& object);
        
        /// Execute all operations atomically
        bool commit();
        
        /// Cancel all operations
        void abort();
        
    private:
        NonoStore& _store;
        std::unique_ptr<LmdbStore::Transaction> _txn;
        std::vector<std::array<NonostoreKeys::Key, 9>> _connect_keys;
        std::vector<std::array<NonostoreKeys::Key, 9>> _disconnect_keys;
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
    
    /// Access to TermDictionary for EntityId integration
    const TermDictionary& getTermDictionary() const { return *_term_dict; }
    TermDictionary& getTermDictionary() { return *_term_dict; }
    
    LmdbStore& getLmdbStore() { return *_store; }
    
    //-------------------------------------------------------------------------
    // TripleStore functionality (consolidated from separate TripleStore class)
    //-------------------------------------------------------------------------
    
    using TID = uint64_t;
    using TermID = uint64_t; 
    using Timestamp = std::chrono::system_clock::time_point;
    
    /// Triple data structure with full provenance
    struct TripleData {
        TermID subject_id;     ///< Subject TermID from TermDictionary
        TermID predicate_id;   ///< Predicate TermID from TermDictionary
        TermID object_id;      ///< Object TermID from TermDictionary
        Timestamp timestamp;   ///< When this triple was created/last modified
        std::string source;    ///< Source of this triple (URL, file, user, etc.)
        float confidence;      ///< Confidence score (0.0 - 1.0)
        uint32_t flags;        ///< Bitfield for various flags
        
        TripleData() = default;
        TripleData(TermID sid, TermID pid, TermID oid, 
                   const std::string& src = "", float conf = 1.0f, uint32_t fl = 0)
            : subject_id(sid), predicate_id(pid), object_id(oid)
            , timestamp(std::chrono::system_clock::now())
            , source(src), confidence(conf), flags(fl) {}
    };
    
    /// Triple flags for various properties
    enum TripleFlags : uint32_t {
        NONE = 0x00000000,           ///< No special flags
        INFERRED = 0x00000001,       ///< Triple was inferred, not explicitly stated
        TEMPORARY = 0x00000002,      ///< Temporary triple, may be cleaned up
        SYSTEM = 0x00000004,         ///< System-generated triple
        USER_CREATED = 0x00000008,   ///< User explicitly created this triple
        IMPORTED = 0x00000010,       ///< Triple was imported from external source
        VERIFIED = 0x00000020,       ///< Triple has been verified/validated
        DEPRECATED = 0x00000040,     ///< Triple is deprecated but kept for history
        HIGH_CONFIDENCE = 0x00000080 ///< High confidence triple (confidence >= 0.9)
    };
    
    /// Store a new triple, returns the assigned TID
    TID store_triple_internal(MDB_txn* txn, 
                              const std::string& subject, 
                              const std::string& predicate, 
                              const std::string& object,
                              const std::string& source = "",
                              float confidence = 1.0f,
                              uint32_t flags = TripleFlags::NONE);
    
    /// Store a triple with pre-interned TermIDs (more efficient)
    TID store_triple_internal(MDB_txn* txn, 
                              TermID subject_id,
                              TermID predicate_id, 
                              TermID object_id,
                              const std::string& source = "",
                              float confidence = 1.0f,
                              uint32_t flags = TripleFlags::NONE);
    
    /// Retrieve triple data by TID
    std::optional<TripleData> get_triple_internal(MDB_txn* txn, TID tid) const;
    
    /// Retrieve triple as strings (resolves TermIDs to strings)
    struct StringTriple {
        std::string subject;
        std::string predicate;
        std::string object;
        Timestamp timestamp;
        std::string source;
        float confidence;
        uint32_t flags;
        TID tid;  ///< Include TID for reference
    };
    std::optional<StringTriple> get_triple_as_strings_internal(MDB_txn* txn, TID tid) const;
    
    /// Remove triple by TID
    bool remove_triple_internal(MDB_txn* txn, TID tid);
    
private:
    std::unique_ptr<LmdbStore> _store;
    mutable Result _last_error;
    
    // TID-based architecture components
    std::unique_ptr<TermDictionary> _term_dict;
    std::unique_ptr<TIDSequenceGenerator> _tid_gen;
    
    // TripleStore DBIs (consolidated from separate TripleStore class)
    MDB_dbi _triple_dbi;              ///< Main triple storage DBI
    MDB_dbi _subject_index_dbi;       ///< Subject TermID → list of TIDs
    MDB_dbi _predicate_index_dbi;     ///< Predicate TermID → list of TIDs  
    MDB_dbi _object_index_dbi;        ///< Object TermID → list of TIDs
    
    /// Internal helpers
    bool set_error(ErrorCode code, const std::string& message);
    
    /// Parse triples from query results
    std::vector<Triple> parse_query_results(
        const std::vector<std::pair<std::string, std::string>>& raw_results,
        NonostoreKeys::IndexType index_type);
    
    /// Parse TID-based query results
    std::vector<Triple> parse_query_results_tid(
        const std::vector<std::pair<std::string, std::string>>& raw_results,
        MDB_txn* mdb_txn);
    
    /// Generate TID-based query prefix for efficient binary key matching
    std::string generate_tid_based_query_prefix(
        std::optional<uint64_t> subject_id,
        std::optional<uint64_t> predicate_id,
        std::optional<uint64_t> object_id);
    
    bool remove_triple_impl(LmdbStore::Transaction& txn,
                         const std::string& subject,
                         const std::string& predicate,
                         const std::string& object);
    
    /// TripleStore internal helpers (consolidated from separate TripleStore class)
    void init_triple_store_dbis();
    std::string serialize_triple_data(const TripleData& data) const;
    TripleData deserialize_triple_data(const std::string& serialized) const;
    bool update_indices(MDB_txn* txn, TID tid, const TripleData& data, bool add);
    bool add_to_index(MDB_txn* txn, MDB_dbi index_dbi, TermID term_id, TID tid);
    bool remove_from_index(MDB_txn* txn, MDB_dbi index_dbi, TermID term_id, TID tid);
    std::vector<TID> get_tids_from_index(MDB_txn* txn, MDB_dbi index_dbi, TermID term_id) const;
    bool validate_triple_data(const TripleData& data) const;
    bool validate_tid(TID tid) const;
    static std::string encode_tid(TID tid);
    static TID decode_tid(const std::string& encoded);
    static std::string encode_term_id(TermID term_id);
    static TermID decode_term_id(const std::string& encoded);
    
    /// TID-based architecture helpers
    std::string encode_tid_for_storage(uint64_t tid);
    uint64_t decode_tid_from_storage(const std::string& stored);
    std::string generate_tid_based_crown_key(NonostoreKeys::IndexType, uint64_t subject_id, uint64_t predicate_id, uint64_t object_id);
    std::vector<std::string> generate_tid_based_crown_keys(uint64_t subject_id, uint64_t predicate_id, uint64_t object_id);
    std::string generate_vocabulary_key_for_term_id(NonostoreKeys::IndexType vocab_type, uint64_t term_id);
};

} // namespace LabDb
