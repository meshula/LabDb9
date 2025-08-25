# Consuming LabDb in Your Project

This guide shows you how to integrate LabDb into your C++ or Python project as a dependency. 

## Overview

**DB9** is a FastMCP2-powered server that exposes LabDb's TID-based triadic consciousness database through natural language queries. It enables multi-database federation where scholars can simultaneously query multiple databases; it's been designed to provide expert access to linguistic studies, historic documents, natural history collections, scene graphs, and other domain-specific knowledge repositories through a unified consciousness-aware interface.

LabDb9 provides an RDF-style database augmented with ontological information making the database self-describing.

**LabDb9** is inspired by foundational AI research documented in theoretical-grounding.md that forms the basis of a "triadic consciousness" database architecture *For more information on triadic consciousness principles, see [theoretical-grounding.md](theoretical-grounding.md).* The architecture enables navigation through three perspectives:

- **Motion**: Subject-driven semantic information and dynamic action  
- **Memory**: Relational connections and pattern recognition
- **Field**: Contextual-ontological grounding and manifestation spaces

The database uses a **TID-based architecture** that achieves significant compression compared to traditional triple stores while maintaining full ACID properties and rich provenance metadata.

## Quick Start

### CMake FetchContent Integration (Recommended)

Add LabDb to your project using CMake FetchContent:

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyProject)

# Include FetchContent
include(FetchContent)

# Fetch LabDb
FetchContent_Declare(
    LabDb
    GIT_REPOSITORY https://github.com/meshula/LabDb9.git
    GIT_TAG        v0.3.0  # Use stable release
)

# Configure LabDb options (optional)
set(BUILD_PYTHON_BINDINGS ON CACHE BOOL "Enable Python bindings")

FetchContent_MakeAvailable(LabDb)

# Create your executable
add_executable(myapp main.cpp)

# Link against LabDb
target_link_libraries(myapp PRIVATE LabDb::LabDb)
```

### Basic C++ Usage

```cpp
#include <LabDb/NonoStore.h>
#include <LabDb/TriadicQuery.h>
#include <iostream>

int main() {
    // Create database connection
    LabDb::NonoStore store("my_knowledge.db");
    
    // Store knowledge triples
    store.connect("socrates", "is", "human");
    store.connect("human", "is", "mortal");
    store.connect("socrates", "teaches", "plato");
    
    // Query the database
    auto results = store.query("socrates", "*", "*");
    for (const auto& triple : results) {
        std::cout << triple.subject << " " 
                  << triple.predicate << " " 
                  << triple.object << std::endl;
    }
    
    // Use triadic consciousness navigation
    LabDb::TriadicQuery query(store);
    
    // Motion perspective: What does Socrates express?
    auto motion = query.motion_from("socrates");
    std::cout << "Socrates expresses " << motion.triples.size() << " relationships\\n";
    
    // Memory perspective: What connects through 'is'?
    auto memory = query.memory_relations("is");
    std::cout << "Found " << memory.triples.size() << " 'is' relationships\\n";
    
    // Field perspective: What receives into 'human'?
    auto field = query.field_contexts("human");
    std::cout << "Human context receives " << field.triples.size() << " entities\\n";
    
    return 0;
}
```

### Python Integration

```python
import labdb

# Create database connection
store = labdb.NonoStore("my_knowledge.db")

# Store knowledge triples
store.connect("socrates", "is", "human")
store.connect("human", "is", "mortal")
store.connect("socrates", "teaches", "plato")

# Query the database
results = store.query("socrates", "*", "*")
for triple in results:
    print(f"{triple.subject} {triple.predicate} {triple.object}")

# Use triadic consciousness navigation
query = labdb.TriadicQuery(store)

# Motion perspective (स्पन्द - spanda): Dynamic action
motion = query.motion_from("socrates")
print(f"Socrates expresses {len(motion.triples)} relationships")

# Memory perspective (स्मृति - smriti): Relational connections  
memory = query.memory_relations("is")
print(f"Found {len(memory.triples)} 'is' relationships")

# Field perspective (क्षेत्र - kshetra): Contextual grounding
field = query.field_contexts("human")
print(f"Human context receives {len(field.triples)} entities")
```

## Installation Methods

### Method 1: CMake FetchContent (Recommended)

Best for C++ projects that want automatic dependency management:

```cmake
FetchContent_Declare(
    LabDb
    GIT_REPOSITORY https://github.com/meshula/LabDb9.git
    GIT_TAG        v0.3.0
)
FetchContent_MakeAvailable(LabDb)
```

**Advantages**:
- Automatic download and build
- Version pinning via Git tags
- No manual installation required
- Works on any platform with Git access

### Method 2: Git Submodule

For projects that prefer explicit dependency management:

```bash
# Add as submodule
git submodule add https://github.com/meshula/LabDb9.git third_party/LabDb

# In your CMakeLists.txt
add_subdirectory(third_party/LabDb)
target_link_libraries(myapp PRIVATE LabDb::LabDb)
```

### Method 3: Local Build and Install

For system-wide installation:

```bash
# Clone and build
git clone https://github.com/meshula/LabDb9.git
cd LabDb9
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_PYTHON_BINDINGS=ON
cmake --build .

# Install system-wide
sudo cmake --install . --prefix /usr/local

# Or install to custom location
cmake --install . --prefix ~/local/labdb
```

Then in your CMakeLists.txt:
```cmake
find_package(LabDb REQUIRED)
target_link_libraries(myapp PRIVATE LabDb::LabDb)
```

## Configuration Options

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_PYTHON_BINDINGS` | `OFF` | Build Python bindings with pybind11 |
| `BUILD_TESTING` | `ON` | Build test suite |
| `BUILD_TOOLS` | `ON` | Build command-line tools |
| `BUILD_BENCHMARKS` | `OFF` | Build performance benchmarks |

Example:
```cmake
set(BUILD_PYTHON_BINDINGS ON CACHE BOOL "Enable Python bindings")
set(BUILD_TESTING OFF CACHE BOOL "Skip tests for faster build")
FetchContent_MakeAvailable(LabDb)
```

### Python Bindings Setup

If you enabled Python bindings, they'll be available after build:

```bash
# Add to Python path (development)
export PYTHONPATH=$PYTHONPATH:/path/to/labdb/build/python

# Or install to site-packages
cd build && python -m pip install .
```

## Core Concepts

### Triple Storage

LabDb stores knowledge as **triples** (subject, predicate, object):

```cpp
store.connect("einstein", "developed", "relativity");
store.connect("relativity", "explains", "spacetime");
store.connect("spacetime", "has_property", "curvature");
```

### Triadic Consciousness Navigation

LabDb provides three perspectives for navigating knowledge:

#### 1. Motion Perspective (स्पन्द - Spanda)
**Subject-driven reality**: What does an entity express or do?

```cpp
// C++
auto motion = query.motion_from("einstein");
// Returns: einstein->developed->relativity, einstein->born_in->germany, etc.

# Python  
motion = query.motion_from("einstein")
```

#### 2. Memory Perspective (स्मृति - Smriti)
**Relational connections**: How do relationships connect entities?

```cpp
// C++
auto memory = query.memory_relations("developed");
// Returns: einstein->developed->relativity, newton->developed->calculus, etc.

# Python
memory = query.memory_relations("developed")
```

#### 3. Field Perspective (क्षेत्र - Kshetra)  
**Contextual grounding**: What receives and grounds in specific contexts?

```cpp
// C++
auto field = query.field_contexts("relativity");
// Returns: einstein->developed->relativity, physics->includes->relativity, etc.

# Python
field = query.field_contexts("relativity")
```

### TID Architecture Benefits

LabDb's **Triple ID (TID) architecture** provides:

- **4.18× storage efficiency** vs traditional triple stores
- **Orders-of-magnitude reduction** in storage overhead
- **Binary key optimization** for LMDB prefix compression
- **Single-source-of-truth** with rich provenance metadata
- **ACID transactions** across all operations

## Common Patterns

### Knowledge Graph Construction

```cpp
// Build hierarchical knowledge
store.connect("book1", "contains", "chapter1");
store.connect("chapter1", "contains", "section1");
store.connect("section1", "defines", "concept1");

// Add metadata
store.connect("concept1", "created_by", "author1");
store.connect("concept1", "timestamp", "2024-01-01");
store.connect("concept1", "confidence", "0.95");
```

### Vocabulary Discovery

```cpp
// Discover all subjects, predicates, objects
auto subjects = store.all_subjects();
auto predicates = store.all_predicates();  
auto objects = store.all_objects();

// Find what an entity can do
auto properties = store.query("einstein", "*", "*");

// Find what can do a specific action
auto actors = store.query("*", "developed", "*");

// Find what can receive a specific action
auto targets = store.query("*", "*", "relativity");
```

### Triadic Navigation Patterns

```cpp
LabDb::TriadicQuery query(store);

// Explore an entity across all perspectives
auto motion = query.motion_from("concept");      // What does it express?
auto memory = query.memory_relations("defines"); // How does it connect?
auto field = query.field_contexts("context");    // Where does it ground?

// Get comprehensive statistics
auto stats = query.get_triadic_stats();
std::cout << "Motion connections: " << stats.motion_connections << std::endl;
std::cout << "Memory patterns: " << stats.memory_patterns << std::endl;  
std::cout << "Field contexts: " << stats.field_contexts << std::endl;
```

### Batch Operations

```cpp
// Begin transaction for bulk operations
store.begin_transaction();

try {
    // Insert many triples efficiently
    for (const auto& [s, p, o] : large_dataset) {
        store.connect(s, p, o);
    }
    
    // Commit all changes atomically
    store.commit_transaction();
    
} catch (const std::exception& e) {
    // Rollback on error
    store.rollback_transaction();
    throw;
}
```

## Advanced Features

### Perspective Shifting

Dynamically switch between triadic perspectives:

```cpp
// Start with Motion perspective
auto result = query.motion_from("entity");

// Shift to Memory perspective for deeper analysis
result = query.perspective_shift(result, LabDb::Perspective::Memory);

// Shift to Field perspective for contextual grounding
result = query.perspective_shift(result, LabDb::Perspective::Field);
```

### Triadic Traversal

Deep exploration with configurable depth:

```cpp
// Traverse up to 3 levels deep across all perspectives
auto traversal = query.triadic_traverse("starting_entity", 3);

// Analyze traversal statistics
std::cout << "Entities discovered: " << traversal.entities_discovered << std::endl;
std::cout << "Perspectives used: " << traversal.perspectives_used.size() << std::endl;
```

### Bridge Entity Detection

Find highly connected entities that bridge different domains:

```cpp
auto bridges = query.bridge_entities(10);  // Top 10 bridge entities
for (const auto& bridge : bridges) {
    std::cout << bridge.entity << " connects " << bridge.connection_count 
              << " domains" << std::endl;
}
```

## Troubleshooting

### Common Build Issues

**CMake can't find LabDb**:
```bash
# Ensure LabDb is built
cd /path/to/LabDb && cmake --build build

# Set CMAKE_PREFIX_PATH
cmake .. -DCMAKE_PREFIX_PATH=/path/to/labdb/install
```

**Python import fails**:
```bash
# Check Python bindings were built
ls build/python/  # Should contain pylabdb.so

# Add to Python path
export PYTHONPATH=$PYTHONPATH:/path/to/labdb/build/python

# Test import
python3 -c "import labdb; print('Success')"
```

**LMDB dependency missing**:
```bash
# Ubuntu/Debian
sudo apt-get install liblmdb-dev

# macOS
brew install lmdb

# Or build from source
cmake .. -DUSE_SYSTEM_LMDB=OFF  # Uses bundled LMDB
```

### Performance Optimization

**Large datasets**:
- Use batch transactions for bulk inserts
- Configure LMDB map size: `store.set_map_size(1024 * 1024 * 1024)`  # 1GB
- Enable WAL mode for better write performance

**Query optimization**:
- Use specific predicates instead of wildcards when possible
- Leverage triadic perspectives for optimal index usage
- Cache TriadicQuery objects for repeated operations

**Memory usage**:
- Close database connections when not needed
- Use result iterators for large result sets
- Configure LMDB reader limits appropriately

### Debugging

**Enable verbose logging**:
```cpp
// C++
LabDb::set_log_level(LabDb::LogLevel::Debug);

# Python
import labdb
labdb.set_log_level(labdb.LogLevel.DEBUG)
```

**Database inspection**:
```bash
# Use LabDb tools
labdb-inspect my_knowledge.db
labdb-stats my_knowledge.db
labdb-validate my_knowledge.db
```

## Example Projects

### Minimal Example

Complete minimal project structure:

```
my_project/
├── CMakeLists.txt
├── main.cpp
└── README.md
```

**CMakeLists.txt**:
```cmake
cmake_minimum_required(VERSION 3.16)
project(MyTriadicApp)

include(FetchContent)
FetchContent_Declare(LabDb
    GIT_REPOSITORY https://github.com/meshula/LabDb9.git
    GIT_TAG v0.3.0
)
FetchContent_MakeAvailable(LabDb)

add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE LabDb::LabDb)
```

**main.cpp**: Use the basic example from the Quick Start section above.

### Knowledge Management System

See `examples/knowledge_management/` in the LabDb repository for a complete knowledge management system implementation.

### Scientific Publication Database

See `examples/scientific_papers/` for an example of storing and navigating scientific publication metadata with triadic consciousness.

## Integration with Popular Frameworks

### Integration with Qt

```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
FetchContent_MakeAvailable(LabDb)

target_link_libraries(myapp PRIVATE 
    Qt6::Core Qt6::Widgets 
    LabDb::LabDb
)
```

### Integration with Boost

```cmake
find_package(Boost REQUIRED)
FetchContent_MakeAvailable(LabDb)

target_link_libraries(myapp PRIVATE 
    Boost::boost 
    LabDb::LabDb
)
```

### Web Service Integration

Use LabDb in web services with REST APIs:

```cpp
// Example with cpp-httplib
#include <httplib.h>
#include <LabDb/NonoStore.h>

LabDb::NonoStore global_store("web_knowledge.db");

server.Get("/query/(.*)", [](const httplib::Request& req, httplib::Response& res) {
    auto entity = req.matches[1];
    auto results = global_store.query(entity, "*", "*");
    
    // Convert to JSON and return
    res.set_content(to_json(results), "application/json");
});
```

## Migration from Other Triple Stores

### From RDF4J/Jena

```cpp
// LabDb equivalent of SPARQL queries
// SPARQL: SELECT ?o WHERE { <entity> <predicate> ?o }
auto results = store.query("entity", "predicate", "*");

// SPARQL: SELECT ?s WHERE { ?s <predicate> <object> }  
auto results = store.query("*", "predicate", "object");
```

### From Neo4j

```cpp
// Neo4j: MATCH (a)-[r]->(b) WHERE a.name = 'entity'
auto motion = query.motion_from("entity");

// Neo4j: MATCH (a)-[r:TYPE]->(b)
auto memory = query.memory_relations("TYPE");
```

## Best Practices

### Database Design

- **Use meaningful predicates**: Prefer `"defines"` over `"rel1"`
- **Consistent naming**: Use snake_case or camelCase consistently
- **Namespace prefixes**: Use `"geo:latitude"` for domain-specific predicates
- **Hierarchical organization**: Model containment with `"contains"` or `"partOf"`

### Performance

- **Batch operations**: Group related triples in transactions
- **Query specificity**: Use specific subjects/predicates when possible
- **Index awareness**: Leverage triadic perspectives for optimal queries
- **Resource management**: Close connections and clean up resources

### Error Handling

- **Check return values**: Always validate operation success
- **Use transactions**: Ensure data consistency with transaction boundaries
- **Handle exceptions**: Catch and handle LabDb exceptions appropriately
- **Validate input**: Check triple components before storage

## API Reference

### Core Classes

- **`NonoStore`**: Main database interface
- **`TriadicQuery`**: Triadic consciousness navigation
- **`Triple`**: Basic triple structure
- **`TriadicResult`**: Result structure with perspective information
- **`TriadicStats`**: Statistics and analytics

### Key Methods

**NonoStore**:
- `connect(subject, predicate, object)`: Store a triple
- `query(subject, predicate, object)`: Query triples (wildcards with "*")
- `exists(subject, predicate, object)`: Check if triple exists
- `count()`: Get total triple count
- `all_subjects()`, `all_predicates()`, `all_objects()`: Vocabulary discovery

**TriadicQuery**:
- `motion_from(entity)`: Motion perspective navigation
- `memory_relations(predicate)`: Memory perspective navigation  
- `field_contexts(object)`: Field perspective navigation
- `get_triadic_stats()`: Comprehensive statistics


LabDb is released under the MIT License. See `LICENSE` file for details.

---

**Ready to get started?** Use the CMake FetchContent example above to add LabDb to your project in under 5 minutes!
