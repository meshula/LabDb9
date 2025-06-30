#include "LabDb/TermDictionary.h"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace LabDb {

TermDictionary::TermDictionary(MDB_env* env)
    : _env(env)
    , _dict_dbi(0)
    , _reverse_dict_dbi(0)
    , _sequence_dbi(0) {
    if (!_env) {
        throw TermDictionaryException("Invalid LMDB environment provided");
    }
    init();
}

TermDictionary::~TermDictionary() {
    // DBIs are automatically closed when environment is closed
    // No explicit cleanup needed
}

void TermDictionary::init() {
    // Open/create the three required DBIs
    MDB_txn* txn = nullptr;
    int rc = mdb_txn_begin(_env, nullptr, 0, &txn);
    if (rc != 0) {
        throw TermDictionaryException("Failed to begin transaction for initialization", rc);
    }
    
    try {
        // Primary dictionary: string → TermID
        rc = mdb_dbi_open(txn, "dict", MDB_CREATE, &_dict_dbi);
        if (rc != 0) {
            throw TermDictionaryException("Failed to open dict DBI", rc);
        }
        
        // Reverse dictionary: TermID → string
        rc = mdb_dbi_open(txn, "dict_reverse", MDB_CREATE, &_reverse_dict_dbi);
        if (rc != 0) {
            throw TermDictionaryException("Failed to open dict_reverse DBI", rc);
        }
        
        // Sequence counter: stores next available TermID
        rc = mdb_dbi_open(txn, "dict_sequence", MDB_CREATE, &_sequence_dbi);
        if (rc != 0) {
            throw TermDictionaryException("Failed to open dict_sequence DBI", rc);
        }
        
        // Initialize sequence counter if it doesn't exist
        MDB_val key, data;
        key.mv_data = const_cast<char*>(SEQUENCE_KEY);
        key.mv_size = strlen(SEQUENCE_KEY);
        
        rc = mdb_get(txn, _sequence_dbi, &key, &data);
        if (rc == MDB_NOTFOUND) {
            // Initialize sequence to first valid ID
            std::string initial_value = encode_term_id(FIRST_VALID_ID);
            data.mv_data = const_cast<char*>(initial_value.c_str());
            data.mv_size = initial_value.size();
            
            rc = mdb_put(txn, _sequence_dbi, &key, &data, 0);
            if (rc != 0) {
                throw TermDictionaryException("Failed to initialize sequence counter", rc);
            }
        } else if (rc != 0) {
            throw TermDictionaryException("Failed to check sequence counter", rc);
        }
        
        rc = mdb_txn_commit(txn);
        if (rc != 0) {
            throw TermDictionaryException("Failed to commit initialization transaction", rc);
        }
        
    } catch (...) {
        mdb_txn_abort(txn);
        throw;
    }
}

TermDictionary::TermID TermDictionary::intern(MDB_txn* txn, const std::string& term) {
    if (!validate_term(term)) {
        throw TermDictionaryException("Invalid term: " + term);
    }
    
    // First check if term already exists
    auto existing_id = lookup(txn, term);
    if (existing_id) {
        return *existing_id;
    }
    
    // Generate new TermID
    TermID new_id = next_sequence(txn);
    
    // Insert forward mapping: string → TermID
    MDB_val key, data;
    key.mv_data = const_cast<char*>(term.c_str());
    key.mv_size = term.size();
    
    std::string encoded_id = encode_term_id(new_id);
    data.mv_data = const_cast<char*>(encoded_id.c_str());
    data.mv_size = encoded_id.size();
    
    int rc = mdb_put(txn, _dict_dbi, &key, &data, MDB_NOOVERWRITE);
    if (rc == MDB_KEYEXIST) {
        // Race condition: another transaction inserted this term
        // Look it up again and return the existing ID
        auto race_id = lookup(txn, term);
        if (race_id) {
            return *race_id;
        }
        throw TermDictionaryException("Concurrent insertion race condition for term: " + term);
    } else if (rc != 0) {
        throw TermDictionaryException("Failed to insert forward mapping for term: " + term, rc);
    }
    
    // Insert reverse mapping: TermID → string
    key.mv_data = const_cast<char*>(encoded_id.c_str());
    key.mv_size = encoded_id.size();
    data.mv_data = const_cast<char*>(term.c_str());
    data.mv_size = term.size();
    
    rc = mdb_put(txn, _reverse_dict_dbi, &key, &data, 0);
    if (rc != 0) {
        // This is a serious consistency error - we have forward but not reverse mapping
        throw TermDictionaryException("Failed to insert reverse mapping for term: " + term + 
                                    " (CRITICAL: dictionary consistency compromised)", rc);
    }
    
    return new_id;
}

std::optional<TermDictionary::TermID> TermDictionary::lookup(MDB_txn* txn, const std::string& term) const {
    MDB_val key, data;
    key.mv_data = const_cast<char*>(term.c_str());
    key.mv_size = term.size();
    
    int rc = mdb_get(txn, _dict_dbi, &key, &data);
    if (rc == MDB_NOTFOUND) {
        return std::nullopt;
    } else if (rc != 0) {
        throw TermDictionaryException("Failed to lookup term: " + term, rc);
    }
    
    std::string encoded_id(static_cast<const char*>(data.mv_data), data.mv_size);
    return decode_term_id(encoded_id);
}

std::optional<std::string> TermDictionary::resolve(MDB_txn* txn, TermID term_id) const {
    if (term_id == INVALID_TERM_ID) {
        return std::nullopt;
    }
    
    std::string encoded_id = encode_term_id(term_id);
    MDB_val key, data;
    key.mv_data = const_cast<char*>(encoded_id.c_str());
    key.mv_size = encoded_id.size();
    
    int rc = mdb_get(txn, _reverse_dict_dbi, &key, &data);
    if (rc == MDB_NOTFOUND) {
        return std::nullopt;
    } else if (rc != 0) {
        throw TermDictionaryException("Failed to resolve TermID: " + std::to_string(term_id), rc);
    }
    
    return std::string(static_cast<const char*>(data.mv_data), data.mv_size);
}

bool TermDictionary::exists(MDB_txn* txn, const std::string& term) const {
    return lookup(txn, term).has_value();
}

bool TermDictionary::exists(MDB_txn* txn, TermID term_id) const {
    return resolve(txn, term_id).has_value();
}

size_t TermDictionary::count(MDB_txn* txn) const {
    MDB_stat stat;
    int rc = mdb_stat(txn, _dict_dbi, &stat);
    if (rc != 0) {
        throw TermDictionaryException("Failed to get dictionary statistics", rc);
    }
    return stat.ms_entries;
}

TermDictionary::TermID TermDictionary::next_available_id(MDB_txn* txn) const {
    MDB_val key, data;
    key.mv_data = const_cast<char*>(SEQUENCE_KEY);
    key.mv_size = strlen(SEQUENCE_KEY);
    
    int rc = mdb_get(txn, _sequence_dbi, &key, &data);
    if (rc != 0) {
        throw TermDictionaryException("Failed to get next available ID", rc);
    }
    
    std::string encoded_id(static_cast<const char*>(data.mv_data), data.mv_size);
    return decode_term_id(encoded_id);
}

TermDictionary::TermID TermDictionary::next_sequence(MDB_txn* txn) {
    MDB_val key, data;
    key.mv_data = const_cast<char*>(SEQUENCE_KEY);
    key.mv_size = strlen(SEQUENCE_KEY);
    
    // Get current sequence value
    int rc = mdb_get(txn, _sequence_dbi, &key, &data);
    if (rc != 0) {
        throw TermDictionaryException("Failed to get sequence counter", rc);
    }
    
    std::string encoded_current(static_cast<const char*>(data.mv_data), data.mv_size);
    TermID current_id = decode_term_id(encoded_current);
    TermID next_id = current_id + 1;
    
    // Update sequence counter
    std::string encoded_next = encode_term_id(next_id);
    data.mv_data = const_cast<char*>(encoded_next.c_str());
    data.mv_size = encoded_next.size();
    
    rc = mdb_put(txn, _sequence_dbi, &key, &data, 0);
    if (rc != 0) {
        throw TermDictionaryException("Failed to update sequence counter", rc);
    }
    
    return current_id;
}

std::vector<std::string> TermDictionary::all_terms(MDB_txn* txn, size_t limit) const {
    std::vector<std::string> terms;
    Iterator iter(txn, const_cast<TermDictionary&>(*this));
    
    if (iter.first()) {
        do {
            terms.push_back(iter.current_term());
            if (limit > 0 && terms.size() >= limit) {
                break;
            }
        } while (iter.next());
    }
    
    return terms;
}

std::vector<TermDictionary::TermID> TermDictionary::all_term_ids(MDB_txn* txn, size_t limit) const {
    std::vector<TermID> ids;
    Iterator iter(txn, const_cast<TermDictionary&>(*this));
    
    if (iter.first()) {
        do {
            ids.push_back(iter.current_id());
            if (limit > 0 && ids.size() >= limit) {
                break;
            }
        } while (iter.next());
    }
    
    return ids;
}

std::vector<TermDictionary::TermID> TermDictionary::intern_batch(MDB_txn* txn, const std::vector<std::string>& terms) {
    std::vector<TermID> ids;
    ids.reserve(terms.size());
    
    for (const auto& term : terms) {
        ids.push_back(intern(txn, term));
    }
    
    return ids;
}

std::vector<std::optional<std::string>> TermDictionary::resolve_batch(MDB_txn* txn, const std::vector<TermID>& term_ids) const {
    std::vector<std::optional<std::string>> terms;
    terms.reserve(term_ids.size());
    
    for (TermID id : term_ids) {
        terms.push_back(resolve(txn, id));
    }
    
    return terms;
}

TermDictionary::Stats TermDictionary::get_stats(MDB_txn* txn) const {
    Stats stats = {};
    
    // Get total term count
    stats.total_terms = count(txn);
    stats.next_available_id = next_available_id(txn);
    
    // Get DBI sizes
    MDB_stat dict_stat, reverse_stat;
    int rc = mdb_stat(txn, _dict_dbi, &dict_stat);
    if (rc == 0) {
        stats.dict_size_bytes = dict_stat.ms_psize * (dict_stat.ms_branch_pages + dict_stat.ms_leaf_pages + dict_stat.ms_overflow_pages);
    }
    
    rc = mdb_stat(txn, _reverse_dict_dbi, &reverse_stat);
    if (rc == 0) {
        stats.reverse_dict_size_bytes = reverse_stat.ms_psize * (reverse_stat.ms_branch_pages + reverse_stat.ms_leaf_pages + reverse_stat.ms_overflow_pages);
    }
    
    // Find min/max TermIDs
    if (stats.total_terms > 0) {
        Iterator iter(txn, const_cast<TermDictionary&>(*this));
        if (iter.first()) {
            stats.min_term_id = iter.current_id();
            
            // Find max by iterating to the end
            TermID max_id = stats.min_term_id;
            while (iter.next()) {
                max_id = iter.current_id();
            }
            stats.max_term_id = max_id;
        }
    } else {
        stats.min_term_id = stats.max_term_id = INVALID_TERM_ID;
    }
    
    return stats;
}

bool TermDictionary::validate_consistency(MDB_txn* txn) const {
    // Check that every forward mapping has a corresponding reverse mapping
    Iterator iter(txn, const_cast<TermDictionary&>(*this));
    if (!iter.first()) {
        return true; // Empty dictionary is consistent
    }
    
    do {
        std::string term = iter.current_term();
        TermID id = iter.current_id();
        
        // Check reverse mapping exists
        auto resolved_term = resolve(txn, id);
        if (!resolved_term || *resolved_term != term) {
            return false;
        }
        
        // Check forward mapping is consistent
        auto looked_up_id = lookup(txn, term);
        if (!looked_up_id || *looked_up_id != id) {
            return false;
        }
        
    } while (iter.next());
    
    return true;
}

// Static utility methods
std::string TermDictionary::encode_term_id(TermID id) {
    // Encode as 8-byte big-endian for consistent sorting
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(16) << std::hex << id;
    return oss.str();
}

TermDictionary::TermID TermDictionary::decode_term_id(const std::string& encoded) {
    if (encoded.size() != 16) {
        throw TermDictionaryException("Invalid encoded TermID length: " + std::to_string(encoded.size()));
    }
    
    std::istringstream iss(encoded);
    TermID id;
    iss >> std::hex >> id;
    
    if (iss.fail()) {
        throw TermDictionaryException("Failed to decode TermID: " + encoded);
    }
    
    return id;
}

bool TermDictionary::validate_term(const std::string& term) const {
    // Basic validation: non-empty, reasonable length
    if (term.empty()) {
        return false;
    }
    
    // Prevent extremely long terms that could cause issues
    if (term.size() > 1024 * 1024) { // 1MB limit
        return false;
    }
    
    // Could add more validation here (UTF-8 validation, character restrictions, etc.)
    return true;
}

// Iterator implementation
TermDictionary::Iterator::Iterator(MDB_txn* txn, TermDictionary& dict, bool reverse_order)
    : _txn(txn)
    , _dict(dict)
    , _cursor(nullptr)
    , _reverse_order(reverse_order)
    , _valid(false) {
    
    int rc = mdb_cursor_open(_txn, _dict._dict_dbi, &_cursor);
    if (rc != 0) {
        throw TermDictionaryException("Failed to open dictionary cursor", rc);
    }
}

TermDictionary::Iterator::~Iterator() {
    if (_cursor) {
        mdb_cursor_close(_cursor);
    }
}

bool TermDictionary::Iterator::first() {
    int rc = mdb_cursor_get(_cursor, &_key, &_value, _reverse_order ? MDB_LAST : MDB_FIRST);
    _valid = (rc == 0);
    return _valid;
}

bool TermDictionary::Iterator::next() {
    if (!_valid) return false;
    
    int rc = mdb_cursor_get(_cursor, &_key, &_value, _reverse_order ? MDB_PREV : MDB_NEXT);
    _valid = (rc == 0);
    return _valid;
}

bool TermDictionary::Iterator::prev() {
    if (!_valid) return false;
    
    int rc = mdb_cursor_get(_cursor, &_key, &_value, _reverse_order ? MDB_NEXT : MDB_PREV);
    _valid = (rc == 0);
    return _valid;
}

std::string TermDictionary::Iterator::current_term() const {
    if (!_valid) {
        throw TermDictionaryException("Iterator not valid - cannot get current term");
    }
    return std::string(static_cast<const char*>(_key.mv_data), _key.mv_size);
}

TermDictionary::TermID TermDictionary::Iterator::current_id() const {
    if (!_valid) {
        throw TermDictionaryException("Iterator not valid - cannot get current ID");
    }
    std::string encoded_id(static_cast<const char*>(_value.mv_data), _value.mv_size);
    return TermDictionary::decode_term_id(encoded_id);
}

bool TermDictionary::Iterator::seek(const std::string& term) {
    _key.mv_data = const_cast<char*>(term.c_str());
    _key.mv_size = term.size();
    
    int rc = mdb_cursor_get(_cursor, &_key, &_value, MDB_SET_RANGE);
    _valid = (rc == 0);
    return _valid;
}

bool TermDictionary::Iterator::seek(TermID term_id) {
    // For seeking by TermID, we need to search through the values
    // This is less efficient than string-based seeking, but still useful
    if (first()) {
        do {
            if (current_id() >= term_id) {
                return true;
            }
        } while (next());
    }
    _valid = false;
    return false;
}

// Static utility methods for binary encoding

std::string TermDictionary::encode_term_id_for_storage(TermID term_id) {
    // Use same fixed-width hex encoding as sequence counter for consistency
    // This ensures optimal LMDB prefix compression
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(16) << std::hex << term_id;
    return oss.str();
}

TermDictionary::TermID TermDictionary::decode_term_id_from_storage(const std::string& encoded) {
    if (encoded.length() != 16) {
        throw TermDictionaryException("Invalid encoded TermID length: " + std::to_string(encoded.length()));
    }
    
    char* end;
    TermID term_id = std::strtoull(encoded.c_str(), &end, 16);
    if (end != encoded.c_str() + encoded.length()) {
        throw TermDictionaryException("Invalid encoded TermID format: " + encoded);
    }
    
    return term_id;
}

} // namespace LabDb
