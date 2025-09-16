#pragma once

#include "RopeUtils.h"
#include "LabDb/Verbs.h"
#include "LabDb/Db9Response.h"
#include "LabDb/LabText.hpp"
#include <string>
#include <vector>

namespace LabDb {

//-----------------------------------------------------------------------------
// RopeListVerb - List Available Ropes
//-----------------------------------------------------------------------------

/// List all available ropes in database
class RopeListVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "rope-list"; }
    std::string getDescription() const override;
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;

private:
    struct ListParameters {
        std::string dbid;
        std::string pattern = "*";     // Filter pattern
        bool include_stats = true;     // Include entity counts
    };

    struct RopeInfo {
        std::string name;
        std::string description;
        size_t entity_count;
        std::string created_at;
    };

    ListParameters extractParameters(const lab::Text::Sexpr& sexpr);
    Db9Response performList(const ListParameters& params);
    
    /// Get all rope metadata from database
    std::vector<RopeInfo> getRopeList(const ListParameters& params);
    
    /// Check if rope name matches pattern
    bool matchesPattern(const std::string& name, const std::string& pattern);
};

} // namespace LabDb