#include "GetVerbDescription.h"
#include "LabDb/Db9Dispatcher.h"
#include "LabDb/AutoReflexiveMetrics.h"

#include <chrono>
#include <sstream>
#include <stdexcept>

namespace LabDb {

extern Db9Dispatcher& getGlobalDb9Dispatcher();

std::string GetVerbDescriptionVerb::getDescription() const {
    return "Get detailed description and usage information for any registered verb.\n"
           "Usage:\n"
           "```lisp\n"
           "(get-verb-description :verb section fio-search section)\n"
           ";; Returns: complete description with parameters, examples, and usage patterns\n"
           "\n"
           "(get-verb-description :verb section add-triple-semantic section)\n"
           ";; Returns: documentation for database verbs\n"
           "```\n"
           "\n"
           "**Parameters:**\n"
           "- `:verb` - Name of the verb to get description for (required)\n"
           "\n"
           "**Returns:**\n"
           "- `verb_name` - Name of the verb being described\n"
           "- `description` - Complete description text with usage examples\n"
           "- `available` - Boolean indicating if verb exists in system\n"
           "- `status` - Operation status\n"
           "\n"
           "**Self-Documenting System:**\n"
           "- Any registered verb can provide its own documentation\n"
           "- Perfect for discovering capabilities and usage patterns\n"
           "- Enables dynamic help system and documentation generation\n"
           "- Meta-verb: can describe itself recursively!\n";
}

GetVerbDescriptionVerb::DescriptionParameters GetVerbDescriptionVerb::extractParameters(const lab::Text::Sexpr& sexpr) {
    DescriptionParameters params;
    params.verb_name = extractStringParam(sexpr, "verb");
    return params;
}

Db9Response GetVerbDescriptionVerb::execute(const lab::Text::Sexpr& sexpr) {
    auto start_time = std::chrono::steady_clock::now();
    
    auto params = extractParameters(sexpr);
    
    if (params.verb_name.empty()) {
        AutoReflexiveMetrics metrics;
        return Db9Response{Db9Response::Error, "", "missing_verb", 
                          "get-verb-description requires :verb parameter", metrics};
    }
    
    try {
        auto& dispatcher = getGlobalDb9Dispatcher();
        IDb9Verb* verb = dispatcher.getVerbByName(params.verb_name);
        
        std::ostringstream result;
        
        if (verb == nullptr) {
            result << "{"
                   << "\"verb_name\": \"" << params.verb_name << "\""
                   << ", \"available\": false"
                   << ", \"description\": \"Verb '" << params.verb_name << "' not found in system\""
                   << ", \"status\": \"not_found\""
                   << ", \"suggestion\": \"Use getAvailableVerbs() to see all registered verbs\""
                   << "}";
        } else {
            std::string description = verb->getDescription();
            
            // Simple JSON escaping
            // Simple JSON escaping
            std::string escaped_description;
            for (char c : description) {
                if (c == '\n') {
                    escaped_description += "\\n";
                } else if (c == '\r') {
                    escaped_description += "\\r";
                } else if (c == '\t') {
                    escaped_description += "\\t";
                } else if (c == '"') {
                    escaped_description += "\\"; escaped_description += '"';
                } else if (c == '\\') {
                    escaped_description += "\\\\";
                    escaped_description += "\\\\";
                } else {
                    escaped_description += c;
                }
            }
            
            result << "{"
                   << "\"verb_name\": \"" << params.verb_name << "\""
                   << ", \"available\": true"
                   << ", \"description\": \"" << escaped_description << "\""
                   << ", \"status\": \"found\""
                   << "}";
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        AutoReflexiveMetrics metrics;
        metrics.operation_time_ms = std::chrono::milliseconds(duration_ms);
        metrics.items_processed = 1;
        
        return Db9Response{Db9Response::Success, result.str(), "", "", metrics};
        
    } catch (const std::exception& e) {
        AutoReflexiveMetrics metrics;
        return Db9Response{Db9Response::Error, "", "description_failed", 
                          "Failed to get verb description: " + std::string(e.what()), metrics};
    }
}

} // namespace LabDb