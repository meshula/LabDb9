#include "RopeTraverseVerb.h"
#include "LabDb/DatabaseManager.h"
#include "LabDb/LabText.hpp"
#include <sstream>

namespace LabDb {

std::string RopeTraverseVerb::getDescription() const {
    return R"Rope({
        "verb": "rope-traverse",
        "description": "Traverse rope and retrieve entity sequences (sequential navigation)",
        "parameters": {
            "rope-name": "Required. Name of the rope to traverse",
            "dbid": "Required. Database identifier",
            "start-position": "Optional. Starting position (0-based, default: 0)",
            "count": "Optional. Number of entities to retrieve (default: 10)", 
            "include-content": "Optional. Whether to fetch entity content (default: false)",
            "include-metadata": "Optional. Whether to include entity metadata (default: false)"
        },
        "examples": [
            "(rope-traverse :rope-name \"euclid-complete-text\" :start-position 100 :count 25 :dbid db1)",
            "(rope-traverse :rope-name \"golden-ratio-study\" :include-content true :dbid db1)",
            "(rope-traverse :rope-name \"memex-trail\" :start-position 0 :count 5 :include-metadata true :dbid db1)"
        ],
        "response": {
            "status": "traversal_complete",
            "rope_name": "string",
            "start_position": "number",
            "count_requested": "number",
            "count_returned": "number",
            "entities": [
                {
                    "entity_id": "string",
                    "position": "number",
                    "content": "string (if requested)",
                    "metadata": "object (if requested)"
                }
            ],
            "traversal_info": {
                "has_more_forward": "boolean",
                "has_more_backward": "boolean",
                "total_rope_length": "number"
            }
        }
    })Rope";
}

RopeTraverseVerb::TraverseParameters RopeTraverseVerb::extractParameters(const lab::Text::Sexpr& sexpr) {
    TraverseParameters params;
    
    // Extract required parameters
    params.rope_name = extractStringParam(sexpr, "rope-name");
    params.dbid = extractStringParam(sexpr, "dbid");
    
    // Extract optional parameters
    // Optional start-position parameter
    std::string start_str = extractStringParam(sexpr, "start-position");
    if (!start_str.empty()) {
        params.start_position = static_cast<size_t>(std::stoi(start_str));
    }
    
    // Optional count parameter
    std::string count_str = extractStringParam(sexpr, "count");
    if (!count_str.empty()) {
        params.count = static_cast<size_t>(std::stoi(count_str));
    }
    
    // Optional include-content parameter
    std::string content_str = extractStringParam(sexpr, "include-content");
    if (!content_str.empty()) {
        params.include_content = (content_str == "true");
    }
    
    // Optional include-metadata parameter
    std::string metadata_str = extractStringParam(sexpr, "include-metadata");
    if (!metadata_str.empty()) {
        params.include_metadata = (metadata_str == "true");
    }
    
    return params;
}

Db9Response RopeTraverseVerb::execute(const lab::Text::Sexpr& sexpr) {
    try {
        auto params = extractParameters(sexpr);
        return performTraverse(params);
    } catch (const std::exception& e) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "rope-traverse execution failed: " + std::string(e.what());
        return response;
    }
}

Db9Response RopeTraverseVerb::performTraverse(const TraverseParameters& params) {
    // Validate rope name
    if (!RopeUtils::isValidRopeName(params.rope_name)) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "Invalid rope name: " + params.rope_name;
        return response;
    }
    
    // Get entity sequence
    auto entities = getEntitySequence(params);
    
    // Fetch content if requested
    if (params.include_content) {
        fetchEntityContent(entities, params.dbid);
    }
    
    // Generate response
    auto response_json = formatTraverseResponse(params, entities);
    
    Db9Response response;
    response.status = Db9Response::Success;
    response.result = response_json;
    return response;
}

std::vector<RopeTraverseVerb::EntityInfo> RopeTraverseVerb::getEntitySequence(const TraverseParameters& params) {
    std::vector<EntityInfo> entities;
    
    auto& manager = DatabaseManager::instance();
    auto store = manager.getDatabase(params.dbid);
    if (!store) {
        return entities; // Empty result if database not found
    }
    
    for (size_t i = 0; i < params.count; ++i) {
        size_t position = params.start_position + i;
        
        // Generate sequence ID and rope key
        std::string sequence_id = RopeUtils::generateSequenceId(position);
        std::string rope_key = RopeUtils::generateRopeKey(params.rope_name, sequence_id);
        
        // Look up entity_id from rope_key in database: rope_key -> contains -> entity_id
        auto results = store->query(rope_key, "contains", "*");
        
        if (!results.empty()) {
            EntityInfo info;
            info.entity_id = results[0].object; // The entity_id is the object of the triple
            info.position = position;
            
            entities.push_back(info);
        }
        // If no entity found at this position, we've reached the end of the rope
        else {
            break;
        }
    }
    
    return entities;
}

void RopeTraverseVerb::fetchEntityContent(std::vector<EntityInfo>& entities, const std::string& dbid) {
    auto& manager = DatabaseManager::instance();
    auto store = manager.getDatabase(dbid);
    if (!store) {
        return; // Can't fetch content without database
    }
    
    for (auto& entity : entities) {
        // Fetch entity content: entity_id -> text-content -> actual_text
        auto content_results = store->query(entity.entity_id, "text-content", "*");
        if (!content_results.empty()) {
            entity.content = content_results[0].object;
        } else {
            entity.content = "[content not found]";
        }
        
        // Fetch entity metadata by gathering all predicates for this entity
        auto metadata_results = store->query(entity.entity_id, "*", "*");
        
        std::ostringstream metadata_json;
        metadata_json << "{";
        bool first = true;
        for (const auto& result : metadata_results) {
            if (result.predicate != "text-content") { // Skip content, it's already captured
                if (!first) metadata_json << ", ";
                metadata_json << "\"" << result.predicate << "\": \"" << result.object << "\"";
                first = false;
            }
        }
        metadata_json << "}";
        
        entity.metadata = metadata_json.str();
    }
}

std::string RopeTraverseVerb::formatTraverseResponse(const TraverseParameters& params,
                                                   const std::vector<EntityInfo>& entities) {
    std::ostringstream json;
    
    json << "{"
         << "\"status\": \"traversal_complete\""
         << ", \"rope_name\": \"" << params.rope_name << "\""
         << ", \"start_position\": " << params.start_position
         << ", \"count_requested\": " << params.count
         << ", \"count_returned\": " << entities.size()
         << ", \"entities\": [";
    
    for (size_t i = 0; i < entities.size(); ++i) {
        if (i > 0) json << ", ";
        
        json << "{"
             << "\"entity_id\": \"" << entities[i].entity_id << "\""
             << ", \"position\": " << entities[i].position;
        
        if (params.include_content) {
            json << ", \"content\": \"" << entities[i].content << "\"";
        }
        
        if (params.include_metadata) {
            json << ", \"metadata\": " << entities[i].metadata;
        }
        
        json << "}";
    }
    
    json << "]";
    
    // Calculate actual rope length from metadata
    size_t total_rope_length = 0;
    auto& manager = DatabaseManager::instance();
    auto store = manager.getDatabase(params.dbid);
    if (store) {
        std::string metadata_key = "rope:meta:" + params.rope_name;
        auto size_results = store->query(metadata_key, "size", "*");
        if (!size_results.empty()) {
            try {
                total_rope_length = static_cast<size_t>(std::stoi(size_results[0].object));
            } catch (const std::exception&) {
                total_rope_length = 0;
            }
        }
    }
    
    bool has_more_forward = (params.start_position + entities.size()) < total_rope_length;
    bool has_more_backward = params.start_position > 0;
    
    json << ", \"traversal_info\": {"
         << "\"has_more_forward\": " << (has_more_forward ? "true" : "false")
         << ", \"has_more_backward\": " << (has_more_backward ? "true" : "false")
         << ", \"total_rope_length\": " << total_rope_length
         << "}";
    
    json << "}";
    
    return json.str();
}

} // namespace LabDb