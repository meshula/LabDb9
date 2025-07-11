#pragma once

#include "LabDb/Db9Dispatcher.h"
#include "LabDb/DatabaseVerbs.h"
#include <memory>

namespace LabDb {

//-----------------------------------------------------------------------------
// Enhanced API: Rich Object Returns (find-entity returns rich objects)
// Lean API: EID/TID Only Returns (find-eid returns just identifiers)
//-----------------------------------------------------------------------------

/// Enhanced Find Entity Verb - Returns rich objects with EID and value
class FindEntityEnhancedVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "find-entity-enhanced"; }
    std::string getDescription() const override { 
        return "Search entities with wildcard patterns and return rich objects with EID and value"; 
    }
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

/// Lean Find EID Verb - Returns only EIDs for precise operations
class FindEidVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "find-eid"; }
    std::string getDescription() const override { 
        return "Search entities with wildcard patterns and return only EIDs"; 
    }
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

/// Enhanced Find Triple Verb - Returns rich objects with hydrated triple information
class FindTripleEnhancedVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "find-triple-enhanced"; }
    std::string getDescription() const override { 
        return "Find triples with wildcard patterns and return rich objects with TID, subject, predicate, object, and resolved values"; 
    }
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

/// Lean Find TID Verb - Returns only TIDs for precise operations
class FindTidVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "find-tid"; }
    std::string getDescription() const override { 
        return "Find triples with wildcard patterns and return only TIDs"; 
    }
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

//-----------------------------------------------------------------------------
// Dual-Layer API Design: Semantic vs Storage Operations
//-----------------------------------------------------------------------------

/// Semantic Add Triple - Auto-creates entities if needed (friendly)
class AddTripleSemanticVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "add-triple-semantic"; }
    std::string getDescription() const override { 
        return "Add triple with automatic entity creation if needed (semantic layer)"; 
    }
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

/// Storage Add TID - Requires existing EIDs (precise)
class AddTidVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "add-tid"; }
    std::string getDescription() const override { 
        return "Add triple using existing EIDs only (storage layer)"; 
    }
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

/// Enhanced Get Entity - Returns rich object with EID and value in single call
class GetEntityEnhancedVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "get-entity-enhanced"; }
    std::string getDescription() const override { 
        return "Retrieve entity with rich object containing EID, value, and metadata"; 
    }
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

/// Enhanced Get Triple - Returns rich object with full triple information
class GetTripleEnhancedVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "get-triple-enhanced"; }
    std::string getDescription() const override { 
        return "Retrieve triple with rich object containing TID, EIDs, and resolved values"; 
    }
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

/// Find Relationships Enhanced - Returns both incoming and outgoing relationships for an entity
class FindRelationshipsEnhancedVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "find-relationships-enhanced"; }
    std::string getDescription() const override {
        return "Find all relationships (incoming and outgoing) for an entity with EID chain resolution";
    }

    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

/// Find Entities Enhanced - Multi-pattern search across multiple terms
class FindEntitiesEnhancedVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "find-entities-enhanced"; }
    std::string getDescription() const override {
        return "Search entities using multiple patterns and return rich objects grouped by relevance";
    }

    Db9Response execute(const lab::Text::Sexpr& sexpr) override;
};

//-----------------------------------------------------------------------------
// Helper Functions for Rich Object Generation
//-----------------------------------------------------------------------------

/// Generate rich entity object JSON
std::string generateRichEntityJson(const std::string& eid, const std::string& value);

/// Generate rich triple object JSON  
std::string generateRichTripleJson(const std::string& tid, 
                                 const std::string& subject_eid, const std::string& subject_value,
                                 const std::string& predicate_eid, const std::string& predicate_value,
                                 const std::string& object_eid, const std::string& object_value);

/// Resolve entity value from EID (helper for triple enhancement)
std::string resolveEntityValue(const std::string& entity_term, std::shared_ptr<NonoStore> store);

} // namespace LabDb

void initEnhancedDatabaseVerbRegistration(LabDb::Db9Dispatcher&);
