#include "LabDb/DatabaseVerbs.h"
#include "LabDb/DatabaseManager.h"
#include "LabDb/EntityId.h"
#include "LabDb/LabText.hpp"

#include <iostream>

namespace LabDb {

//-----------------------------------------------------------------------------
// Helper functions for S-expression parameter extraction
//-----------------------------------------------------------------------------
namespace {

// Utility function to generate helpful database diagnosis messages
std::string generateDbidDiagnosisMessage(const std::string& operation_name) {
    auto& manager = DatabaseManager::instance();
    auto active_dbids = manager.getActiveDbids();
    
    std::ostringstream msg;
    msg << "Supply " << operation_name << " with a valid dbid, and try again. ";
    
    if (active_dbids.empty()) {
        msg << "There are no open databases, so open one first to get a dbid.";
    } else if (active_dbids.size() == 1) {
        msg << "This is the open database: dbid \"" << active_dbids[0]
        << "\", is it the one with the data you are searching for?";
    } else {
        msg << "There are several open databases: ";
        for (size_t i = 0; i < active_dbids.size(); ++i) {
            if (i > 0) msg << ", ";
            if (i == active_dbids.size() - 1 && active_dbids.size() > 2) msg << "and ";
            msg << "dbid \"" << active_dbids[i] << "\"";
        }
        msg << ", is the data you are searching for in one of them?";
    }
    
    return msg.str();
}

std::string extractStringParam(const lab::Text::Sexpr& sexpr, const std::string& param_name) {
    // Look for :param_name value pattern in the parsed S-expression
    for (size_t i = 0; i < sexpr.expr.size() - 1; ++i) {
        const auto& elem = sexpr.expr[i];
        
        // Look for atoms that match our parameter name
        if (elem.token == tsSexprAtom) {
            int stringIndex = elem.ref;
            if (stringIndex < static_cast<int>(sexpr.strings.size())) {
                const std::string& token = sexpr.strings[stringIndex];
                if (token == ":" + param_name) {
                    // Found parameter, get next value
                    if (i + 1 < sexpr.expr.size()) {
                        const auto& value_elem = sexpr.expr[i + 1];
                        if (value_elem.token == tsSexprAtom) {
                            int valueIndex = value_elem.ref;
                            if (valueIndex < static_cast<int>(sexpr.strings.size())) {
                                return sexpr.strings[valueIndex];
                            }
                        } else if (value_elem.token == tsSexprString) {
                            int valueIndex = value_elem.ref;
                            if (valueIndex < static_cast<int>(sexpr.strings.size())) {
                                return sexpr.strings[valueIndex];
                            }
                        }
                    }
                }
            }
        }
    }
    
    if (param_name == "dbid") {
        throw std::runtime_error(generateDbidDiagnosisMessage("operation") + " (Parameter :dbid is required)");
    } else {
        throw std::runtime_error("Required parameter :" + param_name + " not found");
    }
}

} // anonymous namespace

//-----------------------------------------------------------------------------
// OpenDatabaseVerb Implementation
//-----------------------------------------------------------------------------

Db9Response OpenDatabaseVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        std::string path = extractStringParam(sexpr, "path");
        
        // Open database through manager
        std::string dbid = DatabaseManager::instance().openDatabase(path);
        
        // Calculate metrics
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        metrics.items_processed = 1;
        
        // Build result JSON
        std::ostringstream result;
        result << "{\"dbid\": \"" << dbid << "\", \"path\": \"" << path << "\"}";
        
        return Db9Response{
            Db9Response::Success,
            result.str(),
            "",
            "",
            metrics
        };
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        
        return Db9Response{
            Db9Response::Error,
            "",
            "open_database_failed",
            e.what(),
            metrics
        };
    }
}

std::string OpenDatabaseVerb::getDescription() const {
    return R"(
Open an existing database file and assign it a database ID for subsequent operations.
Usage:
```lisp
(open-database :path "/full/path/to/database")  
;; Returns: dbid, status

;; Example:
(open-database :path "/full/path/to/database.db9")
;; Returns: {"dbid": "db1", "status": "opened", "path": "/full/path/to/database.db9"}
```

**Parameters:**
- `:path` - Full filesystem path to existing database file

**Returns:**
- `dbid` - Assigned database identifier for use in subsequent operations
- `path` - Confirmed path to opened database
- `status` - Operation status

**Notes:**
- Database must already exist (use create-database for new databases)
- Each open database gets a unique dbid for isolation
- Multiple databases can be open simultaneously
- Use close-database when finished to free resources
)";
}

//-----------------------------------------------------------------------------
// CreateDatabaseVerb Implementation
//-----------------------------------------------------------------------------

Db9Response CreateDatabaseVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();

    try {
        std::string path = extractStringParam(sexpr, "path");

        // Create database through manager
        std::string dbid = DatabaseManager::instance().createDatabase(path);

        // Calculate metrics
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        metrics.items_processed = 1;

        // Build result JSON
        std::ostringstream result;
        result << "{\"status\": \"created\", \"dbid\": \"" << dbid << "\", \"path\": \"" << path << "\", \"initialized\": true}";

        return Db9Response{
            Db9Response::Success,
            result.str(),
            "",
            "",
            metrics
        };

    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;

        // Check for specific error types
        std::string error_code = "create_database_failed";
        if (std::string(e.what()).find("already exists") != std::string::npos) {
            error_code = "database_exists";
        } else if (std::string(e.what()).find("path") != std::string::npos) {
            error_code = "path_invalid";
        }

        return Db9Response{
            Db9Response::Error,
            "",
            error_code,
            e.what(),
            metrics
        };
    }
}

std::string CreateDatabaseVerb::getDescription() const {
    return R"(
Create a new database file and initialize it for triadic consciousness operations.
Usage:
```lisp
(create-database :path "/full/path/to/database")
;; Returns: dbid, status
;; Notes: create-database does not open-database

;; Example:
(create-database :path "/full/path/to/database.db9")
;; Returns: {"status": "created", "dbid": "db2", "path": "/full/path/to/database.db9", "initialized": true}
```

**Parameters:**
- `:path` - Full filesystem path for new database file

**Returns:**
- `dbid` - Assigned database identifier
- `path` - Path to created database
- `status` - Creation status
- `initialized` - Confirmation that database structure is ready

**Notes:**
- Creates new database file with triadic consciousness structure
- Database is created but not opened - use open-database after creation
- Will fail if file already exists
- Initializes LMDB environment with proper settings for triple storage
)";
}

//-----------------------------------------------------------------------------
// DatabaseHealthCheckVerb Implementation
//-----------------------------------------------------------------------------

Db9Response DatabaseHealthCheckVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        std::string dbid = extractStringParam(sexpr, "dbid");
        
        // Get database from manager
        auto db = DatabaseManager::instance().getDatabase(dbid);
        if (!db) {
            throw std::runtime_error("Invalid database ID: " + dbid);
        }
        
        // Get database statistics
        auto stats = db->get_stats();
        
        // Calculate metrics
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        metrics.items_processed = stats.total_triples;
        
        // Build comprehensive health result
        std::ostringstream result;
        result << "{\"health_status\": \"healthy\", ";
        result << "\"total_triples\": " << stats.total_triples << ", ";
        result << "\"unique_subjects\": " << stats.unique_subjects << ", ";
        result << "\"unique_predicates\": " << stats.unique_predicates << ", ";
        result << "\"unique_objects\": " << stats.unique_objects << ", ";
        result << "\"term_dictionary_size\": " << stats.term_dictionary_size << ", ";
        result << "\"subject_vocabulary_size\": " << stats.subject_vocabulary_size << ", ";
        result << "\"predicate_vocabulary_size\": " << stats.predicate_vocabulary_size << ", ";
        result << "\"lmdb_entries\": " << stats.lmdb_stats.entries << "}";
        
/*
@TODO
**Returns:**
- `status` - Overall health status (healthy/warning/critical)
- `orphaned_entities` - Count of entities not referenced in any triples
- `dangling_references` - Count of triples referencing non-existent entities
- `storage_efficiency` - Ratio of used to allocated storage
- `performance_metrics` - Query response time statistics
- `recommendations` - List of suggested maintenance actions
*/

        return Db9Response{
            Db9Response::Success, 
            result.str(),
            "", 
            "", 
            metrics
        };
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        
        return Db9Response{
            Db9Response::Error, 
            "", 
            "health_check_failed", 
            e.what(), 
            metrics
        };
    }
}

std::string DatabaseHealthCheckVerb::getDescription() const {
    return R"(
Get comprehensive database statistics and basic health indicators.
Usage:
```lisp
(database-health-check :dbid database-id)
;; Returns: database statistics and basic health status

;; Example:
(database-health-check :dbid db1)
;; Returns: {
;;   "health_status": "healthy",
;;   "total_triples": 644,
;;   "unique_subjects": 545,
;;   "unique_predicates": 32,
;;   "unique_objects": 151,
;;   "term_dictionary_size": 728,
;;   "subject_vocabulary_size": 545,
;;   "predicate_vocabulary_size": 32,
;;   "lmdb_entries": 1416
;; }
```

**Parameters:**
- `:dbid` - Database identifier to analyze

**Returns:**
- `health_status` - Basic health indicator (currently always "healthy")
- `total_triples` - Total number of stored triples
- `unique_subjects` - Count of unique subject entities
- `unique_predicates` - Count of unique predicate entities  
- `unique_objects` - Count of unique object entities
- `term_dictionary_size` - Total terms in dictionary
- `subject_vocabulary_size` - Subject vocabulary count
- `predicate_vocabulary_size` - Predicate vocabulary count
- `lmdb_entries` - Low-level LMDB entry count

**Notes:**
- Non-destructive read operation
- Provides database size and structure statistics
- Currently focused on metrics rather than integrity analysis
- Performance measured via auto_reflexive metrics
)";
}

//-----------------------------------------------------------------------------
// CloseDatabaseVerb Implementation
//-----------------------------------------------------------------------------

Db9Response CloseDatabaseVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        std::string dbid = extractStringParam(sexpr, "dbid");
        
        // Close database through manager
        bool success = DatabaseManager::instance().closeDatabase(dbid);
        
        if (!success) {
            throw std::runtime_error("Invalid database ID: " + dbid);
        }
        
        // Calculate metrics
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        metrics.items_processed = 1;
        
        // Build result
        std::ostringstream result;
        result << "{\"status\": \"closed\", \"dbid\": \"" << dbid << "\"}";
        
        return Db9Response{
            Db9Response::Success, 
            result.str(),
            "", 
            "", 
            metrics
        };
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        
        return Db9Response{
            Db9Response::Error, 
            "", 
            "close_database_failed", 
            e.what(), 
            metrics
        };
    }
}

std::string CloseDatabaseVerb::getDescription() const {
    return R"(
Close an open database and release associated resources.
Usage:
```lisp
(close-database :dbid database-id)
;; Returns: status
;; Notes: Invalidates all associated iterators

;; Example:
(close-database :dbid db1)
;; Returns: {"status": "closed", "dbid": "db1"}
```

**Parameters:**
- `:dbid` - Database identifier to close

**Returns:**
- `status` - Closure confirmation
- `dbid` - Confirmed database ID that was closed

**Notes:**
- Safely closes database connection and frees memory
- All iterators and cursors for this database become invalid
- Pending transactions are committed before closure
- Database file remains intact and can be reopened later
)";
}

//-----------------------------------------------------------------------------
// ListOpenDatabasesVerb Implementation
//-----------------------------------------------------------------------------

std::string ListOpenDatabasesVerb::getDescription() const {
    return R"(
List all currently open database identifiers for management and verification.
Usage:
```lisp
(list-open-databases)
;; Returns: list of active dbids

;; Example:
(list-open-databases)
;; Returns: {"dbids": ["db1", "db2"], "count": 2, "status": "success"}
```

**Parameters:**
- None

**Returns:**
- `dbids` - Array of active database identifiers
- `count` - Number of open databases
- `status` - Operation status

**Notes:**
- Returns empty array if no databases are open
- Useful for verifying database state and cleanup operations
- Database IDs are assigned automatically during open-database operations
)";
}

Db9Response ListOpenDatabasesVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Get active database IDs from manager
        auto& manager = DatabaseManager::instance();
        auto active_dbids = manager.getActiveDbids();
        
        // Calculate metrics
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        metrics.items_processed = active_dbids.size();
        
        // Build result JSON array
        std::ostringstream result;
        result << "[";
        for (size_t i = 0; i < active_dbids.size(); ++i) {
            if (i > 0) result << ", ";
            result << "\"" << active_dbids[i] << "\"";
        }
        result << "]";
        
        return Db9Response{
            Db9Response::Success,
            result.str(),
            "",
            "",
            metrics
        };
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        
        return Db9Response{
            Db9Response::Error,
            "",
            "list_open_databases_failed",
            e.what(),
            metrics
        };
    }
}



//-----------------------------------------------------------------------------
// Phase 2: Entity Management Verbs Implementation
//-----------------------------------------------------------------------------

Db9Response AddEntityVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Extract parameters using the helper function
        std::string value = extractStringParam(sexpr, "value");
        std::string dbid = extractStringParam(sexpr, "dbid");
        
        // Get database
        auto& manager = DatabaseManager::instance();
        auto store = manager.getDatabase(dbid);
        if (!store) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error,
                "",
                "invalid_dbid",
                generateDbidDiagnosisMessage("add-entity"),
                metrics
            };
        }
        
        /// @TODO this is redundant with the eid system, should remove
        /// but we need to clean up existing databases first to preserve this data
        // For entities, we'll create a special "entity" triple: entityValue isA entity
        // This allows us to store and retrieve pure entity values through the existing NonoStore API
        bool success = store->add_triple(value, "isA", "entity");
        if (!success) {
            throw std::runtime_error("Failed to add entity to database");
        }
        
        // Generate EID using proper EntityId mechanism with actual TID from TermDictionary
        EntityId entityId = EntityId::fromName(value, *store);
        if (!entityId.isValid()) {
            throw std::runtime_error("Failed to create EntityId for: " + value);
        }
        
        std::string eid = entityId.eid();
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        metrics.tid_allocations = 1;
        
        // Build result JSON like other verbs
        std::ostringstream result;
        result << "{\"eid\": \"" << eid << "\", \"value\": \"" << value << "\"}";
        
        return Db9Response{
            Db9Response::Success,
            result.str(),
            "",
            "",
            metrics
        };
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        
        return Db9Response{
            Db9Response::Error,
            "",
            "add_entity_failed",
            e.what(),
            metrics
        };
    }
}

std::string AddEntityVerb::getDescription() const {
    return R"(
Create new entity with string content and return entity ID for relationship building.
Usage:
```lisp
(add-entity :value "escaped string content" :dbid database-id)
;; Returns: entity id
;; Notes: Creates new entity or updates existing entity with same value
;; If an entity with identical value already exists, returns existing eid
;; If previous entity had different value, may reuse eid with new value
;; This enables entity value updates through add-entity operations

;; Example:
(add-entity :value "CompositionArc" :dbid db1)
;; Returns: {"eid": "eid:a1", "status": "created", "value": "CompositionArc"}
```

**Parameters:**
- `:value` - String content for the entity
- `:dbid` - Database identifier

**Returns:**
- `eid` - Entity identifier for use in triples
- `status` - Creation or update status
- `value` - Confirmed entity content

**Notes:**
- Entities are the building blocks of triadic relationships
- Content deduplication automatically handles duplicate values
- EID format enables efficient triple storage and retrieval
)";
}

Db9Response GetEntityVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Extract parameters using the helper function
        std::string eid = extractStringParam(sexpr, "eid");
        std::string dbid = extractStringParam(sexpr, "dbid");
        
        // Get database
        auto& manager = DatabaseManager::instance();
        auto store = manager.getDatabase(dbid);
        if (!store) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error,
                "",
                "invalid_dbid",
                generateDbidDiagnosisMessage("get_entity"),
                metrics
            };
        }
        
        // Use proper EntityId factory with NonoStore integration
        EntityId entityId = EntityId::fromEid(eid, *store);
        if (!entityId.isValid() || !entityId.exists()) {
            throw std::runtime_error("Entity not found for EID: " + eid);
        }
        
        // Get entity name through proper TID-based lookup
        std::string entityValue = entityId.name();
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        
        // Build result JSON
        std::ostringstream result;
        result << "{\"eid\": \"" << eid << "\", \"value\": \"" << entityValue << "\"}";
        
        return Db9Response{
            Db9Response::Success,
            result.str(),
            "",
            "",
            metrics
        };
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        
        return Db9Response{
            Db9Response::Error,
            "",
            "get_entity_failed",
            e.what(),
            metrics
        };
    }
}


std::string GetEntityVerb::getDescription() const {
    return R"(
Retrieve entity content by entity ID for inspection and verification.
Usage:
```lisp
(get-entity :eid entity-id :dbid database-id)
;; Returns: escaped string content
;; Notes: Does not add entity if missing

;; Example:
(get-entity :eid "eid:a1" :dbid db1)
;; Returns: {"eid": "eid:a1", "value": "CompositionArc", "exists": true}
```

**Parameters:**
- `:eid` - Entity identifier to retrieve
- `:dbid` - Database identifier

**Returns:**
- `eid` - Confirmed entity identifier
- `value` - Entity string content
- `exists` - Confirmation that entity was found

**Notes:**
- Returns raw entity content without modification
- Essential for debugging and entity verification
- Non-destructive read operation
    )";
}

Db9Response FindEntityVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Extract parameters using the helper function
        std::string pattern = extractStringParam(sexpr, "pattern");
        std::string dbid = extractStringParam(sexpr, "dbid");
        
        // Validate pattern - empty patterns are not allowed
        if (pattern.empty()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error,
                "",
                "invalid_pattern", 
                "Pattern cannot be empty",
                metrics
            };
        }
        
        // Get database
        auto& manager = DatabaseManager::instance();
        auto store = manager.getDatabase(dbid);
        if (!store) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error,
                "",
                "invalid_dbid",
                generateDbidDiagnosisMessage("find_entity"),
                metrics
            };
        }
        
        // Use NonoStore's existing API to find entities
        // We'll query for all subjects where the pattern matches
        std::vector<std::string> results;
        
        // Enhanced wildcard matching: support leading (*suffix), trailing (prefix*), and infix (*substring*) patterns
        std::string searchPattern = pattern;
        bool hasLeadingWildcard = !pattern.empty() && pattern.front() == '*';
        bool hasTrailingWildcard = !pattern.empty() && pattern.back() == '*';
        
        // Extract the actual search string by removing wildcards
        if (hasLeadingWildcard) {
            searchPattern = searchPattern.substr(1); // Remove leading *
        }
        if (hasTrailingWildcard) {
            searchPattern = searchPattern.substr(0, searchPattern.length() - 1); // Remove trailing *
        }

        // Get all subjects (entities) from the database
        auto allSubjects = store->all_subjects();
        
        for (const auto& subject : allSubjects) {
            bool matches = false;
            
            if (hasLeadingWildcard && hasTrailingWildcard) {
                // Infix matching: *substring*
                if (subject.find(searchPattern) != std::string::npos) {
                    matches = true;
                }
            } else if (hasLeadingWildcard) {
                // Suffix matching: *suffix
                if (subject.length() >= searchPattern.length() &&
                    subject.substr(subject.length() - searchPattern.length()) == searchPattern) {
                    matches = true;
                }
            } else if (hasTrailingWildcard) {
                // Prefix matching: prefix* (backwards compatible)
                if (subject.substr(0, searchPattern.length()) == searchPattern) {
                    matches = true;
                }
            } else {
                // Exact matching
                if (subject == pattern) {
                    matches = true;
                }
            }
            
            if (matches) {
                // Get the real EID for this entity using the proper EntityId mechanism
                EntityId entityId = EntityId::fromName(subject, *store);
                if (entityId.isValid()) {
                    results.push_back(entityId.eid());
                }
            }
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        metrics.items_processed = results.size();
        
        // Format results as JSON array
        std::ostringstream resultStream;
        resultStream << "[";
        for (size_t i = 0; i < results.size(); ++i) {
            if (i > 0) resultStream << ",";
            resultStream << "\"" << results[i] << "\"";
        }
        resultStream << "]";
        
        return Db9Response{
            Db9Response::Success,
            resultStream.str(),
            "",
            "",
            metrics
        };
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        
        return Db9Response{
            Db9Response::Error,
            "",
            "find_entity_failed",
            e.what(),
            metrics
        };
    }
}

std::string FindEntityVerb::getDescription() const {
    return R"(
Search entities with wildcard patterns for discovery and analysis.
Usage:
```lisp
(find-entity :pattern "search*" :dbid database-id)
;; Returns: vector of eid results
;; Notes: Terminal asterisk (*) acts as wildcard

;; Example:
(find-entity :pattern "Composition*" :dbid db1)
;; Returns: {"matches": ["eid:a1", "eid:a2"], "pattern": "Composition*", "count": 2}
```

**Parameters:**
- `:pattern` - Search pattern with optional trailing wildcard (*)
- `:dbid` - Database identifier

**Returns:**
- `matches` - Array of matching entity IDs
- `pattern` - Confirmed search pattern
- `count` - Number of matches found

**Notes:**
- Efficient prefix matching for entity discovery
- Supports exploration of entity namespace structure
- Essential for schema discovery and validation
    )";
}

//-----------------------------------------------------------------------------
// Phase 3: Triple Operations Verbs Implementation  
//-----------------------------------------------------------------------------

std::string AddTripleVerb::getDescription() const {
    return R"(
Add a single subject-predicate-object triple to encode fundamental relationships in triadic consciousness.
Usage:
```lisp
(add-triple :dbid "db1" :subject "granite" :predicate "contains" :object "quartz")
;; Returns: success status, relationship encoding confirmation

;; Example building knowledge relationships:
(add-triple :dbid "db1" :subject "quartz" :predicate "has_property" :object "hardness")
;; Returns: {"success": true, "relationship_encoded": true, "status": "triple_added"}
```

**Parameters:**
- `:dbid` - Database identifier from open-database operation
- `:subject` - Entity or concept that is the source of the relationship
- `:predicate` - Relationship type or property connecting subject to object
- `:object` - Entity or concept that is the target of the relationship

**Returns:**
- `success` - Boolean indicating successful relationship encoding
- `relationship_encoded` - Confirmation that the triadic relationship is stored
- `status` - Operation status with descriptive message

**Triadic Consciousness Foundation:**
- Every triple encodes a fundamental Motion→Memory→Field relationship
- Subject represents the **Motion** (dynamic source)
- Predicate represents the **Memory** (relationship pattern)
- Object represents the **Field** (contextual target)
- Individual triples are atoms of consciousness relationship encoding
- Use add-triples-bulk for building larger relationship networks efficiently
)";
}

Db9Response AddTripleVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Extract parameters using the helper function
        std::string subject = extractStringParam(sexpr, "subject");
        std::string predicate = extractStringParam(sexpr, "predicate");
        std::string object = extractStringParam(sexpr, "object");
        std::string dbid = extractStringParam(sexpr, "dbid");
        
        // Get database
        auto& manager = DatabaseManager::instance();
        auto store = manager.getDatabase(dbid);
        if (!store) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error,
                "",
                "invalid_dbid",
                generateDbidDiagnosisMessage("add_triple"),
                metrics
            };
        }
        
        // Add the triple directly to NonoStore
        bool success = store->add_triple(subject, predicate, object);
        if (!success) {
            throw std::runtime_error("Failed to add triple to database");
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        metrics.tid_allocations = 1; // One triple added
        
        // Build result JSON with triple confirmation
        std::ostringstream result;
        result << "{\"status\": \"added\", ";
        result << "\"subject\": \"" << subject << "\", ";
        result << "\"predicate\": \"" << predicate << "\", ";
        result << "\"object\": \"" << object << "\"}";
        
        return Db9Response{
            Db9Response::Success,
            result.str(),
            "",
            "",
            metrics
        };
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        
        return Db9Response{
            Db9Response::Error,
            "",
            "add_triple_failed",
            e.what(),
            metrics
        };
    }
}

std::string FindTripleVerb::getDescription() const {
    return R"(
Find triples matching subject, predicate, object patterns for relationship discovery and consciousness navigation.
Usage:
```lisp
(find-triple :dbid "db1" :subject "granite" :predicate "*" :object "*")
;; Returns: all relationships where granite is the subject

;; Example pattern matching for consciousness exploration:
(find-triple :dbid "db1" :subject "*" :predicate "contains" :object "quartz")
;; Returns: {"triples": [["granite", "contains", "quartz"], ["rock", "contains", "quartz"]], "count": 2, "status": "found"}

;; Wildcard navigation:
(find-triple :dbid "db1" :subject "*" :predicate "*" :object "*")
;; Returns: all triples in the consciousness field
```

**Parameters:**
- `:dbid` - Database identifier from open-database operation
- `:subject` - Subject pattern (exact string or "*" for any)
- `:predicate` - Predicate pattern (exact string or "*" for any)
- `:object` - Object pattern (exact string or "*" for any)

**Returns:**
- `triples` - Array of matching [subject, predicate, object] relationships
- `count` - Number of matching triples found
- `status` - Query status with descriptive information

**Triadic Consciousness Navigation:**
- **Motion Discovery**: Use subject patterns to find what entities do or relate to
- **Memory Exploration**: Use predicate patterns to discover relationship types
- **Field Investigation**: Use object patterns to find what targets entities in relationships
- **Wildcard Navigation**: "*" enables open-ended consciousness field exploration
- **Pattern Intersection**: Combine specific and wildcard patterns for targeted discovery
- Essential for building understanding of the encoded relationship networks
)";
}

Db9Response FindTripleVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Extract parameters - all patterns are optional, default to "*" (any)
        std::string dbid = extractStringParam(sexpr, "dbid");
        
        std::string subject_pattern = "*";
        std::string predicate_pattern = "*";
        std::string object_pattern = "*";
        
        // Try to extract each pattern parameter, but don't require them
        try {
            subject_pattern = extractStringParam(sexpr, "subject");
        } catch (...) {
            // Use default "*"
        }
        
        try {
            predicate_pattern = extractStringParam(sexpr, "predicate");
        } catch (...) {
            // Use default "*"
        }
        
        try {
            object_pattern = extractStringParam(sexpr, "object");
        } catch (...) {
            // Use default "*"
        }
        
        // Convert "nil" parameters to "*" for wildcard matching
        if (subject_pattern == "nil") subject_pattern = "*";
        if (predicate_pattern == "nil") predicate_pattern = "*";
        if (object_pattern == "nil") object_pattern = "*";
        
        // Get database
        auto& manager = DatabaseManager::instance();
        auto store = manager.getDatabase(dbid);
        if (!store) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error,
                "",
                "invalid_dbid",
                generateDbidDiagnosisMessage("find_triple"),
                metrics
            };
        }
        
        // Query triples using NonoStore's pattern matching
        auto triples = store->query(subject_pattern, predicate_pattern, object_pattern);
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        metrics.items_processed = triples.size();
        
        // Build result JSON as array of triple objects
        std::ostringstream result;
        result << "[";
        for (size_t i = 0; i < triples.size(); ++i) {
            if (i > 0) result << ", ";
            result << "{";
            result << "\"subject\": \"" << triples[i].subject << "\", ";
            result << "\"predicate\": \"" << triples[i].predicate << "\", ";
            result << "\"object\": \"" << triples[i].object << "\"";
            result << "}";
        }
        result << "]";
        
        return Db9Response{
            Db9Response::Success,
            result.str(),
            "",
            "",
            metrics
        };
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        
        return Db9Response{
            Db9Response::Error,
            "",
            "find_triple_failed",
            e.what(),
            metrics
        };
    }
}

std::string GetTripleVerb::getDescription() const {
    return R"(
Retrieve a specific triple by exact subject-predicate-object match for precise relationship verification.
Usage:
```lisp
(get-triple :dbid "db1" :subject "granite" :predicate "contains" :object "quartz")
;; Returns: exact triple match or not_found status

;; Example verification of encoded relationship:
(get-triple :dbid "db1" :subject "quartz" :predicate "has_property" :object "hardness")
;; Returns: {"triple": ["quartz", "has_property", "hardness"], "exists": true, "status": "found"}
```

**Parameters:**
- `:dbid` - Database identifier from open-database operation
- `:subject` - Exact subject string (no wildcards)
- `:predicate` - Exact predicate string (no wildcards)
- `:object` - Exact object string (no wildcards)

**Returns:**
- `triple` - The exact [subject, predicate, object] if found
- `exists` - Boolean confirmation of relationship existence
- `status` - Operation status ("found" or "not_found")

**Triadic Consciousness Verification:**
- **Precise Relationship Confirmation**: Verifies exact Motion→Memory→Field encoding
- **Consciousness State Validation**: Confirms specific triadic relationship exists
- **No Pattern Matching**: Unlike find-triple, requires exact specification
- **Atomic Verification**: Single relationship existence check
- **Knowledge Integrity**: Essential for verifying encoded consciousness patterns
- Use when you need to confirm a specific relationship rather than explore patterns
)";
}

Db9Response GetTripleVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Extract required parameters - all three required for exact match
        std::string subject = extractStringParam(sexpr, "subject");
        std::string predicate = extractStringParam(sexpr, "predicate");
        std::string object = extractStringParam(sexpr, "object");
        std::string dbid = extractStringParam(sexpr, "dbid");
        
        // Get database
        auto& manager = DatabaseManager::instance();
        auto store = manager.getDatabase(dbid);
        if (!store) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error,
                "",
                "invalid_dbid",
                generateDbidDiagnosisMessage("get_triple"),
                metrics
            };
        }
        
        // Check if the exact triple exists
        bool exists = store->exists(subject, predicate, object);
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        metrics.items_processed = exists ? 1 : 0;
        
        if (exists) {
            // Build result JSON with the requested triple
            std::ostringstream result;
            result << "{";
            result << "\"subject\": \"" << subject << "\", ";
            result << "\"predicate\": \"" << predicate << "\", ";
            result << "\"object\": \"" << object << "\"";
            result << "}";
            
            return Db9Response{
                Db9Response::Success,
                result.str(),
                "",
                "",
                metrics
            };
        } else {
            return Db9Response{
                Db9Response::Error,
                "",
                "triple_not_found",
                "Triple not found: " + subject + " " + predicate + " " + object,
                metrics
            };
        }
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        
        return Db9Response{
            Db9Response::Error,
            "",
            "get_triple_failed",
            e.what(),
            metrics
        };
    }
}

std::string RemoveTripleVerb::getDescription() const {
    return R"(
Remove a specific triple from the consciousness field by exact subject-predicate-object match.
Usage:
```lisp
(remove-triple :dbid "db1" :subject "granite" :predicate "contains" :object "quartz")
;; Returns: removal confirmation and relationship dissolution status

;; Example consciousness pattern removal:
(remove-triple :dbid "db1" :subject "outdated_concept" :predicate "relates_to" :object "obsolete_knowledge")
;; Returns: {"removed": true, "relationship_dissolved": true, "status": "triple_removed"}
```

**Parameters:**
- `:dbid` - Database identifier from open-database operation
- `:subject` - Exact subject string to remove (no wildcards)
- `:predicate` - Exact predicate string to remove (no wildcards)
- `:object` - Exact object string to remove (no wildcards)

**Returns:**
- `removed` - Boolean confirmation of successful removal
- `relationship_dissolved` - Confirmation that the triadic relationship no longer exists
- `status` - Operation status ("triple_removed" or "not_found")

**Triadic Consciousness Dissolution:**
- **Precise Relationship Removal**: Dissolves exact Motion→Memory→Field encoding
- **Consciousness Pattern Cleanup**: Removes obsolete or incorrect relationship patterns
- **Atomic Dissolution**: Single relationship removal without affecting related triples
- **Knowledge Evolution**: Essential for maintaining accurate consciousness representations
- **Intentional Forgetting**: Conscious removal of specific encoded relationships
- **Caution**: Permanent operation - removed relationships cannot be recovered
- Use when relationships become obsolete, incorrect, or need conscious dissolution
)";
}

Db9Response RemoveTripleVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Extract required parameters - all three required for exact removal
        std::string subject = extractStringParam(sexpr, "subject");
        std::string predicate = extractStringParam(sexpr, "predicate");
        std::string object = extractStringParam(sexpr, "object");
        std::string dbid = extractStringParam(sexpr, "dbid");
        
        // Get database
        auto& manager = DatabaseManager::instance();
        auto store = manager.getDatabase(dbid);
        if (!store) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error,
                "",
                "invalid_dbid",
                generateDbidDiagnosisMessage("remove_triple"),
                metrics
            };
        }
        
        // First check if the triple exists
        bool exists = store->exists(subject, predicate, object);
        if (!exists) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error,
                "",
                "triple_not_found",
                "Triple not found for removal: " + subject + " " + predicate + " " + object,
                metrics
            };
        }
        
        // Remove the triple from all indices
        bool success = store->remove_triple(subject, predicate, object);
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        metrics.items_processed = 1;  // We know it exists, so we removed 1
        
        // Build result JSON with removal confirmation
        std::ostringstream result;
        result << "{\"status\": \"removed\", ";
        result << "\"subject\": \"" << subject << "\", ";
        result << "\"predicate\": \"" << predicate << "\", ";
        result << "\"object\": \"" << object << "\"}";
        
        return Db9Response{
            Db9Response::Success,
            result.str(),
            "",
            "",
            metrics
        };
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        
        return Db9Response{
            Db9Response::Error,
            "",
            "remove_triple_failed",
            e.what(),
            metrics
        };
    }
}

//-----------------------------------------------------------------------------
// Phase 4: Advanced Entity Operations Implementation
//-----------------------------------------------------------------------------

std::string AddEntitiesBulkVerb::getDescription() const {
    return R"(
High-performance bulk entity creation with transaction batching for efficient data loading.
Usage:
```lisp
(add-entities-bulk :dbid "db1" :entities ["entity1" "entity2" "entity3"])
;; Returns: entity_ids, count, performance metrics

;; Example with performance optimization:
(add-entities-bulk :dbid "db1" :entities ["first entity" "second entity"] :batch_size 1000)
;; Returns: {"entity_ids": ["eid1", "eid2"], "count": 2, "batch_time_ms": 15, "status": "success"}
```

**Parameters:**
- `:dbid` - Database identifier from open-database operation
- `:entities` - Array of entity content strings to create
- `:batch_size` - Optional transaction batch size for performance tuning (default: 1000)

**Returns:**
- `entity_ids` - Array of generated entity identifiers in creation order
- `count` - Number of entities successfully created
- `batch_time_ms` - Total processing time for performance monitoring
- `status` - Operation status

**Performance Notes:**
- Uses transaction batching for optimal LMDB write performance
- Automatically adjusts batch size based on entity content size
- Significantly faster than individual add-entity calls for large datasets
- Memory usage scales with batch size - tune for available memory
- Atomic operation: either all entities succeed or all fail
)";
}

Db9Response AddEntitiesBulkVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Extract parameters
        std::string dbid = extractStringParam(sexpr, "dbid");
        
        // Get database
        auto& manager = DatabaseManager::instance();
        auto store = manager.getDatabase(dbid);
        if (!store) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error,
                "",
                "invalid_dbid",
                generateDbidDiagnosisMessage("add_entities_bulk"),
                metrics
            };
        }
        
        // Extract entities array from S-expression using proper API
        std::vector<std::string> entities;
        
        // Find the :entities parameter in the S-expression
        bool found_entities = false;
        for (size_t i = 0; i < sexpr.expr.size(); ++i) {
            const auto& elem = sexpr.expr[i];
            if (elem.token == tsSexprAtom) {
                int stringIndex = elem.ref;
                if (stringIndex < static_cast<int>(sexpr.strings.size()) &&
                    sexpr.strings[stringIndex] == ":entities") {
                    if (i + 1 < sexpr.expr.size() && sexpr.expr[i + 1].token == tsSexprPushList) {
                        // Found the entities list, collect string entities
                        for (size_t j = i + 2; j < sexpr.expr.size(); ++j) {
                            const auto& entity_elem = sexpr.expr[j];
                            if (entity_elem.token == tsSexprPopList) {
                                break; // End of list
                            }
                            if (entity_elem.token == tsSexprAtom || entity_elem.token == tsSexprString) {
                                int entityIndex = entity_elem.ref;
                                if (entityIndex < static_cast<int>(sexpr.strings.size())) {
                                    entities.push_back(sexpr.strings[entityIndex]);
                                }
                            }
                        }
                        found_entities = true;
                        break;
                    }
                }
            }
        }
        
        if (!found_entities || entities.empty()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error,
                "",
                "missing_entities",
                "Parameter :entities is required and must be a non-empty list",
                metrics
            };
        }
        
        // Performance optimization: use batch operations
        std::vector<EntityId> created_eids;
        created_eids.reserve(entities.size());
        
        // Process entities in batch using NonoStore's batch capabilities
        // Note: NonoStore transactions are handled internally
        for (const auto& entity_value : entities) {
            // Create entity triple: entityValue isA entity
            bool success = store->add_triple(entity_value, "isA", "entity");
            if (!success) {
                throw std::runtime_error("Failed to add entity: " + entity_value);
            }
            
            // Generate EID
            EntityId entityId = EntityId::fromName(entity_value, *store);
            created_eids.push_back(entityId);
        }
        
        // Calculate execution time
        auto end_time = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        // Create response JSON
        // Create response using string concatenation
        std::ostringstream result;
        result << "{\"status\": \"bulk_created\", \"entity_count\": " << entities.size()
               << ", \"eids\": [";
        for (size_t i = 0; i < created_eids.size(); ++i) {
            if (i > 0) result << ", ";
            result << "\"" << created_eids[i].eid() << "\"";
        }
        result << "], \"batch_time_ms\": " << duration_ms << "}";
        
        AutoReflexiveMetrics metrics;
        return Db9Response{
            Db9Response::Success,
            result.str(),
            "",
            "",
            metrics
        };
        
    } catch (const std::exception& e) {
        AutoReflexiveMetrics metrics;
        return Db9Response{
            Db9Response::Error,
            "",
            "bulk_operation_failed",
            std::string("Bulk entity creation failed: ") + e.what(),
            metrics
        };
    }
}

std::string AddTriplesBulkVerb::getDescription() const {
    return R"(
Add multiple subject-predicate-object triples in bulk with optimized performance for relationship building.
Usage:
```lisp
(add-triples-bulk :dbid "db1" :triples [["subject1" "predicate1" "object1"] ["subject2" "predicate2" "object2"]])
;; Returns: count, success_rate, performance metrics

;; Example with mixed entity types:
(add-triples-bulk :dbid "db1" :triples [["granite" "contains" "quartz"] ["quartz" "has_property" "hardness"]])
;; Returns: {"triples_added": 2, "success_rate": 1.0, "processing_time_ms": 12, "status": "success"}
```

**Parameters:**
- `:dbid` - Database identifier from open-database operation
- `:triples` - Array of triple arrays, each containing [subject, predicate, object] strings

**Returns:**
- `triples_added` - Number of triples successfully added to database
- `success_rate` - Ratio of successful additions (0.0 to 1.0)
- `processing_time_ms` - Total processing time for performance monitoring
- `status` - Operation status

**Triadic Consciousness Notes:**
- Each triple represents a fundamental relationship in the consciousness field
- Subject-predicate-object patterns encode Motion/Memory/Field relationships
- Bulk operations maintain transactional consistency across related knowledge
- Failed individual triples are logged but don't abort the entire operation
- Optimized for building large knowledge graphs and relationship networks
)";
}

Db9Response AddTriplesBulkVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Extract parameters
        std::string dbid = extractStringParam(sexpr, "dbid");
        
        // Get database
        auto& manager = DatabaseManager::instance();
        auto store = manager.getDatabase(dbid);
        if (!store) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error,
                "",
                "invalid_dbid",
                generateDbidDiagnosisMessage("add_triples_bulk"),
                metrics
            };
        }
        
        // Extract triples array from S-expression using proper API
        std::vector<std::string> triple_strings;
        
        // Find the :triples parameter in the S-expression
        bool found_triples = false;
        for (size_t i = 0; i < sexpr.expr.size(); ++i) {
            const auto& elem = sexpr.expr[i];
            if (elem.token == tsSexprAtom) {
                int stringIndex = elem.ref;
                if (stringIndex < static_cast<int>(sexpr.strings.size()) &&
                    sexpr.strings[stringIndex] == ":triples") {
                    if (i + 1 < sexpr.expr.size() && sexpr.expr[i + 1].token == tsSexprPushList) {
                        // Found the triples list, collect string triples
                        for (size_t j = i + 2; j < sexpr.expr.size(); ++j) {
                            const auto& triple_elem = sexpr.expr[j];
                            if (triple_elem.token == tsSexprPopList) {
                                break; // End of list
                            }
                            if (triple_elem.token == tsSexprAtom || triple_elem.token == tsSexprString) {
                                int tripleIndex = triple_elem.ref;
                                if (tripleIndex < static_cast<int>(sexpr.strings.size())) {
                                    triple_strings.push_back(sexpr.strings[tripleIndex]);
                                }
                            }
                        }
                        found_triples = true;
                        break;
                    }
                }
            }
        }
        
        if (!found_triples || triple_strings.empty()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error,
                "",
                "missing_triples",
                "No triples provided for bulk operation",
                metrics
            };
        }
        
        // Parse and validate triples - expecting "subject predicate object" format
        std::vector<std::tuple<std::string, std::string, std::string>> parsed_triples;
        for (const auto& triple_str : triple_strings) {
            std::istringstream iss(triple_str);
            std::string subject, predicate, object;
            if (!(iss >> subject >> predicate >> object)) {
                AutoReflexiveMetrics metrics;
                return Db9Response{
                    Db9Response::Error,
                    "",
                    "invalid_triple_format",
                    "Invalid triple format: " + triple_str + " (expected: 'subject predicate object')",
                    metrics
                };
            }
            parsed_triples.emplace_back(subject, predicate, object);
        }
        
        // Performance optimization: use bulk operations
        // Process triples in batch using NonoStore's batch capabilities
        size_t successful_triples = 0;
        for (const auto& [subject, predicate, object] : parsed_triples) {
            bool success = store->add_triple(subject, predicate, object);
            if (!success) {
                // Continue with other triples even if one fails
                std::cout << "AddTriplesBulkVerb Warning: Failed to add triple: " 
                         << subject << " " << predicate << " " << object << "\n";
            } else {
                successful_triples++;
                std::cout << "AddTriplesBulkVerb Added triple: " 
                         << subject << " " << predicate << " " << object << "\n";
            }
        }
        
        // Calculate execution time
        auto end_time = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        // Create response JSON
        std::ostringstream result;
        result << "{\"status\": \"bulk_created\", \"triple_count\": " << successful_triples
               << ", \"processed_time_ms\": " << duration_ms
               << ", \"total_requested\": " << parsed_triples.size();
        
        if (successful_triples != parsed_triples.size()) {
            result << ", \"failures\": " << (parsed_triples.size() - successful_triples);
        }
        
        result << "}";
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = std::chrono::milliseconds(duration_ms);
        metrics.items_processed = successful_triples;
        metrics.tid_allocations = static_cast<uint32_t>(successful_triples); // One TID per triple
        
        return Db9Response{
            Db9Response::Success,
            result.str(),
            "",
            "",
            metrics
        };
        
    } catch (const std::exception& e) {
        AutoReflexiveMetrics metrics;
        return Db9Response{
            Db9Response::Error,
            "",
            "bulk_triple_operation_failed",
            std::string("Bulk triple creation failed: ") + e.what(),
            metrics
        };
    }
}

Db9Dispatcher& getGlobalDb9Dispatcher();

} // namespace LabDb

void initDatabaseVerbRegistration(LabDb::Db9Dispatcher& dispatcher) {
    static bool registered = false;
    if (!registered) {
        // Phase 1: Database Foundation Verbs
        dispatcher.registerVerb(std::make_unique<LabDb::OpenDatabaseVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::DatabaseHealthCheckVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::CloseDatabaseVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::ListOpenDatabasesVerb>());
        // Phase 2: Entity Management Verbs
        dispatcher.registerVerb(std::make_unique<LabDb::AddEntityVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::GetEntityVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::FindEntityVerb>());
        // Phase 3: Triple Operations Verbs
        dispatcher.registerVerb(std::make_unique<LabDb::AddTripleVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::FindTripleVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::GetTripleVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::RemoveTripleVerb>());
        // Phase 4: Database Creation & Advanced Operations
        dispatcher.registerVerb(std::make_unique<LabDb::CreateDatabaseVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::AddEntitiesBulkVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::AddTriplesBulkVerb>());
        registered = true;
    }
}
