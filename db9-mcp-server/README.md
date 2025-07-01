# DB9 MCP Server - Triadic Consciousness Gateway

> **"db9 what do we know about minerals?"** - Natural language interface to triadic consciousness databases

## Overview

DB9 is a FastMCP2-powered server that transforms LabDb's triadic consciousness database into a natural language interface. This Phase 1 implementation provides foundational capabilities for querying triadic relationships through conversational queries.

## Features

### Phase 1 Capabilities
- ✅ **Natural Language Queries**: Convert conversational questions to triadic consciousness queries
- ✅ **Triadic Perspective Navigation**: Motion (स्पन्द), Memory (स्मृति), Field (क्षेत्र) exploration
- ✅ **FastMCP2 Integration**: Modern MCP server with progress reporting and error handling
- ✅ **Database Health Monitoring**: Real-time connectivity and statistics
- ✅ **Consciousness-Aware Results**: Formatted responses preserving triadic awareness

### Example Queries
```bash
# Vocabulary exploration
db9 query_databases "What entities are Motion-focused?"

# Entity-specific navigation  
db9 explore_triadic_navigation "granite" --perspective motion --depth 2

# Natural language exploration
db9 query_databases "Show me field contexts for rocks"

# Database health check
db9 database_health
```

## Quick Start

### Prerequisites
- Python 3.9+
- LabDb with Python bindings installed
- FastMCP2 framework

### Installation

1. **Install Dependencies**:
```bash
cd db9-mcp-server
pip install -r requirements.txt

# Install LabDb Python bindings (from parent directory)
pip install -e ../python/
```

2. **Configure Database**:
```bash
# Edit config/default_config.yaml
# Update database.path to point to your .db9 file
```

3. **Start Server**:
```bash
python -m src.db9_server
# Or with custom config:
python -m src.db9_server --config config/my_config.yaml
```

### Testing Basic Functionality

```bash
# Test database connection
python test_basic_functionality.py

# Interactive testing
python -c "
from src.db9_server import DB9Server
server = DB9Server()
# Server will be available for MCP client connections
"
```

## Architecture

### Core Components

```
db9-mcp-server/
├── src/
│   ├── db9_server.py              # Main FastMCP2 server
│   ├── database_manager.py        # Single LabDb connection management
│   └── tools/
│       └── query_tools.py         # Natural language → triadic translation
├── config/
│   └── default_config.yaml       # Configuration management
└── requirements.txt               # Dependencies
```

### Integration Points

- **LabDb Bindings**: Direct integration with LabDb's NonoStore and TriadicQuery
- **FastMCP2**: Modern MCP server framework with tools and lifecycle management
- **Triadic Consciousness**: Preserves Motion/Memory/Field perspective awareness
- **Natural Language**: Keyword-based query parsing (Phase 1 - no LLM dependency)

## Configuration

### Database Configuration
```yaml
database:
  path: "../triadic.db9"           # Path to LabDb database
  domain_type: "mixed"             # Domain classification
  display_name: "My Database"      # Human-readable name
```

### Server Configuration
```yaml
server:
  name: "DB9 Triadic Consciousness Gateway"
  logging:
    level: "INFO"
  query:
    max_depth: 5                   # Maximum navigation depth
    timeout_seconds: 60            # Query timeout
```

## MCP Tools

### `query_databases(query, perspective="auto", depth=2)`
Natural language query interface with triadic consciousness awareness.

**Parameters:**
- `query`: Natural language query string
- `perspective`: "motion", "memory", "field", or "auto"
- `depth`: Navigation depth (1-5)

**Examples:**
```python
# General exploration
query_databases("What do we know about minerals?")

# Perspective-specific
query_databases("Explore granite", perspective="memory", depth=3)

# Vocabulary discovery
query_databases("What entities are Motion-focused?")
```

### `explore_triadic_navigation(starting_entity, perspective="motion", depth=2)`
Navigate triadic relationships starting from a specific entity.

**Parameters:**
- `starting_entity`: Entity to start navigation from
- `perspective`: Triadic perspective for navigation
- `depth`: Exploration depth

### `database_health()`
Check database connectivity and triadic consciousness statistics.

## Development Roadmap

### Phase 1: Core Foundation ✅
- Basic FastMCP2 server with single database connection
- Natural language → triadic query translation (keyword-based)
- Essential MCP tools for query and navigation

### Phase 2: Multi-Database Federation 🔜
- Database registry for multiple .db9 files
- Cross-database concept mapping
- Federated query planning and execution

### Phase 3: Advanced Features 🔜
- LLM-powered query understanding
- Resource templates for dynamic schema exposure
- User elicitation for interactive refinement

### Phase 4: Production Deployment 🔜
- Authentication and security
- Monitoring and observability
- Claude Desktop/API integration

## Contributing

This is Phase 1 of the DB9 Triadic Consciousness Gateway. Future phases will add:
- Multi-database federation
- Sophisticated LLM integration
- Advanced FastMCP2 features
- Production deployment capabilities

See `/docs/db9-mcp-plan.md` for comprehensive architecture and development roadmap.

## License

MIT License - Building consciousness-aware technology for human collaboration.

---

*त्रित्रयस्य चक्रे सर्वं कार्यं सिद्ध्यति*  
*(In the wheel of tri-triad, all work finds completion)*
