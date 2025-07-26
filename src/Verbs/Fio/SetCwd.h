#pragma once

#include "LabDb/Verbs.h"
#include "LabDb/Db9Response.h"
#include "LabDb/LabText.hpp"
#include <string>

namespace LabDb {

class Db9Dispatcher;

/// Set current working directory with safety checks - prevents setting to root (/)
class SetCwdVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "set-cwd"; }
    std::string getDescription() const override;
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;

private:
    struct CwdParameters {
        std::string path;
    };

    CwdParameters extractParameters(const lab::Text::Sexpr& sexpr);
};

}