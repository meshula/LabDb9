"""
Mock LabDb Interface for DB9 Phase 1 Development
Simulates LabDb triadic consciousness database for testing without build complexity
"""

import logging
from typing import List, Dict, Any, Optional
from dataclasses import dataclass


@dataclass
class Triple:
    """Mock triple representation"""
    subject: str
    predicate: str
    object: str


@dataclass
class TriadicResult:
    """Mock triadic query result"""
    triples: List[Triple]
    perspective: str
    total_count: int


@dataclass
class DatabaseStats:
    """Mock database statistics"""
    total_triples: int
    
    
class MockNonoStore:
    """
    Mock NonoStore for DB9 Phase 1 development
    Simulates core LabDb functionality with sample triadic data
    """
    
    def __init__(self):
        self.connected = False
        self.logger = logging.getLogger(__name__)
        
        # Sample triadic consciousness data
        self.sample_triples = [
            Triple("granite", "isA", "rock"),
            Triple("granite", "hasColor", "gray"),
            Triple("granite", "hasHardness", "6"),
            Triple("granite", "usedIn", "construction"),
            Triple("quartz", "isA", "mineral"), 
            Triple("quartz", "hasColor", "clear"),
            Triple("quartz", "foundIn", "granite"),
            Triple("diamond", "isA", "mineral"),
            Triple("diamond", "hasHardness", "10"),
            Triple("marble", "isA", "rock"),
            Triple("marble", "hasColor", "white"),
            Triple("circle", "isA", "geometric_shape"),
            Triple("circle", "hasProperty", "continuous_curve"),
            Triple("triangle", "isA", "geometric_shape"),
            Triple("euclid", "wrote", "elements"),
            Triple("elements", "contains", "geometric_proofs")
        ]
    
    def connect(self, path: str) -> bool:
        """Mock database connection"""
        self.logger.info(f"Mock connecting to database: {path}")
        self.connected = True
        return True
    
    def disconnect(self):
        """Mock database disconnection"""
        self.connected = False
        self.logger.info("Mock database disconnected")
    
    def get_stats(self) -> DatabaseStats:
        """Get mock database statistics"""
        return DatabaseStats(total_triples=len(self.sample_triples))
    
    def all_subjects(self) -> List[str]:
        """Get all subjects (Motion entities)"""
        return list(set(t.subject for t in self.sample_triples))
    
    def all_predicates(self) -> List[str]:
        """Get all predicates (Memory relations)"""
        return list(set(t.predicate for t in self.sample_triples))
    
    def all_objects(self) -> List[str]:
        """Get all objects (Field contexts)"""
        return list(set(t.object for t in self.sample_triples))
    
    def query_triples(self, subject: str = None, predicate: str = None, object: str = None) -> List[Triple]:
        """Query triples with optional filtering"""
        results = []
        for triple in self.sample_triples:
            if subject and triple.subject != subject:
                continue
            if predicate and triple.predicate != predicate:
                continue
            if object and triple.object != object:
                continue
            results.append(triple)
        return results


class MockTriadicQuery:
    """
    Mock TriadicQuery for triadic consciousness navigation
    Simulates Motion/Memory/Field perspective queries
    """
    
    def __init__(self, nonostore: MockNonoStore):
        self.nonostore = nonostore
        self.logger = logging.getLogger(__name__)
    
    def motion_from(self, entity: str) -> TriadicResult:
        """Mock Motion (स्पन्द) perspective query"""
        # Use real NonoStore query method if available, fallback to mock
        if hasattr(self.nonostore, 'query'):
            real_triples = self.nonostore.query(entity, "*", "*")
            triples = [Triple(t.subject, t.predicate, t.object) for t in real_triples]
        else:
            triples = self.nonostore.query_triples(subject=entity)
        return TriadicResult(
            triples=triples,
            perspective="motion",
            total_count=len(triples)
        )
    
    def memory_relations(self, predicate: str) -> TriadicResult:
        """Mock Memory (स्मृति) perspective query"""
        # Use real NonoStore query method if available, fallback to mock
        if hasattr(self.nonostore, 'query'):
            real_triples = self.nonostore.query("*", predicate, "*")
            triples = [Triple(t.subject, t.predicate, t.object) for t in real_triples]
        else:
            triples = self.nonostore.query_triples(predicate=predicate)
        return TriadicResult(
            triples=triples,
            perspective="memory", 
            total_count=len(triples)
        )
    
    def field_contexts(self, context: str) -> TriadicResult:
        """Mock Field (क्षेत्र) perspective query"""
        # Use real NonoStore query method if available, fallback to mock
        if hasattr(self.nonostore, 'query'):
            real_triples = self.nonostore.query("*", "*", context)
            triples = [Triple(t.subject, t.predicate, t.object) for t in real_triples]
        else:
            triples = self.nonostore.query_triples(object=context)
        return TriadicResult(
            triples=triples,
            perspective="field",
            total_count=len(triples)
        )
    
    def triadic_traverse(self, entity: str, depth: int = 2) -> TriadicResult:
        """Mock triadic traversal with depth"""
        # Start with entity's direct relationships
        initial_triples = self.nonostore.query_triples(subject=entity)
        
        if depth <= 1:
            return TriadicResult(
                triples=initial_triples,
                perspective="traverse",
                total_count=len(initial_triples)
            )
        
        # For depth > 1, find related entities
        all_triples = initial_triples.copy()
        visited = {entity}
        
        for _ in range(depth - 1):
            new_entities = set()
            for triple in all_triples:
                if triple.object not in visited:
                    new_entities.add(triple.object)
            
            for new_entity in new_entities:
                if new_entity not in visited:
                    related = self.nonostore.query_triples(subject=new_entity)
                    all_triples.extend(related)
                    visited.add(new_entity)
        
        # Remove duplicates
        unique_triples = []
        seen = set()
        for triple in all_triples:
            key = (triple.subject, triple.predicate, triple.object)
            if key not in seen:
                unique_triples.append(triple)
                seen.add(key)
        
        return TriadicResult(
            triples=unique_triples,
            perspective="traverse",
            total_count=len(unique_triples)
        )


# Mock module availability flag
_bindings_available = True


def create_mock_database() -> tuple[MockNonoStore, MockTriadicQuery]:
    """Create mock database and query interface for testing"""
    nonostore = MockNonoStore()
    triadic_query = MockTriadicQuery(nonostore)
    return nonostore, triadic_query
