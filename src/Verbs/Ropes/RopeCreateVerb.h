#pragma once

#include "RopeUtils.h"
#include "LabDb/Verbs.h"
#include "LabDb/Db9Response.h"
#include "LabDb/LabText.hpp"
#include <string>

namespace LabDb {

//-----------------------------------------------------------------------------
// RopeCreateVerb - Create New Rope Sequences
//-----------------------------------------------------------------------------

/// Create a new rope for ordered entity sequences
class RopeCreateVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "rope-create"; }
    std::string getDescription() const override;
    
    /// ENHANCED: Contextual help for user guidance
    std::string getContextualHelp() const;
    
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;

private:
    struct CreateParameters {
        std::string rope_name;
        std::string description;
        std::string dbid;
        bool overwrite = false;
    };

    CreateParameters extractParameters(const lab::Text::Sexpr& sexpr);
    Db9Response performCreate(const CreateParameters& params);
    
    /// Check if rope already exists
    bool ropeExists(const std::string& rope_name, const std::string& dbid);
    
    /// Create rope metadata entry
    bool createRopeMetadata(const CreateParameters& params);
};

} // namespace LabDb