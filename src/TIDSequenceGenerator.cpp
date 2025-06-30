#include "LabDb/TIDSequenceGenerator.h"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace LabDb {

TIDSequenceGenerator::TIDSequenceGenerator(MDB_env* env)
    : _env(env)
    , _sequence_dbi(0) {
    if (!_env) {
        throw TIDSequenceException("Invalid LMDB environment provided");
    }
    init();
}

TIDSequenceGenerator::~TIDSequenceGenerator() {
    // DBI is automatically closed when environment is closed
    // No explicit cleanup needed
}

void TIDSequenceGenerator::init() {
    // Open/create the sequence DBI
    MDB_txn* txn = nullptr;
    int rc = mdb_txn_begin(_env, nullptr, 0, &txn);
    if (rc != 0) {
        throw TIDSequenceException("Failed to begin transaction for TID sequence initialization", rc);
    }
    
    try {
        // Create sequence DBI: stores the current sequence counter
        rc = mdb_dbi_open(txn, "tid_sequence", MDB_CREATE, &_sequence_dbi);
        if (rc != 0) {
            throw TIDSequenceException("Failed to open tid_sequence DBI", rc);
        }
        
        // Initialize sequence counter if it doesn't exist
        MDB_val key, data;
        key.mv_data = const_cast<char*>(SEQUENCE_KEY);
        key.mv_size = strlen(SEQUENCE_KEY);
        
        rc = mdb_get(txn, _sequence_dbi, &key, &data);
        if (rc == MDB_NOTFOUND) {
            // Initialize sequence to first valid TID
            std::string initial_value = encode_tid(FIRST_VALID_TID);
            data.mv_data = const_cast<char*>(initial_value.c_str());
            data.mv_size = initial_value.size();
            
            rc = mdb_put(txn, _sequence_dbi, &key, &data, 0);
            if (rc != 0) {
                throw TIDSequenceException("Failed to initialize TID sequence counter", rc);
            }
        } else if (rc != 0) {
            throw TIDSequenceException("Failed to check TID sequence counter", rc);
        }
        
        rc = mdb_txn_commit(txn);
        if (rc != 0) {
            throw TIDSequenceException("Failed to commit TID sequence initialization transaction", rc);
        }
        
    } catch (...) {
        mdb_txn_abort(txn);
        throw;
    }
}

TIDSequenceGenerator::TID TIDSequenceGenerator::next_sequence(MDB_txn* txn) {
    if (!validate_transaction(txn)) {
        throw TIDSequenceException("Invalid transaction provided to next_sequence");
    }
    
    // Get current sequence value
    TID current_tid = get_sequence_value(txn);
    
    // Check for overflow
    if (current_tid >= MAX_VALID_TID) {
        throw TIDSequenceException("TID sequence overflow - maximum TID reached");
    }
    
    TID next_tid = current_tid + 1;
    
    // Update sequence counter atomically
    if (!set_sequence_value(txn, next_tid)) {
        throw TIDSequenceException("Failed to update TID sequence counter");
    }
    
    return current_tid;
}

TIDSequenceGenerator::TID TIDSequenceGenerator::current_sequence(MDB_txn* txn) const {
    if (!validate_transaction(txn)) {
        throw TIDSequenceException("Invalid transaction provided to current_sequence");
    }
    
    return get_sequence_value(txn);
}

TIDSequenceGenerator::TID TIDSequenceGenerator::peek_next_sequence(MDB_txn* txn) const {
    if (!validate_transaction(txn)) {
        throw TIDSequenceException("Invalid transaction provided to peek_next_sequence");
    }
    
    TID current_tid = get_sequence_value(txn);
    
    // Check for overflow
    if (current_tid >= MAX_VALID_TID) {
        throw TIDSequenceException("TID sequence overflow - no next TID available");
    }
    
    return current_tid + 1;
}

TIDSequenceGenerator::TID TIDSequenceGenerator::total_allocated(MDB_txn* txn) const {
    TID current_tid = current_sequence(txn);
    return (current_tid >= FIRST_VALID_TID) ? (current_tid - FIRST_VALID_TID + 1) : 0;
}

bool TIDSequenceGenerator::is_valid_tid(MDB_txn* txn, TID tid) const {
    if (!validate_tid(tid)) {
        return false;
    }
    
    TID current_tid = current_sequence(txn);
    return tid >= FIRST_VALID_TID && tid <= current_tid;
}

bool TIDSequenceGenerator::reset_sequence(MDB_txn* txn, TID new_value) {
    if (!validate_transaction(txn)) {
        return false;
    }
    
    if (!validate_tid(new_value) || new_value < FIRST_VALID_TID) {
        return false;
    }
    
    return set_sequence_value(txn, new_value);
}

TIDSequenceGenerator::Stats TIDSequenceGenerator::get_stats(MDB_txn* txn) const {
    Stats stats = {};
    
    try {
        stats.current_sequence = current_sequence(txn);
        stats.next_available = peek_next_sequence(txn);
        stats.total_allocated = total_allocated(txn);
        stats.sequence_healthy = validate_sequence(txn);
        
        // Get DBI size
        MDB_stat dbi_stat;
        int rc = mdb_stat(txn, _sequence_dbi, &dbi_stat);
        if (rc == 0) {
            stats.sequence_dbi_size = dbi_stat.ms_psize * 
                (dbi_stat.ms_branch_pages + dbi_stat.ms_leaf_pages + dbi_stat.ms_overflow_pages);
        }
        
    } catch (const TIDSequenceException&) {
        stats.sequence_healthy = false;
    }
    
    return stats;
}

bool TIDSequenceGenerator::validate_sequence(MDB_txn* txn) const {
    try {
        TID current_tid = current_sequence(txn);
        
        // Basic validation
        if (!validate_tid(current_tid)) {
            return false;
        }
        
        // Sequence should be at least FIRST_VALID_TID
        if (current_tid < FIRST_VALID_TID) {
            return false;
        }
        
        // Check that we can peek next (no overflow)
        try {
            peek_next_sequence(txn);
        } catch (const TIDSequenceException&) {
            // If we can't peek next due to overflow, that's still valid
            // as long as current is at MAX_VALID_TID
            return (current_tid == MAX_VALID_TID);
        }
        
        return true;
        
    } catch (const TIDSequenceException&) {
        return false;
    }
}

TIDSequenceGenerator::TID TIDSequenceGenerator::allocate_batch(MDB_txn* txn, size_t count) {
    if (!validate_transaction(txn)) {
        throw TIDSequenceException("Invalid transaction provided to allocate_batch");
    }
    
    if (count == 0) {
        throw TIDSequenceException("Batch allocation count must be greater than 0");
    }
    
    // Get current sequence value
    TID start_tid = get_sequence_value(txn);
    
    // Check for overflow
    if (start_tid > MAX_VALID_TID - count) {
        throw TIDSequenceException("TID sequence batch allocation would overflow");
    }
    
    TID new_sequence = start_tid + count;
    
    // Update sequence counter atomically
    if (!set_sequence_value(txn, new_sequence)) {
        throw TIDSequenceException("Failed to update TID sequence counter for batch allocation");
    }
    
    return start_tid + 1; // Return first allocated TID
}

bool TIDSequenceGenerator::is_valid_tid_range(MDB_txn* txn, TID start_tid, size_t count) const {
    if (count == 0 || !validate_tid(start_tid)) {
        return false;
    }
    
    TID end_tid = start_tid + count - 1;
    if (end_tid < start_tid) { // Overflow check
        return false;
    }
    
    TID current_tid = current_sequence(txn);
    return start_tid >= FIRST_VALID_TID && end_tid <= current_tid;
}

bool TIDSequenceGenerator::sync_sequence(MDB_txn* txn) {
    // In LMDB, data is automatically synced when transaction commits
    // This method is mainly for consistency with interface expectations
    return validate_sequence(txn);
}

// Private helper methods

TIDSequenceGenerator::TID TIDSequenceGenerator::get_sequence_value(MDB_txn* txn) const {
    MDB_val key, data;
    key.mv_data = const_cast<char*>(SEQUENCE_KEY);
    key.mv_size = strlen(SEQUENCE_KEY);
    
    int rc = mdb_get(txn, _sequence_dbi, &key, &data);
    if (rc != 0) {
        throw TIDSequenceException("Failed to get TID sequence value", rc);
    }
    
    std::string encoded_tid(static_cast<const char*>(data.mv_data), data.mv_size);
    return decode_tid(encoded_tid);
}

bool TIDSequenceGenerator::set_sequence_value(MDB_txn* txn, TID value) {
    MDB_val key, data;
    key.mv_data = const_cast<char*>(SEQUENCE_KEY);
    key.mv_size = strlen(SEQUENCE_KEY);
    
    std::string encoded_tid = encode_tid(value);
    data.mv_data = const_cast<char*>(encoded_tid.c_str());
    data.mv_size = encoded_tid.size();
    
    int rc = mdb_put(txn, _sequence_dbi, &key, &data, 0);
    return (rc == 0);
}

bool TIDSequenceGenerator::validate_tid(TID tid) const {
    return tid >= FIRST_VALID_TID && tid <= MAX_VALID_TID;
}

bool TIDSequenceGenerator::validate_transaction(MDB_txn* txn) const {
    // Basic validation - check that transaction pointer is not null
    // LMDB will validate the transaction handle internally
    return (txn != nullptr);
}

// Static utility methods
std::string TIDSequenceGenerator::encode_tid(TID tid) {
    // Encode as 16-character hex string for consistent sorting and storage
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(16) << std::hex << tid;
    return oss.str();
}

TIDSequenceGenerator::TID TIDSequenceGenerator::decode_tid(const std::string& encoded) {
    if (encoded.size() != 16) {
        throw TIDSequenceException("Invalid encoded TID length: " + std::to_string(encoded.size()));
    }
    
    std::istringstream iss(encoded);
    TID tid;
    iss >> std::hex >> tid;
    
    if (iss.fail()) {
        throw TIDSequenceException("Failed to decode TID: " + encoded);
    }
    
    return tid;
}

// Static utility methods for binary encoding

std::string TIDSequenceGenerator::encode_tid_for_storage(TID tid) {
    // Use same fixed-width hex encoding as internal sequence counter
    // This ensures optimal LMDB prefix compression and consistency
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(16) << std::hex << tid;
    return oss.str();
}

TIDSequenceGenerator::TID TIDSequenceGenerator::decode_tid_from_storage(const std::string& encoded) {
    if (encoded.length() != 16) {
        throw TIDSequenceException("Invalid encoded TID length: " + std::to_string(encoded.length()));
    }
    
    char* end;
    TID tid = std::strtoull(encoded.c_str(), &end, 16);
    if (end != encoded.c_str() + encoded.length()) {
        throw TIDSequenceException("Invalid encoded TID format: " + encoded);
    }
    
    return tid;
}

} // namespace LabDb
