#pragma once

#include "MemexUtils.h"
#include "LabDb/Verbs.h"
#include "LabDb/Db9Response.h"
#include "LabDb/LabText.hpp"
#include <string>
#include <vector>

namespace LabDb {

//-----------------------------------------------------------------------------
// MemexChunkPrecedingVerb - Navigate to Previous Chunk
//-----------------------------------------------------------------------------

/// Navigate to the chunk immediately preceding the current chunk
/// Core component of Bush's associative navigation system
class MemexChunkPrecedingVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "memex-chunk-preceding"; }
    std::string getDescription() const override;
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;

private:
    struct NavigationParameters {
        std::string chunk_id;              // "chunk_002847_24frags"
        std::string dbid;                  // Database identifier
        bool include_text = true;          // Fetch entity text content
        bool include_metadata = true;      // Include navigation metadata
        bool concatenate_text = false;     // Provide ready-to-read text
    };

    NavigationParameters extractParameters(const lab::Text::Sexpr& sexpr);
    Db9Response performNavigation(const NavigationParameters& params);
    
    /// Parse chunk ID to extract rope info and position
    MemexUtils::ChunkInfo parseChunkId(const std::string& chunk_id);
    
    /// Calculate preceding chunk position and bounds
    MemexUtils::ChunkBounds calculatePrecedingBounds(const MemexUtils::ChunkInfo& current_chunk);
    
    /// Generate new chunk with navigation metadata
    std::string formatNavigationResponse(const NavigationParameters& params,
                                       const MemexUtils::ChunkInfo& new_chunk,
                                       const std::vector<MemexUtils::ChunkEntity>& entities);
};

} // namespace LabDb
