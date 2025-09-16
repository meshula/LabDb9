#include "RopeInsertVerb.h"
#include "LabDb/LabText.hpp"
#include <sstream>

namespace LabDb {

std::string RopeInsertVerb::getDescription() const {
    return R"Rope({
        "verb": "rope-insert",
        "description": "Insert entity at specific position in rope",
        "parameters": {
            "rope-name": "Required. Name of the rope",
            "entity-id": "Required. Entity to insert",
            "dbid": "Required. Database identifier",
            "position": "Required. Position to insert at (1-based)",
            "shift-existing": "Optional. Whether to shift existing entities (default: true)"
        },
        "examples": [
            "(rope-insert :rope-name \"euclid-text\" :entity-id \"b1-def-new\" :position 5 :dbid db1)",
            "(rope-insert :rope-name \"study-trail\" :entity-id \"insight-42\" :position 1 :shift-existing false :dbid db1)"
        ],
        "response": {
            "status": "entity_inserted",
            "rope_name": "string",
            "entity_id": "string",
            "position": "number",
            "entities_shifted": "number"
        }
    })Rope";
}

RopeInsertVerb::InsertParameters RopeInsertVerb::extractParameters(const lab::Text::Sexpr& sexpr) {
    InsertParameters params;
    
    params.rope_name = extractStringParam(sexpr, "rope-name");
    params.entity_id = extractStringParam(sexpr, "entity-id");
    params.dbid = extractStringParam(sexpr, "dbid");
    
    // Optional position parameter
    std::string pos_str = extractStringParam(sexpr, "position");
    if (!pos_str.empty()) {
        params.position = static_cast<size_t>(std::stoi(pos_str));
    }
    
    // Optional shift-existing parameter
    std::string shift_str = extractStringParam(sexpr, "shift-existing");
    if (!shift_str.empty()) {
        params.shift_existing = (shift_str == "true");
    }
    
    return params;
}

Db9Response RopeInsertVerb::execute(const lab::Text::Sexpr& sexpr) {
    try {
        auto params = extractParameters(sexpr);
        return performInsert(params);
    } catch (const std::exception& e) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "rope-insert execution failed: " + std::string(e.what());
        return response;
    }
}

Db9Response RopeInsertVerb::performInsert(const InsertParameters& params) {
    if (!RopeUtils::isValidRopeName(params.rope_name)) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "Invalid rope name: " + params.rope_name;
        return response;
    }
    
    if (params.position == 0) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "Position must be 1-based (1, 2, 3, ...)";
        return response;
    }
    
    // TODO: Implement actual insertion logic with entity shifting
    
    std::ostringstream json;
    json << "{"
         << "\"status\": \"entity_inserted\""
         << ", \"rope_name\": \"" << params.rope_name << "\""
         << ", \"entity_id\": \"" << params.entity_id << "\""
         << ", \"position\": " << params.position
         << ", \"entities_shifted\": 0"  // TODO: Calculate actual shifted count
         << "}";
    
    Db9Response response;
    response.status = Db9Response::Success;
    response.result = json.str();
    return response;
}

} // namespace LabDb