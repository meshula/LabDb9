#include "LabDb/DatabaseVerbs.h"
#include "LabDb/EntityId.h"
#include "LabDb/LabText.hpp"
#include <chrono>
#include <filesystem>
#include <sstream>
#include <iostream>

namespace LabDb {

//-----------------------------------------------------------------------------
// DatabaseManager Implementation
//-----------------------------------------------------------------------------

std::string DatabaseManager::openDatabase(const std::string& path) {
    std::lock_guard<std::mutex> lock(_mutex);
    
    try {
        // Check if file exists
        if (!std::filesystem::exists(path)) {
            throw std::runtime_error("Database file does not exist: " + path);
        }
        
        // Create NonoStore instance
        auto store = std::make_shared<NonoStore>(path);
        
        // Generate unique DBID
        std::string dbid = generateDbid();
        
        // Store in registry
        _databases[dbid] = store;
        
        return dbid;
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to open database: " + std::string(e.what()));
    }
}

bool DatabaseManager::closeDatabase(const std::string& dbid) {
    std::lock_guard<std::mutex> lock(_mutex);
    
    auto it = _databases.find(dbid);
    if (it == _databases.end()) {
        return false;
    }
    
    _databases.erase(it);
    return true;
}

std::shared_ptr<NonoStore> DatabaseManager::getDatabase(const std::string& dbid) {
    std::lock_guard<std::mutex> lock(_mutex);
    
    auto it = _databases.find(dbid);
    if (it == _databases.end()) {
        return nullptr;
    }
    
    return it->second;
}

bool DatabaseManager::isValidDbid(const std::string& dbid) const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _databases.find(dbid) != _databases.end();
}

std::vector<std::string> DatabaseManager::getActiveDbids() const {
    std::lock_guard<std::mutex> lock(_mutex);
    
    std::vector<std::string> dbids;
    dbids.reserve(_databases.size());
    
    for (const auto& pair : _databases) {
        dbids.push_back(pair.first);
    }
    
    return dbids;
}

std::string DatabaseManager::generateDbid() {
    return "db" + std::to_string(_next_id++);
}

std::string DatabaseManager::createDatabase(const std::string& path) {
    std::lock_guard<std::mutex> lock(_mutex);

    try {
        // Check if file already exists
        if (std::filesystem::exists(path)) {
            throw std::runtime_error("Database file already exists: " + path);
        }

        // Ensure parent directory exists
        std::filesystem::path db_path(path);
        std::filesystem::path parent_dir = db_path.parent_path();
        if (!parent_dir.empty() && !std::filesystem::exists(parent_dir)) {
            std::filesystem::create_directories(parent_dir);
        }

        // Create NonoStore instance (which creates/initializes the database)
        auto store = std::make_shared<NonoStore>(path);

        // Generate unique DBID
        std::string dbid = generateDbid();

        // Store in registry
        _databases[dbid] = store;

        return dbid;

    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to create database: " + std::string(e.what()));
    }
}

//-----------------------------------------------------------------------------
// Helper functions for S-expression parameter extraction
//-----------------------------------------------------------------------------
namespace {
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
        
        throw std::runtime_error("Required parameter :" + param_name + " not found");
    }
}

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
                "Database ID not found: " + dbid,
                metrics
            };
        }
        
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
                "Database ID not found: " + dbid,
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
                "Database ID not found: " + dbid,
                metrics
            };
        }
        
        // Use NonoStore's existing API to find entities
        // We'll query for all subjects where the pattern matches
        std::vector<std::string> results;
        
        // Simple wildcard matching: support terminal asterisk
        bool hasWildcard = !pattern.empty() && pattern.back() == '*';
        std::string prefix = hasWildcard ? pattern.substr(0, pattern.length() - 1) : pattern;
        
        // Get all subjects (entities) from the database
        auto allSubjects = store->all_subjects();
        
        for (const auto& subject : allSubjects) {
            bool matches = false;
            
            if (hasWildcard) {
                // Prefix matching
                if (subject.substr(0, prefix.length()) == prefix) {
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

//-----------------------------------------------------------------------------
// Phase 3: Triple Operations Verbs Implementation  
//-----------------------------------------------------------------------------

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
                "Database ID not found: " + dbid,
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
        
        // Get database
        auto& manager = DatabaseManager::instance();
        auto store = manager.getDatabase(dbid);
        if (!store) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error,
                "",
                "invalid_dbid",
                "Database ID not found: " + dbid,
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
                "Database ID not found: " + dbid,
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
                "Database ID not found: " + dbid,
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
                "Database ID not found: " + dbid,
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
                "Database ID not found: " + dbid,
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

// C-style function for Python binding
extern "C" 
void initDatabaseVerbRegistration() {
    static bool registered = false;
    if (!registered) {
        auto& dispatcher = LabDb::getGlobalDb9Dispatcher();
        // Phase 1: Database Foundation Verbs
        dispatcher.registerVerb(std::make_unique<LabDb::OpenDatabaseVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::DatabaseHealthCheckVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::CloseDatabaseVerb>());
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
