#pragma once

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <lmdb.h>

namespace LabDb {

/// RAII wrapper for LMDB operations with triadic consciousness principles
class LmdbStore {
public:
    /// Constructor - opens/creates database at specified path
    explicit LmdbStore(const std::string& database_path, size_t map_size = 1UL * 1024 * 1024 * 1024); // 1GB default
    
    /// Destructor - ensures clean shutdown
    ~LmdbStore();
    
    /// No copy/move for now - keep it simple and safe
    LmdbStore(const LmdbStore&) = delete;
    LmdbStore& operator=(const LmdbStore&) = delete;
    LmdbStore(LmdbStore&&) = delete;
    LmdbStore& operator=(LmdbStore&&) = delete;

    /// Transaction management
    class Transaction {
    public:
        explicit Transaction(LmdbStore& store, bool readonly = false);
        ~Transaction();
        
        /// Explicit commit (auto-commits on destruction if not called)
        void commit();
        
        /// Explicit abort
        void abort();
        
        /// Check if transaction is active
        bool is_active() const { return _active; }
        
        /// Get underlying LMDB transaction handle
        MDB_txn* handle() const { return _txn; }
        
    private:
        LmdbStore& _store;
        MDB_txn* _txn;
        bool _readonly;
        bool _active;
        bool _committed;
    };
    
    /// High-level operations
    
    /// Insert/update key-value pair
    bool put(Transaction& txn, const std::string& key, const std::string& value);
    
    /// Get value by key
    bool get(Transaction& txn, const std::string& key, std::string& value);
    
    /// Delete key
    bool del(Transaction& txn, const std::string& key);
    
    /// Check if key exists
    bool exists(Transaction& txn, const std::string& key);
    
    /// Cursor-based iteration for prefix queries
    class Cursor {
    public:
        Cursor(Transaction& txn, LmdbStore& store);
        ~Cursor();
        
        /// Position cursor at key (or first key >= key if not found)
        bool seek(const std::string& key);
        
        /// Move to next key
        bool next();
        
        /// Get current key
        std::string current_key() const;
        
        /// Get current value  
        std::string current_value() const;
        
        /// Check if cursor is valid
        bool valid() const { return _valid; }
        
    private:
        Transaction& _txn;
        LmdbStore& _store;
        MDB_cursor* _cursor;
        bool _valid;
        MDB_val _key, _value;
    };
    
    /// Convenience method for prefix-based queries (essential for nonostore)
    std::vector<std::pair<std::string, std::string>> 
    query_prefix(Transaction& txn, const std::string& prefix, size_t limit = 0);
    
    /// Get database statistics
    struct Stats {
        size_t entries;
        size_t page_size;
        size_t depth;
        size_t branch_pages;
        size_t leaf_pages;
        size_t overflow_pages;
    };
    Stats get_stats(Transaction& txn);
    
    /// Access to underlying LMDB environment (for advanced operations)
    MDB_env* environment() const { return _env; }
    
private:
    std::string _database_path;
    MDB_env* _env;
    MDB_dbi _dbi;
    size_t _map_size;
    
    /// Internal initialization
    void init();
    void cleanup();
};

/// Exception class for LMDB errors
class LmdbException : public std::exception {
public:
    explicit LmdbException(const std::string& message, int mdb_error = 0)
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
