#include "LabDb/EnhancedDatabaseVerbs.h"
#include "LabDb/DatabaseVerbs.h"
#include "LabDb/EntityId.h"
#include "LabDb/LabText.hpp"
#include <chrono>
#include <sstream>
#include <iostream>
#include <fstream>
#include <unordered_set>

namespace LabDb {

//-----------------------------------------------------------------------------
// Helper function implementations (from DatabaseVerbs.cpp)
//-----------------------------------------------------------------------------

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
                        if (value_elem.token == tsSexprAtom || value_elem.token == tsSexprString) {
                            int valueIndex = value_elem.ref;
                            if (valueIndex < static_cast<int>(sexpr.strings.size())) {
                                return sexpr.strings[valueIndex];
                            }
                        }
                    }
                    break;
                }
            }
        }
    }
    
    return ""; // Parameter not found
}

// Implementation file for enhanced db9 API
// This provides rich object returns and dual-layer semantic/storage operations

//-----------------------------------------------------------------------------
// EID Chain Resolution for Enhanced Verbs
//-----------------------------------------------------------------------------

// EID chain resolution helper for enhanced verbs
struct EidResolutionResult {
    std::string final_eid;
    std::string final_value;
    std::vector<std::string> chain;
    bool had_cycles;
    bool had_indirection;
};

EidResolutionResult resolveEidChain(const std::string& start_eid, const std::string& start_value, 
                                   std::shared_ptr<NonoStore> store) {
    EidResolutionResult result;
    result.final_eid = start_eid;
    result.final_value = start_value;
    result.had_cycles = false;
    result.had_indirection = false;
    
    std::unordered_set<std::string> visited;
    std::string current_eid = start_eid;
    std::string current_value = start_value;
    
    // Add initial EID to chain
    result.chain.push_back(current_eid);
    visited.insert(current_eid);
    
    // DIAGNOSTIC: Write to file for debugging
    std::ofstream debug_file("/tmp/eid_resolution_debug.log", std::ios::app);
    debug_file << "🔍 EID Resolution: Starting with eid=" << start_eid << " value='" << start_value << "'" << std::endl;
    
    // Follow EID chain until we reach a non-EID value or detect cycle
    while (current_value.starts_with("eid:")) {
        debug_file << "🔗 EID Chain: Found EID value '" << current_value << "', attempting to resolve..." << std::endl;
        
        // Check for cycle
        if (visited.find(current_value) != visited.end()) {
            debug_file << "🚫 EID Chain: Cycle detected! " << current_value << " already visited" << std::endl;
            result.had_cycles = true;
            break;
        }
        
        // Try to resolve the EID value to the next entity
        EntityId nextEntity = EntityId::fromEid(current_value, *store);
        debug_file << "🏷️  EID Chain: EntityId lookup for '" << current_value << "' -> valid=" << nextEntity.isValid() << " exists=" << nextEntity.exists() << std::endl;
        
        if (!nextEntity.isValid() || !nextEntity.exists()) {
            // Dead end - value looks like EID but doesn't resolve
            debug_file << "💀 EID Chain: Dead end - '" << current_value << "' doesn't resolve to valid entity" << std::endl;
            break;
        }
        
        std::string next_value = nextEntity.name();
        debug_file << "✅ EID Chain: '" << current_value << "' resolves to '" << next_value << "'" << std::endl;
        
        // Follow the chain
        visited.insert(current_value);
        result.chain.push_back(current_value);
        current_eid = current_value;
        current_value = next_value;
        result.had_indirection = true;
        
        debug_file << "➡️  EID Chain: Moving to eid=" << current_eid << " value='" << current_value << "'" << std::endl;
    }
    
    result.final_eid = current_eid;
    result.final_value = current_value;
    
    debug_file << "🎯 EID Resolution: Final result eid=" << result.final_eid << " value='" << result.final_value << "' had_indirection=" << result.had_indirection << std::endl;
    debug_file << "---" << std::endl;
    debug_file.close();
    
    return result;
}

//-----------------------------------------------------------------------------
// Helper Functions for Rich Object Generation
//-----------------------------------------------------------------------------

std::string generateRichEntityJson(const std::string& eid, const std::string& value) {
    std::ostringstream json;
    json << "{"
         << "\"eid\":\"" << eid << "\","
         << "\"value\":\"" << value << "\","
         << "\"type\":\"entity\""
         << "}";
    return json.str();
}

std::string generateRichTripleJson(const std::string& tid,
                                 const std::string& subject_eid, const std::string& subject_value,
                                 const std::string& predicate_eid, const std::string& predicate_value,
                                 const std::string& object_eid, const std::string& object_value,
                                 std::shared_ptr<NonoStore> store) {
    
    // Resolve EID chains for all components
    auto subject_resolved = resolveEidChain(subject_eid, subject_value, store);
    auto predicate_resolved = resolveEidChain(predicate_eid, predicate_value, store);
    auto object_resolved = resolveEidChain(object_eid, object_value, store);
    
    std::ostringstream json;
    json << "{"
         << "\"tid\":\"" << tid << "\","
         << "\"subject\":{\"eid\":\"" << subject_resolved.final_eid << "\",\"value\":\"" << subject_resolved.final_value << "\"},"
         << "\"predicate\":{\"eid\":\"" << predicate_resolved.final_eid << "\",\"value\":\"" << predicate_resolved.final_value << "\"},"
         << "\"object\":{\"eid\":\"" << object_resolved.final_eid << "\",\"value\":\"" << object_resolved.final_value << "\"},"
         << "\"type\":\"triple\"";
    
    // Add awareness report if we had indirection
    if (subject_resolved.had_indirection || predicate_resolved.had_indirection || object_resolved.had_indirection) {
        json << ",\"indirection_report\":{";
        
        bool needsComma = false;
        
        if (subject_resolved.had_indirection) {
            json << "\"subject_chain\":[";
            for (size_t i = 0; i < subject_resolved.chain.size(); ++i) {
                if (i > 0) json << ",";
                json << "\"" << subject_resolved.chain[i] << "\"";
            }
            json << "]";
            if (subject_resolved.had_cycles) json << ",\"subject_cycles\":true";
            needsComma = true;
        }
        
        if (predicate_resolved.had_indirection) {
            if (needsComma) json << ",";
            json << "\"predicate_chain\":[";
            for (size_t i = 0; i < predicate_resolved.chain.size(); ++i) {
                if (i > 0) json << ",";
                json << "\"" << predicate_resolved.chain[i] << "\"";
            }
            json << "]";
            if (predicate_resolved.had_cycles) json << ",\"predicate_cycles\":true";
            needsComma = true;
        }
        
        if (object_resolved.had_indirection) {
            if (needsComma) json << ",";
            json << "\"object_chain\":[";
            for (size_t i = 0; i < object_resolved.chain.size(); ++i) {
                if (i > 0) json << ",";
                json << "\"" << object_resolved.chain[i] << "\"";
            }
            json << "]";
            if (object_resolved.had_cycles) json << ",\"object_cycles\":true";
        }
        
        json << "}";
    }
    
    json << "}";
    return json.str();
}

//-----------------------------------------------------------------------------
// Enhanced Find Entity Implementation - Supports Single + Multiple Patterns
// Replaces both find-entity-enhanced and find-entities-enhanced
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Helper: Extract pattern list from S-expression (based on our test)
//-----------------------------------------------------------------------------
std::vector<std::string> extractPatternList(const lab::Text::Sexpr& sexpr, const std::string& param_name) {
    std::vector<std::string> patterns;
    
    bool in_target_param = false;
    int list_depth = 0;
    
    for (size_t i = 0; i < sexpr.expr.size(); ++i) {
        const auto& elem = sexpr.expr[i];
        
        // Check if we hit the target parameter atom (e.g., ":patterns")
        if (elem.token == lab::Text::tsSexprAtom && elem.ref < sexpr.strings.size()) {
            if (sexpr.strings[elem.ref] == param_name) {
                in_target_param = true;
                continue;
            }
        }
        
        if (in_target_param) {
            if (elem.token == lab::Text::tsSexprPushList) {
                list_depth++;
            } else if (elem.token == lab::Text::tsSexprPopList) {
                list_depth--;
                if (list_depth == 0) {
                    in_target_param = false; // End of parameter list
                }
            } else if (elem.token == lab::Text::tsSexprString && list_depth > 0) {
                // This is a string inside the list
                if (elem.ref < sexpr.strings.size()) {
                    patterns.push_back(sexpr.strings[elem.ref]);
                }
            }
        }
    }
    
    return patterns;
}

//-----------------------------------------------------------------------------
// Helper: Factored entity search logic (reusable for single/multiple patterns)
//-----------------------------------------------------------------------------
struct EntitySearchResult {
    std::string eid;
    std::string value;
    std::string type = "entity";
};


//-----------------------------------------------------------------------------
// Enhanced Find Entity Implementation (returns rich objects)
//-----------------------------------------------------------------------------

Db9Response FindEntityEnhancedVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        std::string pattern = extractStringParam(sexpr, "pattern");
        std::string dbid = extractStringParam(sexpr, "dbid");
        
        if (pattern.empty()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "invalid_pattern", 
                "Pattern cannot be empty", metrics
            };
        }
        
        auto& manager = DatabaseManager::instance();
        auto store = manager.getDatabase(dbid);
        if (!store) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "invalid_dbid",
                generateDbidDiagnosisMessage("find_entity_enhanced"), metrics
            };
        }
        
        // Pattern matching logic (same as existing FindEntityVerb)
        std::vector<std::string> results;
        std::string searchPattern = pattern;
        bool hasLeadingWildcard = !pattern.empty() && pattern.front() == '*';
        bool hasTrailingWildcard = !pattern.empty() && pattern.back() == '*';
        
        if (hasLeadingWildcard) searchPattern = searchPattern.substr(1);
        if (hasTrailingWildcard) searchPattern = searchPattern.substr(0, searchPattern.length() - 1);
        
        // Get all subjects (entities) from the database - same as regular FindEntityVerb
        auto allSubjects = store->all_subjects();
        
        for (const auto& subject : allSubjects) {
            const std::string& entity_name = subject;
            bool matches = false;
            
            if (pattern == "*") {
                matches = true;
            } else if (hasLeadingWildcard && hasTrailingWildcard) {
                matches = (entity_name.find(searchPattern) != std::string::npos);
            } else if (hasLeadingWildcard) {
                matches = (entity_name.length() >= searchPattern.length() && 
                          entity_name.substr(entity_name.length() - searchPattern.length()) == searchPattern);
            } else if (hasTrailingWildcard) {
                matches = (entity_name.length() >= searchPattern.length() && 
                          entity_name.substr(0, searchPattern.length()) == searchPattern);
            } else {
                matches = (entity_name == searchPattern);
            }
            
            if (matches) {
                results.push_back(entity_name);
            }
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        metrics.items_processed = results.size();
        
        // Format results as JSON array of rich objects
        std::ostringstream resultStream;
        resultStream << "[";
        for (size_t i = 0; i < results.size(); ++i) {
            if (i > 0) resultStream << ",";
            
            EntityId entityId = EntityId::fromName(results[i], *store);
            std::string eid = entityId.isValid() ? entityId.eid() : "eid:unknown";
            
            std::string richObject = generateRichEntityJson(eid, results[i]);
            resultStream << richObject;
        }
        resultStream << "]";
        
        return Db9Response{
            Db9Response::Success, resultStream.str(), "", "", metrics
        };
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        
        return Db9Response{
            Db9Response::Error, "", "find_entity_enhanced_failed", e.what(), metrics
        };
    }
}

//-----------------------------------------------------------------------------
// FindEidVerb Implementation
//-----------------------------------------------------------------------------

Db9Response FindEidVerb::execute(const lab::Text::Sexpr& sexpr) {
    try {
        std::string pattern = extractStringParam(sexpr, "pattern");
        std::string dbid = extractStringParam(sexpr, "dbid");
        
        if (dbid.empty()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error,
                "",
                "invalid_dbid",
                generateDbidDiagnosisMessage("find_eid"),
                metrics
            };
        }
        
        auto& manager = DatabaseManager::instance();
        auto store = manager.getDatabase(dbid);
        if (!store) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error,
                "",
                "invalid_dbid", 
                "Database not found: " + dbid,
                metrics
            };
        }
        
        // Use similar pattern matching as FindEntityVerb
        std::vector<std::string> results;
        
        // Enhanced wildcard matching
        std::string searchPattern = pattern;
        bool hasLeadingWildcard = !pattern.empty() && pattern.front() == '*';
        bool hasTrailingWildcard = !pattern.empty() && pattern.back() == '*';
        
        // Extract the actual search string by removing wildcards
        if (hasLeadingWildcard) {
            searchPattern = searchPattern.substr(1);
        }
        if (hasTrailingWildcard) {
            searchPattern = searchPattern.substr(0, searchPattern.length() - 1);
        }

        // Get all subjects (entities) from the database
        auto allSubjects = store->all_subjects();
        
        for (const auto& subject : allSubjects) {
            bool matches = false;
            
            if (hasLeadingWildcard && hasTrailingWildcard) {
                // *substring* - infix matching
                matches = subject.find(searchPattern) != std::string::npos;
            } else if (hasLeadingWildcard) {
                // *suffix - suffix matching
                matches = subject.length() >= searchPattern.length() &&
                         subject.substr(subject.length() - searchPattern.length()) == searchPattern;
            } else if (hasTrailingWildcard) {
                // prefix* - prefix matching
                matches = subject.length() >= searchPattern.length() &&
                         subject.substr(0, searchPattern.length()) == searchPattern;
            } else {
                // Exact match
                matches = subject == searchPattern;
            }
            
            if (matches) {
                // Generate EID for this entity
                EntityId entityId = EntityId::fromName(subject, *store);
                if (entityId.isValid()) {
                    results.push_back(entityId.eid());
                }
            }
        }
        
        AutoReflexiveMetrics metrics;
        metrics.items_processed = results.size();
        
        // Build result as JSON array of EIDs
        std::ostringstream result;
        result << "[";
        for (size_t i = 0; i < results.size(); ++i) {
            if (i > 0) result << ", ";
            result << "\"" << results[i] << "\"";
        }
        result << "]";
        
        return Db9Response{
            Db9Response::Success,
            result.str(),
            "",
            "",
            metrics
        };
    }
    catch (const std::exception& e) {
        AutoReflexiveMetrics metrics;
        return Db9Response{
            Db9Response::Error,
            "",
            "find_eid_failed",
            std::string("find_eid failed: ") + e.what(),
            metrics
        };
    }
}

//-----------------------------------------------------------------------------
// FindTripleEnhancedVerb Implementation
//-----------------------------------------------------------------------------

Db9Response FindTripleEnhancedVerb::execute(const lab::Text::Sexpr& sexpr) {
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
        
        if (dbid.empty()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "invalid_dbid",
                generateDbidDiagnosisMessage("find_triple_enhanced"), metrics
            };
        }
        
        auto& manager = DatabaseManager::instance();
        auto store = manager.getDatabase(dbid);
        if (!store) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "invalid_dbid",
                "Database not found: " + dbid, metrics
            };
        }
        
        // Query triples using NonoStore's pattern matching
        auto triples = store->query(subject_pattern, predicate_pattern, object_pattern);
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        metrics.items_processed = triples.size();
        
        // Build result JSON as array of rich triple objects
        std::ostringstream result;
        result << "[";
        for (size_t i = 0; i < triples.size(); ++i) {
            if (i > 0) result << ",";
            
            // For rich objects, we need to generate EIDs for each component
            // and create full objects with both EID and resolved values
            
            // Generate EIDs for subject, predicate, object
            EntityId subjectId = EntityId::fromName(triples[i].subject, *store);
            EntityId predicateId = EntityId::fromName(triples[i].predicate, *store);
            EntityId objectId = EntityId::fromName(triples[i].object, *store);
            
            std::string subject_eid = subjectId.isValid() ? subjectId.eid() : "eid:unknown";
            std::string predicate_eid = predicateId.isValid() ? predicateId.eid() : "eid:unknown";
            std::string object_eid = objectId.isValid() ? objectId.eid() : "eid:unknown";
            
            // Generate a TID for this triple (this might need refinement based on how TIDs work)
            std::string tid = "tid:triple:" + std::to_string(i); // Placeholder TID generation
            
            // Use our rich triple JSON generator
            std::string richTriple = generateRichTripleJson(
                tid,
                subject_eid, triples[i].subject,
                predicate_eid, triples[i].predicate,
                object_eid, triples[i].object,
                store
            );
            
            result << richTriple;
        }
        result << "]";
        
        return Db9Response{
            Db9Response::Success, result.str(), "", "", metrics
        };
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        
        return Db9Response{
            Db9Response::Error, "", "find_triple_enhanced_failed", e.what(), metrics
        };
    }
}

//-----------------------------------------------------------------------------
// FindTidVerb Implementation - Returns only TIDs for lean triple operations
//-----------------------------------------------------------------------------

Db9Response FindTidVerb::execute(const lab::Text::Sexpr& sexpr) {
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
        
        if (dbid.empty()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "invalid_dbid",
                generateDbidDiagnosisMessage("find_tid"), metrics
            };
        }
        
        auto& manager = DatabaseManager::instance();
        auto store = manager.getDatabase(dbid);
        if (!store) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "invalid_dbid",
                "Database not found: " + dbid, metrics
            };
        }
        
        // Query triples using NonoStore's pattern matching
        auto triples = store->query(subject_pattern, predicate_pattern, object_pattern);
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        metrics.items_processed = triples.size();
        
        // For lean operations, we need to find the TIDs for each matching triple
        // Since NonoStore::query returns Triple objects, we need to look up their TIDs
        // For now, generate TIDs based on a deterministic approach until we have proper TID lookup
        
        std::ostringstream result;
        result << "[";
        for (size_t i = 0; i < triples.size(); ++i) {
            if (i > 0) result << ",";
            
            // Generate deterministic TID based on triple content
            // This should be replaced with proper TID lookup when available
            std::string triple_key = triples[i].subject + ":" + triples[i].predicate + ":" + triples[i].object;
            std::hash<std::string> hasher;
            size_t tid_value = hasher(triple_key);
            
            result << "\"tid:" << tid_value << "\"";
        }
        result << "]";
        
        return Db9Response{
            Db9Response::Success, result.str(), "", "", metrics
        };
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        
        return Db9Response{
            Db9Response::Error, "", "find_tid_failed", e.what(), metrics
        };
    }
}

//-----------------------------------------------------------------------------
// AddTripleSemanticVerb Implementation
//-----------------------------------------------------------------------------

Db9Response AddTripleSemanticVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        std::string subject = extractStringParam(sexpr, "subject");
        std::string predicate = extractStringParam(sexpr, "predicate");
        std::string object = extractStringParam(sexpr, "object");
        std::string dbid = extractStringParam(sexpr, "dbid");
        
        if (subject.empty() || predicate.empty() || object.empty()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "missing_parameters",
                "Parameters :subject, :predicate, and :object are all required", metrics
            };
        }
        
        if (dbid.empty()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "invalid_dbid",
                generateDbidDiagnosisMessage("add_triple_semantic"), metrics
            };
        }
        
        auto& manager = DatabaseManager::instance();
        auto store = manager.getDatabase(dbid);
        if (!store) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "invalid_dbid",
                "Database not found: " + dbid, metrics
            };
        }
        
        // SEMANTIC LAYER: Auto-create entities if they don't exist
        // This is what makes this verb "semantic" vs the storage-layer AddTidVerb
        
        // Check if subject entity exists, create if needed
        EntityId subjectId = EntityId::fromName(subject, *store);
        if (!subjectId.isValid() || !subjectId.exists()) {
            // Create the subject entity
            bool success = store->add_triple(subject, "isA", "entity");
            if (!success) {
                AutoReflexiveMetrics metrics;
                return Db9Response{
                    Db9Response::Error, "", "entity_creation_failed",
                    "Failed to auto-create subject entity: " + subject, metrics
                };
            }
            subjectId = EntityId::fromName(subject, *store);
        }
        
        // Check if predicate entity exists, create if needed
        EntityId predicateId = EntityId::fromName(predicate, *store);
        if (!predicateId.isValid() || !predicateId.exists()) {
            // Create the predicate entity
            bool success = store->add_triple(predicate, "isA", "entity");
            if (!success) {
                AutoReflexiveMetrics metrics;
                return Db9Response{
                    Db9Response::Error, "", "entity_creation_failed",
                    "Failed to auto-create predicate entity: " + predicate, metrics
                };
            }
            predicateId = EntityId::fromName(predicate, *store);
        }
        
        // Check if object entity exists, create if needed
        EntityId objectId = EntityId::fromName(object, *store);
        if (!objectId.isValid() || !objectId.exists()) {
            // Create the object entity
            bool success = store->add_triple(object, "isA", "entity");
            if (!success) {
                AutoReflexiveMetrics metrics;
                return Db9Response{
                    Db9Response::Error, "", "entity_creation_failed",
                    "Failed to auto-create object entity: " + object, metrics
                };
            }
            objectId = EntityId::fromName(object, *store);
        }
        
        // Now add the main triple (this is the same as regular AddTripleVerb)
        bool success = store->add_triple(subject, predicate, object);
        if (!success) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "triple_creation_failed",
                "Failed to add triple to database", metrics
            };
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        metrics.tid_allocations = 1; // One main triple added (plus potentially up to 3 entity triples)
        
        // Build result JSON with confirmation and the EIDs that were created/used
        std::ostringstream result;
        result << "{"
               << "\"status\":\"added\","
               << "\"subject\":{\"eid\":\"" << subjectId.eid() << "\",\"value\":\"" << subject << "\"},"
               << "\"predicate\":{\"eid\":\"" << predicateId.eid() << "\",\"value\":\"" << predicate << "\"},"
               << "\"object\":{\"eid\":\"" << objectId.eid() << "\",\"value\":\"" << object << "\"},"
               << "\"type\":\"triple\","
               << "\"semantic_layer\":true"
               << "}";
        
        return Db9Response{
            Db9Response::Success, result.str(), "", "", metrics
        };
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        
        return Db9Response{
            Db9Response::Error, "", "add_triple_semantic_failed", e.what(), metrics
        };
    }
}

//-----------------------------------------------------------------------------
// AddTidVerb Implementation
//-----------------------------------------------------------------------------

Db9Response AddTidVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        std::string subject_eid = extractStringParam(sexpr, "subject_eid");
        std::string predicate_eid = extractStringParam(sexpr, "predicate_eid");
        std::string object_eid = extractStringParam(sexpr, "object_eid");
        std::string dbid = extractStringParam(sexpr, "dbid");
        
        if (subject_eid.empty() || predicate_eid.empty() || object_eid.empty()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "missing_parameters",
                "Parameters :subject_eid, :predicate_eid, and :object_eid are all required", metrics
            };
        }
        
        if (dbid.empty()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "invalid_dbid",
                generateDbidDiagnosisMessage("add_tid"), metrics
            };
        }
        
        auto& manager = DatabaseManager::instance();
        auto store = manager.getDatabase(dbid);
        if (!store) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "invalid_dbid",
                "Database not found: " + dbid, metrics
            };
        }
        
        // STORAGE LAYER: Requires existing EIDs (no auto-creation)
        // This is what makes this verb "storage" vs the semantic-layer AddTripleSemanticVerb
        
        // Validate that all EIDs exist and resolve to entities
        EntityId subjectId = EntityId::fromEid(subject_eid, *store);
        if (!subjectId.isValid() || !subjectId.exists()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "invalid_subject_eid",
                "Subject EID not found: " + subject_eid, metrics
            };
        }
        
        EntityId predicateId = EntityId::fromEid(predicate_eid, *store);
        if (!predicateId.isValid() || !predicateId.exists()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "invalid_predicate_eid",
                "Predicate EID not found: " + predicate_eid, metrics
            };
        }
        
        EntityId objectId = EntityId::fromEid(object_eid, *store);
        if (!objectId.isValid() || !objectId.exists()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "invalid_object_eid",
                "Object EID not found: " + object_eid, metrics
            };
        }
        
        // Get entity names from validated EIDs
        std::string subject = subjectId.name();
        std::string predicate = predicateId.name();
        std::string object = objectId.name();
        
        // Add the triple using existing entity names
        bool success = store->add_triple(subject, predicate, object);
        if (!success) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "triple_creation_failed",
                "Failed to add triple to database", metrics
            };
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        metrics.tid_allocations = 1; // One triple added
        
        // Generate a TID for the response (deterministic approach)
        std::string triple_key = subject + ":" + predicate + ":" + object;
        std::hash<std::string> hasher;
        size_t tid_value = hasher(triple_key);
        std::string tid = "tid:" + std::to_string(tid_value);
        
        // Build result JSON with confirmation and the TID that was allocated
        std::ostringstream result;
        result << "{"
               << "\"status\":\"added\","
               << "\"tid\":\"" << tid << "\","
               << "\"subject\":{\"eid\":\"" << subject_eid << "\",\"value\":\"" << subject << "\"},"
               << "\"predicate\":{\"eid\":\"" << predicate_eid << "\",\"value\":\"" << predicate << "\"},"
               << "\"object\":{\"eid\":\"" << object_eid << "\",\"value\":\"" << object << "\"},"
               << "\"type\":\"triple\","
               << "\"storage_layer\":true"
               << "}";
        
        return Db9Response{
            Db9Response::Success, result.str(), "", "", metrics
        };
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        
        return Db9Response{
            Db9Response::Error, "", "add_tid_failed", e.what(), metrics
        };
    }
}

//-----------------------------------------------------------------------------
// GetEntityEnhancedVerb Implementation
//-----------------------------------------------------------------------------

Db9Response GetEntityEnhancedVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        std::string eid = extractStringParam(sexpr, "eid");
        std::string dbid = extractStringParam(sexpr, "dbid");
        
        if (eid.empty()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "missing_eid",
                "Parameter :eid is required", metrics
            };
        }
        
        if (dbid.empty()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "invalid_dbid",
                generateDbidDiagnosisMessage("get_entity_enhanced"), metrics
            };
        }
        
        auto& manager = DatabaseManager::instance();
        auto store = manager.getDatabase(dbid);
        if (!store) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "invalid_dbid",
                "Database not found: " + dbid, metrics
            };
        }
        
        // Use EntityId to resolve the entity
        EntityId entityId = EntityId::fromEid(eid, *store);
        if (!entityId.isValid() || !entityId.exists()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "entity_not_found",
                "Entity not found for EID: " + eid, metrics
            };
        }
        
        // Get entity value through proper TID-based lookup
        std::string entityValue = entityId.name();
        
        // 🔧 DIAGNOSTIC: Check if we need EID chain resolution
        std::ofstream debug_file("/tmp/enhanced_verbs_debug.log", std::ios::app);
        debug_file << "🔍 GetEntityEnhanced: eid=" << eid << " rawValue='" << entityValue << "'" << std::endl;
        
        // Apply EID chain resolution if needed
        auto resolved = resolveEidChain(eid, entityValue, store);
        if (resolved.had_indirection) {
            debug_file << "🔗 GetEntityEnhanced: Chain resolved! " << eid << " -> '" << resolved.final_value << "'" << std::endl;
            entityValue = resolved.final_value;  // Use resolved value
        } else {
            debug_file << "ℹ️  GetEntityEnhanced: No chain resolution needed for " << eid << std::endl;
        }
        debug_file.close();
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        metrics.items_processed = 1;
        
        // Generate rich entity object
        std::string richObject = generateRichEntityJson(eid, entityValue);
        
        return Db9Response{
            Db9Response::Success, richObject, "", "", metrics
        };
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        
        return Db9Response{
            Db9Response::Error, "", "get_entity_enhanced_failed", e.what(), metrics
        };
    }
}

//-----------------------------------------------------------------------------
// GetTripleEnhancedVerb Implementation
//-----------------------------------------------------------------------------

Db9Response GetTripleEnhancedVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        std::string tid = extractStringParam(sexpr, "tid");
        std::string dbid = extractStringParam(sexpr, "dbid");
        
        if (tid.empty()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "missing_tid",
                "Parameter :tid is required", metrics
            };
        }
        
        if (dbid.empty()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "invalid_dbid",
                generateDbidDiagnosisMessage("get_triple_enhanced"), metrics
            };
        }
        
        auto& manager = DatabaseManager::instance();
        auto store = manager.getDatabase(dbid);
        if (!store) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "invalid_dbid",
                "Database not found: " + dbid, metrics
            };
        }
        
        // Parse TID - for now we need to reverse-engineer from our deterministic approach
        // Extract the hash value from "tid:12345" format
        std::string tid_value_str;
        if (tid.starts_with("tid:")) {
            tid_value_str = tid.substr(4);
        } else {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "invalid_tid_format",
                "TID must be in format 'tid:value'", metrics
            };
        }
        
        size_t tid_hash;
        try {
            tid_hash = std::stoull(tid_value_str);
        } catch (const std::exception&) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "invalid_tid_value",
                "Invalid TID numeric value: " + tid_value_str, metrics
            };
        }
        
        // Since we don't have direct TID->Triple lookup yet, we need to search through all triples
        // and find the one with matching hash. This is O(n) but will work for now
        
        auto all_triples = store->query("*", "*", "*");
        
        // Find the triple with matching TID hash
        for (const auto& triple : all_triples) {
            std::string triple_key = triple.subject + ":" + triple.predicate + ":" + triple.object;
            std::hash<std::string> hasher;
            size_t computed_hash = hasher(triple_key);
            
            if (computed_hash == tid_hash) {
                // Found matching triple - generate rich object
                
                // Generate EIDs for each component
                EntityId subjectId = EntityId::fromName(triple.subject, *store);
                EntityId predicateId = EntityId::fromName(triple.predicate, *store);
                EntityId objectId = EntityId::fromName(triple.object, *store);
                
                std::string subject_eid = subjectId.isValid() ? subjectId.eid() : "eid:unknown";
                std::string predicate_eid = predicateId.isValid() ? predicateId.eid() : "eid:unknown";
                std::string object_eid = objectId.isValid() ? objectId.eid() : "eid:unknown";
                
                auto end_time = std::chrono::steady_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                
                AutoReflexiveMetrics metrics;
                metrics.operation_time_ms = duration;
                metrics.items_processed = 1;
                
                // Generate rich triple object
                std::string richTriple = generateRichTripleJson(
                    tid,
                    subject_eid, triple.subject,
                    predicate_eid, triple.predicate,
                    object_eid, triple.object,
                    store
                );
                
                return Db9Response{
                    Db9Response::Success, richTriple, "", "", metrics
                };
            }
        }
        
        // Triple not found for given TID
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        metrics.items_processed = all_triples.size(); // We searched through all triples
        
        return Db9Response{
            Db9Response::Error, "", "triple_not_found",
            "No triple found for TID: " + tid, metrics
        };
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        
        return Db9Response{
            Db9Response::Error, "", "get_triple_enhanced_failed", e.what(), metrics
        };
    }
}

//-----------------------------------------------------------------------------
// FindRelationshipsEnhancedVerb Implementation
//-----------------------------------------------------------------------------

Db9Response FindRelationshipsEnhancedVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        std::string entity = extractStringParam(sexpr, "entity");
        std::string dbid = extractStringParam(sexpr, "dbid");
        
        if (entity.empty()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "missing_entity",
                "Parameter :entity is required", metrics
            };
        }
        
        if (dbid.empty()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "invalid_dbid",
                generateDbidDiagnosisMessage("find_relationships_enhanced"), metrics
            };
        }
        
        auto& manager = DatabaseManager::instance();
        auto store = manager.getDatabase(dbid);
        if (!store) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "invalid_dbid",
                "Database not found: " + dbid, metrics
            };
        }
        
        std::ofstream debug_file("/tmp/enhanced_verbs_debug.log", std::ios::app);
        debug_file << "🔍 FindRelationshipsEnhanced: Searching for entity=" << entity << std::endl;
        
        // Find outgoing relationships (entity as subject)
        std::ostringstream outgoing_json;
        std::string find_outgoing_cmd = "(find-triple-enhanced :subject \"" + entity + "\" :predicate \"*\" :object \"*\" :dbid \"" + dbid + "\")";
        auto outgoing_response = getGlobalDb9Dispatcher().executeCommand(find_outgoing_cmd);
        
        // Find incoming relationships (entity as object)  
        std::string find_incoming_cmd = "(find-triple-enhanced :subject \"*\" :predicate \"*\" :object \"" + entity + "\" :dbid \"" + dbid + "\")";
        auto incoming_response = getGlobalDb9Dispatcher().executeCommand(find_incoming_cmd);
        
        debug_file << "🔗 FindRelationshipsEnhanced: Found relationships for " << entity << std::endl;
        debug_file.close();
        
        // Combine results
        std::ostringstream combined_json;
        combined_json << "{";
        combined_json << "\"entity\":\"" << entity << "\",";
        combined_json << "\"outgoing\":" << outgoing_response.result << ",";
        combined_json << "\"incoming\":" << incoming_response.result;
        combined_json << "}";
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        metrics.items_processed = 2; // Two relationship searches
        
        return Db9Response{
            Db9Response::Success, combined_json.str(), "", "", metrics
        };
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        
        return Db9Response{
            Db9Response::Error, "", "find_relationships_enhanced_failed", e.what(), metrics
        };
    }
}

//-----------------------------------------------------------------------------
// FindEntitiesEnhancedVerb Implementation  
//-----------------------------------------------------------------------------

Db9Response FindEntitiesEnhancedVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        std::string patterns = extractStringParam(sexpr, "patterns");
        std::string dbid = extractStringParam(sexpr, "dbid");
        
        if (patterns.empty()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "missing_patterns",
                "Parameter :patterns is required (comma-separated list)", metrics
            };
        }
        
        if (dbid.empty()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "invalid_dbid",
                generateDbidDiagnosisMessage("find_entities_enhanced"), metrics
            };
        }
        
        auto& manager = DatabaseManager::instance();
        auto store = manager.getDatabase(dbid);
        if (!store) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "invalid_dbid",
                "Database not found: " + dbid, metrics
            };
        }
        
        std::ofstream debug_file("/tmp/enhanced_verbs_debug.log", std::ios::app);
        debug_file << "🔍 FindEntitiesEnhanced: Searching patterns=" << patterns << std::endl;
        
        // Parse comma-separated patterns
        std::vector<std::string> pattern_list;
        std::istringstream pattern_stream(patterns);
        std::string pattern;
        while (std::getline(pattern_stream, pattern, ',')) {
            // Trim whitespace
            pattern.erase(0, pattern.find_first_not_of(" \t"));
            pattern.erase(pattern.find_last_not_of(" \t") + 1);
            if (!pattern.empty()) {
                pattern_list.push_back(pattern);
            }
        }
        
        std::ostringstream combined_json;
        combined_json << "{\"patterns_searched\":[";
        
        // Search each pattern and collect results
        std::vector<std::string> all_results;
        for (size_t i = 0; i < pattern_list.size(); ++i) {
            if (i > 0) combined_json << ",";
            combined_json << "\"" << pattern_list[i] << "\"";
            
            std::string find_cmd = "(find-entity-enhanced :dbid \"" + dbid + "\" :pattern \"" + pattern_list[i] + "\")";
            auto pattern_response = getGlobalDb9Dispatcher().executeCommand(find_cmd);
            
            if (pattern_response.status == Db9Response::Success && pattern_response.result != "[]") {
                all_results.push_back(pattern_response.result);
            }
        }
        
        combined_json << "],\"results\":[";
        
        // Combine all results
        for (size_t i = 0; i < all_results.size(); ++i) {
            if (i > 0) combined_json << ",";
            // Remove outer brackets from individual results and merge
            std::string result = all_results[i];
            if (result.front() == '[') result = result.substr(1);
            if (result.back() == ']') result = result.substr(0, result.length() - 1);
            combined_json << result;
        }
        
        combined_json << "]}";
        
        debug_file << "🎯 FindEntitiesEnhanced: Found entities across " << pattern_list.size() << " patterns" << std::endl;
        debug_file.close();
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        metrics.items_processed = pattern_list.size();
        
        return Db9Response{
            Db9Response::Success, combined_json.str(), "", "", metrics
        };
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        
        return Db9Response{
            Db9Response::Error, "", "find_entities_enhanced_failed", e.what(), metrics
        };
    }
}

//-----------------------------------------------------------------------------
// Enhanced Verb Registration  
//-----------------------------------------------------------------------------

} // namespace LabDb

extern "C" void initEnhancedDatabaseVerbRegistration() {
    static bool registered = false;
    if (!registered) {
        auto& dispatcher = LabDb::getGlobalDb9Dispatcher();
        
        // Register enhanced API verbs (rich objects)
        dispatcher.registerVerb(std::make_unique<LabDb::FindEntityEnhancedVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::FindTripleEnhancedVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::GetEntityEnhancedVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::GetTripleEnhancedVerb>());
        
        // Register new enhanced exploration verbs
        dispatcher.registerVerb(std::make_unique<LabDb::FindRelationshipsEnhancedVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::FindEntitiesEnhancedVerb>());
        
        // Register lean API verbs (EID/TID only)
        dispatcher.registerVerb(std::make_unique<LabDb::FindEidVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::FindTidVerb>());
        
        // Register dual-layer API verbs (semantic vs storage)
        dispatcher.registerVerb(std::make_unique<LabDb::AddTripleSemanticVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::AddTidVerb>());
        
        registered = true;
    }
}
