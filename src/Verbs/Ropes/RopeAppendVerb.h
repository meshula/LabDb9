#pragma once

#include "RopeUtils.h"
#include "LabDb/Verbs.h"
#include "LabDb/Db9Response.h"
#include "LabDb/LabText.hpp"
#include <string>

namespace LabDb {

//-----------------------------------------------------------------------------
// RopeAppendVerb - Add Entities to Rope Sequences
//-----------------------------------------------------------------------------

/// Append entity to rope sequence
class RopeAppendVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "rope-append"; }
    std::string getDescription() const override;
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;

private:
    struct AppendParameters {
        std::string rope_name;
        std::string entity_id;
        std::string dbid;
        size_t position = 0;            // 0 = append at end
        bool validate_entity = true;    // Check if entity exists
    };

    AppendParameters extractParameters(const lab::Text::Sexpr& sexpr);
    Db9Response performAppend(const AppendParameters& params);
    
    /// Get current rope length
    size_t getRopeLength(const std::string& rope_name, const std::string& dbid);
    
    /// Resolve entity ID/value to actual EID (handles both EIDs and entity values)
    std::string resolveEntityId(const std::string& entity_id, const std::string& dbid);
    
    /// Validate entity exists in database
    bool entityExists(const std::string& entity_id, const std::string& dbid);
    
    /// Add entity to rope at specified position
    bool addEntityToRope(const AppendParameters& params, size_t actual_position);
    
    /// Update rope metadata (entity count)
    bool updateRopeMetadata(const std::string& rope_name, 
                          const std::string& dbid, 
                          size_t new_length);
};

} // namespace LabDb