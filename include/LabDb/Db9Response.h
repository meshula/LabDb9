#pragma once

#include "LabDb/AutoReflexiveMetrics.h"
#include <string>

namespace LabDb {

//-----------------------------------------------------------------------------
// Standardized response format for all db9 operations
//-----------------------------------------------------------------------------
struct Db9Response {
    enum Status { Success, Error, Warning } status{Success};
    std::string result;                    // Primary return data
    std::string error_code;               // Optional error identifier
    std::string error_message;            // Human-readable error description
    AutoReflexiveMetrics auto_reflexive;  // Performance metrics
    
    // Convert to JSON for MCP return
    std::string toJson() const;
};

} // namespace LabDb
