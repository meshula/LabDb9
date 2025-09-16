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

RopeCreateVerb::CreateParameters RopeCreateVerb::extractParameters(const lab::Text::Sexpr& sexpr) {
    CreateParameters params;
    
    // Extract required parameters
    params.rope_name = IDb9Verb::extractStringParam(sexpr, "rope-name");
    params.dbid = IDb9Verb::extractStringParam(sexpr, "dbid");
    
    // Extract optional parameters
    params.description = IDb9Verb::extractStringParam(sexpr, "description");
    
    // Optional overwrite parameter
    std::string overwrite_str = IDb9Verb::extractStringParam(sexpr, "overwrite");
    if (!overwrite_str.empty()) {
        params.overwrite = (overwrite_str == "true");
    }
    
    return params;
}

Db9Response RopeCreateVerb::execute(const lab::Text::Sexpr& sexpr) {
    try {
        auto params = extractParameters(sexpr);
        return performCreate(params);
    } catch (const std::exception& e) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "rope-create execution failed: " + std::string(e.what());
        return response;
    }
}

Db9Response RopeCreateVerb::performCreate(const CreateParameters& params) {
    // Validate rope name
    if (!RopeUtils::isValidRopeName(params.rope_name)) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "Invalid rope name. Use alphanumeric characters, hyphens, and underscores only.";
        return response;
    }
    
    // Check if rope already exists
    if (ropeExists(params.rope_name, params.dbid) && !params.overwrite) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "Rope already exists: " + params.rope_name + ". Use :overwrite true to replace.";
        return response;
    }
    
    // Create rope metadata
    if (!createRopeMetadata(params)) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "Failed to create rope metadata for: " + params.rope_name;
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
    if (!store) {
        return false; // Database not found
    }
    
    // Check if rope metadata exists: rope:meta:{rope_name}
    std::string metadata_key = "rope:meta:" + rope_name;
    
    // Query for any triples with metadata_key as subject
    auto results = store->query(metadata_key, "*", "*");
    
    return !results.empty();
}

bool RopeCreateVerb::createRopeMetadata(const CreateParameters& params) {
    auto& manager = DatabaseManager::instance();
    auto store = manager.getDatabase(params.dbid);
    if (!store) {
        return false; // Database not found
    }
    
    std::string metadata_key = "rope:meta:" + params.rope_name;
    
    // If overwriting, remove existing metadata first
    if (params.overwrite) {
        auto old_results = store->query(metadata_key, "*", "*");
        for (const auto& result : old_results) {
            store->remove_triple(metadata_key, result.predicate, result.object);
        }
    }
    
    // Create metadata triples
    bool success = true;
    success &= store->add_triple(metadata_key, "name", params.rope_name);
    success &= store->add_triple(metadata_key, "description", params.description.empty() ? "" : params.description);
    success &= store->add_triple(metadata_key, "size", "0");  // Initial size
    
    // Add timestamp
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    success &= store->add_triple(metadata_key, "created", std::to_string(timestamp));
    success &= store->add_triple(metadata_key, "modified", std::to_string(timestamp));
    
    return success;
}

} // namespace LabDb