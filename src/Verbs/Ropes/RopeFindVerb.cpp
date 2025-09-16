#include "RopeFindVerb.h"
#include "LabDb/LabText.hpp"
#include <sstream>

namespace LabDb {

std::string RopeFindVerb::getDescription() const {
    return R"Rope({
        "verb": "rope-find",
        "description": "Find entity position in rope by entity_id",
        "parameters": {
            "rope-name": "Required. Name of the rope to search",
            "entity-id": "Required. Entity to find",
            "dbid": "Required. Database identifier",
            "include-context": "Optional. Include surrounding entities (default: false)",
            "context-size": "Optional. Number of context entities before/after (default: 5)"
        },
        "examples": [
            "(rope-find :rope-name \"euclid-text\" :entity-id \"b6-def-3\" :dbid db1)",
            "(rope-find :rope-name \"study-trail\" :entity-id \"key-insight\" :include-context true :context-size 3 :dbid db1)"
        ],
        "response": {
            "status": "entity_found | entity_not_found",
            "rope_name": "string",
            "entity_id": "string",
            "position": "number (if found)",
            "sequence_id": "string (if found)",
            "context_entities": ["entity1", "entity2", "..."] 
        }
    })Rope";
}

RopeFindVerb::FindParameters RopeFindVerb::extractParameters(const lab::Text::Sexpr& sexpr) {
    FindParameters params;
    
    params.rope_name = extractStringParam(sexpr, "rope-name");
    params.entity_id = extractStringParam(sexpr, "entity-id");
    params.dbid = extractStringParam(sexpr, "dbid");
    
    // Optional include-context parameter
    std::string context_str = extractStringParam(sexpr, "include-context");
    if (!context_str.empty()) {
        params.include_context = (context_str == "true");
    }
    
    // Optional context-size parameter
    std::string size_str = extractStringParam(sexpr, "context-size");
    if (!size_str.empty()) {
        params.context_size = static_cast<size_t>(std::stoi(size_str));
    }
    
    return params;
}

Db9Response RopeFindVerb::execute(const lab::Text::Sexpr& sexpr) {
    try {
        auto params = extractParameters(sexpr);
        return performFind(params);
    } catch (const std::exception& e) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "rope-find execution failed: " + std::string(e.what());
        return response;
    }
}

Db9Response RopeFindVerb::performFind(const FindParameters& params) {
    if (!RopeUtils::isValidRopeName(params.rope_name)) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "Invalid rope name: " + params.rope_name;
        return response;
    }
    
    // TODO: Implement actual entity position lookup
    auto entity_pos = RopeUtils::findEntityPosition(params.rope_name, params.entity_id, params.dbid);
    
    std::ostringstream json;
    
    if (entity_pos.found) {
        json << "{"
             << "\"status\": \"entity_found\""
             << ", \"rope_name\": \"" << params.rope_name << "\""
             << ", \"entity_id\": \"" << params.entity_id << "\""
             << ", \"position\": " << (entity_pos.position + 1)  // Return 1-based position
             << ", \"sequence_id\": \"" << entity_pos.sequence_id << "\"";
        
        if (params.include_context) {
            json << ", \"context_entities\": [";
            // TODO: Get actual context entities
            json << "\"context_before\", \"" << params.entity_id << "\", \"context_after\"";
            json << "]";
        }
        
        json << "}";
    } else {
        json << "{"
             << "\"status\": \"entity_not_found\""
             << ", \"rope_name\": \"" << params.rope_name << "\""
             << ", \"entity_id\": \"" << params.entity_id << "\""
             << "}";
    }
    
    Db9Response response;
    response.status = Db9Response::Success;
    response.result = json.str();
    return response;
}

} // namespace LabDb