# LabDb9 Rope System Technical Specification

**Version**: 1.0  
**Date**: July 2025  
**Status**: Draft

## Overview

The **Rope System** extends LabDb9's triadic consciousness framework with ordered sequences of entities that support efficient navigation, insertion, and thematic organization. Ropes enable scholars to create contemplative pathways through knowledge graphs, maintaining semantic relationships while providing linear navigation.

## Motivation

Traditional graph databases excel at modeling relationships but lack support for **ordered traversal** and **contextual sequences**. The rope system addresses this by:

- **Contemplative Navigation**: Scholars can create study sequences like "pythagorean-theorem-development" 
- **Thematic Organization**: Group related entities (definitions, propositions, proofs) in meaningful order
- **Dynamic Insertion**: Add entities relative to existing ones without global reordering
- **Efficient Traversal**: O(1) navigation between adjacent entities

## Architecture

### Core Components

1. **Rope Storage**: LMDB-based ordered sequences with fractional indexing
2. **Adjacency Table**: O(1) navigation lookup table
3. **Metadata System**: Rope properties, statistics, and lifecycle management
4. **S-Expression Interface**: Integration with LabDb9's existing verb system

### Storage Layout

#### Primary Rope Storage
```
Key Format: "rope:{rope_name}:{sequence_id}"
Value: entity_id

Examples:
rope:pythagorean-study:000100 → "b1-def-10"
rope:pythagorean-study:000200 → "b1-post-5"
rope:pythagorean-study:000300 → "b1-p47"
```

#### Adjacency Table
```
Key Format: "adj:{rope_name}:{entity_id}"
Value: JSON adjacency record

Example:
adj:pythagorean-study:b1-p47 → {
  "prev": "b1-post-5",
  "next": "b1-p48",
  "sequence_id": "000300",
  "rope_name": "pythagorean-study",
  "position": 3
}
```

#### Rope Metadata
```
Key Format: "rope_meta:{rope_name}"
Value: JSON metadata record

Example:
rope_meta:pythagorean-study → {
  "name": "pythagorean-study",
  "description": "Study sequence for Pythagorean theorem",
  "length": 4,
  "first_entity": "b1-def-10",
  "last_entity": "b1-p48",
  "created": "2025-07-14T10:30:00Z",
  "modified": "2025-07-14T15:45:00Z",
  "creator": "scholar-123",
  "theme": "geometric-proofs"
}
```

## Data Structures

### Rope Entity
```cpp
struct RopeEntity {
    std::string entity_id;
    std::string sequence_id;
    std::optional<std::string> prev_entity;
    std::optional<std::string> next_entity;
    size_t position;
};
```

### Rope Metadata
```cpp
struct RopeMetadata {
    std::string name;
    std::string description;
    size_t length;
    std::string first_entity;
    std::string last_entity;
    std::chrono::system_clock::time_point created;
    std::chrono::system_clock::time_point modified;
    std::string creator;
    std::string theme;
    std::unordered_map<std::string, std::string> properties;
};
```

### Adjacency Record
```cpp
struct AdjacencyRecord {
    std::string entity_id;
    std::string rope_name;
    std::string sequence_id;
    std::optional<std::string> prev_entity;
    std::optional<std::string> next_entity;
    size_t position;
    
    nlohmann::json to_json() const;
    static AdjacencyRecord from_json(const nlohmann::json& j);
};
```

## S-Expression Interface

### Rope Management
```lisp
;; Create new rope
(create-rope :name "rope-name" :description "description" :dbid database-id)
;; Returns: {"status": "created", "rope_name": "rope-name"}

;; List existing ropes
(list-ropes :dbid database-id)
;; Returns: {"ropes": [{"name": "rope1", "length": 5, "theme": "..."}]}

;; Get rope information
(get-rope-info :name "rope-name" :dbid database-id)
;; Returns: complete rope metadata

;; Delete rope
(delete-rope :name "rope-name" :confirm true :dbid database-id)
;; Returns: {"status": "deleted", "entities_removed": 5}
```

### Entity Management
```lisp
;; Append to end
(rope-append :rope "rope-name" :entity "entity-id" :dbid database-id)

;; Prepend to beginning  
(rope-prepend :rope "rope-name" :entity "entity-id" :dbid database-id)

;; Insert relative to existing entity
(rope-insert-after :rope "rope-name" :after-entity "existing-id" :new-entity "new-id" :dbid database-id)
(rope-insert-before :rope "rope-name" :before-entity "existing-id" :new-entity "new-id" :dbid database-id)

;; Remove entity from rope
(rope-remove :rope "rope-name" :entity "entity-id" :dbid database-id)

;; Move entity within rope
(rope-move-after :rope "rope-name" :entity "entity-id" :after-entity "target-id" :dbid database-id)
```

### Navigation
```lisp
;; Get adjacent entities
(rope-next :rope "rope-name" :entity "entity-id" :dbid database-id)
(rope-prev :rope "rope-name" :entity "entity-id" :dbid database-id)
(rope-neighbors :rope "rope-name" :entity "entity-id" :dbid database-id)
;; Returns: {"prev": "prev-id", "current": "entity-id", "next": "next-id"}

;; Multi-step traversal
(rope-walk :rope "rope-name" :start "entity-id" :direction "forward" :steps 3 :dbid database-id)
;; Returns: ["start-id", "next-id", "next-next-id", "final-id"]

;; Get rope contents
(rope-contents :rope "rope-name" :dbid database-id)
(rope-slice :rope "rope-name" :start 2 :end 5 :dbid database-id)
```

### Discovery and Analysis
```lisp
;; Find entity position in rope
(rope-find :rope "rope-name" :entity "entity-id" :dbid database-id)
;; Returns: {"found": true, "position": 3, "sequence_id": "000300"}

;; Suggest related entities for insertion
(rope-suggest :rope "rope-name" :based-on "entity-id" :limit 5 :dbid database-id)

;; Find intersections between ropes
(rope-intersect :rope1 "rope1" :rope2 "rope2" :dbid database-id)

;; Analyze rope for gaps or improvements
(rope-analyze :rope "rope-name" :dbid database-id)
```

## Implementation Details

### Sequence ID Generation

The system uses **fractional indexing** to maintain order without global reordering:

```cpp
class SequenceIdGenerator {
public:
    // Generate ID for insertion between two existing IDs
    std::string generate_between(const std::string& before_id, const std::string& after_id);
    
    // Generate ID for append operation
    std::string generate_append(const std::string& last_id);
    
    // Generate ID for prepend operation  
    std::string generate_prepend(const std::string& first_id);
    
private:
    static constexpr size_t ID_LENGTH = 12;
    static constexpr int64_t ID_SPACING = 1000;
    
    std::string encode_base62(int64_t value);
    int64_t decode_base62(const std::string& encoded);
};
```

### Adjacency Management

```cpp
class AdjacencyManager {
public:
    // Update adjacency after insertion
    void update_after_insert(const std::string& rope_name,
                            const std::string& prev_entity,
                            const std::string& new_entity, 
                            const std::string& next_entity,
                            const std::string& sequence_id);
    
    // Update adjacency after removal
    void update_after_remove(const std::string& rope_name,
                           const std::string& removed_entity);
    
    // Get adjacency record
    std::optional<AdjacencyRecord> get_adjacency(const std::string& rope_name,
                                                const std::string& entity_id);
                                                
private:
    LMDB::Database& db_;
    void update_adjacency_record(const AdjacencyRecord& record);
};
```

### Transaction Management

All rope operations are **ACID compliant** using LMDB transactions:

```cpp
class RopeTransaction {
public:
    explicit RopeTransaction(LMDB::Database& db);
    ~RopeTransaction();
    
    void commit();
    void abort();
    
    // Transactional rope operations
    void insert_rope_entry(const std::string& key, const std::string& value);
    void update_adjacency(const AdjacencyRecord& record);
    void update_metadata(const RopeMetadata& metadata);
    
private:
    LMDB::Transaction txn_;
    bool committed_ = false;
};
```

## Performance Characteristics

| Operation | Time Complexity | Space Complexity | Notes |
|-----------|----------------|------------------|-------|
| `rope-next` | O(1) | O(1) | Direct adjacency lookup |
| `rope-prev` | O(1) | O(1) | Direct adjacency lookup |
| `rope-insert-after` | O(log n) | O(1) | LMDB B+ tree insertion |
| `rope-remove` | O(log n) | O(1) | LMDB deletion + adjacency update |
| `rope-contents` | O(n) | O(n) | Full rope traversal |
| `rope-find` | O(1) | O(1) | Adjacency table lookup |
| `rope-walk` | O(k) | O(k) | k = number of steps |

## Error Handling

### Error Types
```cpp
enum class RopeError {
    ROPE_NOT_FOUND,
    ENTITY_NOT_FOUND,
    ENTITY_ALREADY_EXISTS,
    INVALID_POSITION,
    EMPTY_ROPE,
    SEQUENCE_ID_EXHAUSTION,
    TRANSACTION_FAILED
};

class RopeException : public std::exception {
public:
    RopeException(RopeError error, const std::string& message);
    const char* what() const noexcept override;
    RopeError error() const { return error_; }
    
private:
    RopeError error_;
    std::string message_;
};
```

### Error Recovery
- **Sequence ID Exhaustion**: Automatic rope rebalancing
- **Orphaned Entities**: Cleanup during rope validation
- **Inconsistent Adjacency**: Rebuild from rope storage
- **Transaction Failures**: Automatic rollback with detailed logging

## Integration with Triadic Consciousness

Ropes enhance the Motion/Memory/Field framework:

### Motion (Dynamic Navigation)
- **rope-walk**: Contemplative traversal through conceptual space
- **rope-insert-after**: Dynamic knowledge construction
- **rope-neighbors**: Contextual awareness during exploration

### Memory (Relationship Encoding) 
- Rope membership as triadic relationships: `(entity, member-of, rope)`
- Sequential relationships: `(entity-a, rope-precedes, entity-b)`
- Thematic associations: `(rope, embodies-theme, geometric-harmony)`

### Field (Contextual Understanding)
- **rope-suggest**: Field-aware entity recommendations
- **rope-intersect**: Cross-thematic pattern discovery  
- **rope-analyze**: Holistic rope coherence assessment

## Usage Patterns

### Scholarly Study Sequences
```lisp
;; Create focused study path
(create-rope :name "pythagorean-proof-study" :description "Deep dive into Pythagorean theorem")
(rope-append :rope "pythagorean-proof-study" :entity "b1-def-10")  ; right angle
(rope-append :rope "pythagorean-proof-study" :entity "b1-p47")     ; main theorem
(rope-insert-before :rope "pythagorean-proof-study" :before-entity "b1-p47" :entity "b1-p46")  ; prep lemma
```

### Thematic Collections
```lisp
;; Organize by mathematical theme
(create-rope :name "golden-ratio-constructions" :theme "harmonic-geometry")
(rope-append :rope "golden-ratio-constructions" :entity "b6-def-3")   ; proportion definition
(rope-append :rope "golden-ratio-constructions" :entity "b2-p11")     ; geometric mean
(rope-append :rope "golden-ratio-constructions" :entity "b4-p11")     ; pentagon construction
```

### Chronological Sequences
```lisp
;; Historical development order
(create-rope :name "circle-theory-development" :theme "historical-progression")
(rope-append :rope "circle-theory-development" :entity "b1-def-15")   ; basic circle definition
(rope-append :rope "circle-theory-development" :entity "b3-def-1")    ; equal circles
(rope-append :rope "circle-theory-development" :entity "b3-p1")       ; circle construction
```

## Security and Access Control

### Rope Permissions
```cpp
enum class RopePermission {
    READ,           // View rope contents
    NAVIGATE,       // Use navigation verbs
    INSERT,         // Add entities to rope
    REMOVE,         // Remove entities from rope
    MODIFY,         // Change rope metadata
    DELETE,         // Delete entire rope
    ADMIN           // Full rope management
};

class RopeAccessControl {
public:
    bool check_permission(const std::string& user_id,
                         const std::string& rope_name,
                         RopePermission permission);
                         
    void grant_permission(const std::string& user_id,
                         const std::string& rope_name,
                         RopePermission permission);
                         
    void revoke_permission(const std::string& user_id,
                          const std::string& rope_name,
                          RopePermission permission);
};
```

## Testing Strategy

### Unit Tests
- **Sequence ID generation**: Fractional indexing correctness
- **Adjacency management**: Consistency after insertions/deletions
- **ACID transactions**: Rollback behavior on failures
- **S-expression parsing**: Verb parameter validation

### Integration Tests  
- **Multi-rope operations**: Cross-rope entity movements
- **Large rope performance**: Stress testing with 10K+ entities
- **Concurrent access**: Multiple users modifying same rope
- **Recovery scenarios**: Database corruption and repair

### Performance Benchmarks
- **Navigation speed**: 1M rope-next operations/second target
- **Insertion throughput**: 10K insertions/second sustained
- **Memory usage**: <1MB overhead per 1K entity rope
- **Storage efficiency**: <50 bytes overhead per rope entity

## Future Enhancements

### Planned Features
1. **Rope Branching**: Fork ropes for alternative study paths
2. **Temporal Ropes**: Track entity evolution over time
3. **Collaborative Ropes**: Multi-user rope editing with conflict resolution
4. **Rope Templates**: Predefined patterns for common study sequences
5. **Visual Navigation**: Graph-based rope visualization tools

### Research Directions
1. **Machine Learning Integration**: AI-suggested rope completions
2. **Semantic Similarity**: Auto-organize entities by conceptual distance
3. **Cross-Database Ropes**: Ropes spanning multiple LabDb9 instances
4. **Rope Algebras**: Mathematical operations on rope structures

## Conclusion

The Rope System transforms LabDb9 from a static knowledge graph into a **dynamic contemplative environment**. By providing ordered sequences with efficient navigation, scholars can create personalized pathways through complex domains while maintaining the triadic consciousness principles that make LabDb9 unique.

The system's emphasis on **relative insertion**, **O(1) navigation**, and **persistent storage** ensures that contemplative study flows are never interrupted by technical limitations. Integration with the existing S-expression interface maintains consistency with LabDb9's philosophical foundations while adding powerful new capabilities for knowledge exploration.

---

**Implementation Priority**: High  
**Estimated Effort**: 4-6 weeks  
**Dependencies**: LabDb9 core, LMDB, nlohmann/json  
**Target Release**: LabDb9 v0.4.0
