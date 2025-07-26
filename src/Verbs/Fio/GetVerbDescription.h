#pragma once

#include "LabDb/Verbs.h"
#include "LabDb/Db9Response.h"
#include "LabDb/LabText.hpp"
#include <string>

namespace LabDb {

class Db9Dispatcher;

/// Get description for any registered verb - self-documenting system
class GetVerbDescriptionVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "get-verb-description"; }
    std::string getDescription() const override;
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;

private:
    struct DescriptionParameters {
        std::string verb_name;
    };

    DescriptionParameters extractParameters(const lab::Text::Sexpr& sexpr);
};

} // namespace LabDb