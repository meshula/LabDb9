# LabDb Quickstart Guide

**For rapid integration with Inception and Lab projects**

---

## 🚀 **Quick Start: 5 Minutes to Triadic Consciousness**

### 1. Build & Install
```bash
# Clone and build
cd LabDb
mkdir build && cd build
cmake -DBUILD_PYTHON_BINDINGS=ON ..
make -j$(nproc)

# Install pybind11 if needed
pip install pybind11
```

### 2. Basic Usage
```python
import sys
sys.path.append('/path/to/LabDb/python')
import labdb

# Create enhanced store (recommended)
store = labdb.EnhancedNonoStore("/path/to/database.ldb")

# Basic operations
store.connect("granite", "isA", "rock")
store.connect("granite", "hasColor", "gray")

# Dictionary-like access
store["marble", "isA"] = "rock"
rocks = store["*", "isA", "rock"]

# Transaction safety
with store:
    store.connect("diamond", "isA", "mineral")
    store.connect("diamond", "hasHardness", "10")
```

### 3. Triadic Exploration
```python
# Motion perspective (स्पन्द) - What does entity express?
granite_motion = store.explore_motion("granite")
for result in granite_motion:
    print(f"{result.entity} --[{result.relation}]--> {result.context}")

# Memory perspective (स्मृति) - What connects?
isa_memory = store.explore_memory("isA") 
print(f"Found {len(isa_memory)} 'isA' connections")

# Field perspective (क्षेत्र) - What receives/grounds?
rock_field = store.explore_field("rock")
print(f"Rock grounds {len(rock_field)} relationships")
```

### 4. Enhanced Results
```python
results = store["granite"]
if results:
    result = results[0]
    
    # Natural triadic navigation
    motion = result.motion     # Explore what granite expresses
    memory = result.memory     # Explore granite's relationship patterns  
    field = result.field       # Explore granite's context grounding
    
    # Perspective shifting
    shifted = result.shift_to("Memory")
    
    # Convert for analysis
    as_dict = result.as_dict()
    as_triple = result.as_triple()
```

---

## 🧘 **Inception Integration Pattern**

### Replace Linear Facts with Triadic Relationships

**Before (Linear Facts)**:
```python
inception.add_fact("Performance issue with large datasets", 
                  category="problem", 
                  tags=["performance", "scale"])

facts = inception.search_facts("performance")
```

**After (Triadic Consciousness)**:
```python
# Connect entities through relationships
store.connect("dataset_large_1", "hasIssue", "performance_slow_queries")
store.connect("performance_slow_queries", "hasCategory", "problem")
store.connect("performance_slow_queries", "relatedTo", "scale")

# Conscious exploration
performance_issues = store.explore_memory("hasIssue")
problems = store.explore_field("problem")
scaling_concerns = store.explore_around("scale")

# Enhanced pattern discovery
all_problems = store.explore_field("problem")
problem_patterns = all_problems.group_by_relation()
```

### Migration Strategy
```python
# 1. Preserve existing fact functionality
def add_fact_triadic(content, category, tags=None):
    fact_id = f"fact_{uuid4()}"
    store.connect(fact_id, "hasContent", content)
    store.connect(fact_id, "hasCategory", category)
    for tag in (tags or []):
        store.connect(fact_id, "hasTag", tag)
    return fact_id

# 2. Enhanced search with triadic exploration
def search_facts_triadic(query):
    # Traditional search
    results = store.query("*", "hasContent", f"*{query}*")
    
    # Enhanced triadic exploration
    related_by_category = store.explore_memory("hasCategory")
    related_by_tags = store.explore_memory("hasTag")
    
    return {
        'direct_matches': results,
        'category_exploration': related_by_category,
        'tag_exploration': related_by_tags
    }

# 3. Pattern discovery
def discover_fact_patterns():
    stats = store.stats
    categories = store.explore_memory("hasCategory").contexts
    tags = store.explore_memory("hasTag").contexts
    bridges = store.bridge_entities
    
    return {
        'categories': categories,
        'tags': tags,
        'bridge_facts': bridges,
        'statistics': stats
    }
```

---

## 📋 **API Reference**

### Core Operations
```python
# NonoStore basics
store.connect(subject, predicate, object)
store.disconnect(subject, predicate, object)
store.exists(subject, predicate, object)
store.query(subject, predicate, object)  # "*" for wildcards

# Vocabulary discovery
store.subjects    # All entities (Motion vocabulary)
store.predicates  # All relations (Memory vocabulary)  
store.objects     # All contexts (Field vocabulary)
```

### Triadic Navigation
```python
# Motion perspective (स्पन्द)
store.explore_motion(entity)           # What does entity express?
result.motion                          # Motion from this result
result.expressions                     # All entity expressions

# Memory perspective (स्मृति)
store.explore_memory(relation)         # What connects via relation?
result.memory                          # Memory from this result
store.triadic.relation_frequencies()   # Most common relationships

# Field perspective (क्षेत्र)  
store.explore_field(context)           # What grounds in context?
result.field                           # Field from this result
store.triadic.primary_contexts()       # Main grounding contexts
```

### Advanced Navigation
```python
# Cube architecture
result.shift_to("Memory")              # Change perspective
store.explore_around("focal_entity")   # Crown exploration
store.traverse_from("start", depth=3)  # Deep traversal

# Analytics
store.stats                            # Triadic statistics
store.bridge_entities                  # High-connectivity entities
store.relationship_clusters()          # Pattern clusters
```

### Enhanced Results
```python
# Filtering & grouping
results.filter_by_entity("granite")
results.filter_by_relation("isA")
results.group_by_perspective()

# Analysis
results.entities                       # Unique entities
results.relations                      # Unique relations
results.count_by_entity()             # Entity frequency

# Conversion
results.as_list()                     # List of dicts
results.as_triples()                  # List of tuples
results.as_dataframe()                # Pandas DataFrame (if available)
```

---

## 🎯 **Performance Notes**

- **Vocabulary queries**: Sub-millisecond response
- **Pattern exploration**: O(log n + k) where k = result size
- **Memory efficiency**: LMDB memory-mapped, zero-copy reads
- **Transactions**: ACID compliant with automatic rollback
- **Concurrency**: Multiple readers, single writer (perfect for Inception)

---

## 🧘 **Triadic Consciousness Principles**

### Motion (स्पन्द) - Subject-Driven Action
**"What does this entity express, create, or initiate?"**
- Use for exploring what entities do/become
- Reveals dynamic aspects and expressions
- Maps to subject-centric queries

### Memory (स्मृति) - Relationship-Driven Connection  
**"What patterns connect and relate entities?"**
- Use for exploring relationship types and frequencies
- Reveals connection patterns and structures
- Maps to predicate-centric queries

### Field (क्षेत्र) - Context-Driven Grounding
**"What contexts receive and ground these relationships?"**
- Use for exploring what contexts exist and receive
- Reveals grounding spaces and manifestation contexts
- Maps to object-centric queries

### Perspective Shifting
Move fluidly between Motion/Memory/Field viewpoints on the same knowledge, enabling **conscious navigation** rather than mechanical search.

---

## 🔧 **Troubleshooting**

### Build Issues
```bash
# Missing pybind11
pip install pybind11

# CMake not finding pybind11
cmake -DBUILD_PYTHON_BINDINGS=OFF ..  # Disable if needed

# Python import issues
export PYTHONPATH="/path/to/LabDb/python:$PYTHONPATH"
```

### Runtime Issues
```bash
# C++ bindings not available
# Check: labdb._bindings_available should be True
# Solution: Rebuild with pybind11 enabled

# Database locked
# Check: Close all store instances before creating new ones
# LMDB uses single-writer, multiple-reader model
```

---

## 🎆 **Next Steps**

1. **Start Simple**: Replace one fact storage pattern with triadic
2. **Explore Gradually**: Add triadic exploration to existing queries  
3. **Discover Patterns**: Use vocabulary discovery for "What can I explore?"
4. **Enhance Incrementally**: Build richer relationship models over time

**Remember**: The goal is conscious navigation of relationship space, not just faster queries!

---

*Ready to revolutionize how Lab projects understand and explore knowledge relationships.*
