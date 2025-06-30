#pragma once

#include "LabDb/TIDSequenceGenerator.h"
#include "LabDb/TermDictionary.h"
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <lmdb.h>

namespace LabDb {

/// TripleStore: Central storage for triple data with provenance support
/// 
/// This class implements the core TID-based architecture where:
/// - Each triple gets a unique TID (Triple ID) 
/// - Full triple data + provenance is stored once in the central 'triple' DBI
/// - Crown indices store only 8-byte TIDs, reducing storage by orders of magnitude
/// 
/// The TripleStore maps: TID → (sid, pid, oid, timestamp, source, confidence, flags)
/// where sid/pid/oid are TermIDs from the TermDictionary.
/// 
/// Key features:
/// - Central single-source-of-truth for triple data
/// - Rich provenance metadata (timestamp, source, confidence, flags)
/// - Integration with TermDictionary for string↔TermID mapping
/// - Integration with TIDSequenceGenerator for unique TID allocation
/// - Efficient binary storage with compact serialization
/// - ACID transactions for data integrity
class TripleStore {
public:
    using TID = TIDSequenceGenerator::TID;
    using TermID = TermDictionary::TermID;
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
    
    /// Constructor - opens/creates TripleStore in existing LMDB environment
    /// Requires TermDictionary and TIDSequenceGenerator instances
    explicit TripleStore(MDB_env* env, TermDictionary& term_dict, TIDSequenceGenerator& tid_gen);
    
    /// Destructor - ensures clean shutdown
    ~TripleStore();
    
    /// No copy/move for now - keep it simple and safe
    TripleStore(const TripleStore&) = delete;
    TripleStore& operator=(const TripleStore&) = delete;
    TripleStore(TripleStore&&) = delete;
    TripleStore& operator=(TripleStore&&) = delete;

    /// Core triple operations
    
    /// Store a new triple, returns the assigned TID
    /// Automatically interns strings through TermDictionary and allocates TID
    TID store_triple(MDB_txn* txn, 
                     const std::string& subject, 
                     const std::string& predicate, 
                     const std::string& object,
                     const std::string& source = "",
                     float confidence = 1.0f,
                     uint32_t flags = TripleFlags::NONE);
    
    /// Store a triple with pre-interned TermIDs (more efficient)
    TID store_triple(MDB_txn* txn, 
                     TermID subject_id,
                     TermID predicate_id, 
                     TermID object_id,
                     const std::string& source = "",
                     float confidence = 1.0f,
                     uint32_t flags = TripleFlags::NONE);
    
    /// Store a triple with full TripleData structure
    TID store_triple(MDB_txn* txn, const TripleData& triple_data);
    
    /// Retrieve triple data by TID
    std::optional<TripleData> get_triple(MDB_txn* txn, TID tid) const;
    
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
    std::optional<StringTriple> get_triple_as_strings(MDB_txn* txn, TID tid) const;
    
    /// Check if a triple exists by TID
    bool exists(MDB_txn* txn, TID tid) const;
    
    /// Update triple metadata (provenance) without changing the core triple
    bool update_provenance(MDB_txn* txn, TID tid, 
                           const std::string& source = "",
                           float confidence = -1.0f,  // -1 means don't change
                           uint32_t flags = UINT32_MAX); // UINT32_MAX means don't change
    
    /// Remove a triple by TID
    bool remove_triple(MDB_txn* txn, TID tid);
    
    /// Bulk operations for efficiency
    
    /// Store multiple triples in batch (more efficient than individual calls)
    std::vector<TID> store_triples_batch(MDB_txn* txn, const std::vector<TripleData>& triples);
    
    /// Retrieve multiple triples by TIDs
    std::vector<std::optional<TripleData>> get_triples_batch(MDB_txn* txn, const std::vector<TID>& tids) const;
    
    /// Query and filtering operations
    
    /// Find TIDs for triples matching specific criteria
    std::vector<TID> find_triples_by_subject(MDB_txn* txn, TermID subject_id) const;
    std::vector<TID> find_triples_by_predicate(MDB_txn* txn, TermID predicate_id) const;
    std::vector<TID> find_triples_by_object(MDB_txn* txn, TermID object_id) const;
    
    /// Find triples by provenance criteria
    std::vector<TID> find_triples_by_source(MDB_txn* txn, const std::string& source) const;
    std::vector<TID> find_triples_by_confidence_range(MDB_txn* txn, float min_confidence, float max_confidence) const;
    std::vector<TID> find_triples_by_flags(MDB_txn* txn, uint32_t flags_mask, bool all_flags = false) const;
    std::vector<TID> find_triples_by_time_range(MDB_txn* txn, Timestamp start, Timestamp end) const;
    
    /// Statistics and introspection
    
    /// TripleStore statistics
    struct Stats {
        size_t total_triples;        ///< Total number of triples stored
        size_t unique_subjects;      ///< Number of unique subject TermIDs
        size_t unique_predicates;    ///< Number of unique predicate TermIDs
        size_t unique_objects;       ///< Number of unique object TermIDs
        size_t total_storage_bytes;  ///< Total storage used by triple DBI
        TID min_tid;                 ///< Minimum TID in store
        TID max_tid;                 ///< Maximum TID in store
        size_t avg_triple_size;      ///< Average size per triple in bytes
        Timestamp oldest_triple;     ///< Timestamp of oldest triple
        Timestamp newest_triple;     ///< Timestamp of newest triple
    };
    Stats get_stats(MDB_txn* txn) const;
    
    /// Get all unique TermIDs for each position (for crown index building)
    std::vector<TermID> get_all_subject_ids(MDB_txn* txn) const;
    std::vector<TermID> get_all_predicate_ids(MDB_txn* txn) const;
    std::vector<TermID> get_all_object_ids(MDB_txn* txn) const;
    
    /// Iteration support for all triples
    class Iterator {
    public:
        Iterator(MDB_txn* txn, TripleStore& store);
        ~Iterator();
        
        /// Move to first triple
        bool first();
        
        /// Move to next triple
        bool next();
        
        /// Check if iterator is valid
        bool valid() const { return _valid; }
        
        /// Get current TID
        TID current_tid() const;
        
        /// Get current triple data
        TripleData current_triple() const;
        
        /// Seek to specific TID
        bool seek(TID tid);
        
    private:
        MDB_txn* _txn;
        TripleStore& _store;
        MDB_cursor* _cursor;
        bool _valid;
        MDB_val _key, _value;
        
        void update_validity();
    };
    
    /// Debugging and validation
    
    /// Validate TripleStore consistency
    bool validate_consistency(MDB_txn* txn) const;
    
    /// Get detailed information about a specific triple
    struct TripleInfo {
        TID tid;
        TripleData data;
        size_t storage_size;     ///< Size in bytes in database
        bool has_subject_index;  ///< Whether subject index entry exists (for debugging)
        bool has_predicate_index; ///< Whether predicate index entry exists
        bool has_object_index;   ///< Whether object index entry exists
    };
    std::optional<TripleInfo> get_triple_info(MDB_txn* txn, TID tid) const;
    
    /// Get the underlying LMDB DBI (for advanced operations)
    MDB_dbi triple_dbi() const { return _triple_dbi; }
    
    /// Access to underlying components
    TermDictionary& term_dictionary() { return _term_dict; }
    TIDSequenceGenerator& tid_generator() { return _tid_gen; }

private:
    MDB_env* _env;                    ///< LMDB environment (not owned)
    MDB_dbi _triple_dbi;              ///< Main triple storage DBI
    MDB_dbi _subject_index_dbi;       ///< Subject TermID → list of TIDs
    MDB_dbi _predicate_index_dbi;     ///< Predicate TermID → list of TIDs  
    MDB_dbi _object_index_dbi;        ///< Object TermID → list of TIDs
    
    TermDictionary& _term_dict;       ///< Reference to TermDictionary
    TIDSequenceGenerator& _tid_gen;   ///< Reference to TIDSequenceGenerator
    
    /// Internal initialization
    void init();
    
    /// Serialization helpers
    std::string serialize_triple_data(const TripleData& data) const;
    TripleData deserialize_triple_data(const std::string& serialized) const;
    
    /// Index management
    bool update_indices(MDB_txn* txn, TID tid, const TripleData& data, bool add);
    bool add_to_index(MDB_txn* txn, MDB_dbi index_dbi, TermID term_id, TID tid);
    bool remove_from_index(MDB_txn* txn, MDB_dbi index_dbi, TermID term_id, TID tid);
    std::vector<TID> get_tids_from_index(MDB_txn* txn, MDB_dbi index_dbi, TermID term_id) const;
    
    /// Validation helpers
    bool validate_triple_data(const TripleData& data) const;
    bool validate_tid(TID tid) const;
    
    /// Utility methods
    static std::string encode_tid(TID tid);
    static TID decode_tid(const std::string& encoded);
    static std::string encode_term_id(TermID term_id);
    static TermID decode_term_id(const std::string& encoded);
    static std::string serialize_timestamp(Timestamp ts);
    static Timestamp deserialize_timestamp(const std::string& serialized);
};

/// Exception class for TripleStore-specific errors
class TripleStoreException : public std::exception {
public:
    explicit TripleStoreException(const std::string& message, int mdb_error = 0)
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
