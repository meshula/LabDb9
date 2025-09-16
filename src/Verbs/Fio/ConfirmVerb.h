#pragma once

#include "LabDb/Verbs.h"
#include "LabDb/Db9Response.h"
#include "LabDb/LabText.hpp"
#include <string>

namespace LabDb {

//-----------------------------------------------------------------------------
// FioConfirmVerb - Preview Confirmation and Ping-Pong Testing
//-----------------------------------------------------------------------------

/// Confirm previewed file operations and handle ping-pong testing protocol
class FioConfirmVerb : public IDb9Verb {
public:
    std::string getVerbName() const override { return "fio-confirm"; }
    std::string getDescription() const override;
    Db9Response execute(const lab::Text::Sexpr& sexpr) override;

private:
    /// Handle ping-pong testing protocol (writes "pong" to specified file)
    Db9Response handlePingCommand(const std::string& path);
    
    /// Handle preview confirmation protocol (executes previewed operation)
    Db9Response handleTokenConfirmation(const std::string& token);
    
    /// Generate timestamp string for operations
    std::string generateTimestamp();
};

} // namespace LabDb
