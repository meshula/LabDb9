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
// Rope Utility Functions and Structures
//-----------------------------------------------------------------------------

namespace RopeUtils {

/// Generate sequence ID for rope position ordering
std::string generateSequenceId(size_t position);

/// Parse sequence ID back to position
size_t parseSequenceId(const std::string& sequence_id);

/// Validate rope name for safety and consistency
bool isValidRopeName(const std::string& name);

/// Generate rope storage key
std::string generateRopeKey(const std::string& rope_name, const std::string& sequence_id);

/// Parse rope key components
struct RopeKeyComponents {
    std::string rope_name;
    std::string sequence_id;
    bool valid;
};

RopeKeyComponents parseRopeKey(const std::string& key);

/// Format rope metadata for JSON response
std::string formatRopeMetadata(const std::string& rope_name, 
                              size_t entity_count, 
                              const std::string& description);

/// Calculate chunk boundaries for bounded navigation
struct ChunkBounds {
    size_t start_position;
    size_t end_position;
    size_t center_position;
    bool can_extend_backward;
    bool can_extend_forward;
};

ChunkBounds calculateChunkBounds(size_t center_position, 
                               size_t chunk_size, 
                               size_t total_entities);

/// Find entity position in rope by entity_id
struct EntityPosition {
    bool found;
    size_t position;
    std::string sequence_id;
};

EntityPosition findEntityPosition(const std::string& rope_name,
                                const std::string& entity_id,
                                const std::string& dbid);

/// Format chunk response for Memex navigation
std::string formatChunkResponse(const std::string& rope_name,
                              const std::string& center_entity,
                              const std::vector<std::string>& entity_ids,
                              const std::vector<std::string>& text_fragments,
                              size_t global_position);

//-------------------------------------------------------------------------
// Triadic Consciousness Rope Context
//-------------------------------------------------------------------------

/// Triadic rope context for conscious text navigation
struct RopeContext {
    std::string rope_name;          // Motion: Navigation intention
    size_t current_position;        // Memory: Current location state  
    size_t total_entities;          // Field: Complete rope extent
    std::string navigation_type;    // "sequential", "entity-centered", "random-access"
    bool boundary_aware;            // Whether at rope boundaries
    std::string traversal_context;  // Context about current traversal
    
    std::string toJsonString() const;
};

/// Create triadic rope context for navigation operations
RopeContext createRopeContext(const std::string& rope_name,
                             size_t position,
                             const std::string& dbid);

/// Generate context-aware guidance for rope navigation
std::string createNavigationGuidance(const RopeContext& context,
                                    const std::string& operation);

} // namespace RopeUtils

} // namespace LabDb