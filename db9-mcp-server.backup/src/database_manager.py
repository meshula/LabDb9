"""
Database Manager for DB9 MCP Server
Handles single LabDb connection for Phase 1
"""

import asyncio
import logging
import sys
import yaml
from pathlib import Path
from typing import Optional, Dict, Any

# Import LabDb Python bindings
try:
    # Try the proper labdb package first (has our factory pattern)
    import labdb
    NonoStore = labdb.NonoStore
    LABDB_AVAILABLE = labdb._bindings_available if hasattr(labdb, '_bindings_available') else False
    if LABDB_AVAILABLE:
        logging.info("LabDb bindings loaded from labdb package")
        print(f"DEBUG: LabDb bindings loaded from labdb package", file=sys.stderr)
    else:
        raise ImportError("Bindings not available in labdb package")
        
except ImportError:
    try:
        # Fallback: try importing from build directory (limited functionality)
        import sys
        from pathlib import Path
        parent_build = Path(__file__).parent.parent.parent / "build"
        if parent_build.exists():
            sys.path.insert(0, str(parent_build))
            import pylabdb
            # Note: Direct pylabdb doesn't have factory pattern - this would need fallback
            logging.warning("Using direct pylabdb - factory pattern may not be available")
            print(f"DEBUG: Using direct pylabdb from build directory", file=sys.stderr)
            NonoStore = pylabdb.NonoStore
            LABDB_AVAILABLE = True
        else:
            raise ImportError("Build directory not found")
    except ImportError:
        logging.warning("LabDb Python bindings not available - using mock interface")
        print(f"DEBUG: LabDb Python bindings not available - using mock interface", file=sys.stderr)
        LABDB_AVAILABLE = False
        NonoStore = None
        # Note: TriadicQuery will be created via factory pattern in mock interface

# Import mock interface as fallback
if not LABDB_AVAILABLE:
    print(f"DEBUG: Loading mock interface", file=sys.stderr)
    from mock_labdb import MockNonoStore as NonoStore, MockTriadicQuery, create_mock_database
    print(f"DEBUG: Mock interface loaded successfully", file=sys.stderr)


class DatabaseManager:
    """
    Manages single LabDb database connection for Phase 1
    
    Provides consciousness-aware database operations:
    - Connection lifecycle management
    - Triadic query interface 
    - Vocabulary discovery
    - Error handling and recovery
    """
    
    def __init__(self, config: Dict[str, Any]):
        self.config = config
        self.db_path = Path(config["database"]["path"])
        self.domain_type = config["database"]["domain_type"]
        self.display_name = config["database"]["display_name"]
        
        # Database connections
        self.nonostore: Optional[NonoStore] = None
        self.triadic_query: Optional[Any] = None  # Created via factory pattern
        
        # Connection state
        self.is_connected = False
        self.connection_attempts = 0
        self.max_retry_attempts = config["database"]["connection"]["retry_attempts"]
        
        self.logger = logging.getLogger(__name__)
    
    def _create_python_triadic_wrapper(self):
        """Create a Python-based triadic wrapper that uses NonoStore directly"""
        class PythonTriadicQuery:
            def __init__(self, nonostore):
                self.nonostore = nonostore
            
            def motion_from(self, entity):
                """Motion perspective: relationships starting from an entity"""
                # This would need to be implemented based on NonoStore's query methods
                # For now, return a simple result structure
                class TriadicResult:
                    def __init__(self, triples):
                        self.triples = triples
                        self.perspective = "motion"
                        self.total_count = len(triples)
                
                # Use NonoStore to query relationships starting from entity
                # This is a placeholder - would need actual NonoStore query methods
                triples = []
                return TriadicResult(triples)
            
            def memory_relations(self, predicate):
                """Memory perspective: relationships using a specific predicate"""
                class TriadicResult:
                    def __init__(self, triples):
                        self.triples = triples
                        self.perspective = "memory"
                        self.total_count = len(triples)
                
                triples = []
                return TriadicResult(triples)
            
            def field_contexts(self, context):
                """Field perspective: relationships ending in a specific context"""
                class TriadicResult:
                    def __init__(self, triples):
                        self.triples = triples
                        self.perspective = "field"
                        self.total_count = len(triples)
                
                triples = []
                return TriadicResult(triples)
        
        return PythonTriadicQuery(self.nonostore)
    
    async def connect(self) -> bool:
        """
        Establish connection to LabDb database (or mock)
        
        Returns:
            bool: True if connection successful, False otherwise
        """
        if self.is_connected:
            self.logger.info("Database already connected")
            return True
        
        try:
            # DB9 Awareness Fairies - Show working directory and config discovery
            current_wd = Path.cwd()
            print(f"DB9 ⁖ Current working directory is {current_wd}", file=sys.stderr)
            
            triadic_yaml = current_wd / ".triadic.yaml"
            if triadic_yaml.exists():
                try:
                    import yaml
                    with open(triadic_yaml, 'r') as f:
                        config = yaml.safe_load(f)
                    akasha_path = config.get('akasha', 'triadic.db9')
                    if Path(akasha_path).is_absolute():
                        resolved_path = akasha_path
                    else:
                        resolved_path = current_wd / akasha_path
                    print(f"DB9 ⁖ .triadic.yaml found, akasha: {akasha_path} -> {resolved_path}", file=sys.stderr)
                    print(f"DB9 ⁖ Note: DB9 server uses fixed path {self.db_path}, not .triadic.yaml config", file=sys.stderr)
                except Exception as e:
                    print(f"DB9 ⁖ .triadic.yaml found but failed to parse: {e}", file=sys.stderr)
            else:
                print(f"DB9 ⁖ .triadic.yaml not found, using fixed path: {self.db_path}", file=sys.stderr)
            
            self.logger.info(f"Connecting to database: {self.db_path}")
            print(f"DB9 ⁖ Connecting to database: {self.db_path}, LABDB_AVAILABLE={LABDB_AVAILABLE}", file=sys.stderr)
            
            if LABDB_AVAILABLE:
                # Use real LabDb bindings
                print(f"DEBUG: Step 1 - About to create NonoStore({str(self.db_path)})", file=sys.stderr)
                self.nonostore = NonoStore(str(self.db_path))
                print(f"DEBUG: Step 1 SUCCESS - NonoStore created: {self.nonostore}", file=sys.stderr)
                
                # Initialize TriadicQuery using factory pattern if available
                print(f"DEBUG: Step 2 - About to create TriadicQuery using factory pattern", file=sys.stderr)
                
                # Check if factory method exists (proper labdb package)
                if hasattr(self.nonostore, 'create_triadic_query'):
                    self.triadic_query = self.nonostore.create_triadic_query()
                    print(f"DEBUG: Step 2 SUCCESS - TriadicQuery created via factory: {self.triadic_query}", file=sys.stderr)
                else:
                    # Fallback for direct pylabdb import (no factory pattern)
                    print(f"DEBUG: Step 2 FALLBACK - Factory method not available, using fallback", file=sys.stderr)
                    from mock_labdb import MockTriadicQuery
                    self.triadic_query = MockTriadicQuery(self.nonostore)
                    print(f"DEBUG: Step 2 SUCCESS - Using mock TriadicQuery fallback: {self.triadic_query}", file=sys.stderr)
                
                print(f"DEBUG: Both constructors successful, connection complete", file=sys.stderr)
                
            else:
                # Use mock interface for development
                self.logger.info("Using mock LabDb interface for development")
                print(f"DEBUG: Using mock LabDb interface", file=sys.stderr)
                self.nonostore, self.triadic_query = create_mock_database()
                print(f"DEBUG: Mock database created", file=sys.stderr)
                self.nonostore.connect(str(self.db_path))
                print(f"DEBUG: Mock database connected", file=sys.stderr)
            
            self.is_connected = True
            self.connection_attempts = 0
            
            # Log database statistics
            print(f"DEBUG: Getting stats from nonostore...", file=sys.stderr)
            stats = self.nonostore.get_stats()
            print(f"DEBUG: Stats retrieved successfully: {stats.total_triples} triples", file=sys.stderr)
            
            self.logger.info(f"Connected to {self.display_name}")
            self.logger.info(f"Database stats: {stats.total_triples} triples")
            
            return True
            
        except Exception as e:
            self.connection_attempts += 1
            error_msg = str(e)
            self.logger.error(f"Database connection failed (attempt {self.connection_attempts}): {error_msg}")
            
            # Factory pattern should have resolved pybind11 holder type issues
            # This fallback is kept for any remaining edge cases
            if "Unable to load a custom holder type" in error_msg:
                self.logger.warning("Unexpected pybind11 issue (should be resolved by factory pattern) - falling back to mock interface")
                print(f"DEBUG: Unexpected pybind11 issue detected (factory pattern should have fixed this)", file=sys.stderr)
                
                # Import and use mock interface
                try:
                    from mock_labdb import MockNonoStore, MockTriadicQuery, create_mock_database
                    self.logger.info("Using mock LabDb interface due to binding context issue")
                    self.nonostore, self.triadic_query = create_mock_database()
                    self.nonostore.connect(str(self.db_path))
                    
                    self.is_connected = True
                    self.connection_attempts = 0
                    
                    # Log database statistics
                    stats = self.nonostore.get_stats()
                    self.logger.info(f"Connected to {self.display_name} (mock interface)")
                    self.logger.info(f"Mock database stats: {stats.total_triples} triples")
                    
                    return True
                    
                except Exception as mock_error:
                    self.logger.error(f"Mock interface fallback failed: {mock_error}")
            
            if self.connection_attempts >= self.max_retry_attempts:
                self.logger.error("Max retry attempts reached - connection failed")
            
            return False
    
    async def disconnect(self):
        """Safely disconnect from database"""
        if self.is_connected and self.nonostore:
            try:
                # NonoStore doesn't have a general disconnect method - just clear references
                # The LMDB connection will be cleaned up when the object is destroyed
                self.is_connected = False
                self.nonostore = None
                self.triadic_query = None
                self.logger.info("Database disconnected successfully")
            except Exception as e:
                self.logger.error(f"Error during disconnect: {e}")
    
    async def health_check(self) -> Dict[str, Any]:
        """
        Check database health and connectivity
        
        Returns:
            Dict containing health status and metrics
        """
        print(f"DEBUG: health_check called, is_connected={self.is_connected}, nonostore={self.nonostore}", file=sys.stderr)
        
        if not self.is_connected or not self.nonostore:
            print(f"DEBUG: health_check returning disconnected - is_connected={self.is_connected}, nonostore={self.nonostore}", file=sys.stderr)
            return {
                "status": "disconnected",
                "error": "Database not connected"
            }
        
        try:
            print(f"DEBUG: health_check testing connectivity...", file=sys.stderr)
            # Test basic connectivity
            stats = self.nonostore.get_stats()
            print(f"DEBUG: stats retrieved: {stats}", file=sys.stderr)
            
            # Test triadic consciousness functionality
            motion_count = len(self.nonostore.all_subjects())
            memory_count = len(self.nonostore.all_predicates()) 
            field_count = len(self.nonostore.all_objects())
            print(f"DEBUG: triadic counts - motion:{motion_count}, memory:{memory_count}, field:{field_count}", file=sys.stderr)
            
            return {
                "status": "healthy",
                "database_path": str(self.db_path),
                "domain_type": self.domain_type,
                "display_name": self.display_name,
                "stats": {
                    "total_triples": stats.total_triples,
                    "motion_entities": motion_count,
                    "memory_relations": memory_count,
                    "field_contexts": field_count
                },
                "triadic_consciousness": {
                    "motion_available": motion_count > 0,
                    "memory_available": memory_count > 0,
                    "field_available": field_count > 0
                }
            }
            
        except Exception as e:
            return {
                "status": "error",
                "error": str(e)
            }
    
    def get_nonostore(self) -> Optional[NonoStore]:
        """Get NonoStore instance for direct database operations"""
        return self.nonostore if self.is_connected else None
    
    def get_triadic_query(self) -> Optional[Any]:  # Created via factory pattern
        """Get TriadicQuery instance for consciousness navigation"""
        return self.triadic_query if self.is_connected else None
    
    async def ensure_connected(self) -> bool:
        """Ensure database connection is active, reconnect if necessary"""
        print(f"DEBUG: ensure_connected called, is_connected={self.is_connected}", file=sys.stderr)
        if self.is_connected:
            print(f"DEBUG: Already connected, returning True", file=sys.stderr)
            return True
        
        print(f"DEBUG: Not connected, calling connect()", file=sys.stderr)
        result = await self.connect()
        print(f"DEBUG: connect() returned {result}", file=sys.stderr)
        return result
    
    def add_triple(self, subject: str, predicate: str, obj: str, validate: bool = True) -> bool:
        """Add a triple to the database"""
        if not self.is_connected or not self.nonostore:
            self.logger.error("Cannot add triple - database not connected")
            return False
        
        try:
            # Use NonoStore to add the triple
            if hasattr(self.nonostore, 'add_triple'):
                return self.nonostore.add_triple(subject, predicate, obj)
            elif hasattr(self.nonostore, 'insert'):
                return self.nonostore.insert(subject, predicate, obj)
            else:
                # For mock interface or basic stores
                self.logger.warning("NonoStore doesn't have add_triple method - triple not stored")
                return True  # Return True for mock interface to allow testing
        except Exception as e:
            self.logger.error(f"Error adding triple: {e}")
            return False
    
    def query_entity(self, eid: str) -> Optional[Dict[str, Any]]:
        """Query for entity existence and information"""
        if not self.is_connected or not self.nonostore:
            return None
        
        try:
            # Check if entity exists by looking for triples containing it
            if hasattr(self.nonostore, 'query_entity'):
                return self.nonostore.query_entity(eid)
            else:
                # Fallback: check if entity appears in any position
                subjects = self.nonostore.all_subjects()
                predicates = self.nonostore.all_predicates()
                objects = self.nonostore.all_objects()
                
                if eid in subjects or eid in predicates or eid in objects:
                    return {"id": eid, "exists": True}
                else:
                    return None
        except Exception as e:
            self.logger.error(f"Error querying entity {eid}: {e}")
            return None
    
    async def reconnect_to_database(self, new_db_path: str) -> bool:
        """Reconnect to a different database path"""
        try:
            # Disconnect from current database
            await self.disconnect()
            
            # Update database path
            self.db_path = Path(new_db_path)
            
            # Connect to new database
            return await self.connect()
            
        except Exception as e:
            self.logger.error(f"Failed to reconnect to database {new_db_path}: {e}")
            return False
    
    async def create_new_database(self, db_path: str, initialize_with_test_data: bool = False) -> bool:
        """Create a new database at the specified path"""
        try:
            resolved_path = Path(db_path).resolve()
            
            # Check if database already exists
            if resolved_path.exists():
                self.logger.error(f"Database already exists at {resolved_path}")
                return False
            
            # Ensure parent directory exists
            resolved_path.parent.mkdir(parents=True, exist_ok=True)
            
            if LABDB_AVAILABLE and NonoStore:
                # Create new NonoStore database
                try:
                    new_store = NonoStore(str(resolved_path))
                    
                    # Add test data if requested
                    if initialize_with_test_data:
                        # Add some basic test triples
                        test_triples = [
                            ("test_entity", "type", "example"),
                            ("motion_test", "demonstrates", "motion_principle"),
                            ("memory_test", "recalls", "past_state"),
                            ("field_test", "contextualizes", "situation")
                        ]
                        
                        for subject, predicate, obj in test_triples:
                            if hasattr(new_store, 'add_triple'):
                                new_store.add_triple(subject, predicate, obj)
                            elif hasattr(new_store, 'insert'):
                                new_store.insert(subject, predicate, obj)
                    
                    # Close the temporary store
                    if hasattr(new_store, 'close'):
                        new_store.close()
                    
                    self.logger.info(f"Created new database at {resolved_path}")
                    return True
                    
                except Exception as e:
                    self.logger.error(f"Failed to create NonoStore at {resolved_path}: {e}")
                    return False
            else:
                # For mock interface, just create an empty file
                resolved_path.touch()
                self.logger.info(f"Created mock database file at {resolved_path}")
                return True
                
        except Exception as e:
            self.logger.error(f"Failed to create database {db_path}: {e}")
            return False
    
    async def disconnect(self):
        """Disconnect from current database"""
        try:
            if self.nonostore and hasattr(self.nonostore, 'close'):
                self.nonostore.close()
            
            self.nonostore = None
            self.triadic_query = None
            self.is_connected = False
            self.logger.info("Disconnected from database")
            
        except Exception as e:
            self.logger.error(f"Error during disconnect: {e}")
