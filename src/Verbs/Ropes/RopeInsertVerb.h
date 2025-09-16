#pragma once

#include "RopeUtils.h"
#include "LabDb/Verbs.h"
#include "LabDb/Db9Response.h"
#include "LabDb/LabText.hpp"
#include <string>

namespace LabDb {

//-----------------------------------------------------------------------------
// RopeInsertVerb - Insert Entity at Specific Position
//-----------------------------------------------------------------------------

/// Insert entity at specific position in rope
class RopeInsertVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "rope-insert"; }
    std::string getDescription() const override;
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;

private:
    struct InsertParameters {
        std::string rope_name;
        std::string entity_id;
        std::string dbid;
        size_t position;           // Required position to insert at
        bool shift_existing = true; // Whether to shift existing entities
    };

    InsertParameters extractParameters(const lab::Text::Sexpr& sexpr);
    Db9Response performInsert(const InsertParameters& params);
};

} // namespace LabDb