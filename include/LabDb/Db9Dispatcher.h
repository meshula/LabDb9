#ifndef DB9_DISPATCHER_H
#define DB9_DISPATCHER_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <chrono>

// Forward declarations
namespace lab { namespace Text { struct Sexpr; } }

namespace LabDb {

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Auto-reflexive metrics for operation monitoring
//-----------------------------------------------------------------------------
struct AutoReflexiveMetrics {
    std::chrono::milliseconds operation_time_ms{0};
    uint64_t items_processed{0};
    double cache_hit_ratio{0.0};
    uint32_t tid_allocations{0};
    uint64_t memory_usage_kb{0};
};

//-----------------------------------------------------------------------------
// Standardized response format for all db9 operations
//-----------------------------------------------------------------------------
struct Db9Response {
    enum Status { Success, Error, Warning } status{Success};
    std::string result;                    // Primary return data
    std::string error_code;               // Optional error identifier
    std::string error_message;            // Human-readable error description
    AutoReflexiveMetrics auto_reflexive;  // Performance metrics
    
    // Convert to JSON for MCP return
    std::string toJson() const;
};

//-----------------------------------------------------------------------------
// Base interface for all db9 verb implementations
//-----------------------------------------------------------------------------
class IDb9Verb {
public:
    virtual ~IDb9Verb() = default;
    
    // Execute the verb with parsed S-expression parameters
    // sexpr contains the full parsed command: (verb :param1 value1 :param2 value2)
    virtual Db9Response execute(const ::lab::Text::Sexpr& sexpr) = 0;
    
    // Get verb name for registration
    virtual std::string getVerbName() const = 0;
    
    // Get help/description for documentation
    virtual std::string getDescription() const = 0;
};

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
