#include "LabDb/Verbs.h"
#include "LabDb/DatabaseVerbs.h"
#include "LabDb/EntityId.h"
#include <unordered_set>
#include <sstream>
#include <fstream>
#include <iostream>

namespace LabDb {

//-----------------------------------------------------------------------------
// EID Chain Resolution Implementation
//-----------------------------------------------------------------------------

EidResolutionResult IDb9Verb::resolveEidChain(const std::string& start_eid, 
                                              const std::string& start_value,
                                              std::shared_ptr<NonoStore> store,
                                              bool enable_debug) {
    EidResolutionResult result;
    result.final_eid = start_eid;
    result.final_value = start_value;
    result.had_cycles = false;
    result.had_indirection = false;
    
    if (!store) {
        writeDebugLog("❌ EID Resolution: NULL store provided", enable_debug);
        return result;
    }
    
    std::unordered_set<std::string> visited;
    std::string current_eid = start_eid;
    std::string current_value = start_value;
    
    // Add initial EID to chain
    result.chain.push_back(current_eid);
    visited.insert(current_eid);
    
    writeDebugLog("🔍 EID Resolution: Starting with eid=" + start_eid + " value='" + start_value + "'", enable_debug);
    
    // Follow EID chain until we reach a non-EID value or detect cycle
    while (current_value.starts_with("eid:")) {
        writeDebugLog("🔗 EID Chain: Found EID value '" + current_value + "', attempting to resolve...", enable_debug);
        
        // Check for cycle
        if (visited.find(current_value) != visited.end()) {
            writeDebugLog("🚫 EID Chain: Cycle detected! " + current_value + " already visited", enable_debug);
            result.had_cycles = true;
            break;
        }
        
        // Try to resolve the EID value to the next entity
        EntityId nextEntity = EntityId::fromEid(current_value, *store);
        writeDebugLog("🏷️  EID Chain: EntityId lookup for '" + current_value + "' -> valid=" + 
                     (nextEntity.isValid() ? "true" : "false") + " exists=" + 
                     (nextEntity.exists() ? "true" : "false"), enable_debug);
        
        if (!nextEntity.isValid() || !nextEntity.exists()) {
            // Dead end - value looks like EID but doesn't resolve
            writeDebugLog("💀 EID Chain: Dead end - '" + current_value + "' doesn't resolve to valid entity", enable_debug);
            break;
        }
        
        std::string next_value = nextEntity.name();
        writeDebugLog("✅ EID Chain: '" + current_value + "' resolves to '" + next_value + "'", enable_debug);
        
        // Follow the chain
        visited.insert(current_value);
        result.chain.push_back(current_value);
        current_eid = current_value;
        current_value = next_value;
        result.had_indirection = true;
        
        writeDebugLog("➡️  EID Chain: Moving to eid=" + current_eid + " value='" + current_value + "'", enable_debug);
    }
    
    result.final_eid = current_eid;
    result.final_value = current_value;
    
    writeDebugLog("🎯 EID Resolution: Final result eid=" + result.final_eid + " value='" + result.final_value + 
                 "' had_indirection=" + (result.had_indirection ? "true" : "false"), enable_debug);
    writeDebugLog("🔗 Chain: " + result.getChainDescription(), enable_debug);
    writeDebugLog("---", enable_debug);
    
    return result;
}

//-----------------------------------------------------------------------------
// S-Expression Parameter Extraction
//-----------------------------------------------------------------------------

std::string IDb9Verb::extractStringParam(const ::lab::Text::Sexpr& sexpr, 
                                        const std::string& param_name) {
    // Look for :param_name value pattern in the parsed S-expression
    for (size_t i = 0; i < sexpr.expr.size() - 1; ++i) {
        const auto& elem = sexpr.expr[i];
        
        // Look for atoms that match our parameter name
        if (elem.token == tsSexprAtom) {
            int stringIndex = elem.ref;
            if (stringIndex >= 0 && stringIndex < static_cast<int>(sexpr.strings.size())) {
                const std::string& atomValue = sexpr.strings[stringIndex];
                
                // Check if this matches our parameter (with or without leading :)
                if (atomValue == ":" + param_name || atomValue == param_name) {
                    // Next element should be the value
                    if (i + 1 < sexpr.expr.size()) {
                        const auto& valueElem = sexpr.expr[i + 1];
                        if (valueElem.token == tsSexprAtom && valueElem.ref >= 0 && 
                            valueElem.ref < static_cast<int>(sexpr.strings.size())) {
                            return sexpr.strings[valueElem.ref];
                        }
                    }
                }
            }
        }
    }
    
    return ""; // Parameter not found
}

//-----------------------------------------------------------------------------
// Rich JSON Generation
//-----------------------------------------------------------------------------

std::string IDb9Verb::generateRichEntityJson(const std::string& eid, 
                                            const std::string& value,
                                            std::shared_ptr<NonoStore> store) {
    std::ostringstream json;
    json << "{"
         << "\"eid\":\"" << eid << "\","
         << "\"value\":\"" << value << "\","
         << "\"type\":\"entity\"";
    
    // Add EID chain resolution if store is available
    if (store && !value.empty()) {
        auto resolved = resolveEidChain(eid, value, store, false);
        if (resolved.had_indirection || resolved.had_cycles) {
            json << ",\"resolution\":{"
                 << "\"final_value\":\"" << resolved.final_value << "\","
                 << "\"had_indirection\":" << (resolved.had_indirection ? "true" : "false") << ","
                 << "\"had_cycles\":" << (resolved.had_cycles ? "true" : "false") << ","
                 << "\"chain_depth\":" << resolved.getChainDepth()
                 << "}";
        }
    }
    
    json << "}";
    return json.str();
}

std::string IDb9Verb::generateRichTripleJson(const std::string& tid,
                                            const std::string& subject_eid, const std::string& subject_value,
                                            const std::string& predicate_eid, const std::string& predicate_value,
                                            const std::string& object_eid, const std::string& object_value,
                                            std::shared_ptr<NonoStore> store) {
    
    // Resolve EID chains for all components
    auto subject_resolved = resolveEidChain(subject_eid, subject_value, store, false);
    auto predicate_resolved = resolveEidChain(predicate_eid, predicate_value, store, false);
    auto object_resolved = resolveEidChain(object_eid, object_value, store, false);
    
    std::ostringstream json;
    json << "{"
         << "\"tid\":\"" << tid << "\","
         << "\"type\":\"triple\","
         << "\"subject\":{"
         << "\"eid\":\"" << subject_eid << "\","
         << "\"value\":\"" << subject_value << "\","
         << "\"resolved_value\":\"" << subject_resolved.final_value << "\""
         << "},"
         << "\"predicate\":{"
         << "\"eid\":\"" << predicate_eid << "\","
         << "\"value\":\"" << predicate_value << "\","
         << "\"resolved_value\":\"" << predicate_resolved.final_value << "\""
         << "},"
         << "\"object\":{"
         << "\"eid\":\"" << object_eid << "\","
         << "\"value\":\"" << object_value << "\","
         << "\"resolved_value\":\"" << object_resolved.final_value << "\""
         << "}";
    
    // Add resolution metadata if any component had indirection
    if (subject_resolved.had_indirection || predicate_resolved.had_indirection || object_resolved.had_indirection) {
        json << ",\"resolution_metadata\":{"
             << "\"subject_chain_depth\":" << subject_resolved.getChainDepth() << ","
             << "\"predicate_chain_depth\":" << predicate_resolved.getChainDepth() << ","
             << "\"object_chain_depth\":" << object_resolved.getChainDepth() << ","
             << "\"total_indirections\":" << (subject_resolved.had_indirection ? 1 : 0) + 
                                           (predicate_resolved.had_indirection ? 1 : 0) + 
                                           (object_resolved.had_indirection ? 1 : 0)
             << "}";
    }
    
    json << "}";
    return json.str();
}

//-----------------------------------------------------------------------------
// Database Diagnosis Helper
//-----------------------------------------------------------------------------

std::string IDb9Verb::generateDbidDiagnosisMessage(const std::string& operation_name) {
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

//-----------------------------------------------------------------------------
// Internal Debug Helper
//-----------------------------------------------------------------------------

void IDb9Verb::writeDebugLog(const std::string& message, bool enable_debug) {
    if (!enable_debug) return;
    
    try {
        std::ofstream debug_file("/tmp/eid_resolution_debug.log", std::ios::app);
        if (debug_file.is_open()) {
            debug_file << message << std::endl;
            debug_file.close();
        }
    } catch (...) {
        // Silently ignore debug logging errors
    }
}

} // namespace LabDb
