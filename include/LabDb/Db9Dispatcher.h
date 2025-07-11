#ifndef DB9_DISPATCHER_H
#define DB9_DISPATCHER_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <chrono>

#include "LabDb/AutoReflexiveMetrics.h"
#include "LabDb/Verbs.h"

// Forward declarations
namespace lab { namespace Text { struct Sexpr; } }

namespace LabDb {

//-----------------------------------------------------------------------------
// Verb registry and dispatcher
//-----------------------------------------------------------------------------
class Db9Dispatcher {
public:
    Db9Dispatcher();
    ~Db9Dispatcher();
    
    // Main entry point - execute S-expression command and return JSON response
    Db9Response executeCommand(const std::string& sexprCommand);
    
    // Execute multiple commands in sequence
    Db9Response executeCommands(const std::vector<std::string>& sexprCommands);
    
    // Register a verb implementation
    void registerVerb(std::unique_ptr<IDb9Verb> verb);
    
    // Get available verbs (for db9-readme)
    std::vector<std::string> getAvailableVerbs() const;
    
    // Get full specification as markdown (for db9-readme implementation)
    std::string getSpecification() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};


//-----------------------------------------------------------------------------
// Global dispatcher access
//-----------------------------------------------------------------------------
Db9Dispatcher& getGlobalDb9Dispatcher();

} // namespace LabDb

#endif // DB9_DISPATCHER_H
