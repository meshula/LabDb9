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

    // Execute commands from a file with dbid substitution
    Db9Response executeCommands(const std::string& filePath, const std::string& dbid);
    
    // Register a verb implementation
    void registerVerb(std::unique_ptr<IDb9Verb> verb);

    // Register verb aliases
    void registerVerbAlias(const std::string& verb, const std::vector<std::string>& aliases);
    
    // Get available verbs (for db9-readme)
    
    // Get specific verb by name for introspection
    IDb9Verb* getVerbByName(const std::string& verbName) const;
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