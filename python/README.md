# LabDb Clean MCP Server

**Minimal C-based Python extension providing direct S-expression interface to LabDb's triadic consciousness database.**

## Architecture

**Clean & Simple**: No pybind11, no Python abstractions, no complex wrappers.

```
C++ LabDb ←→ C Python Extension ←→ FastMCP2 ←→ Claude Desktop
```

## Build & Run

```bash
# 1. Clean build from scratch
cd /Users/nporcino/dev/Lab/LabDb
rm -rf build && mkdir build && cd build

# 2. Configure with MCP and Python settings
cmake .. -DLABDB_MCP=ON -DLABDB_PYDIR=/path/to/python -DLABDB_PYEXE=/path/to/python3

# 3. Build (C extension + MCP server)
make -j8

# 4. Install Python extension and MCP server
make install

# 5. Run MCP server
PYTHONPATH=$PYDIR $PYEXE labdb-mcp-server.py
```

## Python API

**Just 4 functions, no complexity:**

```python
import labdb

# Execute single S-expression command
result = labdb.execute_db9_command('(stats)')

# Execute multiple commands
results = labdb.execute_db9_commands(['(open-database test.db9)', '(stats)'])

# Get complete specification
spec = labdb.get_db9_specification()

# Get available verbs
verbs = labdb.get_db9_available_verbs()
```

## MCP Server

**Just 2 tools, maximum simplicity:**

- `db9(commands)` - Execute S-expression commands
- `db9_readme()` - Get specification

## What We Eliminated

- ❌ pybind11 template complexity (500+ lines)
- ❌ Python abstraction layers (800+ lines) 
- ❌ Mock interfaces and fallbacks (400+ lines)
- ❌ Enhanced wrapper classes (600+ lines)
- ❌ Complex import/loading logic (200+ lines)
- ❌ Streaming billion-scale interfaces (400+ lines)

**Total eliminated: ~3000 lines of complexity**

## What We Kept

- ✅ Direct C++ S-expression interface
- ✅ FastMCP2 server framework  
- ✅ Proven C++ Phase 4.1 capabilities
- ✅ Simple build system integration

**Total implementation: ~200 lines**

## Philosophy

**Consciousness-first technology**: Build exactly what's needed, eliminate everything else. The complex Python abstractions served their purpose during development but became impediments to production deployment.

**Direct path**: S-expressions go straight from Claude → MCP → C extension → C++ → database. No interpretation layers, no abstraction complexity.
