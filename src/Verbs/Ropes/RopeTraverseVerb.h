#pragma once

#include "RopeUtils.h"
#include "LabDb/Verbs.h"
#include "LabDb/Db9Response.h"
#include "LabDb/LabText.hpp"
#include <string>
#include <vector>

namespace LabDb {

//-----------------------------------------------------------------------------
// RopeTraverseVerb - Sequential Navigation Through Ropes
//-----------------------------------------------------------------------------

/// Traverse rope and retrieve entity sequences
class RopeTraverseVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "rope-traverse"; }
    std::string getDescription() const override;
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;

private:
    struct TraverseParameters {
        std::string rope_name;
        std::string dbid;
        size_t start_position = 0;      // 0-based start position
        size_t count = 10;              // Number of entities to retrieve
        bool include_content = false;   // Whether to fetch entity content
        bool include_metadata = false;  // Whether to include entity metadata
    };

    struct EntityInfo {
        std::string entity_id;
        size_t position;
        std::string content;
        std::string metadata;
    };

    TraverseParameters extractParameters(const lab::Text::Sexpr& sexpr);
    Db9Response performTraverse(const TraverseParameters& params);
    
    /// Get entity sequence from rope
    std::vector<EntityInfo> getEntitySequence(const TraverseParameters& params);
    
    /// Fetch content for entities if requested
    void fetchEntityContent(std::vector<EntityInfo>& entities, const std::string& dbid);
    
    /// Format traverse response
    std::string formatTraverseResponse(const TraverseParameters& params,
                                     const std::vector<EntityInfo>& entities);
};

} // namespace LabDb