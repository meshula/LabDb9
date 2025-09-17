#include "MemexUtils.h"
#include "LabDb/DatabaseManager.h"
#include "../Ropes/RopeUtils.h"
#include <regex>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace LabDb {
namespace MemexUtils {

//-----------------------------------------------------------------------------
// Chunk ID Management
//-----------------------------------------------------------------------------

std::string generateChunkId(const std::string& rope_name, 
                          size_t center_position, 
                          size_t fragment_count) {
    std::ostringstream id;
    id << "chunk_" << std::setfill('0') << std::setw(6) << center_position 
       << "_" << fragment_count << "frags";
    return id.str();
}

ChunkInfo parseChunkId(const std::string& chunk_id) {
    ChunkInfo info;
    info.is_valid = false;
    
    // Pattern: chunk_NNNNNN_NNfrags
    std::regex chunk_regex(R"(chunk_(\d+)_(\d+)frags)");
    std::smatch matches;
    
    if (std::regex_match(chunk_id, matches, chunk_regex)) {
        info.center_position = std::stoull(matches[1].str());
        info.fragment_count = std::stoull(matches[2].str());
        info.chunk_id = chunk_id;
        info.rope_name = ""; // Will be populated by caller
        info.is_valid = true;
    }
    
    return info;
}

bool isValidChunkId(const std::string& chunk_id) {
    auto info = parseChunkId(chunk_id);
    return info.is_valid;
}

//-----------------------------------------------------------------------------
// Position and Boundary Calculations
//-----------------------------------------------------------------------------

ChunkBounds calculateChunkBounds(const std::string& rope_name,
                               size_t center_position,
                               size_t fragment_count,
                               const std::string& dbid) {
    ChunkBounds bounds;
    bounds.rope_name = rope_name;
    bounds.center_position = center_position;
    bounds.fragment_count = fragment_count;
    
    // Calculate start/end positions around center
    size_t half_count = fragment_count / 2;
    
    // Handle case where center is near beginning
    if (center_position < half_count) {
        bounds.start_position = 0;
        bounds.end_position = fragment_count - 1;
    } else {
        bounds.start_position = center_position - half_count;
        bounds.end_position = center_position + half_count;
    }
    
    // Determine extension capabilities
    bounds.can_extend_backward = (bounds.start_position > 0);
    
    // Check if we can extend forward by examining rope
    // TODO: Get actual rope length from database
    // For now, assume we can extend unless at position 0
    bounds.can_extend_forward = true;
    
    return bounds;
}

ChunkBounds calculatePrecedingBounds(const ChunkBounds& current_bounds) {
    ChunkBounds preceding;
    preceding.rope_name = current_bounds.rope_name;
    preceding.fragment_count = current_bounds.fragment_count;
    
    // Move center backward by fragment_count to minimize overlap
    if (current_bounds.center_position >= current_bounds.fragment_count) {
        preceding.center_position = current_bounds.center_position - current_bounds.fragment_count;
    } else {
        // Near beginning, just move to position 0
        preceding.center_position = 0;
    }
    
    // Calculate bounds around new center
    size_t half_count = preceding.fragment_count / 2;
    if (preceding.center_position < half_count) {
        preceding.start_position = 0;
        preceding.end_position = preceding.fragment_count - 1;
    } else {
        preceding.start_position = preceding.center_position - half_count;
        preceding.end_position = preceding.center_position + half_count;
    }
    
    preceding.can_extend_backward = (preceding.start_position > 0);
    preceding.can_extend_forward = true;
    
    return preceding;
}

ChunkBounds calculateSucceedingBounds(const ChunkBounds& current_bounds) {
    ChunkBounds succeeding;
    succeeding.rope_name = current_bounds.rope_name;
    succeeding.fragment_count = current_bounds.fragment_count;
    
    // Move center forward by fragment_count to minimize overlap
    succeeding.center_position = current_bounds.center_position + current_bounds.fragment_count;
    
    // Calculate bounds around new center
    size_t half_count = succeeding.fragment_count / 2;
    succeeding.start_position = succeeding.center_position - half_count;
    succeeding.end_position = succeeding.center_position + half_count;
    
    succeeding.can_extend_backward = true;
    // TODO: Check actual rope length to determine forward extension
    succeeding.can_extend_forward = true;
    
    return succeeding;
}

ChunkBounds calculateExtendedBounds(const ChunkBounds& current_bounds,
                                  const std::string& direction,
                                  size_t additional_fragments) {
    ChunkBounds extended = current_bounds;
    
    if (direction == "backward" || direction == "both") {
        if (extended.start_position >= additional_fragments) {
            extended.start_position -= additional_fragments;
        } else {
            extended.start_position = 0;
        }
        extended.fragment_count += additional_fragments;
    }
    
    if (direction == "forward" || direction == "both") {
        extended.end_position += additional_fragments;
        extended.fragment_count += additional_fragments;
    }
    
    // Recalculate center position
    extended.center_position = (extended.start_position + extended.end_position) / 2;
    
    // Update extension capabilities
    extended.can_extend_backward = (extended.start_position > 0);
    extended.can_extend_forward = true; // TODO: Check actual rope length
    
    return extended;
}

//-----------------------------------------------------------------------------
// Entity and Text Operations
//-----------------------------------------------------------------------------

std::vector<ChunkEntity> fetchChunkEntities(const ChunkBounds& bounds,
                                          const std::string& dbid,
                                          bool include_text,
                                          bool include_metadata) {
    std::vector<ChunkEntity> entities;
    
    try {
        auto& manager = DatabaseManager::instance();
        auto store = manager.getDatabase(dbid);
        
        if (!store) {
            std::cerr << "Failed to get database for: " << dbid << std::endl;
            return entities;
        }
        
        // Fetch entities in position range
        for (size_t pos = bounds.start_position; pos <= bounds.end_position; ++pos) {
            // Generate rope key for this position
            auto sequence_id = RopeUtils::generateSequenceId(pos);
            auto rope_key = RopeUtils::generateRopeKey(bounds.rope_name, sequence_id);
            
            // Get entity ID at this position: rope_key -> contains -> entity_id
            auto results = store->query(rope_key, "contains", "*");
            
            if (!results.empty()) {
                ChunkEntity entity;
                entity.entity_id = results[0].object;
                entity.position = pos;
                
                // Fetch text content if requested
                if (include_text) {
                    auto content_results = store->query(entity.entity_id, "text-content", "*");
                    if (!content_results.empty()) {
                        entity.text_content = content_results[0].object;
                    } else {
                        entity.text_content = "[content not found]";
                    }
                }
                
                // Fetch metadata if requested
                if (include_metadata) {
                    auto metadata_results = store->query(entity.entity_id, "*", "*");
                    std::ostringstream metadata_json;
                    metadata_json << "{";
                    bool first = true;
                    for (const auto& result : metadata_results) {
                        if (!first) metadata_json << ", ";
                        metadata_json << "\"" << result.predicate << "\": \"" << result.object << "\"";
                        first = false;
                    }
                    metadata_json << "}";
                    entity.metadata = metadata_json.str();
                }
                
                entities.push_back(entity);
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error fetching chunk entities: " << e.what() << std::endl;
    }
    
    return entities;
}

std::string concatenateChunkText(const std::vector<ChunkEntity>& entities) {
    std::ostringstream text;
    
    for (size_t i = 0; i < entities.size(); ++i) {
        if (i > 0) {
            text << " "; // Add space between fragments
        }
        text << entities[i].text_content;
    }
    
    return text.str();
}

std::string formatNavigationMetadata(const ChunkBounds& bounds,
                                   const std::vector<ChunkEntity>& entities) {
    std::ostringstream json;
    
    json << "{"
         << "\"bounds\": {"
         << "\"start_position\": " << bounds.start_position
         << ", \"end_position\": " << bounds.end_position
         << ", \"center_position\": " << bounds.center_position
         << ", \"fragment_count\": " << bounds.fragment_count
         << ", \"can_extend_backward\": " << (bounds.can_extend_backward ? "true" : "false")
         << ", \"can_extend_forward\": " << (bounds.can_extend_forward ? "true" : "false")
         << "}"
         << ", \"entity_count\": " << entities.size()
         << ", \"rope_name\": \"" << bounds.rope_name << "\""
         << "}";
    
    return json.str();
}

//-----------------------------------------------------------------------------
// Semantic Boundary Detection
//-----------------------------------------------------------------------------

std::vector<SemanticBoundary> detectSemanticBoundaries(const std::string& rope_name,
                                                      size_t around_position,
                                                      size_t search_radius,
                                                      const std::string& dbid) {
    std::vector<SemanticBoundary> boundaries;
    
    // TODO: Implement semantic boundary detection
    // This would analyze entity metadata to find:
    // - Definition boundaries (field-type = "definition")
    // - Proposition boundaries (field-type = "proposition") 
    // - Book boundaries (part-of = "book-N")
    // - Proof boundaries (logical-role = "proof")
    
    // Placeholder implementation
    SemanticBoundary boundary;
    boundary.position = around_position;
    boundary.boundary_type = "definition";
    boundary.description = "Definition boundary detected";
    boundaries.push_back(boundary);
    
    return boundaries;
}

ChunkBounds extendToSemanticBoundary(const ChunkBounds& current_bounds,
                                   const std::string& boundary_type,
                                   const std::string& dbid) {
    // TODO: Implement smart boundary extension
    // For now, just return current bounds
    return current_bounds;
}

} // namespace MemexUtils
} // namespace LabDb
