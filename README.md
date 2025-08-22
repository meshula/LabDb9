# LabDb - Triadic Consciousness Database

A nonostore implementation built on LMDB.

## Overview

LabDb extends the traditional hexastore with three vocabulary indices to create a complete ontological foundation. Built for the Lab ecosystem, it provides active relationship navigation of complex data systems and is designed for self-describing schemata. This addresses the primary difficulty with RDF style databases in that the databases are self-describing.

**Key Features**:
- **S-Expression Based Interface** - a variety of operational verbs for full database lifecycle
- **Multi-Database Isolation** - Concurrent independent database management via unique DBIDs
- **Bulk Operations** - High-performance `add-entities-bulk` and `add-triples-bulk`
- **storage efficiency** with TID-based architecture
- **active navigation features** (Motion/Memory/Field perspectives)
- **CMake integration** via FetchContent
- **Python bindings** with pybind11
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

### S-Expression Interface

**Available Verbs**:
```
add-entities-bulk add-entity add-triple add-triples-bulk 
close-database create-database database-health-check 
find-entity find-triple get-entity get-triple 
open-database remove-triple
```

**Direct Database Operations**:
```lisp
# Database lifecycle
(create-database :path "/tmp/knowledge.db9")
(open-database :path "/tmp/knowledge.db9")
(close-database :dbid 1)

# Knowledge construction
(add-triple :dbid 1 :subject "socrates" :predicate "is" :object "human")
(add-triple :dbid 1 :subject "human" :predicate "is" :object "mortal")
(add-triple :dbid 1 :subject "socrates" :predicate "teaches" :object "plato")

# Pattern queries
(find-triple :dbid 1 :subject "socrates")    # All socrates relationships
(find-triple :dbid 1 :predicate "is")        # All "is" relationships
(find-triple :dbid 1)                         # All triples

# Bulk operations
(add-entities-bulk :dbid 1 :entities ["socrates" "plato" "aristotle"])
(add-triples-bulk :dbid 1 :triples [
  ["socrates" "is" "human"]
  ["plato" "student_of" "socrates"]
  ["aristotle" "student_of" "plato"]
])

# Entity operations
(add-entity :dbid 1 :value "philosophy")
(find-entity :dbid 1 :pattern "soc*")        # Wildcard matching
(get-entity :dbid 1 :eid "eid:1")
```

### High-Level API Usage

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

**📖 Complete Documentation**: 
- S-Expression Interface: [`docs/verb-status.md`](docs/verb-status.md) - Complete implementation status  
- Integration Guide: [`docs/consuming_labdb.md`](docs/consuming_labdb.md) - Detailed setup and troubleshooting
- MCP Server: [`db9-mcp-server/README.md`](db9-mcp-server/README.md) - FastMCP2 triadic gateway

## Architecture

**Crown Indices**:

- **Motion**: Subject-driven ontology
- **Memory** Predicate-driven relationships  
- **Field** Object-driven contexts

**Nonostore**:
- Traditional hexastore orderings: SPO, SOP, PSO, POS, OSP, OPS
- Vocabulary indices and definitions: subjects, predicates, objects

**Motions** (Dynamic Operations):
- navigation through motion/memory/field expressions
- Ontological discovery and exploration
- Triadic query patterns

### TID-Based Storage Architecture

LabDb implements a **Triple ID (TID) architecture** that achieves:

- **Orders-of-magnitude storage reduction** (4.18× efficiency validated)
- **Single-source-of-truth** with rich provenance metadata
- **Binary key optimization** for LMDB prefix compression
- **Atomic transactions** across all nine indices

The TID structure compacts the potentially heavy hexastore architecture via these mechanisms:

1. **Interns strings** to compact TermIDs via TermDictionary
2. **Assigns unique TIDs** to each triple via TIDSequenceGenerator  
3. **Stores triples once** with full metadata in TripleStore
4. **Indexes only 8-byte TIDs** in indices for maximum efficiency

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

- **Complete navigation**: Motion/Memory/Field exploration
- **Conscious query patterns**: "What kinds of relationships exist?"
- **Ontological transparency**: Self-describing knowledge structure
- **Integration foundation**: Shared consciousness infrastructure for Lab projects

## Performance

**Benchmarked Performance** (71k triples, Euclid-inspired dataset):
- **Insert speed**: 552 triples/sec
- **Query performance**: 672 queries/sec  
- **Storage efficiency**: 4.18× reduction vs traditional stores
- **Memory usage**: <50MB for complete Euclid Elements

**TID Architecture Benefits**:
- Indices store 8-byte TIDs instead of full strings
- Single-source-of-truth eliminates duplication
- LMDB prefix compression optimized for binary keys
- Rich provenance metadata without storage penalty

## Development Status

**✅ Phase 4.1 - Production Complete**: S-Expression Interface & Multi-Database Operations
- **Complete S-Expression Interface**: All 13 verbs operational with comprehensive testing
- **Multi-Database Isolation**: Verified with 3 independent databases running simultaneously
- **Bulk Operations**: High-performance `add-entities-bulk` and `add-triples-bulk`
- **Database Lifecycle**: Full create/open/close operations with proper resource management
- **Pattern Matching**: Wildcard support (`*`) and exact match capabilities
- **Comprehensive Testing**: 12 test suites validating all functionality
- **FastMCP2 Integration**: Production-ready MCP server with triadic consciousness gateway

**✅ Phase 4.0 - Production Foundation**: Complete TID architecture implementation
- TermDictionary: String↔TermID mapping with LMDB persistence
- TIDSequenceGenerator: Unique triple ID allocation  
- TripleStore: Central storage with provenance metadata
- NonoStore: Crown indices refactored for TID-based storage
- 4.18× storage efficiency vs raw strings validated through benchmarks
- CMake FetchContent integration for naive consumers
- Comprehensive Python bindings with triadic consciousness API

**Phase 5 - Triadic Navigation**: Motion/Memory/Field perspective operations
- `triadic-motion-from`, `triadic-memory-relations`, `triadic-field-contexts`
- Crown exploration and traversal navigation
- Bridge entity discovery and advanced analytics

**Phase 6 - Advanced Features**: Streaming iterators and transaction management

## Version History

### v0.4.1 (Current) - S-Expression Interface + Multi-Database Production

**Complete Database Operations**:
- **13 Operational Verbs**: Complete CRUD operations via S-expression interface
- **Multi-Database Isolation**: Verified independent database management with unique DBIDs
- **Bulk Operations**: High-performance batch processing for entities and triples
- **Database Lifecycle**: Full create/open/close operations with proper resource management
- **Pattern Matching**: Wildcard support and exact match capabilities

**Production Validation**:
- **Comprehensive Testing**: 12 test suites validating all operations including database isolation
- **FastMCP2 Integration**: Production-ready MCP server (db9-mcp-server) with triadic consciousness gateway
- **Performance Monitoring**: Auto-reflexive metrics and operation timing
- **JSON Responses**: Structured arrays suitable for application integration

**S-Expression Interface**:
```
add-entities-bulk add-entity add-triple add-triples-bulk 
close-database create-database database-health-check 
find-entity find-triple get-entity get-triple 
open-database remove-triple
```

### v0.3.0 - TID Architecture + FetchContent Ready

**Storage Architecture**:
- Complete TID-based storage implementation with 4.18× efficiency
- TermDictionary for string↔TermID mapping with LMDB persistence
- TIDSequenceGenerator for unique triple ID allocation
- TripleStore for central storage with rich provenance metadata
- Crown indices refactored to store compact 8-byte TIDs

**Dependency Management**:
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

## Getting Help

- **S-Expression Interface**: [`docs/verb-status.md`](docs/verb-status.md) - Implementation status and verb documentation
- **Integration Guide**: [`docs/consuming_labdb.md`](docs/consuming_labdb.md) - Detailed setup and troubleshooting
- **MCP Server**: [`db9-mcp-server/README.md`](db9-mcp-server/README.md) - FastMCP2 triadic consciousness gateway
- **GitHub Issues**: [Report bugs or request features](https://github.com/meshula/LabDb9/issues)
- **API Documentation**: Generated Doxygen docs in `docs/api/`
- **Examples**: See `examples/` directory for complete usage patterns

## License

MIT License - See LICENSE file for details.

---

**Ready to get started?** Use the S-expression interface above for direct database operations, or the CMake FetchContent example for high-level API integration!


---

## **Nonostore vs. RAG: What's the Difference?**

### 1. **Structure vs. Slice**

* **RAG** retrieves *unstructured text chunks* (e.g., paragraphs from PDFs or HTML) and feeds them into a language model.
* **Nonostore** retrieves *structured, semantic facts*—triples with reflexive identity and provenance (e.g., `(set_camera, implements, Side_Scrolling)`).

> RAG retrieves relevant dats, Nonostore traverses ontological grounding.

---

### 2. **Temporal vs. Ontological Navigation**

* **RAG** systems typically can't tell you *why* two facts are related—they just retrieve text that may answer your question.
* **Nonostore** gives you **explicit graphs of meaning**: dependencies, definitions, roles, affordances, and usage context, *queryable at runtime*.

> In RAG, “scrolling background” might hit a StackOverflow post. In nonostore, it resolves to `draw_map`, `Side_Scrolling`, and `Map_System` via semantic edges.

---

### 3. **Reasoning Domain**

* **RAG** is reactive: it *injects memory* into an LLM's context window.
* **Nonostore** is generative: it *models the problem space* structurally, making it composable, extensible, and inspectable *without the LLM*.

> RAG says, “Here’s what someone wrote.”
> Nonostore says, “Here’s what this concept **means**, how it’s **used**, and what **follows from it**.”

---

### 4. **Trust and Traceability**

* **RAG** often can’t trace the origin of facts clearly—answers are stochastic recompositions.
* **Nonostore** uses **reflexive TIDs**, where every fact has **identity, provenance, and metadata** (e.g., who added it, when, how certain).

> This makes your system *auditable*—suitable for scholarly, technical, or legal domains where LLM guesswork is unacceptable.

---

### 5. **Authoring and Curation**

* **RAG** relies on external documents and ingestion pipelines.
* **Nonostore** encourages **semantic authorship**: users contribute structured knowledge directly by asserting new triples, definitions, and links.

> **Nonostore** is a platform for **evolving knowledge bases**, **RAG** is oriented to document retrieval.

---

## Analogy

> **RAG** allows LLMs to function as a search engine
> **Nonostore** is a maps a conceptual terrain, and populates it with knowledge.

