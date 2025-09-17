#include "MemexChunkSucceedingVerb.h"
#include "LabDb/DatabaseManager.h"
#include "LabDb/LabText.hpp"
#include <sstream>
#include <algorithm>
#include <iostream>

namespace LabDb {

std::string MemexChunkSucceedingVerb::getDescription() const {
    return R"Memex({
        "verb": "memex-chunk-succeeding",
        "description": "Navigate to the chunk immediately succeeding the current chunk (Bush's associative navigation)",
        "parameters": {
            "chunk-id": "Required. Current chunk identifier (e.g., 'chunk_002847_24frags')",
            "dbid": "Required. Database identifier",
            "include-text": "Optional. Whether to fetch entity text content (default: true)",
            "include-metadata": "Optional. Whether to include navigation metadata (default: true)", 
            "concatenate-text": "Optional. Provide ready-to-read concatenated text (default: false)"
        },
        "examples": [
            "(memex-chunk-succeeding :chunk-id \"chunk_002847_24frags\" :dbid db1)",
            "(memex-chunk-succeeding :chunk-id \"chunk_002847_24frags\" :include-text true :concatenate-text true :dbid db1)",
            "(memex-chunk-succeeding :chunk-id \"chunk_002847_24frags\" :include-metadata false :dbid db1)"
        ],
        "response": {
            "status": "chunk_navigation_succeeded",
            "operation": "succeeding",
            "original_chunk_id": "string",
            "new_chunk_id": "string",
            "rope_name": "string",
            "center_position": "number",
            "fragment_count": "number",
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
                "has_preceding_chunk": "boolean",
                "has_succeeding_chunk": "boolean",
                "preceding_chunk_id": "string",
                "succeeding_chunk_id": "string"
            }
        }
    })Memex";
}

MemexChunkSucceedingVerb::NavigationParameters MemexChunkSucceedingVerb::extractParameters(const lab::Text::Sexpr& sexpr) {
    NavigationParameters params;
    
    // Extract required parameters
    params.chunk_id = extractStringParam(sexpr, "chunk-id");
    params.dbid = extractStringParam(sexpr, "dbid");
    
    // Extract optional parameters with defaults
    std::string include_text_str = extractStringParam(sexpr, "include-text");
    if (!include_text_str.empty()) {
        params.include_text = (include_text_str == "true");
    }
    
    std::string include_metadata_str = extractStringParam(sexpr, "include-metadata");
    if (!include_metadata_str.empty()) {
        params.include_metadata = (include_metadata_str == "true");
    }
    
    std::string concatenate_text_str = extractStringParam(sexpr, "concatenate-text");
    if (!concatenate_text_str.empty()) {
        params.concatenate_text = (concatenate_text_str == "true");
    }
    
    return params;
}

Db9Response MemexChunkSucceedingVerb::execute(const lab::Text::Sexpr& sexpr) {
    try {
        auto params = extractParameters(sexpr);
        return performNavigation(params);
    } catch (const std::exception& e) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "memex-chunk-succeeding execution failed: " + std::string(e.what());
        return response;
    }
}

Db9Response MemexChunkSucceedingVerb::performNavigation(const NavigationParameters& params) {
    // Parse current chunk ID to get position and metadata
    auto current_chunk = MemexUtils::parseChunkId(params.chunk_id);
    if (!current_chunk.is_valid) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "Invalid chunk ID format: " + params.chunk_id;
        return response;
    }
    
    // Calculate succeeding chunk bounds
    auto current_bounds = MemexUtils::calculateChunkBounds(
        current_chunk.rope_name, 
        current_chunk.center_position,
        current_chunk.fragment_count, 
        params.dbid
    );
    
    auto succeeding_bounds = MemexUtils::calculateSucceedingBounds(current_bounds);
    
    // Check if succeeding chunk is possible
    if (!succeeding_bounds.can_extend_forward) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "No succeeding chunk available - already at end of rope";
        return response;
    }
    
    // Fetch entities for succeeding chunk
    auto entities = MemexUtils::fetchChunkEntities(
        succeeding_bounds, 
        params.dbid,
        params.include_text,
        params.include_metadata
    );
    
    if (entities.empty()) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_message = "No entities found in succeeding chunk range";
        return response;
    }
    
    // Generate new chunk info
    MemexUtils::ChunkInfo new_chunk;
    new_chunk.rope_name = current_chunk.rope_name;
    new_chunk.center_position = succeeding_bounds.center_position;
    new_chunk.fragment_count = succeeding_bounds.fragment_count;
    new_chunk.chunk_id = MemexUtils::generateChunkId(
        new_chunk.rope_name,
        new_chunk.center_position,
        new_chunk.fragment_count
    );
    new_chunk.is_valid = true;
    
    // Format response
    auto response_json = formatNavigationResponse(params, new_chunk, entities);
    
    Db9Response response;
    response.status = Db9Response::Success;
    response.result = response_json;
    return response;
}

std::string MemexChunkSucceedingVerb::formatNavigationResponse(
    const NavigationParameters& params,
    const MemexUtils::ChunkInfo& new_chunk,
    const std::vector<MemexUtils::ChunkEntity>& entities) {
    
    std::ostringstream json;
    
    json << "{"
         << "\"status\": \"chunk_navigation_succeeded\""
         << ", \"operation\": \"succeeding\""
         << ", \"original_chunk_id\": \"" << params.chunk_id << "\""
         << ", \"new_chunk_id\": \"" << new_chunk.chunk_id << "\""
         << ", \"rope_name\": \"" << new_chunk.rope_name << "\""
         << ", \"center_position\": " << new_chunk.center_position
         << ", \"fragment_count\": " << new_chunk.fragment_count;
    
    // Add entity IDs
    json << ", \"entities\": [";
    for (size_t i = 0; i < entities.size(); ++i) {
        if (i > 0) json << ", ";
        json << "\"" << entities[i].entity_id << "\"";
    }
    json << "]";
    
    // Add text fragments if requested
    if (params.include_text) {
        json << ", \"text_fragments\": [";
        for (size_t i = 0; i < entities.size(); ++i) {
            if (i > 0) json << ", ";
            json << "\"" << entities[i].text_content << "\"";
        }
        json << "]";
    }
    
    // Add concatenated text if requested
    if (params.concatenate_text) {
        auto concatenated = MemexUtils::concatenateChunkText(entities);
        json << ", \"concatenated_text\": \"" << concatenated << "\"";
    }
    
    // Add chunk bounds and navigation metadata
    if (params.include_metadata) {
        auto bounds = MemexUtils::calculateChunkBounds(
            new_chunk.rope_name,
            new_chunk.center_position, 
            new_chunk.fragment_count,
            params.dbid
        );
        
        json << ", \"chunk_bounds\": {"
             << "\"start_position\": " << bounds.start_position
             << ", \"end_position\": " << bounds.end_position
             << ", \"can_extend_backward\": " << (bounds.can_extend_backward ? "true" : "false")
             << ", \"can_extend_forward\": " << (bounds.can_extend_forward ? "true" : "false")
             << "}";
             
        // Calculate navigation links
        auto preceding_bounds = MemexUtils::calculatePrecedingBounds(bounds);
        auto succeeding_bounds = MemexUtils::calculateSucceedingBounds(bounds);
        
        bool has_preceding = bounds.can_extend_backward;
        bool has_succeeding = succeeding_bounds.can_extend_forward;
        
        json << ", \"navigation\": {"
             << "\"has_preceding_chunk\": " << (has_preceding ? "true" : "false")
             << ", \"has_succeeding_chunk\": " << (has_succeeding ? "true" : "false");
             
        if (has_preceding) {
            auto preceding_chunk_id = MemexUtils::generateChunkId(
                new_chunk.rope_name,
                preceding_bounds.center_position,
                preceding_bounds.fragment_count
            );
            json << ", \"preceding_chunk_id\": \"" << preceding_chunk_id << "\"";
        }
        
        if (has_succeeding) {
            auto succeeding_chunk_id = MemexUtils::generateChunkId(
                new_chunk.rope_name,
                succeeding_bounds.center_position,
                succeeding_bounds.fragment_count
            );
            json << ", \"succeeding_chunk_id\": \"" << succeeding_chunk_id << "\"";
        }
        
        json << "}";
    }
    
    json << "}";
    
    return json.str();
}

} // namespace LabDb
