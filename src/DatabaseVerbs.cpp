#include "LabDb/DatabaseVerbs.h"
#include "LabDb/LabText.hpp"
#include <chrono>
#include <filesystem>
#include <sstream>

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
        
        // Generate entity ID (EID) - use a simple counter-based approach for now
        // In a real implementation, this would be based on the actual TermID
        static uint64_t entityCounter = 1;
        std::ostringstream eidStream;
        eidStream << "eid:" << std::hex << entityCounter++;
        std::string eid = eidStream.str();
        
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
        
        // For now, implement a simple mapping strategy:
        // Since AddEntityVerb stores triples as "entityValue isA entity", 
        // and generates EIDs like "eid:1", we need to map EIDs back to entity values.
        // This is a temporary solution - a real implementation would have proper EID management.
        
        // Extract the numeric part from the EID (e.g., "eid:1" -> "1")
        std::string eidNumeric = eid;
        if (eid.substr(0, 4) == "eid:") {
            eidNumeric = eid.substr(4);
        }
        
        // For this MVP, we'll use a simple approach:
        // Get all subjects that have "isA entity" relationship and return the Nth one
        auto allSubjects = store->all_subjects();
        std::vector<std::string> entityValues;
        
        for (const auto& subject : allSubjects) {
            if (store->exists(subject, "isA", "entity")) {
                entityValues.push_back(subject);
            }
        }
        
        // Convert EID to index (hex to decimal)
        size_t entityIndex = 0;
        try {
            entityIndex = std::stoul(eidNumeric, nullptr, 16);
            if (entityIndex < 1 || entityIndex > entityValues.size()) {
                throw std::runtime_error("Entity not found for EID: " + eid);
            }
            entityIndex--; // Convert to 0-based index
        } catch (const std::exception&) {
            throw std::runtime_error("Invalid EID format: " + eid);
        }
        
        std::string entityValue = entityValues[entityIndex];
        
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
            if (hasWildcard) {
                // Prefix matching
                if (subject.substr(0, prefix.length()) == prefix) {
                    // Generate EID - for now use simple encoding
                    static uint64_t tempCounter = 1000;
                    std::ostringstream eidStream;
                    eidStream << "eid:" << std::hex << (tempCounter++);
                    results.push_back(eidStream.str());
                }
            } else {
                // Exact matching
                if (subject == pattern) {
                    static uint64_t tempCounter = 1000;
                    std::ostringstream eidStream;
                    eidStream << "eid:" << std::hex << (tempCounter++);
                    results.push_back(eidStream.str());
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
        registered = true;
    }
}
