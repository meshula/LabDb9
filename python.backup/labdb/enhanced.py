"""
Enhanced Pythonic interface for LabDb triadic consciousness
Provides natural Python patterns on top of C++ bindings
"""

class EnhancedTriadicResult:
    """
    Enhanced TriadicResult with Pythonic properties and methods
    Provides natural exploration patterns for triadic consciousness
    """
    
    def __init__(self, cpp_result, triadic_query):
        self._result = cpp_result
        self._triadic = triadic_query
    
    @property
    def entity(self):
        """Entity in this triadic relationship"""
        return self._result.entity
    
    @property
    def relation(self):
        """Relation connecting entity to context"""
        return self._result.relation
    
    @property 
    def context(self):
        """Context that grounds this relationship"""
        return self._result.context
    
    @property
    def discovered_through(self):
        """Perspective through which this was discovered"""
        return self._result.discovered_through
    
    @property
    def perspective_name(self):
        """Human-readable perspective name"""
        from . import perspective_name
        return perspective_name(self.discovered_through)
    
    @property
    def sanskrit_name(self):
        """Sanskrit name for the perspective"""
        from . import perspective_sanskrit
        return perspective_sanskrit(self.discovered_through)
    
    # Motion perspective properties (स्पन्द)
    @property 
    def motion(self):
        """Explore Motion perspective from this result's entity"""
        return EnhancedTriadicResults(
            self._triadic.motion_from(self.entity), 
            self._triadic
        )
    
    @property
    def expressions(self):
        """All ways this entity expresses itself"""
        return self._triadic.entity_expressions(self.entity)
    
    # Memory perspective properties (स्मृति)
    @property
    def memory(self):
        """Explore Memory perspective from this result's relation"""
        return EnhancedTriadicResults(
            self._triadic.memory_relations(self.relation),
            self._triadic
        )
    
    def memory_between(self, other_entity):
        """Memory patterns between this entity and another"""
        return EnhancedTriadicResults(
            self._triadic.memory_between(self.entity, other_entity),
            self._triadic
        )
    
    # Field perspective properties (क्षेत्र)
    @property
    def field(self):
        """Explore Field perspective from this result's context"""
        return EnhancedTriadicResults(
            self._triadic.field_contexts(self.context),
            self._triadic
        )
    
    # Cube navigation methods
    def shift_to(self, perspective):
        """Shift to a different triadic perspective"""
        from . import Perspective
        if isinstance(perspective, str):
            perspective = getattr(Perspective, perspective.upper())
        
        shifted = self._triadic.perspective_shift([self._result], perspective)
        return EnhancedTriadicResults(shifted, self._triadic)
    
    def traverse(self, max_depth=3):
        """Traverse the triadic cube from this result"""
        traversal = self._triadic.triadic_traverse(self.entity, max_depth)
        return EnhancedTriadicResults(traversal, self._triadic)
    
    def crown_explore(self):
        """Explore crown structure around this result"""
        crown = self._triadic.crown_exploration(self.entity, self.relation, self.context)
        return EnhancedTriadicResults(crown, self._triadic)
    
    # Python magic methods
    def __str__(self):
        return f"{self.entity} --[{self.relation}]--> {self.context}"
    
    def __repr__(self):
        return f"EnhancedTriadicResult('{self.entity}', '{self.relation}', '{self.context}', {self.perspective_name})"
    
    def __eq__(self, other):
        if not isinstance(other, EnhancedTriadicResult):
            return False
        return (self.entity == other.entity and 
                self.relation == other.relation and 
                self.context == other.context)
    
    def __hash__(self):
        return hash((self.entity, self.relation, self.context))
    
    # Dictionary-like access
    def __getitem__(self, key):
        if key in ['s', 'subject', 'entity']:
            return self.entity
        elif key in ['p', 'predicate', 'relation']:
            return self.relation
        elif key in ['o', 'object', 'context']:
            return self.context
        elif key == 'perspective':
            return self.discovered_through
        else:
            raise KeyError(f"Unknown key: {key}")
    
    def as_dict(self):
        """Convert to dictionary for JSON serialization"""
        return {
            'entity': self.entity,
            'relation': self.relation,
            'context': self.context,
            'perspective': self.perspective_name,
            'sanskrit': self.sanskrit_name
        }
    
    def as_triple(self):
        """Convert to simple (entity, relation, context) tuple"""
        return (self.entity, self.relation, self.context)


class EnhancedTriadicResults:
    """
    Collection of EnhancedTriadicResult objects with Pythonic iteration
    Provides filtering, grouping, and analysis methods
    """
    
    def __init__(self, cpp_results, triadic_query):
        self._results = [EnhancedTriadicResult(r, triadic_query) for r in cpp_results]
        self._triadic = triadic_query
    
    def __iter__(self):
        return iter(self._results)
    
    def __len__(self):
        return len(self._results)
    
    def __getitem__(self, index):
        return self._results[index]
    
    def __repr__(self):
        return f"EnhancedTriadicResults({len(self._results)} results)"
    
    # Filtering methods
    def filter_by_entity(self, entity):
        """Filter results by entity"""
        filtered = [r for r in self._results if r.entity == entity]
        return EnhancedTriadicResults([], self._triadic)._from_enhanced(filtered)
    
    def filter_by_relation(self, relation):
        """Filter results by relation"""
        filtered = [r for r in self._results if r.relation == relation]
        return EnhancedTriadicResults([], self._triadic)._from_enhanced(filtered)
    
    def filter_by_context(self, context):
        """Filter results by context"""
        filtered = [r for r in self._results if r.context == context]
        return EnhancedTriadicResults([], self._triadic)._from_enhanced(filtered)
    
    def filter_by_perspective(self, perspective):
        """Filter results by discovery perspective"""
        filtered = [r for r in self._results if r.discovered_through == perspective]
        return EnhancedTriadicResults([], self._triadic)._from_enhanced(filtered)
    
    def _from_enhanced(self, enhanced_results):
        """Create from already enhanced results"""
        new_collection = EnhancedTriadicResults([], self._triadic)
        new_collection._results = enhanced_results
        return new_collection
    
    # Grouping methods
    def group_by_entity(self):
        """Group results by entity"""
        groups = {}
        for result in self._results:
            if result.entity not in groups:
                groups[result.entity] = []
            groups[result.entity].append(result)
        return {k: self._from_enhanced(v) for k, v in groups.items()}
    
    def group_by_relation(self):
        """Group results by relation"""
        groups = {}
        for result in self._results:
            if result.relation not in groups:
                groups[result.relation] = []
            groups[result.relation].append(result)
        return {k: self._from_enhanced(v) for k, v in groups.items()}
    
    def group_by_perspective(self):
        """Group results by discovery perspective"""
        groups = {}
        for result in self._results:
            perspective = result.perspective_name
            if perspective not in groups:
                groups[perspective] = []
            groups[perspective].append(result)
        return {k: self._from_enhanced(v) for k, v in groups.items()}
    
    # Analysis methods
    @property
    def entities(self):
        """All unique entities in results"""
        return list(set(r.entity for r in self._results))
    
    @property
    def relations(self):
        """All unique relations in results"""
        return list(set(r.relation for r in self._results))
    
    @property
    def contexts(self):
        """All unique contexts in results"""
        return list(set(r.context for r in self._results))
    
    @property
    def perspectives(self):
        """All discovery perspectives in results"""
        return list(set(r.perspective_name for r in self._results))
    
    def count_by_entity(self):
        """Count results by entity"""
        counts = {}
        for result in self._results:
            counts[result.entity] = counts.get(result.entity, 0) + 1
        return counts
    
    def count_by_relation(self):
        """Count results by relation"""
        counts = {}
        for result in self._results:
            counts[result.relation] = counts.get(result.relation, 0) + 1
        return counts
    
    def count_by_perspective(self):
        """Count results by discovery perspective"""
        counts = {}
        for result in self._results:
            perspective = result.perspective_name
            counts[perspective] = counts.get(perspective, 0) + 1
        return counts
    
    # Conversion methods
    def as_list(self):
        """Convert to list of dictionaries"""
        return [r.as_dict() for r in self._results]
    
    def as_triples(self):
        """Convert to list of (entity, relation, context) tuples"""
        return [r.as_triple() for r in self._results]
    
    def as_dataframe(self):
        """Convert to pandas DataFrame (if pandas available)"""
        try:
            import pandas as pd
            return pd.DataFrame(self.as_list())
        except ImportError:
            raise ImportError("pandas required for as_dataframe()")
    
    # Triadic operations on the entire collection
    def shift_all_to(self, perspective):
        """Shift all results to a different perspective"""
        from . import Perspective
        if isinstance(perspective, str):
            perspective = getattr(Perspective, perspective.upper())
        
        cpp_results = [r._result for r in self._results]
        shifted = self._triadic.perspective_shift(cpp_results, perspective)
        return EnhancedTriadicResults(shifted, self._triadic)
    
    def explore_motion(self):
        """Explore Motion perspective from all entities"""
        all_results = []
        for entity in self.entities:
            motion_results = self._triadic.motion_from(entity)
            all_results.extend(motion_results)
        return EnhancedTriadicResults(all_results, self._triadic)
    
    def explore_memory(self):
        """Explore Memory perspective from all relations"""
        all_results = []
        for relation in self.relations:
            memory_results = self._triadic.memory_relations(relation)
            all_results.extend(memory_results)
        return EnhancedTriadicResults(all_results, self._triadic)
    
    def explore_field(self):
        """Explore Field perspective from all contexts"""
        all_results = []
        for context in self.contexts:
            field_results = self._triadic.field_contexts(context)
            all_results.extend(field_results)
        return EnhancedTriadicResults(all_results, self._triadic)


class EnhancedNonoStore:
    """
    Enhanced NonoStore with Pythonic patterns and triadic convenience methods
    """
    
    def __init__(self, db_path):
        from . import NonoStore, TriadicQuery
        self._store = NonoStore(db_path)
        self._triadic = TriadicQuery(self._store)
    
    # Delegate core NonoStore methods
    def connect(self, subject, predicate, object):
        """Connect subject to object via predicate"""
        return self._store.connect(subject, predicate, object)
    
    def disconnect(self, subject, predicate, object):
        """Disconnect specific triple"""
        return self._store.disconnect(subject, predicate, object)
    
    def exists(self, subject, predicate, object):
        """Check if triple exists"""
        return self._store.exists(subject, predicate, object)
    
    def count(self, subject="*", predicate="*", object="*"):
        """Count triples matching pattern"""
        return self._store.count(subject, predicate, object)
    
    def query(self, subject="*", predicate="*", object="*"):
        """Query triples matching pattern"""
        return self._store.query(subject, predicate, object)
    
    # Dictionary-like interface
    def __getitem__(self, key):
        """Dictionary-like access: store[s, p, o] or store[entity]"""
        if isinstance(key, tuple):
            if len(key) == 3:
                s, p, o = key
                return self.query(s, p, o)
            else:
                raise ValueError("Tuple key must have 3 elements (subject, predicate, object)")
        else:
            # Single key - return all properties of entity
            return self.properties_of(key)
    
    def __setitem__(self, key, value):
        """Dictionary-like assignment: store[s, p] = o"""
        if isinstance(key, tuple) and len(key) == 2:
            s, p = key
            self.connect(s, p, value)
        else:
            raise ValueError("Key must be (subject, predicate) tuple")
    
    def __delitem__(self, key):
        """Dictionary-like deletion: del store[s, p, o]"""
        if isinstance(key, tuple) and len(key) == 3:
            s, p, o = key
            self.disconnect(s, p, o)
        else:
            raise ValueError("Key must be (subject, predicate, object) tuple")
    
    def __contains__(self, key):
        """Dictionary-like membership: (s, p, o) in store"""
        if isinstance(key, tuple) and len(key) == 3:
            s, p, o = key
            return self.exists(s, p, o)
        else:
            return key in self.all_subjects()
    
    # Context manager for transactions
    def __enter__(self):
        """Enter transaction context"""
        self._store.begin_bulk_update()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Exit transaction context"""
        if exc_type is None:
            self._store.commit_bulk_update()
        else:
            self._store.rollback_bulk_update()
    
    # Vocabulary discovery
    @property
    def subjects(self):
        """All subjects (Motion vocabulary)"""
        return self._store.all_subjects()
    
    @property
    def predicates(self):
        """All predicates (Memory vocabulary)"""
        return self._store.all_predicates()
    
    @property
    def objects(self):
        """All objects (Field vocabulary)"""
        return self._store.all_objects()
    
    @property
    def vocabulary(self):
        """Complete vocabulary as dictionary"""
        return {
            'motion': self.subjects,
            'memory': self.predicates,
            'field': self.objects
        }
    
    # Convenience methods with enhanced results
    def properties_of(self, entity):
        """Get all properties of entity as enhanced results"""
        results = self._triadic.motion_from(entity)
        return EnhancedTriadicResults(results, self._triadic)
    
    def connections_of(self, relation):
        """Get all connections using relation as enhanced results"""
        results = self._triadic.memory_relations(relation)
        return EnhancedTriadicResults(results, self._triadic)
    
    def contexts_of(self, object):
        """Get all contexts for object as enhanced results"""
        results = self._triadic.field_contexts(object)
        return EnhancedTriadicResults(results, self._triadic)
    
    # Triadic exploration methods
    def explore_motion(self, entity=None):
        """Explore Motion perspective"""
        if entity:
            results = self._triadic.motion_from(entity)
        else:
            # Explore from all entities
            results = []
            for e in self.subjects[:10]:  # Limit to first 10 for performance
                results.extend(self._triadic.motion_from(e))
        return EnhancedTriadicResults(results, self._triadic)
    
    def explore_memory(self, relation=None):
        """Explore Memory perspective"""
        if relation:
            results = self._triadic.memory_relations(relation)
        else:
            # Get relation frequencies
            frequencies = self._triadic.relation_frequencies()
            results = []
            for rel, count in frequencies[:5]:  # Top 5 relations
                results.extend(self._triadic.memory_relations(rel))
        return EnhancedTriadicResults(results, self._triadic)
    
    def explore_field(self, context=None):
        """Explore Field perspective"""
        if context:
            results = self._triadic.field_contexts(context)
        else:
            # Explore primary contexts
            primary = self._triadic.primary_contexts()
            results = []
            for ctx in primary[:5]:  # Top 5 contexts
                results.extend(self._triadic.field_contexts(ctx))
        return EnhancedTriadicResults(results, self._triadic)
    
    def explore_around(self, focal_point):
        """Complete exploration around a focal point"""
        results = self._triadic.crown_exploration(focal_point)
        return EnhancedTriadicResults(results, self._triadic)
    
    def traverse_from(self, starting_point, max_depth=3):
        """Traverse triadic cube from starting point"""
        results = self._triadic.triadic_traverse(starting_point, max_depth)
        return EnhancedTriadicResults(results, self._triadic)
    
    # Analytics
    @property
    def stats(self):
        """Get triadic statistics"""
        return self._triadic.get_triadic_stats()
    
    @property
    def bridge_entities(self):
        """Get entities that bridge different domains"""
        return self._triadic.bridge_entities()
    
    def relationship_clusters(self):
        """Detect relationship clusters"""
        return self._triadic.detect_relationship_clusters()
    
    # Access to underlying objects
    @property
    def store(self):
        """Access to underlying NonoStore"""
        return self._store
    
    @property
    def triadic(self):
        """Access to underlying TriadicQuery"""
        return self._triadic
