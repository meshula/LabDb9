#pragma once

#include "LabDb/Db9Dispatcher.h"
#include "LabDb/EntityId.h"
#include "LabDb/NonoStore.h"
#include <memory>
#include <unordered_map>
#include <mutex>
#include <string>
#include <vector>

namespace LabDb {

/// Database connection management for db9 verbs
/// Maintains active database connections with unique DBIDs
class DatabaseManager {
public:
    static DatabaseManager& instance() {
        static DatabaseManager mgr;
        return mgr;
    }
    
    /// Open database and return unique DBID
    std::string openDatabase(const std::string& path);
    
    /// Create new database and return unique DBID
    std::string createDatabase(const std::string& path);
    
    /// Close database by DBID  
    bool closeDatabase(const std::string& dbid);
    
    /// Get database by DBID (returns nullptr if not found)
    std::shared_ptr<NonoStore> getDatabase(const std::string& dbid);
    
    /// Check if DBID is valid
    bool isValidDbid(const std::string& dbid) const;
    
    /// Get all active DBIDs
    std::vector<std::string> getActiveDbids() const;
    
private:
    DatabaseManager() = default;
    
    mutable std::mutex _mutex;
    std::unordered_map<std::string, std::shared_ptr<NonoStore>> _databases;
    uint32_t _next_id{1};
    
    std::string generateDbid();
};

} // namespace Labdb