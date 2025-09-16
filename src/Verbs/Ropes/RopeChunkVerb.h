#pragma once

#include "RopeUtils.h"
#include "LabDb/Verbs.h"
#include "LabDb/Db9Response.h"
#include "LabDb/LabText.hpp"
#include <string>
#include <vector>

namespace LabDb {

//-----------------------------------------------------------------------------
// RopeChunkVerb - Core Memex Bounded Navigation
//-----------------------------------------------------------------------------

/// Retrieve bounded text chunks for contemplative reading
class RopeChunkVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "rope-chunk"; }
    std::string getDescription() const override;
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;

private:
    struct ChunkParameters {
        std::string rope_name;
        std::string dbid;
        size_t fragment_count = 24;        // Default Memex chunk size
        std::string center_entity;         // Center around this entity
        size_t center_position = 0;        // Alternative: center by position
        bool include_text = true;          // Fetch entity text content
        bool include_metadata = true;      // Include entity metadata
        bool concatenate_text = false;     // Provide ready-to-read text
    };

    struct ChunkEntity {
        std::string entity_id;
        size_t position;
        std::string text_content;
        std::string metadata;
    };

    ChunkParameters extractParameters(const lab::Text::Sexpr& sexpr);
    Db9Response performChunk(const ChunkParameters& params);
    
    /// Resolve entity ID/value to actual EID (handles both EIDs and entity values)
    std::string resolveEntityId(const std::string& entity_id, const std::string& dbid);
    
    /// Find center position from entity_id
    size_t findCenterPosition(const std::string& rope_name, 
                            const std::string& entity_id,
                            const std::string& dbid);
    
    /// Get entity sequence for chunk bounds
    std::vector<ChunkEntity> getChunkEntities(const ChunkParameters& params,
                                            const RopeUtils::ChunkBounds& bounds);
    
    /// Fetch text content for entities
    void fetchEntityContent(std::vector<ChunkEntity>& entities, 
                          const std::string& dbid);
    
    /// Generate chunk response with navigation metadata
    std::string formatChunkResponse(const ChunkParameters& params,
                                  const std::vector<ChunkEntity>& entities,
                                  const RopeUtils::ChunkBounds& bounds);
};

} // namespace LabDb