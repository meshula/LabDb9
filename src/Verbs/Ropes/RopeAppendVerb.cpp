#include "RopeAppendVerb.h"
#include "LabDb/DatabaseManager.h"
#include <sstream>
#include <chrono>

namespace LabDb {

std::string RopeAppendVerb::getDescription() const {
    return R"Rope({
        "verb": "rope-append",
        "description": "Append entity to rope sequence",
        "parameters": {
            "rope-name": "Required. Name of the rope to append to",
            "entity-id": "Required. Entity identifier to append",
            "dbid": "Required. Database identifier",
            "position": "Optional. Position to insert at (0 = append at end, default)",
            "validate-entity": "Optional. Whether to check if entity exists (default: true)"
        },
        "examples": [
            "(rope-append :rope-name \"euclid-complete-text\" :entity-id \"b1-def-1\" :dbid db1)",
            "(rope-append :rope-name \"golden-ratio-study\" :entity-id \"b6-def-3\" :position 5 :dbid db1)",
            "(rope-append :rope-name \"memex-trail\" :entity-id \"insight-42\" :validate-entity false :dbid db1)"
        ],
        "response": {
            "status": "entity_appended",
            "rope_name": "string",
            "entity_id": "string",
            "position": "number",
            "rope_length": "number",
            "sequence_id": "string"
        }
    }
)Rope";
}

RopeAppendVerb::AppendParameters RopeAppendVerb::extractParameters(const lab::Text::Sexpr& sexpr) {
    AppendParameters params;
    
    // Extract required parameters
    params.rope_name = extractStringParam(sexpr, "rope-name");
    params.entity_id = extractStringParam(sexpr, "entity-id");
    params.dbid = extractStringParam(sexpr, "dbid");
    
    // Extract optional parameters
    // Optional position parameter
    std::string position_str = extractStringParam(sexpr, "position");
    if (!position_str.empty()) {
        params.position = static_cast<size_t>(std::stoi(position_str));
    }
    
    // Optional validate-entity parameter
    std::string validate_str = extractStringParam(sexpr, "validate-entity");
    if (!validate_str.empty()) {
        params.validate_entity = (validate_str == "true");
    }
    
    return params;
}

Db9Response RopeAppendVerb::execute(const lab::Text::Sexpr& sexpr) {
    try {
        auto params = extractParameters(sexpr);
        return performAppend(params);
    } catch (const std::exception& e) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "rope-append execution failed: " + std::string(e.what());
        return response;
    }
}

Db9Response RopeAppendVerb::performAppend(const AppendParameters& params) {
    // Validate rope name
    if (!RopeUtils::isValidRopeName(params.rope_name)) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "Invalid rope name: " + params.rope_name;
        return response;
    }
    
    // Validate entity exists if requested
    if (params.validate_entity && !entityExists(params.entity_id, params.dbid)) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "Entity not found: " + params.entity_id;
        return response;
    }
    
    // Get current rope length
    size_t current_length = getRopeLength(params.rope_name, params.dbid);
    
    // Determine actual position
    size_t actual_position;
    if (params.position == 0) {
        // Append at end
        actual_position = current_length;
    } else {
        // Insert at specified position (1-based to 0-based)
        actual_position = params.position - 1;
        if (actual_position > current_length) {
            Db9Response response;
            response.status = Db9Response::Error;
            response.error_message = "Position beyond rope length: " + std::to_string(params.position);
            return response;
        }
    }
    
    // Add entity to rope
    if (!addEntityToRope(params, actual_position)) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "Failed to add entity to rope";
        return response;
    }
    
    // Update rope metadata
    size_t new_length = current_length + 1;
    if (!updateRopeMetadata(params.rope_name, params.dbid, new_length)) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "Failed to update rope metadata";
        return response;
    }
    
    // Generate success response
    std::string sequence_id = RopeUtils::generateSequenceId(actual_position);
    
    std::ostringstream json;
    json << "{"
         << "\"status\": \"entity_appended\""
         << ", \"rope_name\": \"" << params.rope_name << "\""
         << ", \"entity_id\": \"" << params.entity_id << "\""
         << ", \"position\": " << (actual_position + 1)  // Return 1-based position
         << ", \"rope_length\": " << new_length
         << ", \"sequence_id\": \"" << sequence_id << "\""
         << "}";
    
    Db9Response response;
    response.status = Db9Response::Success;
    response.result = json.str();
    return response;
}

size_t RopeAppendVerb::getRopeLength(const std::string& rope_name, const std::string& dbid) {
    auto& manager = DatabaseManager::instance();
    auto store = manager.getDatabase(dbid);
    if (!store) {
        return 0; // Database not found
    }
    
    // Query for rope metadata: rope:{name}:metadata -> size=count
    std::string metadata_key = "rope:" + rope_name + ":metadata";
    
    // Query triples where metadata_key is the subject and predicate is "size"
    auto results = store->query(metadata_key, "size", "*");
    
    if (results.empty()) {
        return 0; // No metadata found, rope doesn't exist or is empty
    }
    
    // Parse the size from the object value
    try {
        return static_cast<size_t>(std::stoi(results[0].object));
    } catch (const std::exception&) {
        return 0; // Invalid size format
    }
}

std::string RopeAppendVerb::resolveEntityId(const std::string& entity_id, const std::string& dbid) {
    auto& manager = DatabaseManager::instance();
    auto store = manager.getDatabase(dbid);
    if (!store) {
        return ""; // Database not found
    }
    
    // Method 1: Check if entity_id is already an EID (direct subject lookup)
    auto direct_results = store->query(entity_id, "*", "*");
    if (!direct_results.empty()) {
        return entity_id; // entity_id is already a valid EID
    }
    
    // Method 2: Resolve entity_id as a VALUE (reverse lookup to find EID)
    // Search for any entity whose value is entity_id
    auto all_subjects = store->all_subjects();
    for (const auto& subject : all_subjects) {
        // Check if this subject has entity_id as its value
        auto value_results = store->query(subject, "has-value", entity_id);
        if (!value_results.empty()) {
            return subject; // Found the EID for this value
        }
    }
    
    return ""; // Entity not found as either EID or VALUE
}

bool RopeAppendVerb::entityExists(const std::string& entity_id, const std::string& dbid) {
    auto& manager = DatabaseManager::instance();
    auto store = manager.getDatabase(dbid);
    if (!store) {
        return false; // Database not found
    }
    
    // ENHANCED: Try to resolve entity_id as either EID or VALUE
    
    // Method 1: Check if entity_id is already an EID (direct subject lookup)
    auto direct_results = store->query(entity_id, "*", "*");
    if (!direct_results.empty()) {
        return true; // entity_id is a valid EID
    }
    
    // Method 2: Check if entity_id is a VALUE (reverse lookup to find EID)
    // Search for any entity whose value is entity_id
    auto all_subjects = store->all_subjects();
    for (const auto& subject : all_subjects) {
        // Check if this subject has entity_id as its value
        auto value_results = store->query(subject, "has-value", entity_id);
        if (!value_results.empty()) {
            return true; // Found an entity with entity_id as its value
        }
    }
    
    return false; // Entity not found as either EID or VALUE
}

bool RopeAppendVerb::addEntityToRope(const AppendParameters& params, size_t actual_position) {
    auto& manager = DatabaseManager::instance();
    auto store = manager.getDatabase(params.dbid);
    if (!store) {
        return false; // Database not found
    }
    
    // CRITICAL: Resolve entity_id to actual EID before storing
    std::string resolved_eid = resolveEntityId(params.entity_id, params.dbid);
    if (resolved_eid.empty()) {
        return false; // Entity resolution failed
    }
    
    // Generate rope key: rope:{name}:{seq_id}
    std::string sequence_id = RopeUtils::generateSequenceId(actual_position);
    std::string rope_key = RopeUtils::generateRopeKey(params.rope_name, sequence_id);
    
    // Store the mapping: rope_key -> resolved_eid (not the original entity_id!)
    // Using "contains" as the predicate to indicate the rope contains this entity
    bool success = store->add_triple(rope_key, "contains", resolved_eid);
    
    return success;
}

bool RopeAppendVerb::updateRopeMetadata(const std::string& rope_name, 
                                      const std::string& dbid, 
                                      size_t new_length) {
    auto& manager = DatabaseManager::instance();
    auto store = manager.getDatabase(dbid);
    if (!store) {
        return false; // Database not found
    }
    
    std::string metadata_key = "rope:" + rope_name + ":metadata";
    
    // Remove old size entry if it exists
    auto old_results = store->query(metadata_key, "size", "*");
    for (const auto& result : old_results) {
        store->remove_triple(metadata_key, "size", result.object);
    }
    
    // Add new size entry
    bool success = store->add_triple(metadata_key, "size", std::to_string(new_length));
    
    if (success) {
        // Also update the "created" timestamp if this is the first entity (length was 0)
        if (new_length == 1) {
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
            store->add_triple(metadata_key, "created", std::to_string(timestamp));
        }
        
        // Update "modified" timestamp
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        
        // Remove old modified timestamp
        auto old_modified = store->query(metadata_key, "modified", "*");
        for (const auto& result : old_modified) {
            store->remove_triple(metadata_key, "modified", result.object);
        }
        
        // Add new modified timestamp
        store->add_triple(metadata_key, "modified", std::to_string(timestamp));
    }
    
    return success;
}

} // namespace LabDb