#include "LabDb/EnhancedDatabaseVerbs.h"
#include "LabDb/DatabaseVerbs.h"
#include "LabDb/EntityId.h"
#include "LabDb/LabText.hpp"
#include <chrono>
#include <sstream>
#include <iostream>

namespace LabDb {

// Implementation file for enhanced db9 API
// This provides rich object returns and dual-layer semantic/storage operations

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
                                 const std::string& object_eid, const std::string& object_value) {
    std::ostringstream json;
    json << "{"
         << "\"tid\":\"" << tid << "\","
         << "\"subject\":{\"eid\":\"" << subject_eid << "\",\"value\":\"" << subject_value << "\"},"
         << "\"predicate\":{\"eid\":\"" << predicate_eid << "\",\"value\":\"" << predicate_value << "\"},"
         << "\"object\":{\"eid\":\"" << object_eid << "\",\"value\":\"" << object_value << "\"},"
         << "\"type\":\"triple\""
         << "}";
    return json.str();
}

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
        
        auto query_results = store->query("*", "isA", "entity");
        
        for (const auto& triple : query_results) {
            const std::string& entity_name = triple.first;
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
            
            EntityId entityId = EntityId::fromSubject(results[i], *store);
            std::string eid = entityId.isValid() ? entityId.getEidString() : "eid:unknown";
            
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
        
        // Register lean API verbs (EID/TID only)
        dispatcher.registerVerb(std::make_unique<LabDb::FindEidVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::FindTidVerb>());
        
        // Register dual-layer API verbs (semantic vs storage)
        dispatcher.registerVerb(std::make_unique<LabDb::AddTripleSemanticVerb>());
        dispatcher.registerVerb(std::make_unique<LabDb::AddTidVerb>());
        
        registered = true;
    }
}
