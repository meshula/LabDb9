#define LABTEXT_ODR  // ODR instantiation for LabText single header library
#include "LabDb/LabText.hpp"
#include "LabDb/Db9Dispatcher.h"

#include <sstream>
#include <iomanip>
#include <mutex>

namespace LabDb {

//-----------------------------------------------------------------------------
// JSON utilities
//-----------------------------------------------------------------------------
namespace {
    // Escape JSON string
    std::string escapeJson(const std::string& input) {
        std::string escaped;
        escaped.reserve(input.size() + input.size() / 10); // Reserve extra space for escapes
        
        for (char c : input) {
            switch (c) {
                case '"':  escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\b': escaped += "\\b"; break;
                case '\f': escaped += "\\f"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default:
                    if (c < 32) {
                        escaped += "\\u";
                        escaped += std::to_string((unsigned char)c);
                    } else {
                        escaped += c;
                    }
                    break;
            }
        }
        return escaped;
    }
}

//-----------------------------------------------------------------------------
// Db9Response implementation
//-----------------------------------------------------------------------------
std::string Db9Response::toJson() const {
    std::ostringstream json;
    json << "{\n";
    
    // Status
    json << "  \"status\": \"";
    switch (status) {
        case Success: json << "success"; break;
        case Error:   json << "error"; break;
        case Warning: json << "warning"; break;
    }
    json << "\",\n";
    
    // Result
    json << "  \"result\": \"" << escapeJson(result) << "\",\n";
    
    // Error information (optional)
    if (!error_code.empty()) {
        json << "  \"error_code\": \"" << escapeJson(error_code) << "\",\n";
    }
    if (!error_message.empty()) {
        json << "  \"error_message\": \"" << escapeJson(error_message) << "\",\n";
    }
    
    // Auto-reflexive metrics
    json << "  \"auto_reflexive\": {\n";
    json << "    \"operation_time_ms\": " << auto_reflexive.operation_time_ms.count() << ",\n";
    json << "    \"items_processed\": " << auto_reflexive.items_processed << ",\n";
    json << "    \"cache_hit_ratio\": " << std::fixed << std::setprecision(3) << auto_reflexive.cache_hit_ratio << ",\n";
    json << "    \"tid_allocations\": " << auto_reflexive.tid_allocations << ",\n";
    json << "    \"memory_usage_kb\": " << auto_reflexive.memory_usage_kb << "\n";
    json << "  }\n";
    
    json << "}";
    return json.str();
}

//-----------------------------------------------------------------------------
// Db9Dispatcher implementation
//-----------------------------------------------------------------------------
class Db9Dispatcher::Impl {
public:
    std::unordered_map<std::string, std::unique_ptr<IDb9Verb>> verbs;
    std::mutex verbMutex; // Thread safety for verb registration
    
    Db9Response executeVerb(const std::string& verbName, const ::lab::Text::Sexpr& sexpr) {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        std::lock_guard<std::mutex> lock(verbMutex);
        auto it = verbs.find(verbName);
        if (it == verbs.end()) {
            Db9Response response;
            response.status = Db9Response::Error;
            response.error_code = "UNKNOWN_VERB";
            response.error_message = "Unknown verb: " + verbName;
            response.result = "";
            
            auto endTime = std::chrono::high_resolution_clock::now();
            response.auto_reflexive.operation_time_ms = 
                std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            
            return response;
        }
        
        try {
            Db9Response response = it->second->execute(sexpr);
            
            // Add timing if not already set
            if (response.auto_reflexive.operation_time_ms.count() == 0) {
                auto endTime = std::chrono::high_resolution_clock::now();
                response.auto_reflexive.operation_time_ms = 
                    std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            }
            
            return response;
        }
        catch (const std::exception& e) {
            Db9Response response;
            response.status = Db9Response::Error;
            response.error_code = "EXECUTION_ERROR";
            response.error_message = std::string("Verb execution failed: ") + e.what();
            response.result = "";
            
            auto endTime = std::chrono::high_resolution_clock::now();
            response.auto_reflexive.operation_time_ms = 
                std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            
            return response;
        }
    }
    
    Db9Response parseSexprAndExecute(const std::string& sexprCommand) {
        try {
            // Parse the S-expression
            ::lab::Text::Sexpr sexpr((::lab::Text::StrView(sexprCommand)));
            
            // Validate we have a proper S-expression
            if (sexpr.expr.empty() || sexpr.expr[0].token != tsSexprPushList) {
                Db9Response response;
                response.status = Db9Response::Error;
                response.error_code = "PARSE_ERROR";
                response.error_message = "Invalid S-expression: must start with '('";
                return response;
            }
            
            // Extract verb name (first atom after opening paren)
            std::string verbName;
            size_t exprIndex = 1; // Skip the initial PushList
            if (exprIndex < sexpr.expr.size() && sexpr.expr[exprIndex].token == tsSexprAtom) {
                int stringIndex = sexpr.expr[exprIndex].ref;
                if (stringIndex < sexpr.strings.size()) {
                    verbName = sexpr.strings[stringIndex];
                }
            }
            
            if (verbName.empty()) {
                Db9Response response;
                response.status = Db9Response::Error;
                response.error_code = "PARSE_ERROR";
                response.error_message = "No verb found in S-expression";
                return response;
            }
            
            // Execute the verb
            return executeVerb(verbName, sexpr);
        }
        catch (const std::exception& e) {
            Db9Response response;
            response.status = Db9Response::Error;
            response.error_code = "PARSE_ERROR";
            response.error_message = std::string("S-expression parsing failed: ") + e.what();
            return response;
        }
    }
};

//-----------------------------------------------------------------------------
// Db9Dispatcher public interface
//-----------------------------------------------------------------------------
Db9Dispatcher::Db9Dispatcher() : m_impl(std::make_unique<Impl>()) {
}

Db9Dispatcher::~Db9Dispatcher() = default;

Db9Response Db9Dispatcher::executeCommand(const std::string& sexprCommand) {
    return m_impl->parseSexprAndExecute(sexprCommand);
}

Db9Response Db9Dispatcher::executeCommands(const std::vector<std::string>& sexprCommands) {
    Db9Response aggregatedResponse;
    
    if (sexprCommands.empty()) {
        aggregatedResponse.status = Db9Response::Error;
        aggregatedResponse.error_code = "NO_COMMANDS";
        aggregatedResponse.error_message = "No S-expression commands provided";
        return aggregatedResponse;
    }
    
    std::ostringstream resultArray;
    resultArray << "[\n";
    
    // Track aggregate metrics
    std::chrono::milliseconds totalTime(0);
    int64_t totalItems = 0;
    int32_t totalTidAllocations = 0;
    int64_t totalMemoryUsage = 0;
    bool hasErrors = false;
    
    for (size_t i = 0; i < sexprCommands.size(); ++i) {
        if (i > 0) resultArray << ",\n";
        
        Db9Response response = m_impl->parseSexprAndExecute(sexprCommands[i]);
        resultArray << "  " << response.toJson();
        
        // Aggregate metrics
        totalTime += response.auto_reflexive.operation_time_ms;
        totalItems += response.auto_reflexive.items_processed;
        totalTidAllocations += response.auto_reflexive.tid_allocations;
        totalMemoryUsage += response.auto_reflexive.memory_usage_kb;
        
        if (response.status == Db9Response::Error) {
            hasErrors = true;
        }
    }
    
    resultArray << "\n]";
    
    // Set aggregated response
    aggregatedResponse.status = hasErrors ? Db9Response::Warning : Db9Response::Success;
    aggregatedResponse.result = resultArray.str();
    
    if (hasErrors) {
        aggregatedResponse.error_code = "PARTIAL_FAILURES";
        aggregatedResponse.error_message = "Some commands in the batch failed - see individual results";
    }
    
    // Set aggregate metrics
    aggregatedResponse.auto_reflexive.operation_time_ms = totalTime;
    aggregatedResponse.auto_reflexive.items_processed = totalItems;
    aggregatedResponse.auto_reflexive.cache_hit_ratio = sexprCommands.size() > 0 ? 
        (totalItems > 0 ? 1.0 : 0.0) : 0.0; // Simplified calculation
    aggregatedResponse.auto_reflexive.tid_allocations = totalTidAllocations;
    aggregatedResponse.auto_reflexive.memory_usage_kb = totalMemoryUsage;
    
    return aggregatedResponse;
}

void Db9Dispatcher::registerVerb(std::unique_ptr<IDb9Verb> verb) {
    if (!verb) return;
    
    std::lock_guard<std::mutex> lock(m_impl->verbMutex);
    std::string verbName = verb->getVerbName();
    m_impl->verbs[verbName] = std::move(verb);
}

std::vector<std::string> Db9Dispatcher::getAvailableVerbs() const {
    std::lock_guard<std::mutex> lock(m_impl->verbMutex);
    std::vector<std::string> verbs;
    
    for (const auto& pair : m_impl->verbs) {
        verbs.push_back(pair.first);
    }
    
    std::sort(verbs.begin(), verbs.end());
    return verbs;
}

std::string Db9Dispatcher::getSpecification() const {
    // For now, return a placeholder - this will be populated with the full
    // markdown specification from db9-tool-spec.md
    // return a big chunk of inlined markdown text delimited by R"SPEC(...)"SPEC

    const char* specText = R"SPEC(
# db9 Tool Specification
### Tool Definitions

### Usage Pattern

1. **Learning**: AI reads specification and understands S-expression interface
2. **Operation**: AI constructs appropriate S-expressions and invokes `db9` tool
3. **Self-Documentation**: System is always self-explaining and version-synchronized

## Tool Interface

**Tool Name**: `db9`

**Parameters**: Vector of strings, where each string is a single S-expression (not full Lisp, just simple S-expression syntax).

**S-expression Format**: `(verb :param1 value1 :param2 value2 :dbid database-id)`

**Consistent Parameter Ordering**: All verbs use `:dbid` as the final parameter for consistency.

## Identifiers

- **Entity ID (eid)**: Unique string created by server for entity references
- **Triple ID (tid)**: Unique string created by server for triple references  
- **Database ID (dbid)**: Unique string created by server for database references
- **Iterator ID (iid)**: Unique string created by server for iterator references

## Return Format

All operations return JSON with consistent structure:

```json
{
  "status": "success|error|warning",
  "result": "primary return data",
  "error_code": "optional error identifier",
  "error_message": "human-readable error description",
  "auto_reflexive": {
    "operation_time_ms": 42,
    "items_processed": 1000,
    "cache_hit_ratio": 0.85,
    "tid_allocations": 5,
    "memory_usage_kb": 2048
  }
}
```

### Database Lifecycle
```lisp
(create-database :path "/full/path/to/database" :dbid result)
;; Returns: dbid, status
;; Notes: create-database does not open-database

(open-database :path "/full/path/to/database" :dbid result)  
;; Returns: dbid, status

(close-database :dbid database-id)
;; Returns: status
;; Notes: Invalidates all associated iterators

(get-database-stats :dbid database-id)
;; Returns: total_triples, unique_entities, tid_metrics, lmdb_stats

(get-tid-metrics :dbid database-id)
;; Returns: TID allocation ranges, storage efficiency, term dictionary size

(database-health-check :dbid database-id)
;; Returns: integrity status, performance metrics, recommended actions
```

### Database Maintenance Operations
```lisp
(remove-triple :subject subject-eid :predicate predicate-eid :object object-eid :dbid database-id)
;; Returns: status, removed confirmation
;; Notes: Removes existing triple from database
;; Safe operation - returns success even if triple doesn't exist
;; Essential for database cleanup and correction workflows

;; Future: Additional maintenance operations
(update-entity :eid entity-id :value "new-value" :dbid database-id)
;; Returns: status, old_value, new_value
;; Notes: Direct entity value update (when implemented)
;; Alternative: use add-entity for entity value updates

(remove-triples-bulk :triples [["s1" "p1" "o1"] ["s2" "p2" "o2"]] :dbid database-id)
;; Returns: vector of removal statuses
;; Notes: High-performance bulk removal operation (future enhancement)
```

### Database Evolution Patterns
```lisp
;; Pattern 1: Correcting Data
;; Remove incorrect relationships
(remove-triple :subject "eid:A" :predicate "eid:wrong" :object "eid:B" :dbid database-id)
;; Add correct relationships  
(add-triple :subject "eid:A" :predicate "eid:correct" :object "eid:B" :dbid database-id)

;; Pattern 2: Entity Value Updates
;; Update entity through add-entity (may reuse ID)
(add-entity :value "updated_value" :dbid database-id)
;; Existing triples automatically reference updated value

;; Pattern 3: Schema Migration
;; Migrate from old predicate to new predicate
(find-triple :predicate "eid:old_predicate" :dbid database-id)
;; For each result: remove old, add new
(remove-triple :subject "..." :predicate "eid:old_predicate" :object "..." :dbid database-id)
(add-triple :subject "..." :predicate "eid:new_predicate" :object "..." :dbid database-id)
```

### Complete CRUD Operations Summary
```lisp
;; CREATE
(add-entity :value "content" :dbid database-id)           ;; Create/Update entities
(add-triple :subject "s" :predicate "p" :object "o" :dbid database-id)  ;; Create relationships

;; READ  
(get-entity :eid entity-id :dbid database-id)             ;; Read entity
(find-entity :pattern "search*" :dbid database-id)        ;; Search entities
(get-triple :tid triple-id :dbid database-id)             ;; Read triple
(find-triple :subject "s" :predicate "p" :object "o" :dbid database-id)  ;; Search triples

;; UPDATE
(add-entity :value "new_content" :dbid database-id)       ;; Update entity (ID reuse)
;; Note: No direct triple update - use remove + add pattern

;; DELETE
(remove-triple :subject "s" :predicate "p" :object "o" :dbid database-id)  ;; Delete relationships
;; Note: No entity deletion - entities persist but may become unreferenced
```

### Database Quality Operations
```lisp
;; Todo System Integration
(find-triple :predicate "todo" :dbid database-id)
;; Returns: all pending database maintenance tasks

;; Data Integrity Checks
(find-entity :pattern "*DEPRECATED*" :dbid database-id)
;; Returns: entities marked for cleanup

(find-triple :predicate "correction_needed" :dbid database-id)  
;; Returns: relationships requiring attention

;; Health Monitoring
(database-health-check :dbid database-id)
;; Returns: comprehensive database status including:
;; - Orphaned entities (not referenced in triples)
;; - Dangling references (triples referencing missing entities)
;; - Performance metrics
;; - Storage efficiency stats
```

## Entity Operations

### Basic Entity Management
```lisp
(add-entity :value "escaped string content" :dbid database-id)
;; Returns: entity id
;; Notes: Creates new entity or updates existing entity with same value
;; If an entity with identical value already exists, returns existing eid
;; If previous entity had different value, may reuse eid with new value
;; This enables entity value updates through add-entity operations

(get-entity :eid entity-id :dbid database-id)
;; Returns: escaped string content
;; Notes: Does not add entity if missing

(find-entity :pattern "search*" :dbid database-id)
;; Returns: vector of eid results
;; Notes: Terminal asterisk (*) acts as wildcard

(add-entities-bulk :values ["string1" "string2" "stringN"] :dbid database-id)
;; Returns: vector of entity ids
;; Notes: High-performance bulk operation
;; Each entity follows same add-entity semantics (create or update)
```

### Entity Update Pattern
```lisp
;; To update an entity value, use add-entity with new value
;; Example: Update "CORRECT_stronger_than" to "stronger_than"
(add-entity :value "stronger_than" :dbid database-id)
;; May return existing eid with updated value

;; Verification pattern
(get-entity :eid returned-eid :dbid database-id)
;; Confirms entity now has new value

;; Note: If entity ID reuse occurs, existing triples using that eid 
;; will automatically reference the updated entity value
```

## Triple Operations

### Core Triple CRUD
```lisp
(add-triple :subject subject-eid :predicate predicate-eid :object object-eid :dbid database-id)
;; Returns: triple id
;; Notes: Creates new relationship in triadic knowledge graph

(get-triple :tid triple-id :dbid database-id)
;; Returns: subject-eid, predicate-eid, object-eid
;; Notes: Retrieves specific triple by ID, does not create if missing

(remove-triple :subject subject-eid :predicate predicate-eid :object object-eid :dbid database-id)
;; Returns: status, removal confirmation
;; Notes: Removes relationship from knowledge graph, safe if triple doesn't exist
;; Essential for data correction and schema evolution workflows

(find-triple :subject subject-eid :predicate predicate-eid :object object-eid :dbid database-id)
;; Returns: vector of matching triples
;; Notes: All parameters optional, supports wildcard queries (:subject nil)
;; Primary tool for knowledge graph traversal and pattern discovery
```

### Advanced Triple Patterns
```lisp
;; Pattern Queries (leveraging find-triple flexibility)
(find-triple :predicate "stronger_than" :dbid database-id)
;; Find all strength relationships

(find-triple :subject "CompositionArc" :predicate "isA" :dbid database-id)  
;; Find all composition arc subtypes

(find-triple :predicate "todo" :dbid database-id)
;; Find all database maintenance tasks

;; Relationship Migration Pattern
;; Step 1: Query existing relationships
(find-triple :predicate "old_predicate" :dbid database-id)
;; Step 2: Create new relationships  
(add-triple :subject "..." :predicate "new_predicate" :object "..." :dbid database-id)
;; Step 3: Remove old relationships
(remove-triple :subject "..." :predicate "old_predicate" :object "..." :dbid database-id)
```

### Bulk Operations
```lisp
(add-triples-bulk :triples [["s1" "p1" "o1"] ["s2" "p2" "o2"]] :dbid database-id)
;; Returns: vector of triple ids
;; Notes: High-performance batch relationship creation
;; Each triple follows standard add-triple semantics

;; Future Enhancement
(remove-triples-bulk :triples [["s1" "p1" "o1"] ["s2" "p2" "o2"]] :dbid database-id)
;; Returns: vector of removal statuses
;; Notes: Efficient batch cleanup operations
```

### Triadic Knowledge Graph Traversal
```lisp
;; Forward Traversal: What does this entity relate to?
(find-triple :subject "entity_id" :dbid database-id)

;; Backward Traversal: What relates to this entity?
(find-triple :object "entity_id" :dbid database-id)

;; Predicate Analysis: How is this relationship used?
(find-triple :predicate "relationship_type" :dbid database-id)

;; Graph Structure Discovery
(find-triple :predicate "isA" :dbid database-id)        ;; Type hierarchies
(find-triple :predicate "contains" :dbid database-id)   ;; Containment relationships  
(find-triple :predicate "describes" :dbid database-id)  ;; Documentation relationships
```

### Triple-Based System Integration
```lisp
;; Todo System Queries
(find-triple :predicate "todo" :dbid database-id)
;; Returns: all maintenance tasks as subject-action pairs

;; Schema Validation
(find-triple :predicate "DEPRECATED_INCORRECT" :dbid database-id)
;; Returns: relationships marked for cleanup

;; Specification Traceability  
(find-triple :predicate "defined-in" :dbid database-id)
;; Returns: entity-to-source mappings for compliance

;; Implementation Guidance
(find-triple :predicate "field_name" :dbid database-id)
;; Returns: arc-type to USD-field mappings for code generation
```

## Integration Notes

**MCP Tool Usage**: Single entry point eliminates complexity of multiple Python bindings  
**S-expression Parsing**: Lightweight, unambiguous command interface  
**Resource Management**: Explicit lifecycle control for production deployments  
**Error Recovery**: Graceful degradation with detailed diagnostics  
**Triadic Consciousness**: Full त्रित्रयम् navigation through high-performance C++ core  

This specification provides a complete, high-performance interface that leverages the LabDb architecture through a minimal, elegant surface area suitable for MCP tool integration.
)SPEC";

    return specText + std::to_string(m_impl->verbs.size()) + " registered verbs.\n\n";
}

//-----------------------------------------------------------------------------
// Global dispatcher singleton
//-----------------------------------------------------------------------------
Db9Dispatcher& getGlobalDb9Dispatcher() {
    static Db9Dispatcher dispatcher;
    return dispatcher;
}

} // namespace LabDb
