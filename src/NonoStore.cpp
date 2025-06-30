#include "LabDb/NonoStore.h"
#include "LabDb/TripleStore.h"
#include "LabDb/TermDictionary.h"
#include "LabDb/TIDSequenceGenerator.h"
#include <algorithm>
#include <set>

namespace LabDb {

NonoStore::NonoStore(const std::string& database_path, size_t map_size)
    : _last_error{ErrorCode::Success, ""} {
    try {
        _store = std::make_unique<LmdbStore>(database_path, map_size);
        
        // Initialize TID-based architecture components
        MDB_env* env = _store->environment();
        _term_dict = std::make_unique<TermDictionary>(env);
        _tid_gen = std::make_unique<TIDSequenceGenerator>(env);
        _triple_store = std::make_unique<TripleStore>(env, *_term_dict, *_tid_gen);
        
    } catch (const LmdbException& e) {
        set_error(ErrorCode::DatabaseError, "Failed to open database: " + std::string(e.what()));
        throw;
    } catch (const TermDictionaryException& e) {
        set_error(ErrorCode::DatabaseError, "Failed to initialize TermDictionary: " + std::string(e.what()));
        throw;
    } catch (const TIDSequenceException& e) {
        set_error(ErrorCode::DatabaseError, "Failed to initialize TIDSequenceGenerator: " + std::string(e.what()));
        throw;
    } catch (const TripleStoreException& e) {
        set_error(ErrorCode::DatabaseError, "Failed to initialize TripleStore: " + std::string(e.what()));
        throw;
    }
}

NonoStore::~NonoStore() = default;

bool NonoStore::connect(const std::string& subject, 
                        const std::string& predicate, 
                        const std::string& object) {
    try {
        LmdbStore::Transaction txn(*_store);
        bool result = connect_impl(txn, subject, predicate, object);
        if (result) {
            txn.commit();
        }
        return result;
    } catch (const LmdbException& e) {
        return set_error(ErrorCode::TransactionError, "Connect failed: " + std::string(e.what()));
    }
}

bool NonoStore::disconnect(const std::string& subject, 
                           const std::string& predicate, 
                           const std::string& object) {
    try {
        LmdbStore::Transaction txn(*_store);
        bool result = disconnect_impl(txn, subject, predicate, object);
        if (result) {
            txn.commit();
        }
        return result;
    } catch (const LmdbException& e) {
        return set_error(ErrorCode::TransactionError, "Disconnect failed: " + std::string(e.what()));
    }
}

std::vector<NonoStore::Triple> NonoStore::query(const std::string& subject_pattern,
                                                 const std::string& predicate_pattern,
                                                 const std::string& object_pattern) {
    try {
        LmdbStore::Transaction txn(*_store, true); // read-only
        
        // Generate optimal query prefix based on patterns
        std::string query_prefix = NonostoreKeys::generate_query_prefix(
            subject_pattern, predicate_pattern, object_pattern);
        
        // Determine which index we're using for parsing
        NonostoreKeys::IndexType index_type;
        if (query_prefix.find("~spo~") == 0) index_type = NonostoreKeys::IndexType::SPO;
        else if (query_prefix.find("~sop~") == 0) index_type = NonostoreKeys::IndexType::SOP;
        else if (query_prefix.find("~pso~") == 0) index_type = NonostoreKeys::IndexType::PSO;
        else if (query_prefix.find("~pos~") == 0) index_type = NonostoreKeys::IndexType::POS;
        else if (query_prefix.find("~osp~") == 0) index_type = NonostoreKeys::IndexType::OSP;
        else if (query_prefix.find("~ops~") == 0) index_type = NonostoreKeys::IndexType::OPS;
        else {
            set_error(ErrorCode::InvalidQuery, "Invalid query pattern");
            return {};
        }
        
        // Execute prefix query
        auto raw_results = _store->query_prefix(txn, query_prefix);
        
        // Parse results into triples
        return parse_query_results(raw_results, index_type);
        
    } catch (const LmdbException& e) {
        set_error(ErrorCode::DatabaseError, "Query failed: " + std::string(e.what()));
        return {};
    }
}

std::vector<std::string> NonoStore::all_subjects() {
    try {
        LmdbStore::Transaction txn(*_store, true);
        std::string prefix = NonostoreKeys::get_vocabulary_prefix(NonostoreKeys::IndexType::SUBJECTS);
        auto raw_results = _store->query_prefix(txn, prefix);
        
        std::vector<std::string> subjects;
        for (const auto& [key, value] : raw_results) {
            auto parsed = NonostoreKeys::parse_key(key);
            if (parsed.is_vocabulary) {
                subjects.push_back(parsed.term);
            }
        }
        return subjects;
        
    } catch (const LmdbException& e) {
        set_error(ErrorCode::DatabaseError, "Failed to get subjects: " + std::string(e.what()));
        return {};
    }
}

std::vector<std::string> NonoStore::all_predicates() {
    try {
        LmdbStore::Transaction txn(*_store, true);
        std::string prefix = NonostoreKeys::get_vocabulary_prefix(NonostoreKeys::IndexType::PREDICATES);
        auto raw_results = _store->query_prefix(txn, prefix);
        
        std::vector<std::string> predicates;
        for (const auto& [key, value] : raw_results) {
            auto parsed = NonostoreKeys::parse_key(key);
            if (parsed.is_vocabulary) {
                predicates.push_back(parsed.term);
            }
        }
        return predicates;
        
    } catch (const LmdbException& e) {
        set_error(ErrorCode::DatabaseError, "Failed to get predicates: " + std::string(e.what()));
        return {};
    }
}

std::vector<std::string> NonoStore::all_objects() {
    try {
        LmdbStore::Transaction txn(*_store, true);
        std::string prefix = NonostoreKeys::get_vocabulary_prefix(NonostoreKeys::IndexType::OBJECTS);
        auto raw_results = _store->query_prefix(txn, prefix);
        
        std::vector<std::string> objects;
        for (const auto& [key, value] : raw_results) {
            auto parsed = NonostoreKeys::parse_key(key);
            if (parsed.is_vocabulary) {
                objects.push_back(parsed.term);
            }
        }
        return objects;
        
    } catch (const LmdbException& e) {
        set_error(ErrorCode::DatabaseError, "Failed to get objects: " + std::string(e.what()));
        return {};
    }
}

NonoStore::Stats NonoStore::get_stats() {
    Stats stats = {};
    
    try {
        LmdbStore::Transaction txn(*_store, true);
        
        // Get LMDB stats
        stats.lmdb_stats = _store->get_stats(txn);
        
        // Count vocabulary items for unique counts
        stats.unique_subjects = all_subjects().size();
        stats.unique_predicates = all_predicates().size();
        stats.unique_objects = all_objects().size();
        
        // Estimate total triples by counting SPO index entries
        auto spo_results = _store->query_prefix(txn, "~spo~");
        stats.total_triples = spo_results.size();
        
    } catch (const LmdbException& e) {
        set_error(ErrorCode::DatabaseError, "Failed to get stats: " + std::string(e.what()));
    }
    
    return stats;
}

bool NonoStore::exists(const std::string& subject, 
                       const std::string& predicate, 
                       const std::string& object) {
    try {
        LmdbStore::Transaction txn(*_store, true);
        std::string key = NonostoreKeys::generate_key(
            NonostoreKeys::IndexType::SPO, subject, predicate, object);
        return _store->exists(txn, key);
        
    } catch (const LmdbException& e) {
        set_error(ErrorCode::DatabaseError, "Exists check failed: " + std::string(e.what()));
        return false;
    }
}

size_t NonoStore::count(const std::string& subject_pattern,
                        const std::string& predicate_pattern,
                        const std::string& object_pattern) {
    try {
        LmdbStore::Transaction txn(*_store, true);
        std::string query_prefix = NonostoreKeys::generate_query_prefix(
            subject_pattern, predicate_pattern, object_pattern);
        auto raw_results = _store->query_prefix(txn, query_prefix);
        return raw_results.size();
        
    } catch (const LmdbException& e) {
        set_error(ErrorCode::DatabaseError, "Count failed: " + std::string(e.what()));
        return 0;
    }
}

// BatchTransaction implementation
NonoStore::BatchTransaction::BatchTransaction(NonoStore& store)
    : _store(store), _active(true) {
    _txn = std::make_unique<LmdbStore::Transaction>(*store._store);
}

NonoStore::BatchTransaction::~BatchTransaction() {
    if (_active) {
        abort();
    }
}

void NonoStore::BatchTransaction::connect(const std::string& subject, 
                                          const std::string& predicate, 
                                          const std::string& object) {
    if (!_active) return;
    
    auto keys = NonostoreKeys::generate_all_keys(subject, predicate, object);
    _connect_keys.push_back(keys);
}

void NonoStore::BatchTransaction::disconnect(const std::string& subject, 
                                             const std::string& predicate, 
                                             const std::string& object) {
    if (!_active) return;
    
    auto keys = NonostoreKeys::generate_all_keys(subject, predicate, object);
    _disconnect_keys.push_back(keys);
}

bool NonoStore::BatchTransaction::commit() {
    if (!_active) return false;
    
    try {
        // Execute all disconnects first
        for (const auto& keys : _disconnect_keys) {
            for (const auto& key : keys) {
                _store._store->del(*_txn, key);
            }
        }
        
        // Then execute all connects
        for (const auto& keys : _connect_keys) {
            for (const auto& key : keys) {
                _store._store->put(*_txn, key, "{}"); // Empty JSON value
            }
        }
        
        _txn->commit();
        _active = false;
        return true;
        
    } catch (const LmdbException& e) {
        _store.set_error(ErrorCode::TransactionError, "Batch commit failed: " + std::string(e.what()));
        return false;
    }
}

void NonoStore::BatchTransaction::abort() {
    if (_active) {
        _txn->abort();
        _active = false;
    }
}

std::unique_ptr<NonoStore::BatchTransaction> NonoStore::begin_batch() {
    return std::make_unique<BatchTransaction>(*this);
}

// Private implementation methods
bool NonoStore::set_error(ErrorCode code, const std::string& message) {
    _last_error = {code, message};
    return false;
}

std::vector<NonoStore::Triple> NonoStore::parse_query_results(
    const std::vector<std::pair<std::string, std::string>>& raw_results,
    NonostoreKeys::IndexType index_type) {
    
    std::vector<Triple> triples;
    triples.reserve(raw_results.size());
    
    for (const auto& [key, value] : raw_results) {
        auto parsed = NonostoreKeys::parse_key(key);
        if (!parsed.is_vocabulary) {
            triples.emplace_back(parsed.subject, parsed.predicate, parsed.object);
        }
    }
    
    return triples;
}

bool NonoStore::connect_impl(LmdbStore::Transaction& txn,
                             const std::string& subject,
                             const std::string& predicate,
                             const std::string& object) {
    try {
        // Phase 2.3: TID-based architecture implementation
        // 1. Store triple in central TripleStore to get TID
        MDB_txn* mdb_txn = txn.handle();
        TripleStore::TID tid = _triple_store->store_triple(mdb_txn, subject, predicate, object);
        
        // 2. Get TermIDs for crown indices
        TermDictionary::TermID subject_id = _term_dict->intern(mdb_txn, subject);
        TermDictionary::TermID predicate_id = _term_dict->intern(mdb_txn, predicate);
        TermDictionary::TermID object_id = _term_dict->intern(mdb_txn, object);
        
        // 3. Generate crown index keys with TermIDs (not strings!)
        std::string tid_value = encode_tid_for_storage(tid);
        
        // SPO/SOP/PSO/POS/OSP/OPS indices store TermID triplets → TID
        auto crown_keys = generate_tid_based_crown_keys(subject_id, predicate_id, object_id);
        for (const auto& key : crown_keys) {
            bool success = _store->put(txn, key, tid_value);
            if (!success) {
                set_error(ErrorCode::DatabaseError, "Failed to insert crown key: " + key);
                return false;
            }
        }
        
        // 4. Vocabulary indices store TermID → TID (for discovery)
        std::string subject_vocab_key = generate_vocabulary_key_for_term_id(NonostoreKeys::IndexType::SUBJECTS, subject_id);
        std::string predicate_vocab_key = generate_vocabulary_key_for_term_id(NonostoreKeys::IndexType::PREDICATES, predicate_id);
        std::string object_vocab_key = generate_vocabulary_key_for_term_id(NonostoreKeys::IndexType::OBJECTS, object_id);
        
        _store->put(txn, subject_vocab_key, tid_value);
        _store->put(txn, predicate_vocab_key, tid_value);
        _store->put(txn, object_vocab_key, tid_value);
        
        _last_error = {ErrorCode::Success, ""};
        return true;
        
    } catch (const std::exception& e) {
        return set_error(ErrorCode::KeyGenerationError, "TID-based connect failed: " + std::string(e.what()));
    }
}

bool NonoStore::disconnect_impl(LmdbStore::Transaction& txn,
                                const std::string& subject,
                                const std::string& predicate,
                                const std::string& object) {
    try {
        // Phase 2.3: TID-based architecture disconnect
        // 1. Find the existing triple in TripleStore to get TID
        MDB_txn* mdb_txn = txn.handle();
        
        // Get TermIDs
        auto subject_id_opt = _term_dict->lookup(mdb_txn, subject);
        auto predicate_id_opt = _term_dict->lookup(mdb_txn, predicate);
        auto object_id_opt = _term_dict->lookup(mdb_txn, object);
        
        // If any term doesn't exist, the triple can't exist
        if (!subject_id_opt || !predicate_id_opt || !object_id_opt) {
            _last_error = {ErrorCode::Success, ""}; // Not an error - triple doesn't exist
            return true;
        }
        
        TermDictionary::TermID subject_id = *subject_id_opt;
        TermDictionary::TermID predicate_id = *predicate_id_opt;
        TermDictionary::TermID object_id = *object_id_opt;
        
        // 2. Find TIDs for this triple pattern from crown indices
        std::string query_key = "~spo~" + 
                               TermDictionary::encode_term_id_for_storage(subject_id) +
                               TermDictionary::encode_term_id_for_storage(predicate_id) +
                               TermDictionary::encode_term_id_for_storage(object_id);
        
        // Query SPO index to find the TID
        auto query_results = _store->query_prefix(txn, query_key);
        if (query_results.empty()) {
            _last_error = {ErrorCode::Success, ""}; // Triple doesn't exist
            return true;
        }
        
        // 3. Remove from TripleStore and crown indices
        for (const auto& [key, tid_value] : query_results) {
            uint64_t tid = decode_tid_from_storage(tid_value);
            
            // Remove from central TripleStore
            _triple_store->remove_triple(mdb_txn, tid);
            
            // Remove from all crown indices
            auto crown_keys = generate_tid_based_crown_keys(subject_id, predicate_id, object_id);
            for (const auto& crown_key : crown_keys) {
                _store->del(txn, crown_key);
            }
            
            // Remove from vocabulary indices (only if this was the last reference)
            // Note: For now, we'll leave vocabulary entries - they can be cleaned up separately
            // This avoids the complexity of reference counting in this initial implementation
        }
        
        _last_error = {ErrorCode::Success, ""};
        return true;
        
    } catch (const std::exception& e) {
        return set_error(ErrorCode::KeyGenerationError, "TID-based disconnect failed: " + std::string(e.what()));
    }
}

// TID-based architecture helper methods

std::string NonoStore::encode_tid_for_storage(uint64_t tid) {
    // Use same encoding as TIDSequenceGenerator for consistency
    return TIDSequenceGenerator::encode_tid_for_storage(tid);
}

uint64_t NonoStore::decode_tid_from_storage(const std::string& stored) {
    // Use same decoding as TIDSequenceGenerator for consistency
    return TIDSequenceGenerator::decode_tid_from_storage(stored);
}

std::vector<std::string> NonoStore::generate_tid_based_crown_keys(uint64_t subject_id, uint64_t predicate_id, uint64_t object_id) {
    // Generate the six crown index keys using TermIDs instead of strings
    // These store compact binary keys: TermID₁|TermID₂|TermID₃ → TID
    
    std::vector<std::string> keys;
    keys.reserve(6);
    
    // Convert TermIDs to fixed-width binary strings for optimal LMDB prefix compression
    std::string s_bin = TermDictionary::encode_term_id_for_storage(subject_id);
    std::string p_bin = TermDictionary::encode_term_id_for_storage(predicate_id);
    std::string o_bin = TermDictionary::encode_term_id_for_storage(object_id);
    
    // Six crown orderings - these are now compact 24-byte keys (3 × 8 bytes)
    keys.push_back("~spo~" + s_bin + p_bin + o_bin);  // SPO
    keys.push_back("~sop~" + s_bin + o_bin + p_bin);  // SOP
    keys.push_back("~pso~" + p_bin + s_bin + o_bin);  // PSO
    keys.push_back("~pos~" + p_bin + o_bin + s_bin);  // POS
    keys.push_back("~osp~" + o_bin + s_bin + p_bin);  // OSP
    keys.push_back("~ops~" + o_bin + p_bin + s_bin);  // OPS
    
    return keys;
}

std::string NonoStore::generate_vocabulary_key_for_term_id(NonostoreKeys::IndexType vocab_type, uint64_t term_id) {
    // Generate vocabulary index keys: TermID → TID (for discovery)
    std::string prefix;
    switch (vocab_type) {
        case NonostoreKeys::IndexType::SUBJECTS:
            prefix = "~subjects~";
            break;
        case NonostoreKeys::IndexType::PREDICATES:
            prefix = "~predicates~";
            break;
        case NonostoreKeys::IndexType::OBJECTS:
            prefix = "~objects~";
            break;
        default:
            throw std::invalid_argument("Invalid vocabulary index type");
    }
    
    return prefix + TermDictionary::encode_term_id_for_storage(term_id);
}

} // namespace LabDb
