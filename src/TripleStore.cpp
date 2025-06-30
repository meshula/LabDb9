#include "LabDb/TripleStore.h"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <set>

namespace LabDb {

TripleStore::TripleStore(MDB_env* env, TermDictionary& term_dict, TIDSequenceGenerator& tid_gen)
    : _env(env)
    , _triple_dbi(0)
    , _subject_index_dbi(0)
    , _predicate_index_dbi(0)
    , _object_index_dbi(0)
    , _term_dict(term_dict)
    , _tid_gen(tid_gen) {
    if (!_env) {
        throw TripleStoreException("Invalid LMDB environment provided");
    }
    init();
}

TripleStore::~TripleStore() {
    // DBIs are automatically closed when environment is closed
    // No explicit cleanup needed
}

void TripleStore::init() {
    // Open/create the required DBIs
    MDB_txn* txn = nullptr;
    int rc = mdb_txn_begin(_env, nullptr, 0, &txn);
    if (rc != 0) {
        throw TripleStoreException("Failed to begin transaction for TripleStore initialization", rc);
    }
    
    try {
        // Main triple storage: TID → TripleData
        rc = mdb_dbi_open(txn, "triple", MDB_CREATE, &_triple_dbi);
        if (rc != 0) {
            throw TripleStoreException("Failed to open triple DBI", rc);
        }
        
        // Subject index: TermID → list of TIDs
        rc = mdb_dbi_open(txn, "triple_subject_index", MDB_CREATE | MDB_DUPSORT, &_subject_index_dbi);
        if (rc != 0) {
            throw TripleStoreException("Failed to open subject index DBI", rc);
        }
        
        // Predicate index: TermID → list of TIDs
        rc = mdb_dbi_open(txn, "triple_predicate_index", MDB_CREATE | MDB_DUPSORT, &_predicate_index_dbi);
        if (rc != 0) {
            throw TripleStoreException("Failed to open predicate index DBI", rc);
        }
        
        // Object index: TermID → list of TIDs
        rc = mdb_dbi_open(txn, "triple_object_index", MDB_CREATE | MDB_DUPSORT, &_object_index_dbi);
        if (rc != 0) {
            throw TripleStoreException("Failed to open object index DBI", rc);
        }
        
        rc = mdb_txn_commit(txn);
        if (rc != 0) {
            throw TripleStoreException("Failed to commit TripleStore initialization transaction", rc);
        }
        
    } catch (...) {
        mdb_txn_abort(txn);
        throw;
    }
}

// Core triple operations

TripleStore::TID TripleStore::store_triple(MDB_txn* txn,
                                           const std::string& subject,
                                           const std::string& predicate,
                                           const std::string& object,
                                           const std::string& source,
                                           float confidence,
                                           uint32_t flags) {
    // Intern strings to TermIDs
    TermID subject_id = _term_dict.intern(txn, subject);
    TermID predicate_id = _term_dict.intern(txn, predicate);
    TermID object_id = _term_dict.intern(txn, object);
    
    return store_triple(txn, subject_id, predicate_id, object_id, source, confidence, flags);
}

TripleStore::TID TripleStore::store_triple(MDB_txn* txn,
                                           TermID subject_id,
                                           TermID predicate_id,
                                           TermID object_id,
                                           const std::string& source,
                                           float confidence,
                                           uint32_t flags) {
    TripleData data(subject_id, predicate_id, object_id, source, confidence, flags);
    return store_triple(txn, data);
}

TripleStore::TID TripleStore::store_triple(MDB_txn* txn, const TripleData& triple_data) {
    if (!validate_triple_data(triple_data)) {
        throw TripleStoreException("Invalid triple data provided");
    }
    
    // Allocate new TID
    TID new_tid = _tid_gen.next_sequence(txn);
    
    // Serialize triple data
    std::string serialized = serialize_triple_data(triple_data);
    
    // Store in main triple DBI
    std::string tid_key = encode_tid(new_tid);
    MDB_val key, data;
    key.mv_data = const_cast<char*>(tid_key.c_str());
    key.mv_size = tid_key.size();
    data.mv_data = const_cast<char*>(serialized.c_str());
    data.mv_size = serialized.size();
    
    int rc = mdb_put(txn, _triple_dbi, &key, &data, 0);
    if (rc != 0) {
        throw TripleStoreException("Failed to store triple in main DBI", rc);
    }
    
    // Update indices
    if (!update_indices(txn, new_tid, triple_data, true)) {
        throw TripleStoreException("Failed to update indices for new triple");
    }
    
    return new_tid;
}

std::optional<TripleStore::TripleData> TripleStore::get_triple(MDB_txn* txn, TID tid) const {
    if (!validate_tid(tid)) {
        return std::nullopt;
    }
    
    std::string tid_key = encode_tid(tid);
    MDB_val key, data;
    key.mv_data = const_cast<char*>(tid_key.c_str());
    key.mv_size = tid_key.size();
    
    int rc = mdb_get(txn, _triple_dbi, &key, &data);
    if (rc == MDB_NOTFOUND) {
        return std::nullopt;
    } else if (rc != 0) {
        throw TripleStoreException("Failed to retrieve triple", rc);
    }
    
    std::string serialized(static_cast<const char*>(data.mv_data), data.mv_size);
    return deserialize_triple_data(serialized);
}

std::optional<TripleStore::StringTriple> TripleStore::get_triple_as_strings(MDB_txn* txn, TID tid) const {
    auto triple_data = get_triple(txn, tid);
    if (!triple_data) {
        return std::nullopt;
    }
    
    // Resolve TermIDs to strings
    auto subject = _term_dict.resolve(txn, triple_data->subject_id);
    auto predicate = _term_dict.resolve(txn, triple_data->predicate_id);
    auto object = _term_dict.resolve(txn, triple_data->object_id);
    
    if (!subject || !predicate || !object) {
        throw TripleStoreException("Failed to resolve TermIDs to strings for TID: " + std::to_string(tid));
    }
    
    StringTriple result;
    result.subject = *subject;
    result.predicate = *predicate;
    result.object = *object;
    result.timestamp = triple_data->timestamp;
    result.source = triple_data->source;
    result.confidence = triple_data->confidence;
    result.flags = triple_data->flags;
    result.tid = tid;
    
    return result;
}

bool TripleStore::exists(MDB_txn* txn, TID tid) const {
    return get_triple(txn, tid).has_value();
}

// Serialization helpers
std::string TripleStore::serialize_triple_data(const TripleData& data) const {
    std::ostringstream oss;
    
    // Simple binary serialization format:
    // [subject_id:8][predicate_id:8][object_id:8][timestamp:8][confidence:4][flags:4][source_len:4][source:N]
    
    // TermIDs (8 bytes each)
    oss.write(reinterpret_cast<const char*>(&data.subject_id), sizeof(data.subject_id));
    oss.write(reinterpret_cast<const char*>(&data.predicate_id), sizeof(data.predicate_id));
    oss.write(reinterpret_cast<const char*>(&data.object_id), sizeof(data.object_id));
    
    // Timestamp (8 bytes)
    auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        data.timestamp.time_since_epoch()).count();
    oss.write(reinterpret_cast<const char*>(&timestamp_ms), sizeof(timestamp_ms));
    
    // Confidence (4 bytes)
    oss.write(reinterpret_cast<const char*>(&data.confidence), sizeof(data.confidence));
    
    // Flags (4 bytes)
    oss.write(reinterpret_cast<const char*>(&data.flags), sizeof(data.flags));
    
    // Source string (4 bytes length + string data)
    uint32_t source_len = data.source.size();
    oss.write(reinterpret_cast<const char*>(&source_len), sizeof(source_len));
    oss.write(data.source.c_str(), source_len);
    
    return oss.str();
}

TripleStore::TripleData TripleStore::deserialize_triple_data(const std::string& serialized) const {
    std::istringstream iss(serialized);
    TripleData data;
    
    // Read TermIDs
    iss.read(reinterpret_cast<char*>(&data.subject_id), sizeof(data.subject_id));
    iss.read(reinterpret_cast<char*>(&data.predicate_id), sizeof(data.predicate_id));
    iss.read(reinterpret_cast<char*>(&data.object_id), sizeof(data.object_id));
    
    // Read timestamp
    int64_t timestamp_ms;
    iss.read(reinterpret_cast<char*>(&timestamp_ms), sizeof(timestamp_ms));
    data.timestamp = std::chrono::system_clock::time_point(std::chrono::milliseconds(timestamp_ms));
    
    // Read confidence and flags
    iss.read(reinterpret_cast<char*>(&data.confidence), sizeof(data.confidence));
    iss.read(reinterpret_cast<char*>(&data.flags), sizeof(data.flags));
    
    // Read source string
    uint32_t source_len;
    iss.read(reinterpret_cast<char*>(&source_len), sizeof(source_len));
    data.source.resize(source_len);
    iss.read(&data.source[0], source_len);
    
    if (iss.fail()) {
        throw TripleStoreException("Failed to deserialize triple data");
    }
    
    return data;
}

// Index management
bool TripleStore::update_indices(MDB_txn* txn, TID tid, const TripleData& data, bool add) {
    bool success = true;
    
    if (add) {
        success &= add_to_index(txn, _subject_index_dbi, data.subject_id, tid);
        success &= add_to_index(txn, _predicate_index_dbi, data.predicate_id, tid);
        success &= add_to_index(txn, _object_index_dbi, data.object_id, tid);
    } else {
        success &= remove_from_index(txn, _subject_index_dbi, data.subject_id, tid);
        success &= remove_from_index(txn, _predicate_index_dbi, data.predicate_id, tid);
        success &= remove_from_index(txn, _object_index_dbi, data.object_id, tid);
    }
    
    return success;
}

bool TripleStore::add_to_index(MDB_txn* txn, MDB_dbi index_dbi, TermID term_id, TID tid) {
    std::string term_key = encode_term_id(term_id);
    std::string tid_value = encode_tid(tid);
    
    MDB_val key, data;
    key.mv_data = const_cast<char*>(term_key.c_str());
    key.mv_size = term_key.size();
    data.mv_data = const_cast<char*>(tid_value.c_str());
    data.mv_size = tid_value.size();
    
    int rc = mdb_put(txn, index_dbi, &key, &data, 0);
    return (rc == 0);
}

bool TripleStore::remove_from_index(MDB_txn* txn, MDB_dbi index_dbi, TermID term_id, TID tid) {
    std::string term_key = encode_term_id(term_id);
    std::string tid_value = encode_tid(tid);
    
    MDB_val key, data;
    key.mv_data = const_cast<char*>(term_key.c_str());
    key.mv_size = term_key.size();
    data.mv_data = const_cast<char*>(tid_value.c_str());
    data.mv_size = tid_value.size();
    
    int rc = mdb_del(txn, index_dbi, &key, &data);
    return (rc == 0);
}

std::vector<TripleStore::TID> TripleStore::get_tids_from_index(MDB_txn* txn, MDB_dbi index_dbi, TermID term_id) const {
    std::vector<TID> results;
    
    std::string term_key = encode_term_id(term_id);
    MDB_cursor* cursor;
    int rc = mdb_cursor_open(txn, index_dbi, &cursor);
    if (rc != 0) {
        return results;
    }
    
    MDB_val key, data;
    key.mv_data = const_cast<char*>(term_key.c_str());
    key.mv_size = term_key.size();
    
    rc = mdb_cursor_get(cursor, &key, &data, MDB_SET);
    if (rc == 0) {
        do {
            std::string tid_str(static_cast<const char*>(data.mv_data), data.mv_size);
            TID tid = decode_tid(tid_str);
            results.push_back(tid);
        } while (mdb_cursor_get(cursor, &key, &data, MDB_NEXT_DUP) == 0);
    }
    
    mdb_cursor_close(cursor);
    return results;
}

// Validation helpers
bool TripleStore::validate_triple_data(const TripleData& data) const {
    return data.subject_id > 0 && data.predicate_id > 0 && data.object_id > 0 &&
           data.confidence >= 0.0f && data.confidence <= 1.0f;
}

bool TripleStore::validate_tid(TID tid) const {
    return tid >= TIDSequenceGenerator::FIRST_VALID_TID && tid <= TIDSequenceGenerator::MAX_VALID_TID;
}

// Static utility methods
std::string TripleStore::encode_tid(TID tid) {
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(16) << std::hex << tid;
    return oss.str();
}

TripleStore::TID TripleStore::decode_tid(const std::string& encoded) {
    std::istringstream iss(encoded);
    TID tid;
    iss >> std::hex >> tid;
    return tid;
}

std::string TripleStore::encode_term_id(TermID term_id) {
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(16) << std::hex << term_id;
    return oss.str();
}

TripleStore::TermID TripleStore::decode_term_id(const std::string& encoded) {
    std::istringstream iss(encoded);
    TermID term_id;
    iss >> std::hex >> term_id;
    return term_id;
}

bool TripleStore::remove_triple(MDB_txn* txn, TID tid) {
    try {
        // 1. Get triple data before removing (needed for index cleanup)
        auto triple_data_opt = get_triple(txn, tid);
        if (!triple_data_opt) {
            return false; // Triple doesn't exist
        }
        
        TripleData triple_data = *triple_data_opt;
        
        // 2. Remove from indices first
        if (!update_indices(txn, tid, triple_data, false)) { // false = remove
            return false;
        }
        
        // 3. Remove from central triple DBI
        std::string key = encode_tid(tid);
        MDB_val mdb_key = { key.size(), (void*)key.c_str() };
        
        int rc = mdb_del(txn, _triple_dbi, &mdb_key, nullptr);
        if (rc != 0 && rc != MDB_NOTFOUND) {
            throw TripleStoreException("Failed to remove triple from main DBI", rc);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        throw TripleStoreException("remove_triple failed: " + std::string(e.what()));
    }
}

} // namespace LabDb
