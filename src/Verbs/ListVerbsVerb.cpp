#include "ListVerbsVerb.h"
#include "LabDb/Db9Dispatcher.h"
#include "LabDb/LabText.hpp"
#include <algorithm>
#include <sstream>
#include <mutex>

namespace LabDb {

bool ListVerbsVerb::registered = false;
std::once_flag ListVerbsVerb::register_flag;

ListVerbsVerb::ListVerbsVerb() {
    // Constructor - verb setup if needed
}

std::string ListVerbsVerb::getVerbName() const {
    return "list-verbs";
}

std::string ListVerbsVerb::getDescription() const {
    return R"(
List all available db9 S-expression verbs for discovery and validation.
Usage:
```lisp
(list-verbs)
;; Returns: {
;;   "status": "success", 
;;   "verbs": ["add-entity", "add-triple", "find-triple", "get-entity", ...],
;;   "count": 42,
;;   "categories": {
;;     "database": ["create-database", "open-database", "close-database"],
;;     "entities": ["add-entity", "get-entity", "find-entity"],
;;     "triples": ["add-triple", "find-triple", "get-triple"],
;;     "enhanced": ["find-triple-enhanced", "get-entity-enhanced"],
;;     "rope": ["rope-create", "rope-append", "rope-chunk"],
;;     "memex": ["memex-chunk-preceding", "memex-chunk-succeeding"],
;;     "system": ["list-verbs", "get-verb-description"]
;;   }
;; }
```

Perfect for:
- Discovering new functionality after updates
- Validating verb deployment status  
- Building auto-completion and help systems
- Confirming Memex and Rope verb availability
    )";
}

Db9Response ListVerbsVerb::execute(const ::lab::Text::Sexpr& sexpr) {
    try {
        // Get the global dispatcher to access available verbs
        extern Db9Dispatcher& getGlobalDb9Dispatcher();
        auto& dispatcher = getGlobalDb9Dispatcher();
        
        // Get all available verbs
        auto verbs = dispatcher.getAvailableVerbs();
        
        // Sort verbs alphabetically for better UX
        std::sort(verbs.begin(), verbs.end());
        
        // Format the response
        std::string response = formatVerbList(verbs);
        
        Db9Response result;
        result.status = Db9Response::Success;
        result.result = response;
        return result;
        
    } catch (const std::exception& e) {
        Db9Response error_result;
        error_result.status = Db9Response::Error;
        error_result.error_message = "list-verbs execution failed: " + std::string(e.what());
        return error_result;
    }
}

std::string ListVerbsVerb::formatVerbList(const std::vector<std::string>& verbs) {
    std::ostringstream json;
    
    json << "{"
         << "\"status\": \"success\""
         << ", \"count\": " << verbs.size()
         << ", \"verbs\": [";
    
    // Add all verbs as JSON array
    for (size_t i = 0; i < verbs.size(); ++i) {
        if (i > 0) json << ", ";
        json << "\"" << verbs[i] << "\"";
    }
    
    json << "]";
    
    // Categorize verbs for enhanced discovery
    json << ", \"categories\": {";
    
    // Database lifecycle verbs
    json << "\"database\": [";
    bool first_db = true;
    for (const auto& verb : verbs) {
        if (verb.find("database") != std::string::npos || 
            verb == "create-database" || verb == "open-database" || verb == "close-database" ||
            verb == "list-open-databases" || verb == "database-health-check") {
            if (!first_db) json << ", ";
            json << "\"" << verb << "\"";
            first_db = false;
        }
    }
    json << "]";
    
    // Entity operations
    json << ", \"entities\": [";
    bool first_entity = true;
    for (const auto& verb : verbs) {
        if (verb.find("entity") != std::string::npos && verb.find("enhanced") == std::string::npos) {
            if (!first_entity) json << ", ";
            json << "\"" << verb << "\"";
            first_entity = false;
        }
    }
    json << "]";
    
    // Triple operations
    json << ", \"triples\": [";
    bool first_triple = true;
    for (const auto& verb : verbs) {
        if ((verb.find("triple") != std::string::npos && verb.find("enhanced") == std::string::npos) ||
            verb.find("tid") != std::string::npos) {
            if (!first_triple) json << ", ";
            json << "\"" << verb << "\"";
            first_triple = false;
        }
    }
    json << "]";
    
    // Enhanced API verbs
    json << ", \"enhanced\": [";
    bool first_enhanced = true;
    for (const auto& verb : verbs) {
        if (verb.find("enhanced") != std::string::npos) {
            if (!first_enhanced) json << ", ";
            json << "\"" << verb << "\"";
            first_enhanced = false;
        }
    }
    json << "]";
    
    // Rope system verbs
    json << ", \"rope\": [";
    bool first_rope = true;
    for (const auto& verb : verbs) {
        if (verb.find("rope") != std::string::npos) {
            if (!first_rope) json << ", ";
            json << "\"" << verb << "\"";
            first_rope = false;
        }
    }
    json << "]";
    
    // Memex system verbs
    json << ", \"memex\": [";
    bool first_memex = true;
    for (const auto& verb : verbs) {
        if (verb.find("memex") != std::string::npos) {
            if (!first_memex) json << ", ";
            json << "\"" << verb << "\"";
            first_memex = false;
        }
    }
    json << "]";
    
    // System/meta verbs
    json << ", \"system\": [";
    bool first_system = true;
    for (const auto& verb : verbs) {
        if (verb == "list-verbs" || verb == "get-verb-description" || 
            verb == "get-vocabulary-stats" || verb.find("fio-") != std::string::npos) {
            if (!first_system) json << ", ";
            json << "\"" << verb << "\"";
            first_system = false;
        }
    }
    json << "]";
    
    json << "}"; // Close categories
    json << "}"; // Close main object
    
    return json.str();
}

void ListVerbsVerb::registerVerb(Db9Dispatcher& dispatcher) {
    std::call_once(register_flag, [&dispatcher]() {
        if (!registered) {
            dispatcher.registerVerb(std::make_unique<LabDb::ListVerbsVerb>());
            registered = true;
        }
    });
}

} // namespace LabDb
