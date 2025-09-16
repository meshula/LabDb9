#pragma once

#include "RopeUtils.h"
#include "LabDb/Verbs.h"
#include "LabDb/Db9Response.h"
#include "LabDb/LabText.hpp"
#include <string>

namespace LabDb {

//-----------------------------------------------------------------------------
// RopeDeleteVerb - Delete Rope or Remove Entities
//-----------------------------------------------------------------------------

/// Delete rope or remove entities from rope
class RopeDeleteVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "rope-delete"; }
    std::string getDescription() const override;
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;

private:
    struct DeleteParameters {
        std::string rope_name;
        std::string dbid;
        std::string entity_id;     // If specified, remove this entity
        bool delete_entire_rope = false;
        bool confirm = false;      // Safety confirmation
    };

    DeleteParameters extractParameters(const lab::Text::Sexpr& sexpr);
    Db9Response performDelete(const DeleteParameters& params);
};

} // namespace LabDb