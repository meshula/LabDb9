"""
DB9 MCP Server - Triadic Consciousness Gateway
Main FastMCP2 server implementation for Phase 1
"""

import asyncio
import logging
import sys
from pathlib import Path
from typing import List, Optional, Dict, Any

# Add paths for imports if running as script
current_dir = Path(__file__).parent
parent_dir = current_dir.parent
labdb_python_dir = parent_dir.parent / "python"

if str(parent_dir) not in sys.path:
    sys.path.insert(0, str(parent_dir))
if labdb_python_dir.exists() and str(labdb_python_dir) not in sys.path:
    sys.path.insert(0, str(labdb_python_dir))

import yaml
from pydantic import BaseModel

# FastMCP imports - updated for version 2.9.2
try:
    from fastmcp import FastMCP
    from fastmcp.tools import tool
    FASTMCP_AVAILABLE = True
    print("✅ FastMCP 2.9.2 available", file=sys.stderr)
except ImportError as e:
    logging.error("FastMCP not available - install with: pip install fastmcp")
    print(f"❌ FastMCP import error: {e}", file=sys.stderr)
    FASTMCP_AVAILABLE = False
    FastMCP = None
    tool = None

# Local imports - support both relative and absolute imports
try:
    from .database_manager import DatabaseManager
    from .tools.query_tools import QueryTools
    from .tools.add_triple_tool import AddTripleTool
except ImportError:
    # Fallback for when run as script
    from database_manager import DatabaseManager
    from tools.query_tools import QueryTools
    from tools.add_triple_tool import AddTripleTool


class DB9Server:
    """
    DB9 Triadic Consciousness Gateway
    
    FastMCP2-powered server that exposes LabDb's triadic consciousness database
    through natural language queries. Phase 1 focuses on single database connection
    with basic query tools.
    
    Features:
    - Natural language → triadic query translation
    - Motion/Memory/Field perspective navigation
    - Progress reporting for long-running queries
    - Consciousness-aware result formatting
    """
    
    def __init__(self, config_path: Optional[str] = None):
        # Load configuration
        if config_path is None:
            config_path = Path(__file__).parent.parent / "config" / "default_config.yaml"
        
        self.config = self._load_config(config_path)
        self.logger = self._setup_logging()
        
        if not FASTMCP_AVAILABLE:
            raise RuntimeError("FastMCP not available - cannot create server")
        
        # Initialize FastMCP2 app
        self.app = FastMCP(self.config["server"]["name"])
        
        # Initialize components
        print(f"DEBUG: Initializing DatabaseManager", file=sys.stderr)
        self.db_manager = DatabaseManager(self.config)
        print(f"DEBUG: DatabaseManager initialized successfully", file=sys.stderr)
        self.query_tools = QueryTools()
        print(f"DEBUG: QueryTools initialized successfully", file=sys.stderr)
        self.add_triple_tool = AddTripleTool(self.db_manager)
        print(f"DEBUG: AddTripleTool initialized successfully", file=sys.stderr)
        
        # Setup server
        self._setup_server()
        
        self.logger.info(f"DB9 Server initialized: {self.config['server']['name']}")
    
    def _load_config(self, config_path: Path) -> Dict[str, Any]:
        """Load YAML configuration file"""
        try:
            with open(config_path, 'r') as f:
                config = yaml.safe_load(f)
            return config
        except Exception as e:
            raise RuntimeError(f"Failed to load config from {config_path}: {e}")
    
    def _setup_logging(self) -> logging.Logger:
        """Setup logging configuration"""
        logging.basicConfig(
            level=getattr(logging, self.config["server"]["logging"]["level"]),
            format=self.config["server"]["logging"]["format"]
        )
        return logging.getLogger(__name__)
    
    def _setup_server(self):
        """Setup FastMCP2 server with tools"""
        # Register tools
        self._register_tools()
        
        # Note: FastMCP 2.9.2 doesn't have lifecycle hooks
        # Database initialization will happen on first query
        self.logger.info("DB9 Server setup complete")
    
    def _register_tools(self):
        """Register MCP tools with FastMCP2"""
        
        @self.app.tool
        async def open_database(db_path: str) -> str:
            """
            Explicitly connect to a triadic consciousness database at the specified path.
            
            Args:
                db_path: Full path to the database file (e.g., "/path/to/minerals-test.db9")
                
            Returns:
                Connection status message with database information
                
            Examples:
                - open_database("/Users/nporcino/dev/Lab/inception-mcp/minerals-test.db9")
                - open_database("./my-custom-database.db9")
            """
            try:
                resolved_path = Path(db_path).resolve()
                
                if not resolved_path.exists():
                    return f"❌ Database not found at: {resolved_path}"
                    
                # Use the server's database manager to switch database
                success = await self.db_manager.reconnect_to_database(str(resolved_path))
                
                if success:
                    # Get updated health info
                    health = await self.db_manager.health_check()
                    if health["status"] == "healthy":
                        stats = health["stats"]
                        return (f"🗄️ Connected to triadic consciousness database: {resolved_path}\n"
                               f"   Database: {health['display_name']} ({health['domain_type']})\n"
                               f"   Total triples: {stats['total_triples']}\n"
                               f"   Motion entities: {stats['motion_entities']}\n"
                               f"   Memory relations: {stats['memory_relations']}\n"
                               f"   Field contexts: {stats['field_contexts']}")
                    else:
                        return f"⚠️ Connected to database but health check failed: {health.get('error', 'Unknown error')}"
                else:
                    return f"❌ Failed to connect to database: {resolved_path}"
                    
            except Exception as e:
                self.logger.error(f"Failed to open database {db_path}: {e}")
                return f"❌ Failed to open database: {str(e)}"
        
        @self.app.tool
        async def create_database(db_path: str, initialize_with_test_data: bool = False) -> str:
            """
            Create a new triadic consciousness database at the specified path.
            
            Args:
                db_path: Full path where to create the database
                initialize_with_test_data: Whether to add basic test triples
                
            Returns:
                Creation status message
            """
            try:
                resolved_path = Path(db_path).resolve()
                
                if resolved_path.exists():
                    return f"❌ Database already exists at: {resolved_path}"
                    
                # Ensure parent directory exists
                resolved_path.parent.mkdir(parents=True, exist_ok=True)
                
                # Create the database using DatabaseManager
                success = await self.db_manager.create_new_database(str(resolved_path), initialize_with_test_data)
                
                if success:
                    # Optionally switch to the new database
                    await self.db_manager.reconnect_to_database(str(resolved_path))
                    
                    result = f"✅ Created triadic consciousness database: {resolved_path}"
                    
                    if initialize_with_test_data:
                        health = await self.db_manager.health_check()
                        if health["status"] == "healthy":
                            stats = health["stats"]
                            result += f"\n   Initialized with {stats['total_triples']} test triples"
                    
                    return result
                else:
                    return f"❌ Failed to create database at: {resolved_path}"
                    
            except Exception as e:
                self.logger.error(f"Failed to create database {db_path}: {e}")
                return f"❌ Failed to create database: {str(e)}"
        
        @self.app.tool
        async def query_databases(
            query: str,
            perspective: str = "auto",
            depth: int = 2
        ) -> str:
            """
            Natural language query across triadic consciousness database
            
            Args:
                query: Natural language query (e.g., "What do we know about minerals?")
                perspective: Triadic perspective - "motion", "memory", "field", or "auto"
                depth: Navigation depth for exploration (1-5)
                
            Returns:
                Formatted results preserving triadic consciousness awareness
                
            Examples:
                - query_databases("What entities are Motion-focused?")
                - query_databases("Explore granite", perspective="memory", depth=3)
                - query_databases("Show me field contexts for rocks")
            """
            
            # Parse natural language query
            print(f"DEBUG: query_databases called with query='{query}'", file=sys.stderr)
            query_hints = self.query_tools.parse_natural_query(query)
            
            # Override perspective if explicitly provided
            if perspective != "auto":
                query_hints.perspective = perspective
            
            # Override depth if explicitly provided
            if depth != 2:
                query_hints.depth = depth
            
            # Ensure database connection
            if not await self.db_manager.ensure_connected():
                return "Error: Database not available"
            
            try:
                # Execute triadic query based on hints
                results = await self._execute_triadic_query(query_hints)
                
                # Format results for natural language response
                response = self.query_tools.format_triadic_response(
                    results, query_hints.perspective
                )
                
                return response
                
            except Exception as e:
                self.logger.error(f"Query execution failed: {e}")
                return f"Error executing query: {str(e)}"
        
        @self.app.tool
        async def explore_triadic_navigation(
            starting_entity: str,
            perspective: str = "motion",
            depth: int = 2
        ) -> str:
            """
            Navigate triadic consciousness relationships starting from an entity
            
            Args:
                starting_entity: Entity to start navigation from
                perspective: Triadic perspective for navigation
                depth: Exploration depth
                
            Returns:
                Consciousness navigation results
                
            Examples:
                - explore_triadic_navigation("granite", "motion", 2)
                - explore_triadic_navigation("circle", "memory", 3)
            """
            
            # Ensure database connection
            if not await self.db_manager.ensure_connected():
                return "Error: Database not available"
            
            try:
                triadic_query = self.db_manager.get_triadic_query()
                if not triadic_query:
                    return "Error: Triadic query interface not available"
                
                # Execute perspective-specific navigation
                if perspective == "motion":
                    results = triadic_query.motion_from(starting_entity)
                elif perspective == "memory":
                    results = triadic_query.memory_relations(starting_entity)
                elif perspective == "field":
                    results = triadic_query.field_contexts(starting_entity)
                else:
                    return f"Error: Unknown perspective '{perspective}'"
                
                # Format results
                if results and hasattr(results, 'triples'):
                    response_lines = [
                        f"Triadic navigation from '{starting_entity}' ({perspective} perspective):",
                        f"Found {len(results.triples)} relationships:"
                    ]
                    
                    for triple in results.triples[:10]:  # Limit for readability
                        response_lines.append(f"  • {triple.subject} → {triple.predicate} → {triple.object}")
                    
                    if len(results.triples) > 10:
                        response_lines.append(f"  ... and {len(results.triples) - 10} more")
                    
                    return "\n".join(response_lines)
                else:
                    return f"No relationships found for '{starting_entity}' in {perspective} perspective"
                
            except Exception as e:
                self.logger.error(f"Navigation failed: {e}")
                return f"Error during navigation: {str(e)}"
        
        @self.app.tool
        async def database_health() -> str:
            """
            Check database health and connectivity
            
            Returns:
                Database health status and statistics as structured JSON
            """
            # Ensure database connection
            if not await self.db_manager.ensure_connected():
                error_result = {
                    "connected": False,
                    "error": "Database connection failed",
                    "path": None,
                    "total_triples": 0,
                    "motion": "unavailable",
                    "memory": "unavailable",
                    "field": "unavailable"
                }
                import json
                return json.dumps(error_result, indent=2)
            
            try:
                health = await self.db_manager.health_check()
                
                if health["status"] == "healthy":
                    stats = health["stats"]
                    triadic = health["triadic_consciousness"]
                    
                    # Get enhanced statistics from NonoStore
                    nonostore = self.db_manager.get_nonostore()
                    enhanced_stats = {}
                    tid_metrics = {}
                    
                    if nonostore:
                        try:
                            # Get full statistics including TID architecture metrics
                            db_stats = nonostore.get_stats()
                            enhanced_stats = {
                                "total_triples": db_stats.total_triples,
                                "unique_subjects": db_stats.unique_subjects,
                                "unique_predicates": db_stats.unique_predicates,
                                "unique_objects": db_stats.unique_objects,
                                "term_dictionary_size": getattr(db_stats, 'term_dictionary_size', 0),
                                "subject_vocabulary_size": getattr(db_stats, 'subject_vocabulary_size', 0),
                                "predicate_vocabulary_size": getattr(db_stats, 'predicate_vocabulary_size', 0),
                                "object_vocabulary_size": getattr(db_stats, 'object_vocabulary_size', 0),
                                "hexastore_indices_size": getattr(db_stats, 'hexastore_indices_size', 0),
                                "crown_indices_size": getattr(db_stats, 'crown_indices_size', 0),
                                "lmdb_entries": db_stats.lmdb_stats.entries,
                                "lmdb_page_size": db_stats.lmdb_stats.page_size,
                                "lmdb_depth": db_stats.lmdb_stats.depth,
                                "lmdb_branch_pages": db_stats.lmdb_stats.branch_pages,
                                "lmdb_leaf_pages": db_stats.lmdb_stats.leaf_pages,
                                "lmdb_overflow_pages": db_stats.lmdb_stats.overflow_pages
                            }
                            
                            # Get TID architecture metrics if available
                            if hasattr(nonostore, 'get_tid_metrics'):
                                tid_stats = nonostore.get_tid_metrics()
                                tid_metrics = {
                                    "term_dict_entries": tid_stats.term_dict_entries,
                                    "subject_tid_range": tid_stats.subject_tid_range,
                                    "predicate_tid_range": tid_stats.predicate_tid_range,
                                    "object_tid_range": tid_stats.object_tid_range,
                                    "total_tids_allocated": tid_stats.total_tids_allocated,
                                    "storage_efficiency": tid_stats.storage_efficiency
                                }
                                
                        except Exception as e:
                            self.logger.warning(f"Could not get enhanced statistics: {e}")
                            # Fallback to basic stats
                            enhanced_stats = {
                                "total_triples": stats["total_triples"],
                                "unique_subjects": stats.get("motion_entities", 0),
                                "unique_predicates": stats.get("memory_relations", 0),
                                "unique_objects": stats.get("field_contexts", 0)
                            }
                    
                    result = {
                        "path": health["database_path"],
                        "connected": True,
                        "display_name": health["display_name"],
                        "domain_type": health["domain_type"],
                        
                        # Basic triadic statistics
                        "total_triples": enhanced_stats.get("total_triples", stats["total_triples"]),
                        "motion": "available" if triadic["motion_available"] else "unavailable",
                        "memory": "available" if triadic["memory_available"] else "unavailable",
                        "field": "available" if triadic["field_available"] else "unavailable",
                        "motion_entities": enhanced_stats.get("unique_subjects", stats["motion_entities"]),
                        "memory_relations": enhanced_stats.get("unique_predicates", stats["memory_relations"]),
                        "field_contexts": enhanced_stats.get("unique_objects", stats["field_contexts"]),
                        
                        # Enhanced vocabulary statistics
                        "vocabulary_statistics": {
                            "term_dictionary_size": enhanced_stats.get("term_dictionary_size", 0),
                            "subject_vocabulary_size": enhanced_stats.get("subject_vocabulary_size", 0),
                            "predicate_vocabulary_size": enhanced_stats.get("predicate_vocabulary_size", 0),
                            "object_vocabulary_size": enhanced_stats.get("object_vocabulary_size", 0)
                        },
                        
                        # Storage architecture statistics
                        "storage_statistics": {
                            "hexastore_indices_size": enhanced_stats.get("hexastore_indices_size", 0),
                            "crown_indices_size": enhanced_stats.get("crown_indices_size", 0),
                            "lmdb_entries": enhanced_stats.get("lmdb_entries", 0),
                            "lmdb_page_size": enhanced_stats.get("lmdb_page_size", 0),
                            "lmdb_depth": enhanced_stats.get("lmdb_depth", 0),
                            "lmdb_branch_pages": enhanced_stats.get("lmdb_branch_pages", 0),
                            "lmdb_leaf_pages": enhanced_stats.get("lmdb_leaf_pages", 0),
                            "lmdb_overflow_pages": enhanced_stats.get("lmdb_overflow_pages", 0)
                        }
                    }
                    
                    # Add TID architecture metrics if available
                    if tid_metrics:
                        result["tid_architecture"] = tid_metrics
                    
                    import json
                    return json.dumps(result, indent=2)
                else:
                    error_result = {
                        "connected": False,
                        "error": health.get("error", "Unknown error"),
                        "path": health.get("database_path"),
                        "total_triples": 0,
                        "motion": "unavailable",
                        "memory": "unavailable",
                        "field": "unavailable"
                    }
                    import json
                    return json.dumps(error_result, indent=2)
                    
            except Exception as e:
                error_result = {
                    "connected": False,
                    "error": f"Health check failed: {str(e)}",
                    "path": None,
                    "total_triples": 0,
                    "motion": "unavailable",
                    "memory": "unavailable", 
                    "field": "unavailable"
                }
                import json
                return json.dumps(error_result, indent=2)
        
        @self.app.tool
        async def add_triple(
            subject: str,
            predicate: str,
            object: str,
            validate: bool = True
        ) -> str:
            """
            Add a triple to the triadic consciousness database with dual-mode EID resolution.
            
            Supports both natural language and explicit EID modes:
            - Natural: add_triple("rose_quartz", "has_color", "pink")
            - Explicit: add_triple("rose_quartz", "has_color", "eid:pink_003")
            
            Args:
                subject: Subject term (natural language or eid:explicit_id)
                predicate: Predicate/relationship term  
                object: Object term
                validate: Whether to validate the triple before adding
                
            Returns:
                Operation result with resolution feedback and disambiguation if needed
                
            Examples:
                - add_triple("rose_quartz", "has_color", "pink")
                - add_triple("granite", "eid:contains_relation", "quartz")
                - add_triple("eid:mineral_001", "mohs_hardness", "eid:hardness_7")
            """
            
            try:
                # Use the AddTripleTool to handle the request
                result = await self.add_triple_tool.add_triple(subject, predicate, object, validate)
                
                # Convert result to string for MCP response
                import json
                return json.dumps(result, indent=2)
                
            except Exception as e:
                self.logger.error(f"add_triple tool error: {e}")
                error_result = {
                    "status": "error",
                    "message": f"Tool error: {str(e)}",
                    "error_type": "tool_error"
                }
                import json
                return json.dumps(error_result, indent=2)
    
    async def _execute_triadic_query(self, query_hints) -> Dict[str, Any]:
        """Execute triadic query based on parsed hints"""
        nonostore = self.db_manager.get_nonostore()
        triadic_query = self.db_manager.get_triadic_query()
        
        if not nonostore or not triadic_query:
            raise RuntimeError("Database connection not available")
        
        results = {"triples": [], "vocabulary": [], "stats": {}}
        
        # Execute based on query type and perspective
        if query_hints.query_type == "what" and "entities" in query_hints.entities:
            # Vocabulary query
            if query_hints.perspective == "motion" or query_hints.perspective == "auto":
                results["vocabulary"] = nonostore.all_subjects()
            elif query_hints.perspective == "memory":
                results["vocabulary"] = nonostore.all_predicates()
            elif query_hints.perspective == "field":
                results["vocabulary"] = nonostore.all_objects()
        
        elif query_hints.entities:
            # Entity-focused exploration
            entity = query_hints.entities[0]  # Use first entity
            
            if query_hints.perspective == "motion" or query_hints.perspective == "auto":
                motion_results = triadic_query.motion_from(entity)
                if motion_results and hasattr(motion_results, 'triples'):
                    results["triples"].extend(motion_results.triples)
            
            if query_hints.perspective == "memory" or query_hints.perspective == "auto":
                memory_results = triadic_query.memory_relations(entity)
                if memory_results and hasattr(memory_results, 'triples'):
                    results["triples"].extend(memory_results.triples)
            
            if query_hints.perspective == "field" or query_hints.perspective == "auto":
                field_results = triadic_query.field_contexts(entity)
                if field_results and hasattr(field_results, 'triples'):
                    results["triples"].extend(field_results.triples)
        
        else:
            # General exploration - get sample vocabulary
            stats = nonostore.get_stats()
            results["stats"] = {
                "total_triples": stats.total_triples,
                "motion_entities": len(nonostore.all_subjects()),
                "memory_relations": len(nonostore.all_predicates()),
                "field_contexts": len(nonostore.all_objects())
            }
        
        return results

    def run(self, host: str = "localhost", port: int = 3000):
        """Run the DB9 MCP Server"""
        if not FASTMCP_AVAILABLE:
            raise RuntimeError("FastMCP2 not available")
        
        self.logger.info("Starting DB9 Server with STDIO transport for Claude Desktop")
        
        try:
            # Run the FastMCP2 server with STDIO transport (for Claude Desktop)
            self.app.run()  # No parameters = STDIO transport by default
        except KeyboardInterrupt:
            self.logger.info("Server stopped by user")
        except Exception as e:
            self.logger.error(f"Server error: {e}")
            raise


def main():
    """Main entry point for DB9 server"""
    import argparse
    
    parser = argparse.ArgumentParser(description="DB9 Triadic Consciousness Gateway")
    parser.add_argument("--config", help="Configuration file path")
    parser.add_argument("--host", default="localhost", help="Server host")
    parser.add_argument("--port", type=int, default=3000, help="Server port")
    parser.add_argument("--debug", action="store_true", help="Enable debug logging")
    
    args = parser.parse_args()
    
    if args.debug:
        logging.getLogger().setLevel(logging.DEBUG)
    
    try:
        server = DB9Server(config_path=args.config)
        server.run(host=args.host, port=args.port)
    except Exception as e:
        logging.error(f"Failed to start DB9 server: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
