#include "LabDb/NonoStore.h"
#include <algorithm>
#include <set>

namespace LabDb {

NonoStore::NonoStore(const std::string& database_path, size_t map_size)
    : _last_error{ErrorCode::Success, ""} {
    try {
        _store = std::make_unique<LmdbStore>(database_path, map_size);
    } catch (const LmdbException& e) {
        set_error(ErrorCode::DatabaseError, "Failed to open database: " + std::string(e.what()));
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
        // Generate all nine keys for this triple
        auto keys = NonostoreKeys::generate_all_keys(subject, predicate, object);
        
        // Insert into all nine indices atomically
        for (const auto& key : keys) {
            bool success = _store->put(txn, key, "{}"); // Empty JSON value
            if (!success) {
                set_error(ErrorCode::DatabaseError, "Failed to insert key: " + key);
                return false;
            }
        }
        
        _last_error = {ErrorCode::Success, ""};
        return true;
        
    } catch (const std::exception& e) {
        return set_error(ErrorCode::KeyGenerationError, "Key generation failed: " + std::string(e.what()));
    }
}

bool NonoStore::disconnect_impl(LmdbStore::Transaction& txn,
                                const std::string& subject,
                                const std::string& predicate,
                                const std::string& object) {
    try {
        // Generate all nine keys for this triple
        auto keys = NonostoreKeys::generate_all_keys(subject, predicate, object);
        
        // Remove from all nine indices atomically
        for (const auto& key : keys) {
            bool success = _store->del(txn, key);
            // Note: del() might return false if key doesn't exist, which is OK
        }
        
        _last_error = {ErrorCode::Success, ""};
        return true;
        
    } catch (const std::exception& e) {
        return set_error(ErrorCode::KeyGenerationError, "Key generation failed: " + std::string(e.what()));
    }
}

} // namespace LabDb
