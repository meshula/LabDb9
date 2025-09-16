#pragma once

#include "RopeUtils.h"
#include "LabDb/Verbs.h"
#include "LabDb/Db9Response.h"
#include "LabDb/LabText.hpp"
#include <string>
#include <vector>

namespace LabDb {

//-----------------------------------------------------------------------------
// RopeFindVerb - Find Entity Position in Rope
//-----------------------------------------------------------------------------

/// Find entity position in rope by entity_id
class RopeFindVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "rope-find"; }
    std::string getDescription() const override;
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;

private:
    struct FindParameters {
        std::string rope_name;
        std::string entity_id;
        std::string dbid;
        bool include_context = false;  // Include surrounding entities
        size_t context_size = 5;       // Context entities before/after
    };

    FindParameters extractParameters(const lab::Text::Sexpr& sexpr);
    Db9Response performFind(const FindParameters& params);
};

} // namespace LabDb