#include "LabDb/LmdbStore.h"
#include <filesystem>
#include <iostream>

namespace LabDb {

LmdbStore::LmdbStore(const std::string& database_path, size_t map_size)
    : _database_path(database_path), _env(nullptr), _dbi(0), _map_size(map_size) {
    init();
}

LmdbStore::~LmdbStore() {
    cleanup();
}

void LmdbStore::init() {
    // Create directory if it doesn't exist
    std::filesystem::path db_path(_database_path);
    if (db_path.has_parent_path()) {
        std::filesystem::create_directories(db_path.parent_path());
    }
    
    // Create LMDB environment
    int rc = mdb_env_create(&_env);
    if (rc != 0) {
        throw LmdbException("Failed to create LMDB environment", rc);
    }
    
    // Set map size
    rc = mdb_env_set_mapsize(_env, _map_size);
    if (rc != 0) {
        mdb_env_close(_env);
        throw LmdbException("Failed to set map size", rc);
    }
    
    // Open environment
    rc = mdb_env_open(_env, _database_path.c_str(), MDB_NOSUBDIR, 0664);
    if (rc != 0) {
        mdb_env_close(_env);
        throw LmdbException("Failed to open LMDB environment at " + _database_path, rc);
    }
    
    // Open main database
    MDB_txn* txn;
    rc = mdb_txn_begin(_env, nullptr, 0, &txn);
    if (rc != 0) {
        mdb_env_close(_env);
        throw LmdbException("Failed to begin transaction for database creation", rc);
    }
    
    rc = mdb_dbi_open(txn, nullptr, MDB_CREATE, &_dbi);
    if (rc != 0) {
        mdb_txn_abort(txn);
        mdb_env_close(_env);
        throw LmdbException("Failed to open/create main database", rc);
    }
    
    rc = mdb_txn_commit(txn);
    if (rc != 0) {
        mdb_env_close(_env);
        throw LmdbException("Failed to commit database creation transaction", rc);
    }
}

void LmdbStore::cleanup() {
    if (_env) {
        mdb_dbi_close(_env, _dbi);
        mdb_env_close(_env);
        _env = nullptr;
    }
}

// Transaction implementation
LmdbStore::Transaction::Transaction(LmdbStore& store, bool readonly)
    : _store(store), _txn(nullptr), _readonly(readonly), _active(false), _committed(false) {
    
    unsigned int flags = readonly ? MDB_RDONLY : 0;
    int rc = mdb_txn_begin(_store._env, nullptr, flags, &_txn);
    if (rc != 0) {
        throw LmdbException("Failed to begin transaction", rc);
    }
    _active = true;
}

LmdbStore::Transaction::~Transaction() {
    if (_active && !_committed) {
        // Auto-commit read-only transactions, abort read-write transactions
        if (_readonly) {
            mdb_txn_commit(_txn);
        } else {
            mdb_txn_abort(_txn);
        }
    }
}

void LmdbStore::Transaction::commit() {
    if (!_active) {
        throw LmdbException("Cannot commit inactive transaction");
    }
    
    int rc = mdb_txn_commit(_txn);
    if (rc != 0) {
        throw LmdbException("Failed to commit transaction", rc);
    }
    
    _active = false;
    _committed = true;
}

void LmdbStore::Transaction::abort() {
    if (!_active) {
        return; // Already aborted or committed
    }
    
    mdb_txn_abort(_txn);
    _active = false;
}

// Basic operations
bool LmdbStore::put(Transaction& txn, const std::string& key, const std::string& value) {
    if (!txn.is_active()) {
        return false;
    }
    
    MDB_val mdb_key, mdb_value;
    mdb_key.mv_size = key.size();
    mdb_key.mv_data = const_cast<char*>(key.c_str());
    mdb_value.mv_size = value.size();
    mdb_value.mv_data = const_cast<char*>(value.c_str());
    
    int rc = mdb_put(txn.handle(), _dbi, &mdb_key, &mdb_value, 0);
    return rc == 0;
}

bool LmdbStore::get(Transaction& txn, const std::string& key, std::string& value) {
    if (!txn.is_active()) {
        return false;
    }
    
    MDB_val mdb_key, mdb_value;
    mdb_key.mv_size = key.size();
    mdb_key.mv_data = const_cast<char*>(key.c_str());
    
    int rc = mdb_get(txn.handle(), _dbi, &mdb_key, &mdb_value);
    if (rc == 0) {
        value.assign(static_cast<const char*>(mdb_value.mv_data), mdb_value.mv_size);
        return true;
    }
    return false;
}

bool LmdbStore::del(Transaction& txn, const std::string& key) {
    if (!txn.is_active()) {
        return false;
    }
    
    MDB_val mdb_key;
    mdb_key.mv_size = key.size();
    mdb_key.mv_data = const_cast<char*>(key.c_str());
    
    int rc = mdb_del(txn.handle(), _dbi, &mdb_key, nullptr);
    return rc == 0;
}

bool LmdbStore::exists(Transaction& txn, const std::string& key) {
    std::string dummy;
    return get(txn, key, dummy);
}

// Cursor implementation
LmdbStore::Cursor::Cursor(Transaction& txn, LmdbStore& store)
    : _txn(txn), _store(store), _cursor(nullptr), _valid(false) {
    
    int rc = mdb_cursor_open(_txn.handle(), _store._dbi, &_cursor);
    if (rc != 0) {
        throw LmdbException("Failed to open cursor", rc);
    }
}

LmdbStore::Cursor::~Cursor() {
    if (_cursor) {
        mdb_cursor_close(_cursor);
    }
}

bool LmdbStore::Cursor::seek(const std::string& key) {
    _key.mv_size = key.size();
    _key.mv_data = const_cast<char*>(key.c_str());
    
    int rc = mdb_cursor_get(_cursor, &_key, &_value, MDB_SET_RANGE);
    _valid = (rc == 0);
    return _valid;
}

bool LmdbStore::Cursor::next() {
    if (!_valid) {
        return false;
    }
    
    int rc = mdb_cursor_get(_cursor, &_key, &_value, MDB_NEXT);
    _valid = (rc == 0);
    return _valid;
}

std::string LmdbStore::Cursor::current_key() const {
    if (!_valid) {
        return "";
    }
    return std::string(static_cast<const char*>(_key.mv_data), _key.mv_size);
}

std::string LmdbStore::Cursor::current_value() const {
    if (!_valid) {
        return "";
    }
    return std::string(static_cast<const char*>(_value.mv_data), _value.mv_size);
}

// Prefix query implementation - critical for nonostore performance
std::vector<std::pair<std::string, std::string>> 
LmdbStore::query_prefix(Transaction& txn, const std::string& prefix, size_t limit) {
    std::vector<std::pair<std::string, std::string>> results;
    
    Cursor cursor(txn, *this);
    
    if (!cursor.seek(prefix)) {
        return results; // No keys found at or after prefix
    }
    
    size_t count = 0;
    do {
        std::string key = cursor.current_key();
        
        // Check if key still starts with prefix
        if (key.size() < prefix.size() || 
            key.substr(0, prefix.size()) != prefix) {
            break; // No more keys with this prefix
        }
        
        std::string value = cursor.current_value();
        results.emplace_back(std::move(key), std::move(value));
        
        count++;
        if (limit > 0 && count >= limit) {
            break;
        }
        
    } while (cursor.next());
    
    return results;
}

// Statistics
LmdbStore::Stats LmdbStore::get_stats(Transaction& txn) {
    MDB_stat stat;
    int rc = mdb_stat(txn.handle(), _dbi, &stat);
    if (rc != 0) {
        throw LmdbException("Failed to get database statistics", rc);
    }
    
    return Stats{
        .entries = stat.ms_entries,
        .page_size = stat.ms_psize,
        .depth = stat.ms_depth,
        .branch_pages = stat.ms_branch_pages,
        .leaf_pages = stat.ms_leaf_pages,
        .overflow_pages = stat.ms_overflow_pages
    };
}

} // namespace LabDb
