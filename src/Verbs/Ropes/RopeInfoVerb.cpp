#include "RopeInfoVerb.h"
#include "LabDb/LabText.hpp"
#include <sstream>

namespace LabDb {

std::string RopeInfoVerb::getDescription() const {
    return R"Rope({
        "verb": "rope-info",
        "description": "Get rope information and statistics",
        "parameters": {
            "rope-name": "Required. Name of the rope to get info for",
            "dbid": "Required. Database identifier",
            "include-entities": "Optional. Whether to list sample entities (default: false)",
            "max-entities": "Optional. Maximum entities to show if include-entities true (default: 10)"
        },
        "examples": [
            "(rope-info :rope-name \"euclid-complete-text\" :dbid db1)",
            "(rope-info :rope-name \"golden-ratio-study\" :include-entities true :max-entities 5 :dbid db1)"
        ],
        "response": {
            "status": "rope_info_retrieved",
            "rope_name": "string",
            "description": "string",
            "entity_count": "number",
            "created_at": "string",
            "sample_entities": ["entity1", "entity2", "..."] 
        }
    })Rope";
}

RopeInfoVerb::InfoParameters RopeInfoVerb::extractParameters(const lab::Text::Sexpr& sexpr) {
    InfoParameters params;
    
    params.rope_name = extractStringParam(sexpr, "rope-name");
    params.dbid = extractStringParam(sexpr, "dbid");
    
    // Optional include-entities parameter
    std::string include_str = extractStringParam(sexpr, "include-entities");
    if (!include_str.empty()) {
        params.include_entities = (include_str == "true");
    }
    
    // Optional max-entities parameter
    std::string max_str = extractStringParam(sexpr, "max-entities");
    if (!max_str.empty()) {
        params.max_entities = static_cast<size_t>(std::stoi(max_str));
    }
    
    return params;
}

Db9Response RopeInfoVerb::execute(const lab::Text::Sexpr& sexpr) {
    try {
        auto params = extractParameters(sexpr);
        return performInfo(params);
    } catch (const std::exception& e) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "rope-info execution failed: " + std::string(e.what());
        return response;
    }
}

Db9Response RopeInfoVerb::performInfo(const InfoParameters& params) {
    if (!RopeUtils::isValidRopeName(params.rope_name)) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "Invalid rope name: " + params.rope_name;
        return response;
    }
    
    // TODO: Get actual rope metadata from database
    
    std::ostringstream json;
    json << "{"
         << "\"status\": \"rope_info_retrieved\""
         << ", \"rope_name\": \"" << params.rope_name << "\""
         << ", \"description\": \"Sample rope description\""
         << ", \"entity_count\": 100"
         << ", \"created_at\": \"2025-09-10T12:00:00Z\"";
    
    if (params.include_entities) {
        json << ", \"sample_entities\": [";
        for (size_t i = 0; i < params.max_entities && i < 3; ++i) {
            if (i > 0) json << ", ";
            json << "\"entity_" << i << "\"";
        }
        json << "]";
    }
    
    json << "}";
    
    Db9Response response;
    response.status = Db9Response::Success;
    response.result = json.str();
    return response;
}

} // namespace LabDb