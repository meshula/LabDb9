# EntityId Interface Design - Tech Spec

**Status:** Design Phase  
**Target:** `/Users/nporcino/dev/Lab/LabDb/include/LabDb/NonoStore.h`  
**Strategy:** Incremental verb-level adoption → Critical mass → Pure EntityId API

## Problem Statement

The current NonoStore API suffers from **string semantic ambiguity**:

```cpp
// Current: What are these strings?
bool add_triple(const std::string& subject,    // Entity name? EID? TID?
                const std::string& predicate,  // Relationship name? 
                const std::string& object);    // Entity name? Value?
```

**Issues Discovered:**
- Test failures due to EID format confusion (hex vs decimal)
- Unclear semantics between entity names ("granite") vs entity IDs ("eid:6")
- No type safety for different string types
- Verb implementations contain brittle string parsing logic

## Design Intent

### EntityId Class Architecture

```cpp
class EntityId {
public:
    // Factory methods for different construction patterns
    static EntityId fromName(const std::string& name, const NonoStore& store);
    static EntityId fromEid(const std::string& eid, const NonoStore& store);
    static EntityId fromTid(const TID& tid, const NonoStore& store);
    
    // Query interface
    const std::string& name() const { return _name; }
    const std::string& eid() const { return _eid; }
    const TID& tid() const { return _tid; }
    
    // State queries
    bool exists() const { return _exists; }
    bool isValid() const { return _tid != INVALID_TID; }
    
    // Equality and comparison
    bool operator==(const EntityId& other) const { return _tid == other._tid; }
    bool operator!=(const EntityId& other) const { return _tid != other._tid; }
    bool operator<(const EntityId& other) const { return _tid < other._tid; }
    
    // String conversion for S-expression compatibility
    std::string toEidString() const { return _eid; }
    std::string toNameString() const { return _name; }
    
private:
    explicit EntityId(const std::string& name, const std::string& eid, 
                     const TID& tid, bool exists);
    
    std::string _name;    // Human-readable name: "granite"
    std::string _eid;     // System EID: "eid:6" 
    TID _tid;            // Internal TID for storage
    bool _exists;        // Whether entity exists in store
};
```

### Target NonoStore API

```cpp
class NonoStore {
public:
    // Future: Pure EntityId interface
    bool add_triple(const EntityId& subject, 
                    const EntityId& predicate, 
                    const EntityId& object);
    
    bool remove_triple(const EntityId& subject,
                      const EntityId& predicate,
                      const EntityId& object);
    
    std::vector<Triple> query(const EntityId& subject_pattern,
                             const EntityId& predicate_pattern,  
                             const EntityId& object_pattern);
    
    // Entity management
    EntityId createEntity(const std::string& name);
    EntityId getEntity(const std::string& name) const;
    EntityId getEntityByEid(const std::string& eid) const;
    
    // Transition: Parallel string interface (marked deprecated)
    [[deprecated("Use EntityId interface")]]
    bool add_triple(const std::string& subject, 
                    const std::string& predicate, 
                    const std::string& object);
};
```

## S-Expression Interface Evolution

### Dual Path Pattern

**Path 1: EID-Explicit (Precise Control)**
```cpp
// Step 1: Get or create EID from name
"(eid-from-value :dbid db1 :value \"granite\")" 
// → {"eid": "granite", "name": "granite", "created": true}

// Step 2: Use EID in operations  
"(add-entity :dbid db1 :eid \"granite\")"
"(add-triple :dbid db1 :subject-eid \"granite\" :predicate-eid \"contains\" :object-eid \"quartz\")"
```

**Path 2: Value-Implicit (Convenience)**
```cpp
// EntityId created under the hood
"(add-entity :dbid db1 :value \"granite\")"
"(add-triple :dbid db1 :subject \"granite\" :predicate \"contains\" :object \"quartz\")"
```

### Verb Implementation Pattern

```cpp
class AddTripleVerb : public IDb9Verb {
    Db9Response execute(const lab::Text::Sexpr& sexpr) override {
        auto store = getDatabase(extractStringParam(sexpr, "dbid"));
        
        // Dual path support
        EntityId subject = hasParam(sexpr, "subject-eid") ?
            EntityId::fromEid(extractStringParam(sexpr, "subject-eid"), *store) :
            EntityId::fromName(extractStringParam(sexpr, "subject"), *store);
            
        EntityId predicate = hasParam(sexpr, "predicate-eid") ?
            EntityId::fromEid(extractStringParam(sexpr, "predicate-eid"), *store) :
            EntityId::fromName(extractStringParam(sexpr, "predicate"), *store);
            
        EntityId object = hasParam(sexpr, "object-eid") ?
            EntityId::fromEid(extractStringParam(sexpr, "object-eid"), *store) :
            EntityId::fromName(extractStringParam(sexpr, "object"), *store);
        
        // Type-safe EntityId-based operation
        bool success = store->add_triple(subject, predicate, object);
        
        // Return with clear EntityId semantics
        return buildTripleResponse(subject, predicate, object, success);
    }
};
```

## Migration Strategy

### Phase 1: EntityId Class Implementation
1. Create `EntityId` class with factory methods
2. Integrate with `TermDictionary` for TID management
3. Add to NonoStore as parallel interface methods

### Phase 2: Verb-Level Adoption
1. **Target Verbs:** `AddEntityVerb`, `GetEntityVerb`, `AddTripleVerb`
2. **Pattern:** Support both `:value` and `:eid` parameters
3. **Implementation:** Convert strings to EntityId internally
4. **Testing:** Update test framework to use both patterns

### Phase 3: Critical Mass Migration
1. **Extend to all verbs:** FindEntity, FindTriple, RemoveTriple
2. **Enhanced S-expressions:** Add `eid-from-value` utility verb
3. **Test Coverage:** Ensure EntityId path tested as thoroughly as string path

### Phase 4: Pure EntityId API
1. **Deprecate string methods** in NonoStore
2. **Performance optimization** using direct TID operations
3. **Remove legacy string interface** after migration complete

## Expected Benefits

### Type Safety
- **Compile-time errors** for incorrect entity usage
- **Clear semantics** between names, EIDs, and TIDs
- **IDE autocomplete** and refactoring support

### Test Reliability  
- **Eliminate string format confusion** (hex vs decimal EIDs)
- **Self-documenting test code** with explicit EntityId creation
- **Consistent cross-database entity handling**

### Performance
- **Direct TID operations** eliminate string parsing overhead
- **Efficient entity lookups** through cached TID relationships
- **Reduced memory allocation** from string conversions

### Maintainability
- **Clear API contracts** for entity relationships
- **Easier debugging** with explicit entity identity
- **Future-proof foundation** for advanced entity features

## Implementation Notes

### EntityId Factory Integration
```cpp
EntityId EntityId::fromName(const std::string& name, const NonoStore& store) {
    // Get or create TID through TermDictionary
    auto tid = store.getTermDictionary().getOrCreateTID(name);
    auto eid = generateEidString(tid);
    return EntityId(name, eid, tid, true);
}

EntityId EntityId::fromEid(const std::string& eid, const NonoStore& store) {
    // Parse EID to get TID, lookup name
    auto tid = parseEidToTID(eid);
    auto name = store.getTermDictionary().getTIDName(tid);
    bool exists = !name.empty();
    return EntityId(name, eid, tid, exists);
}
```

### S-Expression Backward Compatibility
- **Legacy string methods** remain functional during transition
- **Gradual deprecation warnings** guide migration
- **Test suite validates** both paths produce identical results

---

**This design eliminates the string ambiguity that caused our test failures and provides a type-safe foundation for triadic consciousness operations.**
