#pragma once

#include "RopeUtils.h"
#include "LabDb/Verbs.h"
#include "LabDb/Db9Response.h"
#include "LabDb/LabText.hpp"
#include <string>

namespace LabDb {

//-----------------------------------------------------------------------------
// RopeInfoVerb - Get Rope Information and Statistics
//-----------------------------------------------------------------------------

/// Get rope information and statistics
class RopeInfoVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "rope-info"; }
    std::string getDescription() const override;
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;

private:
    struct InfoParameters {
        std::string rope_name;
        std::string dbid;
        bool include_entities = false;
        size_t max_entities = 10;
    };

    InfoParameters extractParameters(const lab::Text::Sexpr& sexpr);
    Db9Response performInfo(const InfoParameters& params);
};

} // namespace LabDb