#include "RopeChunkVerb.h"
#include "LabDb/DatabaseManager.h"
#include "LabDb/LabText.hpp"
#include <sstream>
#include <algorithm>
#include <iostream>

namespace LabDb {

std::string RopeChunkVerb::getDescription() const {
    return R"Rope({
        "verb": "rope-chunk",
        "description": "Retrieve bounded text chunks for contemplative reading (core Memex functionality)",
        "parameters": {
            "rope-name": "Required. Name of the rope to traverse",
            "dbid": "Required. Database identifier", 
            "fragment-count": "Optional. Number of fragments in chunk (default: 24)",
            "center-entity": "Optional. Entity ID to center chunk around",
            "center-position": "Optional. Position to center chunk around (alternative to center-entity)",
            "include-text": "Optional. Whether to fetch entity text content (default: true)",
            "include-metadata": "Optional. Whether to include entity metadata (default: true)",
            "concatenate-text": "Optional. Provide ready-to-read concatenated text (default: false)"
        },
        "examples": [
            "(rope-chunk :rope-name \"euclid-complete-text\" :center-entity \"b6-def-3\" :fragment-count 24 :dbid db1)",
            "(rope-chunk :rope-name \"golden-ratio-study\" :center-position 42 :include-text true :dbid db1)",
            "(rope-chunk :rope-name \"memex-trail\" :center-entity \"key-insight\" :concatenate-text true :dbid db1)"
        ],
        "response": {
            "status": "chunk_retrieved",
            "rope_name": "string",
            "center_entity": "string", 
            "fragment_count": "number",
            "global_position": "number",
            "entities": ["entity_id1", "entity_id2", ...],
            "text_fragments": ["content1", "content2", ...],
            "concatenated_text": "string (if requested)",
            "chunk_bounds": {
                "start_position": "number",
                "end_position": "number", 
                "can_extend_backward": "boolean",
                "can_extend_forward": "boolean"
            },
            "navigation": {
                "preceding_chunk_available": "boolean",
                "succeeding_chunk_available": "boolean"
            }
        }
    })Rope";
}

RopeChunkVerb::ChunkParameters RopeChunkVerb::extractParameters(const lab::Text::Sexpr& sexpr) {
    ChunkParameters params;
    
    // Extract required parameters
    params.rope_name = extractStringParam(sexpr, "rope-name");
    params.dbid = extractStringParam(sexpr, "dbid");
    
    // Extract optional parameters with defaults
    // Optional fragment-count parameter
    std::string count_str = extractStringParam(sexpr, "fragment-count");
    if (!count_str.empty()) {
        params.fragment_count = static_cast<size_t>(std::stoi(count_str));
    }
    
    // Optional center-entity parameter
    std::string entity_str = extractStringParam(sexpr, "center-entity");
    if (!entity_str.empty()) {
        params.center_entity = entity_str;
    }
    
    // Optional center-position parameter
    std::string position_str = extractStringParam(sexpr, "center-position");
    if (!position_str.empty()) {
        params.center_position = static_cast<size_t>(std::stoi(position_str));
    }
    
    // Optional include-text parameter
    std::string include_text_str = extractStringParam(sexpr, "include-text");
    if (!include_text_str.empty()) {
        params.include_text = (include_text_str == "true");
    }
    
    // Optional include-metadata parameter
    std::string include_meta_str = extractStringParam(sexpr, "include-metadata");
    if (!include_meta_str.empty()) {
        params.include_metadata = (include_meta_str == "true");
    }
    
    // Optional concatenate-text parameter
    std::string concat_str = extractStringParam(sexpr, "concatenate-text");
    if (!concat_str.empty()) {
        params.concatenate_text = (concat_str == "true");
    }
    
    return params;
}

Db9Response RopeChunkVerb::execute(const lab::Text::Sexpr& sexpr) {
    try {
        auto params = extractParameters(sexpr);
        return performChunk(params);
    } catch (const std::exception& e) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "rope-chunk execution failed: " + std::string(e.what());
        return response;
    }
}

Db9Response RopeChunkVerb::performChunk(const ChunkParameters& params) {
    // Validate rope name
    if (!RopeUtils::isValidRopeName(params.rope_name)) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "Invalid rope name: " + params.rope_name;
        return response;
    }
    
    // Determine center position
    size_t center_pos = params.center_position;
    if (!params.center_entity.empty()) {
        center_pos = findCenterPosition(params.rope_name, params.center_entity, params.dbid);
        if (center_pos == SIZE_MAX) {
            Db9Response response;
            response.status = Db9Response::Error;
            response.error_message = "Entity not found in rope: " + params.center_entity;
            return response;
        }
    }
    
    // Get actual rope length from database metadata  
    size_t total_entities = 0;
    auto& manager = DatabaseManager::instance();
    auto store = manager.getDatabase(params.dbid);
    if (store) {
        // CRITICAL FIX: Use same metadata key format as RopeAppendVerb
        std::string metadata_key = "rope:" + params.rope_name + ":metadata";
        auto size_results = store->query(metadata_key, "size", "*");
        if (!size_results.empty()) {
            try {
                total_entities = static_cast<size_t>(std::stoi(size_results[0].object));
            } catch (const std::exception&) {
                total_entities = 0;
            }
        }
    }
    
    // Calculate chunk boundaries
    auto bounds = RopeUtils::calculateChunkBounds(center_pos, params.fragment_count, total_entities);
    
    // Get chunk entities
    auto entities = getChunkEntities(params, bounds);
    
    // Fetch text content if requested
    if (params.include_text) {
        fetchEntityContent(entities, params.dbid);
    }
    
    // Generate response
    auto response_json = formatChunkResponse(params, entities, bounds);
    
    Db9Response response;
    response.status = Db9Response::Success;
    response.result = response_json;
    return response;
}

std::string RopeChunkVerb::resolveEntityId(const std::string& entity_id, const std::string& dbid) {
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

size_t RopeChunkVerb::findCenterPosition(const std::string& rope_name, 
                                       const std::string& entity_id,
                                       const std::string& dbid) {
    auto& manager = DatabaseManager::instance();
    auto store = manager.getDatabase(dbid);
    if (!store) {
        std::cout << "  🚨 DEBUG: Database not found for dbid: " << dbid << std::endl;
        return SIZE_MAX; // Database not found
    }
    
    // CRITICAL: Resolve entity_id to actual EID before searching rope
    std::string resolved_eid = resolveEntityId(entity_id, dbid);
    std::cout << "  🔍 DEBUG: Entity resolution: " << entity_id << " -> " << resolved_eid << std::endl;
    
    if (resolved_eid.empty()) {
        std::cout << "  🚨 DEBUG: Entity resolution failed for: " << entity_id << std::endl;
        return SIZE_MAX; // Entity resolution failed
    }
    
    // Get rope length first to limit our search  
    // CRITICAL FIX: Use same metadata key format as RopeAppendVerb
    std::string metadata_key = "rope:" + rope_name + ":metadata";
    auto size_results = store->query(metadata_key, "size", "*");
    size_t rope_length = 0;
    if (!size_results.empty()) {
        try {
            rope_length = static_cast<size_t>(std::stoi(size_results[0].object));
        } catch (const std::exception&) {
            std::cout << "  🚨 DEBUG: Failed to parse rope length" << std::endl;
            return SIZE_MAX;
        }
    }
    std::cout << "  📏 DEBUG: Rope length: " << rope_length << std::endl;
    
    // Scan through rope positions to find the resolved EID (not the original entity_id!)
    for (size_t pos = 0; pos < rope_length; ++pos) {
        std::string sequence_id = RopeUtils::generateSequenceId(pos);
        std::string rope_key = RopeUtils::generateRopeKey(rope_name, sequence_id);
        
        // Check if this rope position contains our resolved EID
        auto results = store->query(rope_key, "contains", resolved_eid);
        if (!results.empty()) {
            std::cout << "  ✅ DEBUG: Found " << resolved_eid << " at position: " << pos << std::endl;
            return pos; // Found the entity at this position
        } else {
            // Debug: What's actually at this position?
            auto debug_results = store->query(rope_key, "contains", "*");
            if (!debug_results.empty()) {
                std::cout << "  🔍 DEBUG: Position " << pos << " contains: " << debug_results[0].object << std::endl;
            }
        }
    }
    
    std::cout << "  🚨 DEBUG: Entity " << resolved_eid << " not found in rope after scanning " << rope_length << " positions" << std::endl;
    return SIZE_MAX; // Entity not found in rope
}

std::vector<RopeChunkVerb::ChunkEntity> RopeChunkVerb::getChunkEntities(
    const ChunkParameters& params,
    const RopeUtils::ChunkBounds& bounds) {
    
    std::vector<ChunkEntity> entities;
    
    auto& manager = DatabaseManager::instance();
    auto store = manager.getDatabase(params.dbid);
    if (!store) {
        return entities; // Empty result if database not found
    }
    
    // Retrieve actual entities from rope positions
    for (size_t pos = bounds.start_position; pos <= bounds.end_position; ++pos) {
        std::string sequence_id = RopeUtils::generateSequenceId(pos);
        std::string rope_key = RopeUtils::generateRopeKey(params.rope_name, sequence_id);
        
        // Look up actual entity_id from rope_key: rope_key -> contains -> entity_id
        auto results = store->query(rope_key, "contains", "*");
        
        if (!results.empty()) {
            ChunkEntity entity;
            entity.position = pos;
            entity.entity_id = results[0].object; // Real entity_id from database
            entities.push_back(entity);
        }
        // If no entity found at this position, we skip it (sparse rope handling)
    }
    
    return entities;
}

void RopeChunkVerb::fetchEntityContent(std::vector<ChunkEntity>& entities, 
                                     const std::string& dbid) {
    auto& manager = DatabaseManager::instance();
    auto store = manager.getDatabase(dbid);
    if (!store) {
        return; // Can't fetch content without database
    }
    
    for (auto& entity : entities) {
        // Fetch entity content: entity_id -> text-content -> actual_text
        auto content_results = store->query(entity.entity_id, "text-content", "*");
        if (!content_results.empty()) {
            entity.text_content = content_results[0].object;
        } else {
            entity.text_content = "[content not found]";
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

std::string RopeChunkVerb::formatChunkResponse(const ChunkParameters& params,
                                             const std::vector<ChunkEntity>& entities,
                                             const RopeUtils::ChunkBounds& bounds) {
    std::ostringstream json;
    
    json << "{"
         << "\"status\": \"chunk_retrieved\""
         << ", \"rope_name\": \"" << params.rope_name << "\""
         << ", \"center_entity\": \"" << params.center_entity << "\""
         << ", \"fragment_count\": " << entities.size()
         << ", \"global_position\": " << bounds.center_position
         << ", \"entities\": [";
    
    for (size_t i = 0; i < entities.size(); ++i) {
        if (i > 0) json << ", ";
        json << "\"" << entities[i].entity_id << "\"";
    }
    
    json << "]";
    
    if (params.include_text && !entities.empty()) {
        json << ", \"text_fragments\": [";
        for (size_t i = 0; i < entities.size(); ++i) {
            if (i > 0) json << ", ";
            json << "\"" << entities[i].text_content << "\"";  // TODO: Proper JSON escaping
        }
        json << "]";
    }
    
    if (params.concatenate_text) {
        json << ", \"concatenated_text\": \"";
        for (const auto& entity : entities) {
            json << entity.text_content << " ";
        }
        json << "\"";
    }
    
    json << ", \"chunk_bounds\": {"
         << "\"start_position\": " << bounds.start_position
         << ", \"end_position\": " << bounds.end_position
         << ", \"can_extend_backward\": " << (bounds.can_extend_backward ? "true" : "false")
         << ", \"can_extend_forward\": " << (bounds.can_extend_forward ? "true" : "false")
         << "}";
    
    json << ", \"navigation\": {"
         << "\"preceding_chunk_available\": " << (bounds.can_extend_backward ? "true" : "false")
         << ", \"succeeding_chunk_available\": " << (bounds.can_extend_forward ? "true" : "false")
         << "}";
    
    json << "}";
    
    return json.str();
}

} // namespace LabDb