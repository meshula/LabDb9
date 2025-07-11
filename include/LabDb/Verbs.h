#pragma once

#include "LabDb/Db9Response.h"
#include "LabDb/LabText.hpp"
#include "LabDb/NonoStore.h"
#include <string>
#include <vector>
#include <memory>

namespace LabDb {

//-----------------------------------------------------------------------------
// EID Chain Resolution Utilities
//-----------------------------------------------------------------------------

/// Result of EID chain resolution with full diagnostic information
struct EidResolutionResult {
    std::string final_eid;           ///< Final EID after following chain
    std::string final_value;         ///< Final resolved value (non-EID)
    std::vector<std::string> chain;  ///< Complete resolution chain including initial EID
    bool had_cycles;                 ///< True if cycle was detected and resolution stopped
    bool had_indirection;            ///< True if any EID->EID indirection was followed
    
    /// Check if resolution was successful (no cycles, valid final result)
    bool isValid() const {
        return !had_cycles && !final_eid.empty() && !final_value.empty();
    }
    
    /// Get chain depth (number of indirections followed)
    size_t getChainDepth() const {
        return chain.size();
    }
    
    /// Get human-readable chain representation for debugging
    std::string getChainDescription() const {
        if (chain.empty()) return "empty";
        
        std::string desc = chain[0];
        for (size_t i = 1; i < chain.size(); ++i) {
            desc += " -> " + chain[i];
        }
        if (had_indirection) {
            desc += " -> '" + final_value + "'";
        }
        return desc;
    }
};

//-----------------------------------------------------------------------------
// Base interface for all db9 verb implementations
//-----------------------------------------------------------------------------
class IDb9Verb {
public:
    virtual ~IDb9Verb() = default;
    
    // Execute the verb with parsed S-expression parameters
    // sexpr contains the full parsed command: (verb :param1 value1 :param2 value2)
    virtual Db9Response execute(const ::lab::Text::Sexpr& sexpr) = 0;
    
    // Get verb name for registration
    virtual std::string getVerbName() const = 0;
    
    // Get help/description for documentation
    virtual std::string getDescription() const = 0;

    //-------------------------------------------------------------------------
    // Utility Methods for Verb Implementations
    //-------------------------------------------------------------------------
    
    /// Resolve EID chain with full diagnostic information
    /// Follows eid: references until reaching non-EID value or detecting cycles
    /// @param start_eid Initial entity ID to resolve
    /// @param start_value Initial entity value  
    /// @param store NonoStore instance for entity lookups
    /// @param enable_debug Enable debug logging to /tmp/eid_resolution_debug.log
    /// @return Complete resolution result with diagnostics
    static EidResolutionResult resolveEidChain(const std::string& start_eid, 
                                              const std::string& start_value,
                                              std::shared_ptr<NonoStore> store,
                                              bool enable_debug = false);
    
    /// Extract string parameter from S-expression
    /// @param sexpr Parsed S-expression containing parameters
    /// @param param_name Parameter name to search for (without :)
    /// @return Parameter value or empty string if not found
    static std::string extractStringParam(const ::lab::Text::Sexpr& sexpr, 
                                         const std::string& param_name);
    
    /// Generate rich entity JSON with resolved EID chains
    /// @param eid Entity ID
    /// @param value Entity value
    /// @param store NonoStore for EID resolution (optional)
    /// @return JSON representation with optional chain resolution data
    static std::string generateRichEntityJson(const std::string& eid, 
                                            const std::string& value,
                                            std::shared_ptr<NonoStore> store = nullptr);
                                            
    /// Generate rich triple JSON with resolved EID chains for all components
    /// @param tid Triple ID
    /// @param subject_eid Subject entity ID
    /// @param subject_value Subject entity value
    /// @param predicate_eid Predicate entity ID  
    /// @param predicate_value Predicate entity value
    /// @param object_eid Object entity ID
    /// @param object_value Object entity value
    /// @param store NonoStore for EID resolution
    /// @return JSON representation with resolved chains for all triple components
    static std::string generateRichTripleJson(const std::string& tid,
                                            const std::string& subject_eid, const std::string& subject_value,
                                            const std::string& predicate_eid, const std::string& predicate_value,
                                            const std::string& object_eid, const std::string& object_value,
                                            std::shared_ptr<NonoStore> store);
                                            
    /// Generate database diagnosis message for missing/invalid dbid parameters
    /// @param operation_name Name of the operation that failed
    /// @return Human-readable diagnosis with suggestions
    static std::string generateDbidDiagnosisMessage(const std::string& operation_name);
    
private:
    // Internal helper for debug file management
    static void writeDebugLog(const std::string& message, bool enable_debug);
};

} // namespace LabDb
