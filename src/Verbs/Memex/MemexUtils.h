#pragma once

#include "LabDb/Verbs.h"
#include "LabDb/Db9Response.h"
#include "LabDb/LabText.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

namespace LabDb {

//-----------------------------------------------------------------------------
// Memex Utility Functions and Structures
//-----------------------------------------------------------------------------

namespace MemexUtils {

//-----------------------------------------------------------------------------
// Core Data Structures for Memex Operations
//-----------------------------------------------------------------------------

/// Information about a text chunk entity
struct ChunkEntity {
    std::string entity_id;
    size_t position;
    std::string text_content;
    std::string metadata;
};

/// Chunk boundary and navigation information
struct ChunkBounds {
    size_t start_position;
    size_t end_position;
    size_t center_position;
    size_t fragment_count;
    bool can_extend_backward;
    bool can_extend_forward;
    std::string rope_name;
};

/// Parsed chunk information from chunk ID
struct ChunkInfo {
    std::string rope_name;
    size_t center_position;
    size_t fragment_count;
    std::string chunk_id;
    bool is_valid;
};

//-----------------------------------------------------------------------------
// Chunk ID Management
//-----------------------------------------------------------------------------

/// Generate chunk ID from parameters
std::string generateChunkId(const std::string& rope_name, 
                          size_t center_position, 
                          size_t fragment_count);

/// Parse chunk ID back to components
ChunkInfo parseChunkId(const std::string& chunk_id);

/// Validate chunk ID format
bool isValidChunkId(const std::string& chunk_id);

//-----------------------------------------------------------------------------
// Position and Boundary Calculations
//-----------------------------------------------------------------------------

/// Calculate chunk bounds around center position
ChunkBounds calculateChunkBounds(const std::string& rope_name,
                               size_t center_position,
                               size_t fragment_count,
                               const std::string& dbid);

/// Calculate preceding chunk bounds
ChunkBounds calculatePrecedingBounds(const ChunkBounds& current_bounds);

/// Calculate succeeding chunk bounds  
ChunkBounds calculateSucceedingBounds(const ChunkBounds& current_bounds);

/// Calculate extended chunk bounds
ChunkBounds calculateExtendedBounds(const ChunkBounds& current_bounds,
                                  const std::string& direction,
                                  size_t additional_fragments);

//-----------------------------------------------------------------------------
// Entity and Text Operations
//-----------------------------------------------------------------------------

/// Fetch entities for chunk bounds
std::vector<ChunkEntity> fetchChunkEntities(const ChunkBounds& bounds,
                                          const std::string& dbid,
                                          bool include_text = true,
                                          bool include_metadata = true);

/// Concatenate entity text for reading
std::string concatenateChunkText(const std::vector<ChunkEntity>& entities);

/// Generate navigation metadata for chunk
std::string formatNavigationMetadata(const ChunkBounds& bounds,
                                   const std::vector<ChunkEntity>& entities);

//-----------------------------------------------------------------------------
// Semantic Boundary Detection
//-----------------------------------------------------------------------------

/// Detect semantic boundaries in text sequence
struct SemanticBoundary {
    size_t position;
    std::string boundary_type;  // "definition", "proposition", "proof", "book"
    std::string description;
};

/// Find semantic boundaries around position
std::vector<SemanticBoundary> detectSemanticBoundaries(const std::string& rope_name,
                                                      size_t around_position,
                                                      size_t search_radius,
                                                      const std::string& dbid);

/// Extend chunk to semantic boundary
ChunkBounds extendToSemanticBoundary(const ChunkBounds& current_bounds,
                                   const std::string& boundary_type,
                                   const std::string& dbid);

} // namespace MemexUtils

} // namespace LabDb
