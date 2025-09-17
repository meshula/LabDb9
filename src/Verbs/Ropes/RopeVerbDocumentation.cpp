#include "RopeVerbDocumentation.h"
#include <sstream>

namespace LabDb {
namespace Documentation {

// Static cache for documentation objects
std::map<std::string, RopeVerbDocumentation> RopeDocumentationRegistry::docs_cache;

std::string RopeVerbDocumentation::format_contextual_help() const {
    std::stringstream ss;
    
    ss << "\n=== ROPE VERB: " << verb_name << " ===\n";
    ss << "PURPOSE: " << purpose << "\n";
    ss << "CONTEXT: " << context << "\n\n";
    
    ss << "USAGE: " << usage_syntax << "\n\n";
    
    if (!examples.empty()) {
        ss << "EXAMPLES:\n";
        for (const auto& example : examples) {
            ss << "  " << example << "\n";
        }
        ss << "\n";
    }
    
    return ss.str();
}

// Enhanced contextual help for rope-create
std::string RopeDocumentationRegistry::get_contextual_help(const std::string& verb_name) {
    if (verb_name == "rope-create") {
        return "\n=== ROPE-CREATE: ENHANCED HELP ===\n"
               "PURPOSE: Creates ordered text sequence container (a 'rope')\n"
               "CONTEXT: Foundation step for Bush's associative navigation\n\n"
               "USAGE: (rope-create :rope-name \"name\" :description \"desc\" :dbid db1)\n\n"
               "WORKFLOW:\n"
               "  > rope-create  <- YOU ARE HERE (create container)\n"
               "    rope-append  <- Next: add text entities\n"
               "    rope-chunk   <- Then: navigate bounded segments\n"
               "    rope-traverse<- Finally: linear reading\n\n"
               "MEMEX: Creates foundation for persistent associative trails\n";
    }
    
    if (verb_name == "rope-chunk") {
        return "\n=== ROPE-CHUNK: CORE MEMEX NAVIGATION ===\n"
               "PURPOSE: Extract bounded text segments for contemplative reading\n"
               "CONTEXT: Transforms discovery into readable text context\n\n"
               "USAGE: (rope-chunk :rope-name \"name\" :center-entity \"entity-id\" :fragment-count 24 :dbid db1)\n\n"
               "DISCOVERY-NAVIGATION SYNTHESIS:\n"
               "  1. find-triple-enhanced → discover entry points\n"
               "  2. rope-chunk → navigate to readable context\n"
               "  3. Bush's vision: associative trails with contemplative reading\n\n"
               "MEMEX: Core verb for Bush's 'trails that do not fade'\n";
    }
    
    return "Enhanced contextual help not yet implemented for: " + verb_name;
}

}} // namespace LabDb::Documentation