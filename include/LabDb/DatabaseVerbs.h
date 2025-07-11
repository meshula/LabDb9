#pragma once

#include "LabDb/Db9Dispatcher.h"
#include "LabDb/EntityId.h"
#include "LabDb/NonoStore.h"
#include <memory>
#include <unordered_map>
#include <mutex>

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

/// Open Database Verb Implementation
class OpenDatabaseVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "open-database"; }
    std::string getDescription() const override { return "Open database file and return unique database ID"; }
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
    
private:
    std::string extractPath(const lab::Text::Sexpr& sexpr);
};

/// Create Database Verb Implementation
class CreateDatabaseVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "create-database"; }
    std::string getDescription() const override { return "Create new database file and return unique database ID"; }
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

/// Database Health Check Verb Implementation  
class DatabaseHealthCheckVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "database-health-check"; }
    std::string getDescription() const override { return "Check database health and return comprehensive statistics"; }
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
    
private:
    std::string extractDbid(const lab::Text::Sexpr& sexpr);
};

/// Close Database Verb Implementation
class CloseDatabaseVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "close-database"; }
    std::string getDescription() const override { return "Close database by database ID"; }
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
    
private:
    std::string extractDbid(const lab::Text::Sexpr& sexpr);
};

/// List Open Databases Verb Implementation
class ListOpenDatabasesVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "list-open-databases"; }
    std::string getDescription() const override { return "List all currently open database IDs"; }
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

//-----------------------------------------------------------------------------
// Phase 2: Entity Management Verbs
//-----------------------------------------------------------------------------

/// Add Entity Verb Implementation
class AddEntityVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "add-entity"; }
    std::string getDescription() const override { return "Create new entity with string content and return entity ID"; }
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

/// Get Entity Verb Implementation  
class GetEntityVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "get-entity"; }
    std::string getDescription() const override { return "Retrieve entity content by entity ID"; }
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

/// Find Entity Verb Implementation
class FindEntityVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "find-entity"; }
    std::string getDescription() const override { return "Search entities with wildcard patterns"; }
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

//-----------------------------------------------------------------------------
// Phase 3: Triple Operations Verbs
//-----------------------------------------------------------------------------

/// Add Triple Verb Implementation
class AddTripleVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "add-triple"; }
    std::string getDescription() const override { return "Add subject-predicate-object triple to database"; }
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

/// Add Triples Bulk Verb Implementation
class AddTriplesBulkVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "add-triples-bulk"; }
    std::string getDescription() const override { return "Add multiple triples in bulk with optimized performance"; }
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

/// Find Triple Verb Implementation
class FindTripleVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "find-triple"; }
    std::string getDescription() const override { return "Find triples matching subject, predicate, object patterns"; }
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

/// Get Triple Verb Implementation
class GetTripleVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "get-triple"; }
    std::string getDescription() const override { return "Retrieve specific triple by exact subject, predicate, object match"; }
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

/// Remove Triple Verb Implementation
class RemoveTripleVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "remove-triple"; }
    std::string getDescription() const override { return "Remove triple from database by subject, predicate, object"; }
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

/// Add Entities Bulk Verb Implementation
class AddEntitiesBulkVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "add-entities-bulk"; }
    std::string getDescription() const override { return "High-performance bulk entity creation with transaction batching"; }

    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

} // namespace LabDb

void initDatabaseVerbRegistration(LabDb::Db9Dispatcher& dispatcher);
