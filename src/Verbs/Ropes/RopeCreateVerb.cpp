#include "RopeCreateVerb.h"
#include "LabDb/DatabaseManager.h"
#include <sstream>
#include <chrono>

namespace LabDb {

std::string RopeCreateVerb::getDescription() const {
    return R"Rope({
        "verb": "rope-create",
        "description": "Create a new rope for ordered entity sequences",
        "parameters": {
            "rope-name": "Required. Name of the rope (alphanumeric, hyphens, underscores only)",
            "dbid": "Required. Database identifier",
            "description": "Optional. Human-readable description of the rope purpose",
            "overwrite": "Optional. Whether to overwrite existing rope (default: false)"
        },
        "examples": [
            "(rope-create :rope-name \"euclid-complete-text\" :description \"Complete Euclid Elements text sequence\" :dbid db1)",
            "(rope-create :rope-name \"golden-ratio-study\" :description \"Golden ratio concept development\" :dbid db1)",
            "(rope-create :rope-name \"memex-trail-001\" :description \"Associative trail for proportion theory\" :overwrite true :dbid db1)"
        ],
        "response": {
            "status": "rope_created",
            "rope_name": "string",
            "description": "string",
            "created_at": "timestamp",
            "metadata_key": "string"
        }
    })Rope";
}


std::string RopeCreateVerb::getContextualHelp() const {
    return "Rope creation for ordered entity sequences. "
           "Use alphanumeric characters, hyphens, and underscores for rope names. "
           "The :overwrite parameter allows replacing existing ropes.";
}

RopeCreateVerb::CreateParameters RopeCreateVerb::extractParameters(const lab::Text::Sexpr& sexpr) {
    CreateParameters params;
    
    params.rope_name = extractStringParam(sexpr, "rope-name");
    params.dbid = extractStringParam(sexpr, "dbid");
    params.description = extractStringParam(sexpr, "description");
    
    std::string overwrite_str = extractStringParam(sexpr, "overwrite");
    if (!overwrite_str.empty()) {
        params.overwrite = (overwrite_str == "true");
    }
    
    return params;
}

Db9Response RopeCreateVerb::execute(const lab::Text::Sexpr& sexpr) {
    try {
        auto params = extractParameters(sexpr);
        
        // Validate required parameters
        if (params.rope_name.empty()) {
            Db9Response response;
            response.status = Db9Response::Error;
            response.error_message = "rope-name parameter is required";
            return response;
        }
        if (params.dbid.empty()) {
            Db9Response response;
            response.status = Db9Response::Error;
            response.error_message = "dbid parameter is required";
            return response;
        }
        
        return performCreate(params);
        
    } catch (const std::exception& e) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "rope-create execution failed: " + std::string(e.what());
        return response;
    }
}

Db9Response RopeCreateVerb::performCreate(const CreateParameters& params) {
    if (!RopeUtils::isValidRopeName(params.rope_name)) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "Invalid rope name format. Use alphanumeric, hyphens, underscores only.";
        return response;
    }
    
    if (ropeExists(params.rope_name, params.dbid) && !params.overwrite) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "Rope '" + params.rope_name + "' already exists. Use :overwrite true to replace it.";
        return response;
    }
    
    if (!createRopeMetadata(params)) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "Failed to create rope metadata";
        return response;
    }
    
    // Generate success response with real timestamp
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    
    std::ostringstream json;
    json << "{"
         << "\"status\": \"rope_created\""
         << ", \"rope_name\": \"" << params.rope_name << "\""
         << ", \"description\": \"" << params.description << "\""
         << ", \"created_at\": " << timestamp
         << ", \"metadata_key\": \"rope:meta:" << params.rope_name << "\""
         << "}";
    
    Db9Response response;
    response.status = Db9Response::Success;
    response.result = json.str();
    return response;
}
bool RopeCreateVerb::ropeExists(const std::string& rope_name, const std::string& dbid) {
    auto& manager = DatabaseManager::instance();
    auto store = manager.getDatabase(dbid);
    if (!store) return false;
    
    std::string metadata_key = "rope:meta:" + rope_name;
    auto results = store->query(metadata_key, "*", "*");
    return !results.empty();
}

bool RopeCreateVerb::createRopeMetadata(const CreateParameters& params) {
    auto& manager = DatabaseManager::instance();
    auto store = manager.getDatabase(params.dbid);
    if (!store) return false;
    
    std::string metadata_key = "rope:meta:" + params.rope_name;
    std::stringstream metadata_ss;
    metadata_ss << "{\"name\": \"" << params.rope_name << "\", \"description\": \"" << params.description << "\"}";
    
    // Use simple approach: add metadata as a string relationship
    store->add_triple(metadata_key, "rope-metadata", metadata_ss.str());
    return true;
}

} // namespace LabDb