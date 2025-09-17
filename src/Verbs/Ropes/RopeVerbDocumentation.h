#pragma once
#include <string>
#include <vector>
#include <map>

namespace LabDb {
namespace Documentation {

struct WorkflowStep {
    std::string verb_name;
    std::string description;
    bool is_current = false;
};

struct MemexIntegration {
    std::string discovery_pattern;    // How to find entry points
    std::string navigation_pattern;   // How to use this verb
    std::string bush_principle;      // Which Memex principle this serves
};

struct RopeVerbDocumentation {
    std::string verb_name;
    std::string purpose;              // Plain English: what this does
    std::string context;              // Where in workflow this fits
    std::string usage_syntax;         // Command format
    std::vector<std::string> examples; // Real working commands
    std::vector<WorkflowStep> workflow; // Natural progression
    MemexIntegration memex;           // Discovery-navigation synthesis
    std::vector<std::string> related_verbs; // Next natural steps
    std::string troubleshooting;      // Common issues and fixes
    
    std::string format_contextual_help() const;
    std::string format_json_enhanced() const; // Enhanced JSON with context
    std::string format_workflow() const;
    std::string format_memex_context() const;
};

// Documentation registry for all rope verbs
class RopeDocumentationRegistry {
public:
    static RopeVerbDocumentation get_rope_create_docs();
    static RopeVerbDocumentation get_rope_append_docs();
    static RopeVerbDocumentation get_rope_chunk_docs();
    static RopeVerbDocumentation get_rope_traverse_docs();
    static RopeVerbDocumentation get_rope_list_docs();
    
    // Enhanced JSON with contextual information
    static std::string get_enhanced_json_docs(const std::string& verb_name);
    
    // User-friendly contextual help
    static std::string get_contextual_help(const std::string& verb_name);
    
private:
    static std::vector<WorkflowStep> get_standard_workflow(const std::string& current_verb);
    static std::map<std::string, RopeVerbDocumentation> docs_cache;
};

}} // namespace LabDb::Documentation