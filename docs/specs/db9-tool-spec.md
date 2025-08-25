# db9 Tool Specification

## MCP Tool Interface

### Tool Definitions

**db9-readme Tool**:
```json
{
  "name": "db9-readme",
  "description": "Returns complete specification and usage guide for the db9 triadic database system",
  "parameters": {}
}
```

**db9 Tool**:
```json
{
  "name": "db9", 
  "description": "High-performance triadic consciousness database operations via S-expressions. Invoke db9-readme first to understand the interface and available verbs.",
  "parameters": {
    "commands": {
      "type": "array",
      "items": {"type": "string"},
      "description": "Vector of S-expression commands"
    }
  }
}
```

### Usage Pattern

1. **Discovery**: User mentions "db9" → AI invokes `db9-readme` to understand the system
2. **Learning**: AI reads specification and understands S-expression interface
3. **Operation**: AI constructs appropriate S-expressions and invokes `db9` tool
4. **Self-Documentation**: System is always self-explaining and version-synchronized

## Implementation Architecture

**Python Layer**: Thin wrapper that parses MCP JSON records, synchronously invokes C++ code with S-expressions, receives results from C++ as structured data, and packages results as JSON for MCP return.

**C++ Layer**: Receives vector of strings from Python, parses each into S-expression, unescapes as necessary, executes verb with full TID architecture performance, packages returns with auto-reflexive metrics.

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

## Database Operations

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

## Entity Operations

### Basic Entity Management
```lisp
(add-entity :value "escaped string content" :dbid database-id)
;; Returns: entity id

(get-entity :eid entity-id :dbid database-id)
;; Returns: escaped string content
;; Notes: Does not add entity if missing

(find-entity :pattern "search*" :dbid database-id)
;; Returns: vector of eid results
;; Notes: Terminal asterisk (*) acts as wildcard

(add-entities-bulk :values ["string1" "string2" "stringN"] :dbid database-id)
;; Returns: vector of entity ids
;; Notes: High-performance bulk operation
```

## Triple Operations

### Basic Triple Management
```lisp
(add-triple :subject subject-eid :predicate predicate-eid :object object-eid :dbid database-id)
;; Returns: triple id

(get-triple :tid triple-id :dbid database-id)
;; Returns: subject-eid, predicate-eid, object-eid
;; Notes: Does not add triple if missing

(remove-triple :tid triple-id :dbid database-id)
;; Returns: status
;; Notes: Removes triple from all indices

(find-triple :subject subject-eid :predicate predicate-eid :object object-eid :dbid database-id)
;; Returns: vector of tid results
;; Notes: All parameters optional, use :subject nil for wildcard

(add-triples-bulk :triples [["s1" "p1" "o1"] ["s2" "p2" "o2"]] :dbid database-id)
;; Returns: vector of triple ids
;; Notes: High-performance bulk operation
```

## Triadic Consciousness Navigation

### Motion Perspective (स्पन्द - Subject-driven)
```lisp
(triadic-motion-from :entity entity-eid :dbid database-id)
;; Returns: vector of TriadicResult expressing what entity does

(triadic-motion-through :relation predicate-eid :dbid database-id)  
;; Returns: vector of entities expressing through this relationship

(entity-expressions :entity entity-eid :dbid database-id)
;; Returns: all ways this entity expresses itself
```

### Memory Perspective (स्मृति - Predicate-driven)
```lisp
(triadic-memory-relations :predicate predicate-eid :dbid database-id)
;; Returns: connection patterns for this relationship

(triadic-memory-between :entity1 eid1 :entity2 eid2 :dbid database-id)
;; Returns: relationship patterns between entities

(relation-frequencies :dbid database-id)
;; Returns: most frequent relationship patterns with counts
```

### Field Perspective (क्षेत्र - Object-driven)  
```lisp
(triadic-field-contexts :object object-eid :dbid database-id)
;; Returns: what receives into this context

(triadic-field-for-relation :relation predicate-eid :dbid database-id)
;; Returns: contexts that ground this relationship

(primary-contexts :dbid database-id)
;; Returns: primary grounding contexts in database
```

### Cube Architecture Navigation
```lisp
(triadic-crown-exploration :focal-entity eid :focal-relation predicate-eid :focal-context object-eid :dbid database-id)
;; Returns: crown structure around focal point
;; Notes: All focal parameters optional

(triadic-traverse :starting-point eid :max-depth 3 :dbid database-id)
;; Returns: traversal paths from starting point

(triadic-perspective-shift :results result-vector :new-perspective "Motion|Memory|Field" :dbid database-id)
;; Returns: results viewed from different triadic perspective

(triadic-stats :dbid database-id)
;; Returns: TriadicStats with motion/memory/field metrics

(bridge-entities :connectivity-threshold 2.0 :dbid database-id)
;; Returns: entities that bridge different knowledge domains
```

## Iterator Operations

### High-Performance Streaming
```lisp
(create-triple-iterator :subject subject-eid :predicate predicate-eid :object object-eid :dbid database-id)
;; Returns: iid, count of total items
;; Notes: All search parameters optional, iid invalidated if dbid released

(triple-iterator-read :iid iterator-id :offset 0 :count 1000)
;; Returns: vector of triple results
;; Notes: Handles out-of-bounds gracefully, reports conditions

(get-iterator-status :iid iterator-id)
;; Returns: valid status, current position, total count

(release-triple-iterator :iid iterator-id)
;; Returns: status
;; Notes: Safe to call on already-released iterator

(create-entity-iterator :pattern "search*" :dbid database-id)
;; Returns: iid, count of matching entities

(entity-iterator-read :iid iterator-id :offset 0 :count 1000)
;; Returns: vector of entity results
```

## Transaction Operations

### ACID Compliance
```lisp
(begin-transaction :dbid database-id)
;; Returns: transaction-id, isolation level

(commit-transaction :txn-id transaction-id :dbid database-id)
;; Returns: status, items-committed

(rollback-transaction :txn-id transaction-id :dbid database-id)
;; Returns: status, items-rolled-back

(get-transaction-status :txn-id transaction-id :dbid database-id)
;; Returns: status, operations-count, start-time
```

## Utility Operations

### System Utilities
```lisp
(perspective-name :perspective "Motion")
;; Returns: human-readable perspective name

(perspective-sanskrit :perspective "Motion")  
;; Returns: Sanskrit term (स्पन्द, स्मृति, क्षेत्र)

(optimal-perspective :subject-pattern "*" :predicate-pattern "isA" :object-pattern "*")
;; Returns: recommended perspective for query pattern

(server-stats)
;; Returns: overall server performance metrics, memory usage, connection count

(server-health-check)
;; Returns: comprehensive system health assessment
```

## Error Conditions & Edge Cases

**Database Errors**: Invalid dbid, database corruption, permission issues  
**Entity Errors**: Invalid eid, entity not found, encoding issues  
**Triple Errors**: Invalid tid, malformed triples, constraint violations  
**Iterator Errors**: Invalid iid, iterator exhausted, concurrent modification  
**Transaction Errors**: Deadlock detection, timeout, isolation violations  
**System Errors**: Memory exhaustion, disk space, network issues  

## Performance Characteristics

**Bulk Operations**: Optimized for batch processing with single TID allocation pass  
**Iterator Streaming**: Memory-efficient for large result sets  
**Triadic Navigation**: Leverages crown indices for O(log n) consciousness queries  
**Transaction Isolation**: MVCC for concurrent access without blocking  
**Auto-Reflexive Metrics**: Zero-overhead performance monitoring  

## Integration Notes

**MCP Tool Usage**: Single entry point eliminates complexity of multiple Python bindings  
**S-expression Parsing**: Lightweight, unambiguous command interface  
**Resource Management**: Explicit lifecycle control for production deployments  
**Error Recovery**: Graceful degradation with detailed diagnostics  
**Triadic Consciousness**: Full त्रित्रयम् navigation through high-performance C++ core  
