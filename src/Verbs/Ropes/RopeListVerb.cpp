#include "RopeListVerb.h"
#include "LabDb/LabText.hpp"
#include <sstream>
#include <algorithm>

namespace LabDb {

std::string RopeListVerb::getDescription() const {
    return R"Rope({
        "verb": "rope-list",
        "description": "List all available ropes in database",
        "parameters": {
            "dbid": "Required. Database identifier",
            "pattern": "Optional. Filter pattern for rope names (default: '*' for all)",
            "include-stats": "Optional. Whether to include entity counts (default: true)"
        },
        "examples": [
            "(rope-list :dbid db1)",
            "(rope-list :pattern \"euclid*\" :dbid db1)",
            "(rope-list :pattern \"*study*\" :include-stats false :dbid db1)"
        ],
        "response": {
            "status": "ropes_listed",
            "count": "number",
            "ropes": [
                {
                    "name": "string",
                    "description": "string",
                    "entity_count": "number (if include-stats true)",
                    "created_at": "string"
                }
            ]
        }
    })Rope";
}

RopeListVerb::ListParameters RopeListVerb::extractParameters(const lab::Text::Sexpr& sexpr) {
    ListParameters params;
    
    // Extract required parameters
    params.dbid = extractStringParam(sexpr, "dbid");
    
    // Extract optional parameters
    // Optional pattern parameter
    std::string pattern_str = extractStringParam(sexpr, "pattern");
    if (!pattern_str.empty()) {
        params.pattern = pattern_str;
    }
    
    // Optional include-stats parameter
    std::string stats_str = extractStringParam(sexpr, "include-stats");
    if (!stats_str.empty()) {
        params.include_stats = (stats_str == "true");
    }
    
    return params;
}

Db9Response RopeListVerb::execute(const lab::Text::Sexpr& sexpr) {
    try {
        auto params = extractParameters(sexpr);
        return performList(params);
    } catch (const std::exception& e) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "rope-list execution failed: " + std::string(e.what());
        return response;
    }
}

Db9Response RopeListVerb::performList(const ListParameters& params) {
    // Get all ropes
    auto ropes = getRopeList(params);
    
    // Generate response
    std::ostringstream json;
    
    json << "{"
         << "\"status\": \"ropes_listed\""
         << ", \"count\": " << ropes.size()
         << ", \"ropes\": [";
    
    for (size_t i = 0; i < ropes.size(); ++i) {
        if (i > 0) json << ", ";
        
        json << "{"
             << "\"name\": \"" << ropes[i].name << "\""
             << ", \"description\": \"" << ropes[i].description << "\"";
        
        if (params.include_stats) {
            json << ", \"entity_count\": " << ropes[i].entity_count;
        }
        
        json << ", \"created_at\": \"" << ropes[i].created_at << "\""
             << "}";
    }
    
    json << "]}";
    
    Db9Response response;
    response.status = Db9Response::Success;
    response.result = json.str();
    return response;
}

std::vector<RopeListVerb::RopeInfo> RopeListVerb::getRopeList(const ListParameters& params) {
    std::vector<RopeInfo> ropes;
    
    // TODO: Scan database for all keys matching "rope:meta:*"
    // Parse metadata JSON to extract rope information
    // Filter by pattern if specified
    
    // Placeholder data
    if (matchesPattern("euclid-complete-text", params.pattern)) {
        RopeInfo rope;
        rope.name = "euclid-complete-text";
        rope.description = "Complete Euclid Elements text sequence";
        rope.entity_count = 10000;
        rope.created_at = "2025-09-10T12:00:00Z";
        ropes.push_back(rope);
    }
    
    if (matchesPattern("golden-ratio-study", params.pattern)) {
        RopeInfo rope;
        rope.name = "golden-ratio-study";
        rope.description = "Golden ratio concept development";
        rope.entity_count = 42;
        rope.created_at = "2025-09-10T12:15:00Z";
        ropes.push_back(rope);
    }
    
    return ropes;
}

bool RopeListVerb::matchesPattern(const std::string& name, const std::string& pattern) {
    if (pattern == "*") {
        return true;  // Match all
    }
    
    // TODO: Implement proper glob pattern matching
    // For now, just check if pattern is contained in name
    return name.find(pattern) != std::string::npos;
}

} // namespace LabDb