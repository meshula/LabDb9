#include "LabDb/EnhancedDatabaseVerbs.h"
#include "LabDb/DatabaseVerbs.h"
#include "LabDb/Verbs.h"
#include "LabDb/EntityId.h"
#include "LabDb/LabText.hpp"
#include "Verbs/VocabularyStatsVerb.h"
#include <chrono>
#include <sstream>
#include <iostream>
#include <fstream>
#include <unordered_set>

namespace LabDb {

// Enhanced Database Verbs now use utilities from base IDb9Verb class

// Implementation file for enhanced db9 API
// This provides rich object returns and dual-layer semantic/storage operations

// Enhanced database verbs implementation uses base class utilities for EID resolution

// Rich JSON generation now provided by base IDb9Verb class

//-----------------------------------------------------------------------------
// Enhanced Database Verbs Implementation
//-----------------------------------------------------------------------------

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
        if (elem.token == tsSexprAtom && elem.ref < sexpr.strings.size()) {
            if (sexpr.strings[elem.ref] == ":" + param_name) {
                in_target_param = true;
                continue;
            }
        }
        
        if (in_target_param) {
            if (elem.token == tsSexprPushList) {
                list_depth++;
            } else if (elem.token == tsSexprPopList) {
                list_depth--;
                if (list_depth == 0) {
                    in_target_param = false; // End of parameter list
                }
            } else if (elem.token == tsSexprString && list_depth > 0) {
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

std::vector<EntitySearchResult> performEntitySearch(const std::string& pattern, NonoStore* store) {
    std::vector<EntitySearchResult> results;
    
    // Pattern matching logic (same as original FindEntityVerb)
    std::string searchPattern = pattern;
    bool hasLeadingWildcard = !pattern.empty() && pattern.front() == '*';
    bool hasTrailingWildcard = !pattern.empty() && pattern.back() == '*';
    
    if (hasLeadingWildcard) searchPattern = searchPattern.substr(1);
    if (hasTrailingWildcard) searchPattern = searchPattern.substr(0, searchPattern.length() - 1);
    
    // Get all subjects (entities) from the database
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
            EntitySearchResult result;
            
            // Generate EID using existing logic
            EntityId entityId = EntityId::fromName(entity_name, *store);
            result.eid = entityId.isValid() ? entityId.eid() : "eid:unknown";
            
            // Apply EID chain resolution to get the friendly value
            // Note: We need to convert raw pointer to shared_ptr for resolveEidChain
            std::shared_ptr<NonoStore> store_shared(store, [](NonoStore*) {});
            auto resolved = IDb9Verb::resolveEidChain(result.eid, entity_name, store_shared);
            result.value = resolved.final_value;  // Use resolved value instead of raw entity_name
            
            results.push_back(result);
        }
    }
    
    return results;
}

//-----------------------------------------------------------------------------
// Helper: Format multiple search results as JSON
//-----------------------------------------------------------------------------
std::string formatMultipleSearchResults(const std::vector<EntitySearchResult>& all_results, 
                                       const std::vector<std::string>& patterns_searched) {
    std::ostringstream json;
    
    if (patterns_searched.size() == 1) {
        // Single pattern - return simple array (backward compatible)
        json << "[";
        for (size_t i = 0; i < all_results.size(); ++i) {
            if (i > 0) json << ",";
            json << "{\"eid\":\"" << all_results[i].eid << "\","
                 << "\"value\":\"" << all_results[i].value << "\","
                 << "\"type\":\"" << all_results[i].type << "\"}";
        }
        json << "]";
    } else {
        // Multiple patterns - return enhanced structure with analytics
        json << "{";
        json << "\"patterns_searched\":[";
        for (size_t i = 0; i < patterns_searched.size(); ++i) {
            if (i > 0) json << ",";
            json << "\"" << patterns_searched[i] << "\"";
        }
        json << "],";
        
        json << "\"entities\":[";
        for (size_t i = 0; i < all_results.size(); ++i) {
            if (i > 0) json << ",";
            json << "{\"eid\":\"" << all_results[i].eid << "\","
                 << "\"value\":\"" << all_results[i].value << "\","
                 << "\"type\":\"" << all_results[i].type << "\"}";
        }
        json << "],";
        
        json << "\"search_analytics\":{";
        json << "\"total_patterns\":" << patterns_searched.size() << ",";
        json << "\"total_entities_found\":" << all_results.size();
        json << "}}";
    }
    
    return json.str();
}

//-----------------------------------------------------------------------------
// Main: Upgraded FindEntityEnhancedVerb Implementation
//-----------------------------------------------------------------------------
Db9Response FindEntityEnhancedVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Check for both single pattern and multiple patterns
        std::string single_pattern = IDb9Verb::extractStringParam(sexpr, "pattern");
        std::vector<std::string> pattern_list = extractPatternList(sexpr, "patterns");
        std::string dbid = IDb9Verb::extractStringParam(sexpr, "dbid");
        
        // Determine which patterns to search
        std::vector<std::string> patterns_to_search;
        
        if (!single_pattern.empty()) {
            patterns_to_search.push_back(single_pattern);
        } else if (!pattern_list.empty()) {
            patterns_to_search = pattern_list;
        } else {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "missing_patterns",
                "Either :pattern or :patterns parameter is required", metrics
            };
        }
        
        if (dbid.empty()) {
            AutoReflexiveMetrics metrics;
            return Db9Response{
                Db9Response::Error, "", "invalid_dbid",
                IDb9Verb::generateDbidDiagnosisMessage("find_entity_enhanced"), metrics
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
        
        // Debug logging to stderr (not stdout!)
        std::cerr << "🔍 FindEntityEnhanced: Searching " << patterns_to_search.size() 
                  << " patterns" << std::endl;
        
        // Perform search for each pattern and collect results
        std::vector<EntitySearchResult> all_results;
        
        for (const auto& pattern : patterns_to_search) {
            auto pattern_results = performEntitySearch(pattern, store.get());
            
            // Add pattern results to combined results
            all_results.insert(all_results.end(), pattern_results.begin(), pattern_results.end());
            
            std::cerr << "  Pattern '" << pattern << "': " << pattern_results.size() 
                      << " entities found" << std::endl;
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        metrics.items_processed = all_results.size();
        
        std::string result_json = formatMultipleSearchResults(all_results, patterns_to_search);
        
        std::cerr << "🎯 FindEntityEnhanced: Found " << all_results.size() 
                  << " total entities across " << patterns_to_search.size() << " patterns" << std::endl;
        
        return Db9Response{
            Db9Response::Success, result_json, "", "", metrics
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
// Usage Examples:
//-----------------------------------------------------------------------------

/*

// Single pattern (backward compatible):
(find-entity-enhanced :pattern "*camera*" :dbid "db1")

// Returns:
[
  {"eid":"eid:104","value":"set_camera","type":"entity"}
]

// Multiple patterns (new functionality):
(find-entity-enhanced :patterns ("*camera*" "*background*" "*scroll*") :dbid "db1")

// Returns:
{
  "patterns_searched": ["*camera*", "*background*", "*scroll*"],
  "entities": [
    {"eid":"eid:104","value":"set_camera","type":"entity"}
  ],
  "search_analytics": {
    "total_patterns": 3,
    "total_entities_found": 1
  }
}

*/

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
        
        // Find outgoing relationships (entity as subject) - DIRECT IMPLEMENTATION
        auto outgoing_triples = store->query(entity, "*", "*");
        
        // Find incoming relationships (entity as object) - DIRECT IMPLEMENTATION
        auto incoming_triples = store->query("*", "*", entity);
        
        // Build outgoing relationships JSON
        std::ostringstream outgoing_json;
        outgoing_json << "[";
        for (size_t i = 0; i < outgoing_triples.size(); ++i) {
            if (i > 0) outgoing_json << ",";
            
            // Generate EIDs for each component
            EntityId subjectId = EntityId::fromName(outgoing_triples[i].subject, *store);
            EntityId predicateId = EntityId::fromName(outgoing_triples[i].predicate, *store);
            EntityId objectId = EntityId::fromName(outgoing_triples[i].object, *store);
            
            std::string subject_eid = subjectId.isValid() ? subjectId.eid() : "eid:unknown";
            std::string predicate_eid = predicateId.isValid() ? predicateId.eid() : "eid:unknown";
            std::string object_eid = objectId.isValid() ? objectId.eid() : "eid:unknown";
            
            // Generate a TID for this triple
            std::string triple_key = outgoing_triples[i].subject + ":" + outgoing_triples[i].predicate + ":" + outgoing_triples[i].object;
            std::hash<std::string> hasher;
            size_t tid_value = hasher(triple_key);
            std::string tid = "tid:" + std::to_string(tid_value);
            
            // Generate rich triple JSON directly
            std::string richTriple = generateRichTripleJson(
                tid,
                subject_eid, outgoing_triples[i].subject,
                predicate_eid, outgoing_triples[i].predicate,
                object_eid, outgoing_triples[i].object,
                store
            );
            
            outgoing_json << richTriple;
        }
        outgoing_json << "]";
        
        // Build incoming relationships JSON
        std::ostringstream incoming_json;
        incoming_json << "[";
        for (size_t i = 0; i < incoming_triples.size(); ++i) {
            if (i > 0) incoming_json << ",";
            
            // Generate EIDs for each component
            EntityId subjectId = EntityId::fromName(incoming_triples[i].subject, *store);
            EntityId predicateId = EntityId::fromName(incoming_triples[i].predicate, *store);
            EntityId objectId = EntityId::fromName(incoming_triples[i].object, *store);
            
            std::string subject_eid = subjectId.isValid() ? subjectId.eid() : "eid:unknown";
            std::string predicate_eid = predicateId.isValid() ? predicateId.eid() : "eid:unknown";
            std::string object_eid = objectId.isValid() ? objectId.eid() : "eid:unknown";
            
            // Generate a TID for this triple
            std::string triple_key = incoming_triples[i].subject + ":" + incoming_triples[i].predicate + ":" + incoming_triples[i].object;
            std::hash<std::string> hasher;
            size_t tid_value = hasher(triple_key);
            std::string tid = "tid:" + std::to_string(tid_value);
            
            // Generate rich triple JSON directly
            std::string richTriple = generateRichTripleJson(
                tid,
                subject_eid, incoming_triples[i].subject,
                predicate_eid, incoming_triples[i].predicate,
                object_eid, incoming_triples[i].object,
                store
            );
            
            incoming_json << richTriple;
        }
        incoming_json << "]";
        
        debug_file << "🔗 FindRelationshipsEnhanced: Found relationships for " << entity << std::endl;
        debug_file.close();
        
        // Combine results
        std::ostringstream combined_json;
        combined_json << "{";
        combined_json << "\"entity\":\"" << entity << "\",";
        combined_json << "\"outgoing\":" << outgoing_json.str() << ",";
        combined_json << "\"incoming\":" << incoming_json.str();
        combined_json << "}";
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = duration;
        metrics.items_processed = outgoing_triples.size() + incoming_triples.size(); // Total relationships found
        
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
// Enhanced Verb Registration  
//-----------------------------------------------------------------------------

} // namespace LabDb

void initEnhancedDatabaseVerbRegistration(LabDb::Db9Dispatcher& dispatcher) {
    static bool registered = false;
    if (!registered) {
        // Register enhanced API verbs (rich objects)
        dispatcher.registerVerb(std::make_unique<LabDb::FindEntityEnhancedVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::FindTripleEnhancedVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::GetEntityEnhancedVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::GetTripleEnhancedVerb>());
        
        // Register new enhanced exploration verbs
        dispatcher.registerVerb(std::make_unique<LabDb::FindRelationshipsEnhancedVerb>());
        
        // Register vocabulary and statistics verbs
        dispatcher.registerVerb(std::make_unique<LabDb::VocabularyStatsVerb>());
        
        // Register lean API verbs (EID/TID only)
        dispatcher.registerVerb(std::make_unique<LabDb::FindEidVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::FindTidVerb>());
        
        // Register dual-layer API verbs (semantic vs storage)
        dispatcher.registerVerb(std::make_unique<LabDb::AddTripleSemanticVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::AddTidVerb>());
        
        registered = true;
    }
}
