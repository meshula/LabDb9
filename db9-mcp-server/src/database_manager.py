"""
Database Manager for DB9 MCP Server
Handles single LabDb connection for Phase 1
"""

import asyncio
import logging
from pathlib import Path
from typing import Optional, Dict, Any

# Import LabDb Python bindings
try:
    # Try importing from parent build directory first
    import sys
    from pathlib import Path
    parent_build = Path(__file__).parent.parent.parent / "build"
    if parent_build.exists():
        sys.path.insert(0, str(parent_build))
        import pylabdb as labdb
        # Map to expected interface
        NonoStore = labdb.NonoStore
        TriadicQuery = labdb.TriadicQuery
        LABDB_AVAILABLE = True
        logging.info("LabDb bindings loaded from parent build directory")
    else:
        raise ImportError("Parent build directory not found")
        
except ImportError:
    try:
        # Fallback to standard labdb package
        import labdb
        from labdb import NonoStore, TriadicQuery
        LABDB_AVAILABLE = labdb._bindings_available if hasattr(labdb, '_bindings_available') else False
        if LABDB_AVAILABLE:
            logging.info("LabDb bindings loaded from installed package")
    except ImportError:
        logging.warning("LabDb Python bindings not available - using mock interface")
        LABDB_AVAILABLE = False
        NonoStore = None
        TriadicQuery = None

# Import mock interface as fallback
if not LABDB_AVAILABLE:
    from .mock_labdb import MockNonoStore as NonoStore, MockTriadicQuery as TriadicQuery, create_mock_database


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
        self.triadic_query: Optional[TriadicQuery] = None
        
        # Connection state
        self.is_connected = False
        self.connection_attempts = 0
        self.max_retry_attempts = config["database"]["connection"]["retry_attempts"]
        
        self.logger = logging.getLogger(__name__)
    
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
            self.logger.info(f"Connecting to database: {self.db_path}")
            
            if LABDB_AVAILABLE:
                # Use real LabDb bindings
                self.nonostore = NonoStore()
                success = self.nonostore.connect(str(self.db_path))
                
                if not success:
                    self.logger.error(f"Failed to connect to database: {self.db_path}")
                    return False
                
                # Initialize TriadicQuery for consciousness navigation
                self.triadic_query = TriadicQuery(self.nonostore)
                
            else:
                # Use mock interface for development
                self.logger.info("Using mock LabDb interface for development")
                self.nonostore, self.triadic_query = create_mock_database()
                self.nonostore.connect(str(self.db_path))
            
            self.is_connected = True
            self.connection_attempts = 0
            
            # Log database statistics
            stats = self.nonostore.get_stats()
            self.logger.info(f"Connected to {self.display_name}")
            self.logger.info(f"Database stats: {stats.triple_count} triples")
            
            return True
            
        except Exception as e:
            self.connection_attempts += 1
            self.logger.error(f"Database connection failed (attempt {self.connection_attempts}): {e}")
            
            if self.connection_attempts >= self.max_retry_attempts:
                self.logger.error("Max retry attempts reached - connection failed")
            
            return False
    
    async def disconnect(self):
        """Safely disconnect from database"""
        if self.is_connected and self.nonostore:
            try:
                self.nonostore.disconnect()
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
        if not self.is_connected or not self.nonostore:
            return {
                "status": "disconnected",
                "error": "Database not connected"
            }
        
        try:
            # Test basic connectivity
            stats = self.nonostore.get_stats()
            
            # Test triadic consciousness functionality
            motion_count = len(self.nonostore.all_subjects())
            memory_count = len(self.nonostore.all_predicates()) 
            field_count = len(self.nonostore.all_objects())
            
            return {
                "status": "healthy",
                "database_path": str(self.db_path),
                "domain_type": self.domain_type,
                "display_name": self.display_name,
                "stats": {
                    "triple_count": stats.triple_count,
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
    
    def get_triadic_query(self) -> Optional[TriadicQuery]:
        """Get TriadicQuery instance for consciousness navigation"""
        return self.triadic_query if self.is_connected else None
    
    async def ensure_connected(self) -> bool:
        """Ensure database connection is active, reconnect if necessary"""
        if self.is_connected:
            return True
        
        return await self.connect()
