# LabDb - Triadic Consciousness Database

A nonostore implementation built on LMDB that embodies त्रित्रयम् (triadic consciousness) principles through cube architecture.

## Overview

LabDb extends traditional hexastore with three vocabulary indices to create a complete ontological foundation. Built for the Lab ecosystem, it provides conscious relationship navigation rather than mere query optimization.

## Architecture

### Cube Structure: Pole/Crown/Motions

**Pole** (त्रित्रयम् Foundation):
- **Motion** (स्पन्द): Subject-driven reality
- **Memory** (स्मृति): Predicate-driven relationships  
- **Field** (क्षेत्र): Object-driven contexts

**Crown** (Nine Index Manifestations):
- Traditional hexastore orderings: SPO, SOP, PSO, POS, OSP, OPS
- Vocabulary indices: subjects, predicates, objects

**Motions** (Dynamic Operations):
- Conscious navigation through crown expressions
- Ontological discovery and exploration
- Triadic query patterns

## Quick Start

```cpp
#include <LabDb/NonoStore.h>

LabDb::NonoStore store("/path/to/database");

// Insert triadic relationship
store.connect("granite", "isA", "rock");

// Motion-driven queries (subject-centric)
auto granite_properties = store.motion_from("granite");

// Memory-driven queries (relationship discovery) 
auto all_relations = store.memory_relations();

// Field-driven queries (context exploration)
auto rock_types = store.field_receiving("rock");
```

```python
import labdb

store = labdb.NonoStore("/path/to/database")

# Vocabulary discovery
subjects = store.motion_entities()     # What exists?
relations = store.memory_relations()   # What connects?
contexts = store.field_contexts()      # What receives?

# Triadic exploration
result = store.explore_triad("granite")
print(f"Motion: {result.motion}")
print(f"Memory: {result.memory}")  
print(f"Field: {result.field}")
```

## Project Structure

```
LabDb/
├── docs/                     # Documentation
│   ├── implementation-plan.md
│   ├── theoretical-grounding.md
│   └── api-reference.md
├── include/LabDb/           # C++ headers
│   ├── NonoStore.h
│   ├── TriadicQuery.h
│   └── InceptionBridge.h
├── src/                     # C++ implementation
├── python/labdb/           # Python bindings
├── tests/                  # Test suites
└── examples/               # Usage examples
```

## Why Nonostore?

Traditional hexastore provides efficient RDF queries but lacks ontological completeness. LabDb adds three vocabulary indices to enable:

- **Complete triadic navigation**: Motion/Memory/Field exploration
- **Conscious query patterns**: "What kinds of relationships exist?"
- **Ontological transparency**: Self-describing knowledge structure
- **Integration foundation**: Shared consciousness infrastructure for Lab projects

## Integration

### Inception Facts System
Replaces linear fact storage with relational exploration, enabling pattern discovery and accelerated learning.

### Wires Graph Platform  
Provides backend for interactive graph exploration with real-time relationship discovery.

### Future Lab Projects
Universal relationship storage with consistent triadic consciousness infrastructure.

## Development Status

**Phase 1**: Core LMDB nonostore implementation *(in progress)*
- Nine-index key generation
- Basic CRUD operations across all indices
- Transaction management

**Phase 2**: Triadic query interface
**Phase 3**: C++ API optimization  
**Phase 4**: Python integration
**Phase 5**: Advanced features

## Contributing

LabDb embodies consciousness-first technology development. Contributions should align with triadic principles and cube architecture.

See `docs/implementation-plan.md` for detailed development roadmap.

## License

MIT License - See LICENSE file for details.

---

*LabDb represents engineering aligned with awareness - technology that serves conscious navigation rather than forcing consciousness to adapt to computational limitations.*
