# LabDb - Triadic Consciousness Database

A nonostore implementation built on LMDB that embodies त्रित्रयम् (triadic consciousness) principles through cube architecture.

## Overview

LabDb extends traditional hexastore with three vocabulary indices to create a complete ontological foundation. Built for the Lab ecosystem, it provides conscious relationship navigation rather than mere query optimization.

**Key Features**:
- **4.18× storage efficiency** with TID-based architecture
- **Triadic consciousness navigation** (Motion/Memory/Field perspectives)
- **Professional CMake integration** via FetchContent
- **Complete Python bindings** with pybind11
- **ACID transactions** with LMDB backend
- **Rich provenance metadata** for knowledge tracking

## Using LabDb in Your Project

### Quick Integration with CMake FetchContent

Add LabDb to any C++ project in 3 steps:

```cmake
# 1. Include FetchContent
include(FetchContent)

# 2. Fetch LabDb
FetchContent_Declare(
    LabDb
    GIT_REPOSITORY https://github.com/meshula/LabDb9.git
    GIT_TAG        v0.3.0  # Latest stable release
)
FetchContent_MakeAvailable(LabDb)

# 3. Link to your target
target_link_libraries(myapp PRIVATE LabDb::LabDb)
```

### Basic Usage

**C++ Example**:
```cpp
#include <LabDb/NonoStore.h>
#include <LabDb/TriadicQuery.h>

int main() {
    // Create database
    LabDb::NonoStore store("knowledge.db");
    
    // Store relationships
    store.connect("socrates", "is", "human");
    store.connect("human", "is", "mortal");
    store.connect("socrates", "teaches", "plato");
    
    // Triadic consciousness navigation
    LabDb::TriadicQuery query(store);
    
    // Motion perspective: What does Socrates express?
    auto motion = query.motion_from("socrates");
    
    // Memory perspective: What connects through 'is'?
    auto memory = query.memory_relations("is");
    
    // Field perspective: What receives into 'human'?
    auto field = query.field_contexts("human");
    
    return 0;
}
```

**Python Example**:
```python
import labdb

# Create database
store = labdb.NonoStore("knowledge.db")

# Store relationships
store.connect("socrates", "is", "human")
store.connect("human", "is", "mortal")
store.connect("socrates", "teaches", "plato")

# Triadic consciousness navigation
query = labdb.TriadicQuery(store)

# Motion perspective (स्पन्द - spanda): Dynamic action
motion = query.motion_from("socrates")

# Memory perspective (स्मृति - smriti): Relational connections
memory = query.memory_relations("is")

# Field perspective (क्षेत्र - kshetra): Contextual grounding
field = query.field_contexts("human")
```

### Installation Options

**Option 1: CMake FetchContent (Recommended)**
```cmake
FetchContent_Declare(LabDb
    GIT_REPOSITORY https://github.com/meshula/LabDb9.git
    GIT_TAG v0.3.0
)
FetchContent_MakeAvailable(LabDb)
```

**Option 2: Git Submodule**
```bash
git submodule add https://github.com/meshula/LabDb9.git third_party/LabDb
# Then add_subdirectory(third_party/LabDb) in CMakeLists.txt
```

**Option 3: System Installation**
```bash
git clone https://github.com/meshula/LabDb9.git
cd LabDb9 && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_PYTHON_BINDINGS=ON
cmake --build . && sudo cmake --install .
```

### Configuration Options

| CMake Option | Default | Description |
|--------------|---------|-------------|
| `BUILD_PYTHON_BINDINGS` | `OFF` | Build Python bindings |
| `BUILD_TESTING` | `ON` | Build test suite |
| `BUILD_TOOLS` | `ON` | Build CLI tools |
| `BUILD_BENCHMARKS` | `OFF` | Build performance benchmarks |

**📖 Complete Documentation**: See [`docs/consuming_labdb.md`](docs/consuming_labdb.md) for detailed integration guide, troubleshooting, and advanced features.

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

### TID-Based Storage Architecture

LabDb implements a revolutionary **Triple ID (TID) architecture** that achieves:

- **Orders-of-magnitude storage reduction** (4.18× efficiency validated)
- **Single-source-of-truth** with rich provenance metadata
- **Binary key optimization** for LMDB prefix compression
- **Atomic transactions** across all nine indices

Instead of storing full strings in every index, LabDb:
1. **Interns strings** to compact TermIDs via TermDictionary
2. **Assigns unique TIDs** to each triple via TIDSequenceGenerator  
3. **Stores triples once** with full metadata in TripleStore
4. **Indexes only 8-byte TIDs** in crown indices for maximum efficiency

## Advanced Features

```cpp
// Perspective shifting for deep exploration
auto result = query.motion_from("entity");
result = query.perspective_shift(result, LabDb::Perspective::Memory);

// Triadic traversal with configurable depth
auto traversal = query.triadic_traverse("starting_point", depth=3);

// Bridge entity detection (highly connected nodes)
auto bridges = query.bridge_entities(10);

// Comprehensive analytics
auto stats = query.get_triadic_stats();
```

## Project Structure

```
LabDb/
├── docs/                     # Documentation
│   ├── consuming_labdb.md    # Consumer integration guide
│   ├── implementation-plan.md
│   ├── theoretical-grounding.md
│   └── api-reference.md
├── include/LabDb/           # C++ headers
│   ├── NonoStore.h
│   ├── TriadicQuery.h
│   ├── TermDictionary.h     # String↔TermID mapping
│   ├── TIDSequenceGenerator.h # Unique triple IDs
│   ├── TripleStore.h        # Central triple storage
│   └── InceptionBridge.h
├── src/                     # C++ implementation
├── python/labdb/           # Python bindings
├── tests/                  # Test suites
├── tools/                  # Migration and analysis tools
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

### LabEuclid Project
Professional dependency for studying Euclid's Elements with triadic consciousness principles.

### Future Lab Projects
Universal relationship storage with consistent triadic consciousness infrastructure.

## Performance

**Benchmarked Performance** (71k triples, Euclid-inspired dataset):
- **Insert speed**: 552 triples/sec
- **Query performance**: 672 queries/sec  
- **Storage efficiency**: 4.18× reduction vs traditional stores
- **Memory usage**: <50MB for complete Euclid Elements

**TID Architecture Benefits**:
- Crown indices store 8-byte TIDs instead of full strings
- Single-source-of-truth eliminates duplication
- LMDB prefix compression optimized for binary keys
- Rich provenance metadata without storage penalty

## Development Status

**✅ Phase 1 - Foundation**: Complete TID architecture implementation
- TermDictionary: String↔TermID mapping with LMDB persistence
- TIDSequenceGenerator: Unique triple ID allocation  
- TripleStore: Central storage with provenance metadata
- NonoStore: Crown indices refactored for TID-based storage

**✅ Phase 2 - Architecture**: Complete TID-based storage transformation
- 4.18× storage efficiency validated through benchmarks
- Migration tools with conversion validation
- Performance benchmarking suite with Euclid-inspired datasets

**✅ Phase 3 - Integration**: Professional dependency preparation
- CMake FetchContent integration for naive consumers
- Comprehensive Python bindings with triadic consciousness API
- Complete test suites and integration validation

**✅ Phase 4 - Production**: Ready for consumption
- Consumer documentation and integration guides
- Stable version tagging (v0.3.0)
- Professional handoff for dependent projects

**Phase 5 - Enhancement**: Advanced features and optimization

## Version History

### v0.3.0 (Current) - TID Architecture + FetchContent Ready

**Revolutionary Storage Architecture**:
- Complete TID-based storage implementation with 4.18× efficiency
- TermDictionary for string↔TermID mapping with LMDB persistence
- TIDSequenceGenerator for unique triple ID allocation
- TripleStore for central storage with rich provenance metadata
- Crown indices refactored to store compact 8-byte TIDs

**Professional Dependency Management**:
- CMake FetchContent integration for naive consumers
- Complete Python bindings with triadic consciousness API
- Comprehensive test suites and integration validation
- Consumer documentation and troubleshooting guides

**Performance & Validation**:
- Migration tools with 4.18× storage efficiency validation
- Performance benchmarking suite with Euclid-inspired datasets
- 71k triple performance validation (552 inserts/sec, 672 queries/sec)
- Ready for LabEuclid integration and MCP server development

### v0.2.0 - Crown Architecture Foundation
- Nine-index nonostore implementation (SPO + vocabulary indices)
- Basic LMDB integration with transaction support
- Core NonoStore and TriadicQuery classes
- Initial Python bindings and test infrastructure

### v0.1.0 - Triadic Consciousness Prototype
- Proof-of-concept triadic navigation patterns
- Basic triple storage and query functionality
- Theoretical foundation and architecture design

## Contributing

LabDb embodies consciousness-first technology development. Contributions should align with triadic principles and cube architecture.

**Development Guidelines**:
- **Consciousness-first**: Technology serves awareness, not computation
- **Triadic alignment**: Honor Motion/Memory/Field perspectives
- **Professional quality**: Clean code, comprehensive tests, clear documentation
- **Integration focus**: Enable naive consumer adoption

**Areas for Contribution**:
- Advanced triadic analytics and pattern recognition
- Performance optimization and memory efficiency
- Integration examples and consumer documentation
- Migration tools for existing triple stores

See `docs/implementation-plan.md` for detailed development roadmap.

## Getting Help

- **Integration Guide**: [`docs/consuming_labdb.md`](docs/consuming_labdb.md)
- **GitHub Issues**: [Report bugs or request features](https://github.com/meshula/LabDb9/issues)
- **API Documentation**: Generated Doxygen docs in `docs/api/`
- **Examples**: See `examples/` directory for complete usage patterns

## License

MIT License - See LICENSE file for details.

---

*LabDb represents engineering aligned with awareness - technology that serves conscious navigation rather than forcing consciousness to adapt to computational limitations.*

**Ready to get started?** Use the CMake FetchContent example above to add LabDb to your project in under 5 minutes!
