"""
DB9 MCP Server - Triadic Consciousness Gateway
Main FastMCP2 server implementation for Phase 1
"""

import asyncio
import logging
import sys
from pathlib import Path
from typing import List, Optional, Dict, Any

import yaml
from pydantic import BaseModel

# FastMCP imports
try:
    from fastmcp import FastMCP
    from fastmcp.server import Server
    from fastmcp.tools import tool
    FASTMCP_AVAILABLE = True
except ImportError:
    logging.error("FastMCP not available - install with: pip install fastmcp")
    FASTMCP_AVAILABLE = False
    FastMCP = None
    tool = None

# Local imports
from .database_manager import DatabaseManager
from .tools.query_tools import QueryTools


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
        self.db_manager = DatabaseManager(self.config)
        self.query_tools = QueryTools()
        
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
        """Setup FastMCP2 server with tools and middleware"""
        # Register server lifecycle hooks
        self.app.on_startup(self._on_startup)
        self.app.on_shutdown(self._on_shutdown)
        
        # Register tools
        self._register_tools()
    
    async def _on_startup(self):
        """Server startup hook - establish database connection"""
        self.logger.info("DB9 Server starting up...")
        
        # Connect to database
        connected = await self.db_manager.connect()
        if not connected:
            raise RuntimeError("Failed to connect to LabDb database")
        
        # Log database health
        health = await self.db_manager.health_check()
        self.logger.info(f"Database health: {health}")
        
        self.logger.info("DB9 Server startup complete!")
    
    async def _on_shutdown(self):
        """Server shutdown hook - cleanup connections"""
        self.logger.info("DB9 Server shutting down...")
        await self.db_manager.disconnect()
        self.logger.info("DB9 Server shutdown complete!")
    
    def _register_tools(self):
        """Register MCP tools with FastMCP2"""
        
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
        async def database_health(self) -> str:
            """
            Check database health and connectivity
            
            Returns:
                Database health status and statistics
            """
            health = await self.db_manager.health_check()
            
            if health["status"] == "healthy":
                stats = health["stats"]
                triadic = health["triadic_consciousness"]
                
                response_lines = [
                    f"✅ Database: {health['display_name']}",
                    f"📊 Statistics:",
                    f"  • Total triples: {stats['triple_count']}",
                    f"  • Motion entities: {stats['motion_entities']}",
                    f"  • Memory relations: {stats['memory_relations']}",
                    f"  • Field contexts: {stats['field_contexts']}",
                    f"🧠 Triadic Consciousness:",
                    f"  • Motion available: {triadic['motion_available']}",
                    f"  • Memory available: {triadic['memory_available']}",
                    f"  • Field available: {triadic['field_available']}"
                ]
                
                return "\n".join(response_lines)
            else:
                return f"❌ Database unhealthy: {health.get('error', 'Unknown error')}"
    
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
                "triple_count": stats.triple_count,
                "motion_entities": len(nonostore.all_subjects()),
                "memory_relations": len(nonostore.all_predicates()),
                "field_contexts": len(nonostore.all_objects())
            }
        
        return results

    def run(self, host: str = "localhost", port: int = 3000):
        """Run the DB9 MCP Server"""
        if not FASTMCP_AVAILABLE:
            raise RuntimeError("FastMCP2 not available")
        
        self.logger.info(f"Starting DB9 Server on {host}:{port}")
        
        try:
            # Run the FastMCP2 server
            self.app.run(host=host, port=port)
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
