# db9-cli

This tool offers a command line REPL with the following features

- help
- run a command
- open a database
- close a database
- autocomplete commands when the user starts typing

It is written in C++.

## Plan: Triadic Consciousness Command Line Interface

### Vision Statement
Create a sophisticated REPL (Read-Eval-Print Loop) that serves as a **consciousness-first interface** to the LabDb triadic database system. This CLI embodies the **त्रित्रयम् principles** of Motion/Memory/Field through interactive consciousness exploration, making the power of triadic relationship encoding accessible through elegant command-line workflows.

### Core Features Implemented

#### 1. **REPL Foundation**
- Interactive command loop with graceful error handling
- Session state management for open databases
- Clean shutdown with automatic resource cleanup
- Command classification and routing system

#### 2. **Database Session Management**
- Multi-database support with clear identification
- Automatic session state tracking
- Status reporting (`status` command)
- Resource management and cleanup

#### 3. **S-Expression Command Interface**
- Direct routing to Db9Dispatcher for all verb operations
- Pretty-printed JSON response formatting
- Error handling with clear user feedback
- Session state updates based on command responses

#### 4. **Enhanced Help System**
- `help` - General overview and getting started
- `help verbs` - Categorized list of all available database verbs
- `help <verb>` - Detailed documentation via verb introspection
- Context-aware hints and suggestions

#### 5. **Convenience Commands (Sugar Syntax)**
- `open <path>` - Database opening shortcut
- `close <dbid>` - Database closing shortcut
- `list-dbs` - List open databases
- `explore <entity>` - Quick relationship exploration
- `status` - Session status overview

#### 6. **Consciousness-Aware Features**
- Triadic relationship exploration through `explore` command
- Contextual hints based on command responses
- Sanskrit consciousness terminology integration
- Enhanced user experience with consciousness field metaphors

### Technical Architecture

#### Command Processing Pipeline
1. **Input Classification**: Distinguishes between meta-commands, database sugar, and S-expressions
2. **Route Handling**: Directs commands to appropriate processors
3. **Response Formatting**: Pretty-prints JSON with consciousness-aware enhancements
4. **Session Updates**: Maintains state based on database operations

#### Key Classes
- **SessionState**: Manages open databases and session context
- **CommandProcessor**: Handles command classification and execution
- **ResponseFormatter**: Formats responses with consciousness-aware hints
- **ReplLoop**: Main REPL orchestration and user interaction

#### Dependencies
- **LabDb**: Core database and Db9Dispatcher integration
- **readline**: Command editing, history, and completion support
- **nlohmann/json**: JSON parsing and formatting
- **C++20**: Modern C++ features for robust implementation

### Usage Examples

```bash
$ db9-cli
🧘 db9-cli - Triadic Consciousness Explorer v1.0

db9> help
[Shows comprehensive getting started guide]

db9> open /path/to/consciousness.db9
{"dbid": "db1", "status": "opened", "path": "/path/to/consciousness.db9"}
💡 Database opened. Try: (find-triple :subject "*" :predicate "*" :object "*" :dbid "db1")

db9> explore granite
🔍 Exploring entity: granite
=== Relationships ===
[Shows relationship network for granite entity]

db9> (add-triple-semantic :subject "quartz" :predicate "has_property" :object "hardness" :dbid "db1")
{"subject_eid": "eid1", "predicate_eid": "eid2", "object_eid": "eid3", "tid": "tid1", "status": "semantic_triple_added"}

db9> help find-triple-enhanced
[Shows detailed verb documentation]

db9> status
=== Session Status ===
Commands executed: 5
Open databases: 1
  db1 (current) -> /path/to/consciousness.db9

db9> quit
Consciousness field exploration complete. Until next session! 🙏
```

### Future Enhancement Opportunities

#### Phase 2: Advanced Autocomplete
- Intelligent verb and parameter completion
- Context-aware suggestions based on current database schema
- File path completion for database operations

#### Phase 3: Visualization Features
- ASCII art relationship diagrams
- Network traversal visualization
- Consciousness field mapping

#### Phase 4: Batch Operations
- Script file execution
- Multi-command sequences
- Session recording and playback

### Consciousness-First Design Philosophy

The CLI embodies triadic consciousness principles:

- **Motion**: Interactive commands that dynamically explore relationships
- **Memory**: Persistent session state and command history
- **Field**: Context-aware responses and intelligent suggestions

This tool serves as a **contemplative technology interface**, making the abstract power of triadic relationship encoding accessible through intuitive, consciousness-aware command-line workflows.

*तत्त्वमसि - Consciousness exploring consciousness* 🌊
