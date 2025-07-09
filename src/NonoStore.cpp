#include "LabDb/NonoStore.h"
#include "LabDb/TermDictionary.h"
#include "LabDb/TIDSequenceGenerator.h"
#include "LabDb/TriadicQuery.h"
#include <algorithm>
#include <set>
#include <iostream>
#include <cstring>

namespace LabDb {

NonoStore::NonoStore(const std::string& database_path, size_t map_size)
    : _last_error{ErrorCode::Success, ""} {
    try {
        _store = std::make_unique<LmdbStore>(database_path, map_size);
        
        // Initialize TID-based architecture components
        MDB_env* env = _store->environment();
        _term_dict = std::make_unique<TermDictionary>(env);
        _tid_gen = std::make_unique<TIDSequenceGenerator>(env);
        init_triple_store_dbis();
        
    } catch (const LmdbException& e) {
        set_error(ErrorCode::DatabaseError, "Failed to open database: " + std::string(e.what()));
        throw;
    } catch (const TermDictionaryException& e) {
        set_error(ErrorCode::DatabaseError, "Failed to initialize TermDictionary: " + std::string(e.what()));
        throw;
    } catch (const TIDSequenceException& e) {
        set_error(ErrorCode::DatabaseError, "Failed to initialize TIDSequenceGenerator: " + std::string(e.what()));
        throw;
    }
}

NonoStore::~NonoStore() = default;

bool NonoStore::add_triple(const std::string& subject, 
                           const std::string& predicate, 
                           const std::string& object) {
    try {
        LmdbStore::Transaction txn(*_store);

        // 1. Store triple in central TripleStore to get TID
        MDB_txn* mdb_txn = txn.handle();
        TID tid = store_triple_internal(mdb_txn, subject, predicate, object);
        
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
        
        // Sanity check: ensure vocabulary keys are not empty
        if (subject_vocab_key.empty()) {
            std::cerr << "ERROR: subject_vocab_key is empty!" << std::endl;
            return set_error(ErrorCode::KeyGenerationError, "Generated empty subject vocabulary key");
        }
        if (predicate_vocab_key.empty()) {
            std::cerr << "ERROR: predicate_vocab_key is empty!" << std::endl;
            return set_error(ErrorCode::KeyGenerationError, "Generated empty predicate vocabulary key");
        }
        if (object_vocab_key.empty()) {
            std::cerr << "ERROR: object_vocab_key is empty!" << std::endl;
            return set_error(ErrorCode::KeyGenerationError, "Generated empty object vocabulary key");
        }
        
        // Store vocabulary keys 
        bool subject_put_success = _store->put(txn, subject_vocab_key, tid_value);
        bool predicate_put_success = _store->put(txn, predicate_vocab_key, tid_value);
        bool object_put_success = _store->put(txn, object_vocab_key, tid_value);
        
        _last_error = {ErrorCode::Success, ""};

        txn.commit();
        return true;
    } catch (const LmdbException& e) {
        return set_error(ErrorCode::TransactionError, "Add triple failed: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return set_error(ErrorCode::KeyGenerationError, "TID-based connect failed: " + std::string(e.what()));
    }
}

bool NonoStore::remove_triple(const std::string& subject, 
                              const std::string& predicate, 
                              const std::string& object) {
    try {
        LmdbStore::Transaction txn(*_store);

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
            remove_triple_internal(mdb_txn, tid);
            
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
        
        txn.commit();
        return true;
    } catch (const LmdbException& e) {
        return set_error(ErrorCode::TransactionError, "Remove triple failed: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return set_error(ErrorCode::KeyGenerationError, "TID-based disconnect failed: " + std::string(e.what()));
    }
}

std::vector<NonoStore::Triple> NonoStore::query(const std::string& subject_pattern,
                                                 const std::string& predicate_pattern,
                                                 const std::string& object_pattern) {
    try {
        LmdbStore::Transaction txn(*_store, true); // read-only
        MDB_txn* mdb_txn = txn.handle();
        
        // TID-based query: convert patterns to TermIDs first
        bool subj_wild = (subject_pattern == "*");
        bool pred_wild = (predicate_pattern == "*");
        bool obj_wild = (object_pattern == "*");
        
        // For specific terms, look up TermIDs (early exit if not found)
        std::optional<uint64_t> subject_id, predicate_id, object_id;
        
        if (!subj_wild) {
            auto sid = _term_dict->lookup(mdb_txn, subject_pattern);
            if (!sid.has_value()) {
                // Subject doesn't exist - early exit with empty results
                return {};
            }
            subject_id = *sid;
        }
        
        if (!pred_wild) {
            auto pid = _term_dict->lookup(mdb_txn, predicate_pattern);
            if (!pid.has_value()) {
                // Predicate doesn't exist - early exit with empty results
                return {};
            }
            predicate_id = *pid;
        }
        
        if (!obj_wild) {
            auto oid = _term_dict->lookup(mdb_txn, object_pattern);
            if (!oid.has_value()) {
                // Object doesn't exist - early exit with empty results
                return {};
            }
            object_id = *oid;
        }
        
        // Generate TID-based query prefix
        std::string query_prefix = generate_tid_based_query_prefix(
            subject_id, predicate_id, object_id);
        
        // Execute prefix query
        auto raw_results = _store->query_prefix(txn, query_prefix);
        
        // Parse results using TID resolution
        return parse_query_results_tid(raw_results, mdb_txn);
        
    } catch (const LmdbException& e) {
        set_error(ErrorCode::DatabaseError, "TID-based query failed: " + std::string(e.what()));
        return {};
    }
}

std::vector<std::string> NonoStore::all_subjects() {
    try {
        LmdbStore::Transaction txn(*_store, true);
        std::string prefix = NonostoreKeys::get_vocabulary_prefix(NonostoreKeys::IndexType::SUBJECTS);
        auto raw_results = _store->query_prefix(txn, prefix);
        
        std::vector<std::string> subjects;
        MDB_txn* mdb_txn = txn.handle();
        
        for (const auto& [key, value] : raw_results) {
            // Extract TermID from vocabulary key format: ~subjects~0000000000000001
            if (key.length() > prefix.length()) {
                std::string term_id_hex = key.substr(prefix.length());
                
                // Convert hex string to TermID
                try {
                    TermDictionary::TermID term_id = std::stoull(term_id_hex, nullptr, 16);
                    
                    // Use TermDictionary to resolve TermID back to string
                    auto resolved_term = _term_dict->resolve(mdb_txn, term_id);
                    if (resolved_term.has_value()) {
                        subjects.push_back(*resolved_term);
                    }
                } catch (const std::exception& e) {
                    // Skip malformed keys
                    std::cerr << "Warning: Failed to parse vocabulary key: " << key << std::endl;
                }
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
        MDB_txn* mdb_txn = txn.handle();
        
        for (const auto& [key, value] : raw_results) {
            // Extract TermID from vocabulary key format: ~predicates~0000000000000002
            if (key.length() > prefix.length()) {
                std::string term_id_hex = key.substr(prefix.length());
                
                // Convert hex string to TermID
                try {
                    TermDictionary::TermID term_id = std::stoull(term_id_hex, nullptr, 16);
                    
                    // Use TermDictionary to resolve TermID back to string
                    auto resolved_term = _term_dict->resolve(mdb_txn, term_id);
                    if (resolved_term.has_value()) {
                        predicates.push_back(*resolved_term);
                    }
                } catch (const std::exception& e) {
                    // Skip malformed keys
                    std::cerr << "Warning: Failed to parse vocabulary key: " << key << std::endl;
                }
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
        MDB_txn* mdb_txn = txn.handle();
        
        for (const auto& [key, value] : raw_results) {
            // Extract TermID from vocabulary key format: ~objects~0000000000000003
            if (key.length() > prefix.length()) {
                std::string term_id_hex = key.substr(prefix.length());
                
                // Convert hex string to TermID
                try {
                    TermDictionary::TermID term_id = std::stoull(term_id_hex, nullptr, 16);
                    
                    // Use TermDictionary to resolve TermID back to string
                    auto resolved_term = _term_dict->resolve(mdb_txn, term_id);
                    if (resolved_term.has_value()) {
                        objects.push_back(*resolved_term);
                    }
                } catch (const std::exception& e) {
                    // Skip malformed keys
                    std::cerr << "Warning: Failed to parse vocabulary key: " << key << std::endl;
                }
            }
        }
        return objects;
        
    } catch (const LmdbException& e) {
        set_error(ErrorCode::DatabaseError, "Failed to get objects: " + std::string(e.what()));
        return {};
    }
}

std::unique_ptr<TriadicQuery> NonoStore::create_triadic_query() {
    // Factory pattern implementation using weak_ptr to avoid pybind11 holder type issues
    // This enables proper lifecycle management and prevents circular references
    return std::make_unique<TriadicQuery>(std::weak_ptr<NonoStore>(shared_from_this()));
}

NonoStore::Stats NonoStore::get_stats() {
    Stats stats = {};
    
    try {
        LmdbStore::Transaction txn(*_store, true);
        
        // Get LMDB stats
        stats.lmdb_stats = _store->get_stats(txn);
        
        // Count vocabulary items for unique counts - use the SAME transaction!
        // Note: We need to call vocabulary methods with our transaction instead of
        // letting them create their own, to avoid potential LMDB nested transaction issues
        
        // For now, call the vocabulary methods directly with our transaction
        std::vector<std::string> subjects_list;
        std::vector<std::string> predicates_list; 
        std::vector<std::string> objects_list;
        
        // Inline the all_subjects() logic to use our transaction
        {
            std::string prefix = NonostoreKeys::get_vocabulary_prefix(NonostoreKeys::IndexType::SUBJECTS);
            auto raw_results = _store->query_prefix(txn, prefix);
            
            MDB_txn* mdb_txn = txn.handle();
            
            for (const auto& [key, value] : raw_results) {
                if (key.length() > prefix.length()) {
                    std::string term_id_hex = key.substr(prefix.length());
                    
                    try {
                        TermDictionary::TermID term_id = std::stoull(term_id_hex, nullptr, 16);
                        auto resolved_term = _term_dict->resolve(mdb_txn, term_id);
                        if (resolved_term.has_value()) {
                            subjects_list.push_back(*resolved_term);
                        }
                    } catch (const std::exception&) {
                        // Skip invalid term IDs
                    }
                }
            }
        }
        
        // Inline the all_predicates() logic
        {
            std::string prefix = NonostoreKeys::get_vocabulary_prefix(NonostoreKeys::IndexType::PREDICATES);
            auto raw_results = _store->query_prefix(txn, prefix);
            
            MDB_txn* mdb_txn = txn.handle();
            
            for (const auto& [key, value] : raw_results) {
                if (key.length() > prefix.length()) {
                    std::string term_id_hex = key.substr(prefix.length());
                    
                    try {
                        TermDictionary::TermID term_id = std::stoull(term_id_hex, nullptr, 16);
                        auto resolved_term = _term_dict->resolve(mdb_txn, term_id);
                        if (resolved_term.has_value()) {
                            predicates_list.push_back(*resolved_term);
                        }
                    } catch (const std::exception&) {
                        // Skip invalid term IDs
                    }
                }
            }
        }
        
        // Inline the all_objects() logic
        {
            std::string prefix = NonostoreKeys::get_vocabulary_prefix(NonostoreKeys::IndexType::OBJECTS);
            auto raw_results = _store->query_prefix(txn, prefix);
            
            MDB_txn* mdb_txn = txn.handle();
            
            for (const auto& [key, value] : raw_results) {
                if (key.length() > prefix.length()) {
                    std::string term_id_hex = key.substr(prefix.length());
                    
                    try {
                        TermDictionary::TermID term_id = std::stoull(term_id_hex, nullptr, 16);
                        auto resolved_term = _term_dict->resolve(mdb_txn, term_id);
                        if (resolved_term.has_value()) {
                            objects_list.push_back(*resolved_term);
                        }
                    } catch (const std::exception&) {
                        // Skip invalid term IDs
                    }
                }
            }
        }
        
        stats.unique_subjects = subjects_list.size();
        stats.unique_predicates = predicates_list.size();
        stats.unique_objects = objects_list.size();
        
        // Estimate total triples by counting SPO index entries
        auto spo_results = _store->query_prefix(txn, "~spo~");
        stats.total_triples = spo_results.size();
        
        // TID architecture statistics - simplified for now
        // TODO: Implement proper TID architecture metrics when components support them
        stats.term_dictionary_size = 0;  // _term_dict->size() when available
        stats.subject_vocabulary_size = stats.unique_subjects;    // Fallback to vocabulary counts
        stats.predicate_vocabulary_size = stats.unique_predicates;
        stats.object_vocabulary_size = stats.unique_objects;
        
        // Storage architecture statistics
        // Count different index types
        stats.hexastore_indices_size = _store->query_prefix(txn, "~spo~").size() +
                                       _store->query_prefix(txn, "~sop~").size() +
                                       _store->query_prefix(txn, "~pso~").size() +
                                       _store->query_prefix(txn, "~pos~").size() +
                                       _store->query_prefix(txn, "~ops~").size() +
                                       _store->query_prefix(txn, "~osp~").size();
        
        // Crown indices (if implemented) - for now set to 0
        stats.crown_indices_size = 0;
        
    } catch (const LmdbException& e) {
        set_error(ErrorCode::DatabaseError, "Failed to get stats: " + std::string(e.what()));
    }
    
    return stats;
}

NonoStore::TIDMetrics NonoStore::get_tid_metrics() {
    TIDMetrics metrics = {};
    
    try {
        LmdbStore::Transaction txn(*_store, true);
        
        // Simplified TID metrics - using available data
        // TODO: Implement proper TID architecture metrics when components support them
        
        if (_term_dict && _tid_gen) {
            // For now, use fallback values based on vocabulary counts
            metrics.term_dict_entries = all_subjects().size() + all_predicates().size() + all_objects().size();
            
            // Placeholder TID ranges - would need proper TID tracking
            metrics.subject_tid_range = all_subjects().size();
            metrics.predicate_tid_range = all_predicates().size();
            metrics.object_tid_range = all_objects().size();
            
            metrics.total_tids_allocated = metrics.subject_tid_range + 
                                          metrics.predicate_tid_range + 
                                          metrics.object_tid_range;
            
            // Calculate storage efficiency
            // Efficiency = logical triples / total storage entries
            size_t logical_triples = _store->query_prefix(txn, "~spo~").size();
            size_t total_storage_entries = _store->get_stats(txn).entries;
            
            if (total_storage_entries > 0) {
                metrics.storage_efficiency = static_cast<double>(logical_triples) / total_storage_entries;
            } else {
                metrics.storage_efficiency = 0.0;
            }
        } else {
            // TID components not available
            metrics.term_dict_entries = 0;
            metrics.subject_tid_range = 0;
            metrics.predicate_tid_range = 0;
            metrics.object_tid_range = 0;
            metrics.total_tids_allocated = 0;
            metrics.storage_efficiency = 0.0;
        }
        
    } catch (const LmdbException& e) {
        set_error(ErrorCode::DatabaseError, "Failed to get TID metrics: " + std::string(e.what()));
        // Return zero metrics on error
        metrics = {};
    }
    
    return metrics;
}

bool NonoStore::exists(const std::string& subject, 
                       const std::string& predicate, 
                       const std::string& object) {
    try {
        LmdbStore::Transaction txn(*_store, true);
        MDB_txn* mdb_txn = txn.handle();

        TermDictionary::TermID subject_id = _term_dict->intern(mdb_txn, subject);
        TermDictionary::TermID predicate_id = _term_dict->intern(mdb_txn, predicate);
        TermDictionary::TermID object_id = _term_dict->intern(mdb_txn, object);
        
        std::string key = generate_tid_based_crown_key(NonostoreKeys::IndexType::SPO, subject_id, predicate_id, object_id);
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
        LmdbStore::Transaction txn(*_store, true); // read-only
        MDB_txn* mdb_txn = txn.handle();
        
        // TID-based query: convert patterns to TermIDs first (same logic as query())
        bool subj_wild = (subject_pattern == "*");
        bool pred_wild = (predicate_pattern == "*");
        bool obj_wild = (object_pattern == "*");
        
        // For specific terms, look up TermIDs (early exit if not found)
        std::optional<uint64_t> subject_id, predicate_id, object_id;
        
        if (!subj_wild) {
            auto sid = _term_dict->lookup(mdb_txn, subject_pattern);
            if (!sid.has_value()) {
                // Subject doesn't exist - early exit with zero count
                return 0;
            }
            subject_id = *sid;
        }
        
        if (!pred_wild) {
            auto pid = _term_dict->lookup(mdb_txn, predicate_pattern);
            if (!pid.has_value()) {
                // Predicate doesn't exist - early exit with zero count
                return 0;
            }
            predicate_id = *pid;
        }
        
        if (!obj_wild) {
            auto oid = _term_dict->lookup(mdb_txn, object_pattern);
            if (!oid.has_value()) {
                // Object doesn't exist - early exit with zero count
                return 0;
            }
            object_id = *oid;
        }
        
        // Generate TID-based query prefix (same as query())
        std::string query_prefix = generate_tid_based_query_prefix(
            subject_id, predicate_id, object_id);
        
        // Execute prefix query and return count
        auto raw_results = _store->query_prefix(txn, query_prefix);
        return raw_results.size();
        
    } catch (const LmdbException& e) {
        set_error(ErrorCode::DatabaseError, "TID-based count failed: " + std::string(e.what()));
        return 0;
    }
}

// SynchronousTransaction implementation
NonoStore::SynchronousTransaction::SynchronousTransaction(NonoStore& store)
    : _store(store) {
}

NonoStore::SynchronousTransaction::~SynchronousTransaction() {
}

EntityId NonoStore::SynchronousTransaction::intern_eid(const std::string& term) {
    try {
        auto txn = LmdbStore::Transaction(_store.getLmdbStore());
        auto &term_dict = _store.getTermDictionary();
        auto tid = term_dict.intern(txn.handle(), term);
        auto eid = EntityId::generateEidString(tid);
        txn.commit();
        return EntityId(term, eid, tid, true);
    } catch (const std::exception&) {
        return EntityId(term, "", INVALID_TID, false);
    }
}

EntityId NonoStore::SynchronousTransaction::resolve_eid(const std::string& eid) {
    try {
        auto tid = EntityId::parseEidNumeric(eid);
        if (tid == INVALID_TID) {
            return EntityId("", eid, INVALID_TID, false);
        }
        
        auto txn = LmdbStore::Transaction(_store.getLmdbStore());
        auto &term_dict = _store.getTermDictionary();
        auto name_opt = term_dict.resolve(txn.handle(), tid);
        bool exists = name_opt.has_value();
        std::string name = exists ? name_opt.value() : "";
        txn.commit();
        return EntityId(name, eid, tid, exists);
    } catch (const std::exception&) {
        return EntityId("", eid, INVALID_TID, false);
    }
}

EntityId NonoStore::SynchronousTransaction::resolve_tid(const TID& tid) {
    try {
        auto txn = LmdbStore::Transaction(_store.getLmdbStore());
        auto &term_dict = _store.getTermDictionary();
        auto name_opt = term_dict.resolve(txn.handle(), tid);
        auto eid = EntityId::generateEidString(tid);
        bool exists = name_opt.has_value();
        std::string name = exists ? name_opt.value() : "";
        txn.commit();
        return EntityId(name, eid, tid, exists);
    } catch (const std::exception&) {
        return EntityId("", "", tid, false);
    }
}

std::unique_ptr<NonoStore::SynchronousTransaction> NonoStore::begin_sync() {
    return std::make_unique<SynchronousTransaction>(*this);
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

void NonoStore::BatchTransaction::add_triple(
                                   const std::string& subject,
                                   const std::string& predicate, 
                                   const std::string& object) {
    if (!_active) return;
    
    auto keys = NonostoreKeys::generate_all_keys(subject, predicate, object);
    _connect_keys.push_back(keys);
}

void NonoStore::BatchTransaction::remove_triple(
                                    const std::string& subject, 
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
                _store._store->del(*_txn, key.key);
            }
        }
        
        // Then execute all connects
        for (const auto& keys : _connect_keys) {
            for (const auto& key : keys) {
                _store._store->put(*_txn, key.key, "{}"); // Empty JSON value
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
        auto parsed = NonostoreKeys::parse_key({key});
        if (!parsed.is_vocabulary) {
            triples.emplace_back(parsed.subject, parsed.predicate, parsed.object);
        }
    }
    
    return triples;
}

/// TID-based query result parsing
std::vector<NonoStore::Triple> NonoStore::parse_query_results_tid(
    const std::vector<std::pair<std::string, std::string>>& raw_results,
    MDB_txn* mdb_txn) {
    
    std::vector<Triple> triples;
    triples.reserve(raw_results.size());
    
    for (const auto& [key, value] : raw_results) {
        // TID-based crown indices never contain vocabulary entries by design
        // Skip the incompatible NonostoreKeys::parse_key() vocabulary check
        // which expects string-based keys but we have binary TID-based keys
        
        // Extract TID from value (16-character hex string)
        if (value.length() == 16) {
            uint64_t tid = decode_tid_from_storage(value);
            
            // Resolve TID to triple strings using TripleStore
            auto resolved = get_triple_as_strings_internal(mdb_txn, tid);
            if (resolved.has_value()) {
                triples.emplace_back(
                    resolved->subject,
                    resolved->predicate,
                    resolved->object
                );
            }
        }
    }
    
    return triples;
}

// TID-based architecture helper methods

std::string NonoStore::generate_tid_based_query_prefix(
    std::optional<uint64_t> subject_id,
    std::optional<uint64_t> predicate_id,
    std::optional<uint64_t> object_id) {
    
    // Choose optimal index based on which terms are specified
    std::string prefix;
    
    if (subject_id.has_value() && predicate_id.has_value() && object_id.has_value()) {
        // Specific triple: s-p-o -> use SPO index
        prefix = "~spo~" + 
                TermDictionary::encode_term_id_for_storage(*subject_id) +
                TermDictionary::encode_term_id_for_storage(*predicate_id) +
                TermDictionary::encode_term_id_for_storage(*object_id);
    }
    else if (subject_id.has_value() && predicate_id.has_value()) {
        // s-p-* pattern: use SPO index
        prefix = "~spo~" +
                TermDictionary::encode_term_id_for_storage(*subject_id) +
                TermDictionary::encode_term_id_for_storage(*predicate_id);
    }
    else if (subject_id.has_value() && object_id.has_value()) {
        // s-*-o pattern: use SOP index  
        prefix = "~sop~" +
                TermDictionary::encode_term_id_for_storage(*subject_id) +
                TermDictionary::encode_term_id_for_storage(*object_id);
    }
    else if (predicate_id.has_value() && object_id.has_value()) {
        // *-p-o pattern: use POS index
        prefix = "~pos~" +
                TermDictionary::encode_term_id_for_storage(*predicate_id) +
                TermDictionary::encode_term_id_for_storage(*object_id);
    }
    else if (subject_id.has_value()) {
        // s-*-* pattern: use SPO index
        prefix = "~spo~" +
                TermDictionary::encode_term_id_for_storage(*subject_id);
    }
    else if (predicate_id.has_value()) {
        // *-p-* pattern: use PSO index
        prefix = "~pso~" +
                TermDictionary::encode_term_id_for_storage(*predicate_id);
    }
    else if (object_id.has_value()) {
        // *-*-o pattern: use OSP index
        prefix = "~osp~" +
                TermDictionary::encode_term_id_for_storage(*object_id);
    }
    else {
        // *-*-* pattern: use SPO index (could use any)
        prefix = "~spo~";
    }
    
    return prefix;
}

std::string NonoStore::encode_tid_for_storage(uint64_t tid) {
    // Use same encoding as TIDSequenceGenerator for consistency
    return TIDSequenceGenerator::encode_tid_for_storage(tid);
}

uint64_t NonoStore::decode_tid_from_storage(const std::string& stored) {
    // Use same decoding as TIDSequenceGenerator for consistency
    return TIDSequenceGenerator::decode_tid_from_storage(stored);
}

std::string NonoStore::generate_tid_based_crown_key(NonostoreKeys::IndexType type,
                                                    uint64_t subject_id,
                                                    uint64_t predicate_id,
                                                    uint64_t object_id) {
    std::string ret;
    switch (type) {
        case NonostoreKeys::IndexType::SPO: ret = "~spo~"; break;
        case NonostoreKeys::IndexType::SOP: ret = "~sop~"; break;
        case NonostoreKeys::IndexType::PSO: ret = "~pso~"; break;
        case NonostoreKeys::IndexType::POS: ret = "~pos~"; break;
        case NonostoreKeys::IndexType::OSP: ret = "~osp~"; break;
        case NonostoreKeys::IndexType::OPS: ret = "~ops~"; break;
        case NonostoreKeys::IndexType::SUBJECTS: ret = "~subjects~"; break;
        case NonostoreKeys::IndexType::PREDICATES: ret = "~predicates~"; break;
        case NonostoreKeys::IndexType::OBJECTS: ret = "~objects~"; break;
    }
    return ret + TermDictionary::encode_term_id_for_storage(subject_id) +
                     TermDictionary::encode_term_id_for_storage(predicate_id) +
                     TermDictionary::encode_term_id_for_storage(object_id);
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

//-------------------------------------------------------------------------
// TripleStore functionality implementations (consolidated from separate TripleStore class)
//-------------------------------------------------------------------------

void NonoStore::init_triple_store_dbis() {
    MDB_env* env = _store->environment();
    
    // Open/create the TripleStore DBIs
    int rc;
    MDB_txn* txn;
    
    rc = mdb_txn_begin(env, nullptr, 0, &txn);
    if (rc != 0) {
        throw std::runtime_error("Failed to begin transaction for DBI init: " + std::string(mdb_strerror(rc)));
    }
    
    // Main triple storage DBI
    rc = mdb_dbi_open(txn, "triple", MDB_CREATE, &_triple_dbi);
    if (rc != 0) {
        mdb_txn_abort(txn);
        throw std::runtime_error("Failed to open triple DBI: " + std::string(mdb_strerror(rc)));
    }
    
    // Subject index DBI  
    rc = mdb_dbi_open(txn, "subject_index", MDB_CREATE, &_subject_index_dbi);
    if (rc != 0) {
        mdb_txn_abort(txn);
        throw std::runtime_error("Failed to open subject_index DBI: " + std::string(mdb_strerror(rc)));
    }
    
    // Predicate index DBI
    rc = mdb_dbi_open(txn, "predicate_index", MDB_CREATE, &_predicate_index_dbi);
    if (rc != 0) {
        mdb_txn_abort(txn);
        throw std::runtime_error("Failed to open predicate_index DBI: " + std::string(mdb_strerror(rc)));
    }
    
    // Object index DBI
    rc = mdb_dbi_open(txn, "object_index", MDB_CREATE, &_object_index_dbi);
    if (rc != 0) {
        mdb_txn_abort(txn);
        throw std::runtime_error("Failed to open object_index DBI: " + std::string(mdb_strerror(rc)));
    }
    
    rc = mdb_txn_commit(txn);
    if (rc != 0) {
        throw std::runtime_error("Failed to commit DBI init transaction: " + std::string(mdb_strerror(rc)));
    }
}

NonoStore::TID NonoStore::store_triple_internal(MDB_txn* txn, 
                                                 const std::string& subject, 
                                                 const std::string& predicate, 
                                                 const std::string& object,
                                                 const std::string& source,
                                                 float confidence,
                                                 uint32_t flags) {
    // 1. Intern terms to get TermIDs
    TermID subject_id = _term_dict->intern(txn, subject);
    TermID predicate_id = _term_dict->intern(txn, predicate);
    TermID object_id = _term_dict->intern(txn, object);
    
    // 2. Call the TermID version
    return store_triple_internal(txn, subject_id, predicate_id, object_id, source, confidence, flags);
}

NonoStore::TID NonoStore::store_triple_internal(MDB_txn* txn, 
                                                 TermID subject_id,
                                                 TermID predicate_id, 
                                                 TermID object_id,
                                                 const std::string& source,
                                                 float confidence,
                                                 uint32_t flags) {
    // 1. Allocate new TID
    TID tid = _tid_gen->next_sequence(txn);
    
    // 2. Create TripleData
    TripleData triple_data(subject_id, predicate_id, object_id, source, confidence, flags);
    
    // 3. Serialize and store in main triple DBI
    std::string serialized = serialize_triple_data(triple_data);
    std::string tid_key = encode_tid(tid);
    
    MDB_val key_val = {tid_key.size(), (void*)tid_key.c_str()};
    MDB_val data_val = {serialized.size(), (void*)serialized.c_str()};
    
    int rc = mdb_put(txn, _triple_dbi, &key_val, &data_val, 0);
    if (rc != 0) {
        throw std::runtime_error("Failed to store triple data: " + std::string(mdb_strerror(rc)));
    }
    
    // 4. Update indices
    update_indices(txn, tid, triple_data, true);
    
    return tid;
}

std::optional<NonoStore::StringTriple> NonoStore::get_triple_as_strings_internal(MDB_txn* txn, TID tid) const {
    // 1. Get TripleData by TID
    auto triple_data = get_triple_internal(txn, tid);
    if (!triple_data.has_value()) {
        return std::nullopt;
    }
    
    // 2. Resolve TermIDs to strings
    auto subject_opt = _term_dict->resolve(txn, triple_data->subject_id);
    auto predicate_opt = _term_dict->resolve(txn, triple_data->predicate_id);
    auto object_opt = _term_dict->resolve(txn, triple_data->object_id);
    
    if (!subject_opt || !predicate_opt || !object_opt) {
        return std::nullopt; // TermIDs not found - data corruption?
    }
    
    // 3. Create StringTriple
    StringTriple result;
    result.subject = *subject_opt;
    result.predicate = *predicate_opt;
    result.object = *object_opt;
    result.timestamp = triple_data->timestamp;
    result.source = triple_data->source;
    result.confidence = triple_data->confidence;
    result.flags = triple_data->flags;
    result.tid = tid;
    
    return result;
}

std::optional<NonoStore::TripleData> NonoStore::get_triple_internal(MDB_txn* txn, TID tid) const {
    // 1. Encode TID as key
    std::string tid_key = encode_tid(tid);
    MDB_val key_val = {tid_key.size(), (void*)tid_key.c_str()};
    MDB_val data_val;
    
    // 2. Get from main triple DBI
    int rc = mdb_get(txn, _triple_dbi, &key_val, &data_val);
    if (rc == MDB_NOTFOUND) {
        return std::nullopt;
    }
    if (rc != 0) {
        throw std::runtime_error("Failed to get triple data: " + std::string(mdb_strerror(rc)));
    }
    
    // 3. Deserialize
    std::string serialized((char*)data_val.mv_data, data_val.mv_size);
    return deserialize_triple_data(serialized);
}

bool NonoStore::remove_triple_internal(MDB_txn* txn, TID tid) {
    // 1. Get triple data first (for index cleanup)
    auto triple_data = get_triple_internal(txn, tid);
    if (!triple_data.has_value()) {
        return false; // Triple doesn't exist
    }
    
    // 2. Remove from main triple DBI
    std::string tid_key = encode_tid(tid);
    MDB_val key_val = {tid_key.size(), (void*)tid_key.c_str()};
    
    int rc = mdb_del(txn, _triple_dbi, &key_val, nullptr);
    if (rc != 0 && rc != MDB_NOTFOUND) {
        throw std::runtime_error("Failed to remove triple data: " + std::string(mdb_strerror(rc)));
    }
    
    // 3. Update indices (remove)
    update_indices(txn, tid, *triple_data, false);
    
    return true;
}

//-------------------------------------------------------------------------
// TripleStore helper method implementations  
//-------------------------------------------------------------------------

std::string NonoStore::serialize_triple_data(const TripleData& data) const {
    // Simple binary serialization for TripleData
    // Format: subject_id(8) + predicate_id(8) + object_id(8) + timestamp(8) + confidence(4) + flags(4) + source_len(4) + source
    
    std::string result;
    result.reserve(44 + data.source.size()); // Pre-allocate
    
    // Fixed-size fields (40 bytes total)
    auto append_uint64 = [&](uint64_t val) {
        for (int i = 7; i >= 0; --i) {
            result.push_back((val >> (i * 8)) & 0xFF);
        }
    };
    
    auto append_uint32 = [&](uint32_t val) {
        for (int i = 3; i >= 0; --i) {
            result.push_back((val >> (i * 8)) & 0xFF);
        }
    };
    
    auto append_float = [&](float val) {
        uint32_t int_val;
        std::memcpy(&int_val, &val, 4);
        append_uint32(int_val);
    };
    
    // Serialize timestamp as nanoseconds since epoch
    auto time_point_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        data.timestamp.time_since_epoch()).count();
    
    append_uint64(data.subject_id);
    append_uint64(data.predicate_id);
    append_uint64(data.object_id);
    append_uint64(static_cast<uint64_t>(time_point_ns));
    append_float(data.confidence);
    append_uint32(data.flags);
    
    // Variable-size source string
    append_uint32(static_cast<uint32_t>(data.source.size()));
    result.append(data.source);
    
    return result;
}

NonoStore::TripleData NonoStore::deserialize_triple_data(const std::string& serialized) const {
    if (serialized.size() < 44) {
        throw std::runtime_error("Invalid serialized TripleData: too short");
    }
    
    const uint8_t* data = reinterpret_cast<const uint8_t*>(serialized.c_str());
    size_t offset = 0;
    
    auto read_uint64 = [&]() -> uint64_t {
        uint64_t val = 0;
        for (int i = 0; i < 8; ++i) {
            val = (val << 8) | data[offset++];
        }
        return val;
    };
    
    auto read_uint32 = [&]() -> uint32_t {
        uint32_t val = 0;
        for (int i = 0; i < 4; ++i) {
            val = (val << 8) | data[offset++];
        }
        return val;
    };
    
    auto read_float = [&]() -> float {
        uint32_t int_val = read_uint32();
        float float_val;
        std::memcpy(&float_val, &int_val, 4);
        return float_val;
    };
    
    TripleData result;
    result.subject_id = read_uint64();
    result.predicate_id = read_uint64();
    result.object_id = read_uint64();
    
    uint64_t time_ns = read_uint64();
    result.timestamp = std::chrono::time_point_cast<Timestamp::duration>(
        std::chrono::system_clock::time_point() + std::chrono::nanoseconds(time_ns));
    
    result.confidence = read_float();
    result.flags = read_uint32();
    
    uint32_t source_len = read_uint32();
    if (offset + source_len > serialized.size()) {
        throw std::runtime_error("Invalid serialized TripleData: source length exceeds data");
    }
    
    result.source = std::string(reinterpret_cast<const char*>(data + offset), source_len);
    
    return result;
}

bool NonoStore::update_indices(MDB_txn* txn, TID tid, const TripleData& data, bool add) {
    if (add) {
        return add_to_index(txn, _subject_index_dbi, data.subject_id, tid) &&
               add_to_index(txn, _predicate_index_dbi, data.predicate_id, tid) &&
               add_to_index(txn, _object_index_dbi, data.object_id, tid);
    } else {
        return remove_from_index(txn, _subject_index_dbi, data.subject_id, tid) &&
               remove_from_index(txn, _predicate_index_dbi, data.predicate_id, tid) &&
               remove_from_index(txn, _object_index_dbi, data.object_id, tid);
    }
}

bool NonoStore::add_to_index(MDB_txn* txn, MDB_dbi index_dbi, TermID term_id, TID tid) {
    // For now, simple implementation - each TermID maps to a list of TIDs
    // TODO: Optimize with more efficient data structures for large lists
    
    std::string term_key = encode_term_id(term_id);
    std::string tid_str = encode_tid(tid);
    
    MDB_val key_val = {term_key.size(), (void*)term_key.c_str()};
    MDB_val data_val;
    
    // Get existing TID list
    std::string tid_list;
    int rc = mdb_get(txn, index_dbi, &key_val, &data_val);
    if (rc == 0) {
        tid_list = std::string((char*)data_val.mv_data, data_val.mv_size);
    } else if (rc != MDB_NOTFOUND) {
        return false;
    }
    
    // Add TID to list (simple append for now)
    tid_list.append(tid_str);
    
    // Store updated list
    data_val = {tid_list.size(), (void*)tid_list.c_str()};
    rc = mdb_put(txn, index_dbi, &key_val, &data_val, 0);
    
    return rc == 0;
}

bool NonoStore::remove_from_index(MDB_txn* txn, MDB_dbi index_dbi, TermID term_id, TID tid) {
    // Simple implementation - remove TID from the list
    // TODO: Optimize this for better performance
    
    std::string term_key = encode_term_id(term_id);
    std::string tid_str = encode_tid(tid);
    
    MDB_val key_val = {term_key.size(), (void*)term_key.c_str()};
    MDB_val data_val;
    
    // Get existing TID list
    int rc = mdb_get(txn, index_dbi, &key_val, &data_val);
    if (rc != 0) {
        return false; // Not found
    }
    
    std::string tid_list((char*)data_val.mv_data, data_val.mv_size);
    
    // Remove TID from list (simple string replacement for now)
    size_t pos = tid_list.find(tid_str);
    if (pos != std::string::npos) {
        tid_list.erase(pos, tid_str.size());
        
        if (tid_list.empty()) {
            // Remove key entirely if list is empty
            rc = mdb_del(txn, index_dbi, &key_val, nullptr);
        } else {
            // Store updated list
            data_val = {tid_list.size(), (void*)tid_list.c_str()};
            rc = mdb_put(txn, index_dbi, &key_val, &data_val, 0);
        }
    }
    
    return true;
}

std::string NonoStore::encode_tid(TID tid) {
    // Use same encoding as TIDSequenceGenerator for consistency
    return TIDSequenceGenerator::encode_tid_for_storage(tid);
}

std::string NonoStore::encode_term_id(TermID term_id) {
    // Use same encoding as TermDictionary for consistency
    return TermDictionary::encode_term_id_for_storage(term_id);
}

} // namespace LabDb
