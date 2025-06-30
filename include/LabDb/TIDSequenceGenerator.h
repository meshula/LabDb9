#pragma once

#include "LabDb/LmdbStore.h"
#include <cstdint>
#include <string>
#include <lmdb.h>

namespace LabDb {

/// TIDSequenceGenerator: Manages unique Triple IDs (TIDs) for the TID-based architecture
/// 
/// This class provides atomic sequence generation for unique triple identifiers,
/// enabling the storage architecture transformation where crown indices store
/// 8-byte TIDs instead of full triple strings, reducing storage by orders of magnitude.
/// 
/// Key features:
/// - Atomic sequence generation with LMDB transactions
/// - Thread-safe ID allocation
/// - Persistent counter across database restarts
/// - Efficient u64 sequence with no gaps (except on crash)
/// - Integration with existing LMDB environment
class TIDSequenceGenerator {
public:
    using TID = uint64_t;
    
    /// Constructor - opens/creates sequence generator in existing LMDB environment
    /// Uses separate 'tid_sequence' DBI for sequence management
    explicit TIDSequenceGenerator(MDB_env* env);
    
    /// Destructor - ensures clean shutdown
    ~TIDSequenceGenerator();
    
    /// No copy/move for now - keep it simple and safe
    TIDSequenceGenerator(const TIDSequenceGenerator&) = delete;
    TIDSequenceGenerator& operator=(const TIDSequenceGenerator&) = delete;
    TIDSequenceGenerator(TIDSequenceGenerator&&) = delete;
    TIDSequenceGenerator& operator=(TIDSequenceGenerator&&) = delete;

    /// Core sequence operations
    
    /// Generate next unique TID (thread-safe, atomic)
    /// This is the primary method for TID allocation
    /// Must be called within an active LMDB transaction
    TID next_sequence(MDB_txn* txn);
    
    /// Get current sequence value (read-only, no increment)
    /// Returns the last allocated TID, not the next one
    TID current_sequence(MDB_txn* txn) const;
    
    /// Get next available TID without allocating it (peek operation)
    /// Useful for capacity planning and diagnostics
    TID peek_next_sequence(MDB_txn* txn) const;
    
    /// Sequence management and diagnostics
    
    /// Get total number of TIDs allocated
    TID total_allocated(MDB_txn* txn) const;
    
    /// Check if a TID is valid (within allocated range)
    bool is_valid_tid(MDB_txn* txn, TID tid) const;
    
    /// Reset sequence to specific value (DANGEROUS - use with caution)
    /// Only for migration/repair scenarios
    bool reset_sequence(MDB_txn* txn, TID new_value);
    
    /// Sequence statistics and health monitoring
    
    /// Sequence statistics
    struct Stats {
        TID current_sequence;      ///< Last allocated TID
        TID next_available;        ///< Next TID that will be allocated
        TID total_allocated;       ///< Total TIDs allocated since creation
        size_t sequence_dbi_size;  ///< Storage size of sequence DBI
        bool sequence_healthy;     ///< Sequence consistency check
    };
    Stats get_stats(MDB_txn* txn) const;
    
    /// Validate sequence consistency (detect corruption)
    bool validate_sequence(MDB_txn* txn) const;
    
    /// Bulk operations for efficiency
    
    /// Allocate multiple TIDs in batch (more efficient than individual calls)
    /// Returns the starting TID of the allocated range
    /// All TIDs from start_tid to (start_tid + count - 1) are allocated
    TID allocate_batch(MDB_txn* txn, size_t count);
    
    /// Check if a range of TIDs is valid
    bool is_valid_tid_range(MDB_txn* txn, TID start_tid, size_t count) const;
    
    /// Advanced operations
    
    /// Get the underlying LMDB DBI (for advanced operations)
    MDB_dbi sequence_dbi() const { return _sequence_dbi; }
    
    /// Force sequence sync to disk (normally handled by LMDB transactions)
    bool sync_sequence(MDB_txn* txn);
    
    /// Constants and limits
    static constexpr TID INVALID_TID = 0;           ///< Reserved invalid TID
    static constexpr TID FIRST_VALID_TID = 1;       ///< First valid TID
    static constexpr TID MAX_VALID_TID = UINT64_MAX - 1;  ///< Maximum valid TID
    static constexpr const char* SEQUENCE_KEY = "__tid_sequence__";  ///< Sequence counter key
    
    /// Utility methods for binary encoding (static for use throughout codebase)
    static std::string encode_tid_for_storage(TID tid);
    static TID decode_tid_from_storage(const std::string& encoded);

private:
    MDB_env* _env;           ///< LMDB environment (not owned)
    MDB_dbi _sequence_dbi;   ///< TID sequence counter DBI
    
    /// Internal initialization
    void init();
    
    /// Internal sequence management
    TID get_sequence_value(MDB_txn* txn) const;
    bool set_sequence_value(MDB_txn* txn, TID value);
    
    /// Validation helpers
    bool validate_tid(TID tid) const;
    bool validate_transaction(MDB_txn* txn) const;
    
    /// Key encoding for consistent storage
    static std::string encode_tid(TID tid);
    static TID decode_tid(const std::string& encoded);
};

/// Exception class for TIDSequenceGenerator-specific errors
class TIDSequenceException : public std::exception {
public:
    explicit TIDSequenceException(const std::string& message, int mdb_error = 0)
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
