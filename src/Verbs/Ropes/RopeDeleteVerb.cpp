#include "RopeDeleteVerb.h"
#include "LabDb/LabText.hpp"
#include <sstream>

namespace LabDb {

std::string RopeDeleteVerb::getDescription() const {
    return R"Rope({
        "verb": "rope-delete",
        "description": "Delete rope or remove entities from rope",
        "parameters": {
            "rope-name": "Required. Name of the rope",
            "dbid": "Required. Database identifier",
            "entity-id": "Optional. Specific entity to remove from rope",
            "delete-entire-rope": "Optional. Delete the entire rope (default: false)",
            "confirm": "Required for safety. Must be true to proceed with deletion"
        },
        "examples": [
            "(rope-delete :rope-name \"test-rope\" :delete-entire-rope true :confirm true :dbid db1)",
            "(rope-delete :rope-name \"euclid-text\" :entity-id \"b1-def-1\" :confirm true :dbid db1)"
        ],
        "response": {
            "status": "rope_deleted | entity_removed",
            "rope_name": "string",
            "entity_id": "string (if entity removal)",
            "entities_removed": "number"
        }
    })Rope";
}

RopeDeleteVerb::DeleteParameters RopeDeleteVerb::extractParameters(const lab::Text::Sexpr& sexpr) {
    DeleteParameters params;
    
    params.rope_name = extractStringParam(sexpr, "rope-name");
    params.dbid = extractStringParam(sexpr, "dbid");
    
    // Optional entity-id parameter
    std::string entity_str = extractStringParam(sexpr, "entity-id");
    if (!entity_str.empty()) {
        params.entity_id = entity_str;
    }
    
    // Optional delete-entire-rope parameter
    std::string delete_all_str = extractStringParam(sexpr, "delete-entire-rope");
    if (!delete_all_str.empty()) {
        params.delete_entire_rope = (delete_all_str == "true");
    }
    
    // Optional confirm parameter
    std::string confirm_str = extractStringParam(sexpr, "confirm");
    if (!confirm_str.empty()) {
        params.confirm = (confirm_str == "true");
    }
    
    return params;
}

Db9Response RopeDeleteVerb::execute(const lab::Text::Sexpr& sexpr) {
    try {
        auto params = extractParameters(sexpr);
        return performDelete(params);
    } catch (const std::exception& e) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "rope-delete execution failed: " + std::string(e.what());
        return response;
    }
}

Db9Response RopeDeleteVerb::performDelete(const DeleteParameters& params) {
    if (!params.confirm) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "Safety confirmation required. Use :confirm true";
        return response;
    }
    
    if (!RopeUtils::isValidRopeName(params.rope_name)) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "Invalid rope name: " + params.rope_name;
        return response;
    }
    
    // TODO: Implement actual deletion logic
    
    std::ostringstream json;
    if (params.delete_entire_rope) {
        json << "{"
             << "\"status\": \"rope_deleted\""
             << ", \"rope_name\": \"" << params.rope_name << "\""
             << ", \"entities_removed\": 0"  // TODO: Get actual count
             << "}";
    } else {
        json << "{"
             << "\"status\": \"entity_removed\""
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