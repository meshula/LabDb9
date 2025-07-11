#pragma once

#include "LabDb/Db9Dispatcher.h"
#include "LabDb/EntityId.h"
#include "LabDb/NonoStore.h"
#include "LabDb/LabText.hpp"
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace LabDb {

/// Database connection management for db9 verbs
/// Maintains active database connections with unique DBIDs

/// Open Database Verb Implementation
class OpenDatabaseVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "open-database"; }
    std::string getDescription() const override;
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
    
private:
    std::string extractPath(const lab::Text::Sexpr& sexpr);
};

/// Create Database Verb Implementation
class CreateDatabaseVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "create-database"; }
    std::string getDescription() const override;
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

/// Database Health Check Verb Implementation  
class DatabaseHealthCheckVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "database-health-check"; }
    std::string getDescription() const override;
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
    
private:
    std::string extractDbid(const lab::Text::Sexpr& sexpr);
};

/// Close Database Verb Implementation
class CloseDatabaseVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "close-database"; }
    std::string getDescription() const override;
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
    
private:
    std::string extractDbid(const lab::Text::Sexpr& sexpr);
};

/// List Open Databases Verb Implementation
class ListOpenDatabasesVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "list-open-databases"; }
    std::string getDescription() const override;
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

//-----------------------------------------------------------------------------
// Phase 2: Entity Management Verbs
//-----------------------------------------------------------------------------

/// Add Entity Verb Implementation
class AddEntityVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "add-entity"; }
    std::string getDescription() const override;
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

/// Get Entity Verb Implementation  
class GetEntityVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "get-entity"; }
    std::string getDescription() const override;
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

/// Find Entity Verb Implementation
class FindEntityVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "find-entity"; }
    std::string getDescription() const override;
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

//-----------------------------------------------------------------------------
// Phase 3: Triple Operations Verbs
//-----------------------------------------------------------------------------

/// Add Triple Verb Implementation
class AddTripleVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "add-triple"; }
    std::string getDescription() const override;
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

/// Add Triples Bulk Verb Implementation
class AddTriplesBulkVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "add-triples-bulk"; }
    std::string getDescription() const override;
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

/// Find Triple Verb Implementation
class FindTripleVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "find-triple"; }
    std::string getDescription() const override;
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

/// Get Triple Verb Implementation
class GetTripleVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "get-triple"; }
    std::string getDescription() const override;
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

/// Remove Triple Verb Implementation
class RemoveTripleVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "remove-triple"; }
    std::string getDescription() const override;
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

/// Add Entities Bulk Verb Implementation
class AddEntitiesBulkVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "add-entities-bulk"; }
    std::string getDescription() const override;

    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

} // namespace LabDb

void initDatabaseVerbRegistration(LabDb::Db9Dispatcher& dispatcher);
