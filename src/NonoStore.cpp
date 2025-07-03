#include "LabDb/NonoStore.h"
#include "LabDb/TripleStore.h"
#include "LabDb/TermDictionary.h"
#include "LabDb/TIDSequenceGenerator.h"
#include "LabDb/TriadicQuery.h"
#include <algorithm>
#include <set>
#include <iostream>

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

bool NonoStore::add_triple(const std::string& subject, 
                           const std::string& predicate, 
                           const std::string& object) {
    try {
        LmdbStore::Transaction txn(*_store);
        bool result = add_triple_impl(txn, subject, predicate, object);
        if (result) {
            txn.commit();
        }
        return result;
    } catch (const LmdbException& e) {
        return set_error(ErrorCode::TransactionError, "Add triple failed: " + std::string(e.what()));
    }
}

bool NonoStore::remove_triple(const std::string& subject, 
                              const std::string& predicate, 
                              const std::string& object) {
    try {
        LmdbStore::Transaction txn(*_store);
        bool result = remove_triple_impl(txn, subject, predicate, object);
        if (result) {
            txn.commit();
        }
        return result;
    } catch (const LmdbException& e) {
        return set_error(ErrorCode::TransactionError, "Remove triple failed: " + std::string(e.what()));
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

bool NonoStore::add_triple_impl(LmdbStore::Transaction& txn,
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
        return true;
        
    } catch (const std::exception& e) {
        return set_error(ErrorCode::KeyGenerationError, "TID-based connect failed: " + std::string(e.what()));
    }
}

bool NonoStore::remove_triple_impl(LmdbStore::Transaction& txn,
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
