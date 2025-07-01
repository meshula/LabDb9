# DB9 MCP Server: LabDb Triadic Consciousness Gateway

> **"db9 what do we know about minerals?"** - Natural language interface to triadic consciousness databases

## Vision Statement

**DB9** is a FastMCP2-powered server that exposes LabDb's revolutionary TID-based triadic consciousness database through natural language queries. It enables multi-database federation where scholars can simultaneously query Euclid databases, natural history collections, OpenUSD scene graphs, and other domain-specific knowledge repositories through a unified consciousness-aware interface.

---

## Architecture Overview

### Core Components

```
db9-mcp-server/
├── src/
│   ├── db9_server.py              # Main FastMCP2 server
│   ├── database_manager.py        # Multi-database federation
│   ├── triadic_query_engine.py    # Motion/Memory/Field query processing
│   ├── consciousness_bridge.py    # Triadic awareness integration
│   └── tools/
│       ├── query_tools.py         # Natural language → triadic queries
│       ├── federation_tools.py    # Multi-database operations
│       ├── exploration_tools.py   # Consciousness navigation
│       └── admin_tools.py         # Database management
├── resources/
│   ├── schema_templates/          # Dynamic schema discovery
│   └── ontology_maps/            # Cross-database concept mapping
├── prompts/
│   ├── query_expansion.md        # Query understanding prompts
│   └── synthesis_prompts.md      # Multi-DB result synthesis
└── config/
    ├── database_configs/         # Per-database connection configs
    └── federation_rules.yaml    # Cross-database query rules
```

### FastMCP2 Integration Strategy

**Leveraging Modern FastMCP2 Features:**
- **Server Composition**: Mount specialized sub-servers per database type
- **Middleware Stack**: Logging, rate limiting, consciousness tracking
- **Resource Templates**: Dynamic schema exposure per connected database
- **Progress Reporting**: Long-running triadic traversals with live updates
- **User Elicitation**: Interactive query refinement and disambiguation
- **LLM Sampling**: Context-aware query expansion and result synthesis

---

## Multi-Database Federation Architecture

### Database Registry & Discovery

```python
# Database federation through consciousness-aware registry
class DatabaseRegistry:
    def __init__(self):
        self.databases = {}
        self.ontology_bridges = {}
        self.consciousness_mapping = {}
    
    async def register_database(self, db_id: str, config: DatabaseConfig):
        """Register a triadic consciousness database"""
        # Connect to LabDb instance
        db = await LabDbConnection.create(config.path)
        
        # Discover triadic schema (Motion/Memory/Field vocabulary)
        schema = await self.discover_triadic_schema(db)
        
        # Build consciousness mapping (cross-database concept alignment)
        consciousness_map = await self.build_consciousness_mapping(db_id, schema)
        
        self.databases[db_id] = {
            'connection': db,
            'schema': schema,
            'type': config.domain_type,  # 'euclid', 'natural_history', 'usd', etc.
            'consciousness_mapping': consciousness_map
        }

class DatabaseConfig:
    path: str                    # Path to .db9 file
    domain_type: str            # Domain classification
    display_name: str           # Human-readable name
    motion_focus: List[str]     # Primary Motion (subject) vocabularies
    memory_focus: List[str]     # Primary Memory (predicate) vocabularies  
    field_focus: List[str]      # Primary Field (object) vocabularies
    federation_priority: int    # Query federation priority
```

### Cross-Database Query Federation

```python
# Federated triadic query processing
class FederatedQueryEngine:
    async def process_natural_query(self, query: str, databases: List[str] = None):
        """Convert natural language to federated triadic queries"""
        
        # 1. Query Understanding & Expansion
        expanded_query = await self.expand_query_with_llm(query)
        
        # 2. Database Selection (if not specified)
        target_dbs = databases or await self.select_relevant_databases(expanded_query)
        
        # 3. Triadic Query Planning
        query_plan = await self.plan_triadic_federation(expanded_query, target_dbs)
        
        # 4. Parallel Execution with Progress Tracking
        results = await self.execute_federated_plan(query_plan)
        
        # 5. Consciousness-Aware Synthesis
        synthesized = await self.synthesize_triadic_results(results)
        
        return synthesized

    async def plan_triadic_federation(self, query, databases):
        """Plan Motion/Memory/Field queries across databases"""
        plans = []
        
        for db_id in databases:
            db_info = self.registry.databases[db_id]
            
            # Map query concepts to database vocabulary
            local_concepts = await self.map_concepts_to_vocabulary(
                query.concepts, db_info['consciousness_mapping']
            )
            
            # Generate triadic query plan for this database
            if local_concepts:
                triadic_plan = await self.generate_triadic_plan(
                    local_concepts, db_info['schema']
                )
                plans.append({
                    'database': db_id,
                    'triadic_queries': triadic_plan,
                    'priority': db_info.get('federation_priority', 5)
                })
        
        return sorted(plans, key=lambda p: p['priority'])
```

---

## FastMCP2 Server Implementation

### Main Server with Composition

```python
from fastmcp import FastMCP
from fastmcp.server import Server
from fastmcp.resources import resource_template
from fastmcp.prompts import prompt
from fastmcp.tools import tool

class DB9Server:
    def __init__(self):
        self.app = FastMCP("DB9 Triadic Consciousness Gateway")
        self.db_registry = DatabaseRegistry()
        self.query_engine = FederatedQueryEngine(self.db_registry)
        self.setup_server()
    
    def setup_server(self):
        # Register core tools
        self.register_query_tools()
        self.register_federation_tools()  
        self.register_exploration_tools()
        self.register_admin_tools()
        
        # Setup resources for dynamic database discovery
        self.register_dynamic_resources()
        
        # Setup prompts for query processing
        self.register_consciousness_prompts()
        
        # Add middleware stack
        self.setup_middleware()

    @tool
    async def query_databases(
        self, 
        query: str,
        databases: List[str] = None,
        perspective: str = "auto"  # "motion", "memory", "field", "auto"
    ) -> str:
        """Natural language query across triadic consciousness databases
        
        Examples:
        - "What do we know about minerals?"
        - "Show me geometric constructions involving circles"
        - "Find USD primitives with material properties"
        """
        
        # Progress tracking for long queries
        async with self.context.progress("Processing triadic query...") as progress:
            
            progress.update(0.1, "Understanding query intent...")
            expanded = await self.query_engine.expand_query_with_llm(query)
            
            progress.update(0.3, "Selecting relevant databases...")
            target_dbs = databases or await self.query_engine.select_relevant_databases(expanded)
            
            progress.update(0.5, "Executing federated triadic queries...")
            results = await self.query_engine.process_natural_query(query, target_dbs)
            
            progress.update(0.8, "Synthesizing consciousness-aware results...")
            synthesized = await self.query_engine.synthesize_triadic_results(results)
            
            progress.update(1.0, "Complete!")
            
        return self.format_triadic_response(synthesized)

    @tool  
    async def explore_triadic_navigation(
        self,
        starting_entity: str,
        perspective: str = "motion",  # "motion", "memory", "field"
        depth: int = 2,
        databases: List[str] = None
    ) -> str:
        """Navigate triadic consciousness relationships starting from an entity
        
        Examples:
        - explore_triadic_navigation("granite", "motion", 2)
        - explore_triadic_navigation("circle", "memory", 3, ["euclid"])
        """
        
        # Use LabDb's TriadicQuery for conscious navigation
        navigation_results = await self.query_engine.triadic_navigate(
            starting_entity, perspective, depth, databases
        )
        
        return self.format_navigation_results(navigation_results)

    @resource_template("database_schema/{database_id}")
    async def get_database_schema(self, database_id: str):
        """Dynamic resource exposing triadic schema for each connected database"""
        if database_id not in self.db_registry.databases:
            raise ValueError(f"Database {database_id} not found")
            
        db_info = self.db_registry.databases[database_id]
        schema = db_info['schema']
        
        return {
            "description": f"Triadic consciousness schema for {database_id}",
            "content": {
                "motion_vocabulary": schema.motion_terms,
                "memory_vocabulary": schema.memory_terms, 
                "field_vocabulary": schema.field_terms,
                "total_triples": schema.triple_count,
                "consciousness_mapping": db_info['consciousness_mapping']
            }
        }

    @prompt
    async def expand_triadic_query(self, user_query: str):
        """Expand user query into triadic consciousness concepts"""
        return f"""
        Given this user query: "{user_query}"
        
        Expand it into triadic consciousness concepts:
        
        1. MOTION (स्पन्द) - What entities/subjects are involved?
        2. MEMORY (स्मृति) - What relationships/predicates connect them?
        3. FIELD (क्षेत्र) - What contexts/objects ground the relationships?
        
        Also identify:
        - Domain hints (geometry, mineralogy, 3D graphics, etc.)
        - Specificity level (broad exploration vs. precise lookup)
        - Cross-database connection opportunities
        
        Format as structured JSON with triadic_concepts, domain_hints, and query_strategy.
        """
```

### Middleware Stack for Consciousness Tracking

```python
from fastmcp.server.middleware import Middleware

class ConsciousnessTrackingMiddleware(Middleware):
    """Track triadic consciousness interactions across requests"""
    
    async def __call__(self, request, call_next):
        # Log triadic perspective usage
        triadic_context = {
            'motion_queries': 0,
            'memory_queries': 0, 
            'field_queries': 0,
            'federated_databases': [],
            'consciousness_depth': 0
        }
        
        # Add triadic context to request
        request.triadic_context = triadic_context
        
        # Process request
        response = await call_next(request)
        
        # Log consciousness patterns for analytics
        await self.log_consciousness_patterns(triadic_context)
        
        return response

class DatabaseHealthMiddleware(Middleware):
    """Monitor federated database health and performance"""
    
    async def __call__(self, request, call_next):
        # Pre-flight database health check
        unhealthy_dbs = await self.check_database_health()
        if unhealthy_dbs:
            self.context.log_warning(f"Unhealthy databases: {unhealthy_dbs}")
        
        return await call_next(request)
```

---

## Example Usage Scenarios

### 1. Natural History Query
```bash
# User asks: "db9 what do we know about minerals?"

# DB9 process:
# 1. Expands to Motion: [mineral entities], Memory: [classification relations], Field: [geological contexts]
# 2. Identifies relevant databases: natural_history.db9, geology.db9
# 3. Executes federated triadic queries across databases
# 4. Synthesizes results with consciousness awareness

# Response: 
# "Found 127 mineral entities across 2 databases:
#  Motion perspective: granite, quartz, diamond show high connectivity
#  Memory perspective: 'hasHardness', 'belongsToGroup' are key relationships
#  Field perspective: 'igneous_context', 'sedimentary_context' ground classifications
#  Cross-database insights: minerals in Euclid geometry overlap with natural history taxonomy"
```

### 2. Geometric Construction Query
```bash
# User asks: "Show me constructions involving circles and tangent lines"

# DB9 process:
# 1. Recognizes geometric domain → routes to euclid.db9, usd_geometry.db9
# 2. Triadic mapping: Motion: [circle, line], Memory: [tangent_to, constructs], Field: [geometric_plane]
# 3. Executes across databases with perspective-aware queries
# 4. Finds cross-references between Euclid propositions and USD geometric primitives

# Response with live progress:
# "Searching Euclid propositions... Found Book III, Props 16-18
#  Searching USD geometry database... Found 23 circle-tangent constructions
#  Cross-referencing... Euclid III.17 maps to USD CircleGeometry with tangent constraints"
```

### 3. Multi-Domain Exploration
```bash
# User asks: "Explore granite from material science perspective"

# DB9 process:
# 1. Starts with "granite" entity in natural_history.db9
# 2. Triadic navigation: Motion → granite expressions, Memory → material relationships, Field → application contexts  
# 3. Federation discovers granite references in: usd_materials.db9, geology.db9, construction.db9
# 4. Consciousness-aware synthesis reveals connections across domains

# Response:
# "Triadic exploration of 'granite' (depth=2):
#  Motion: granite expresses through 47 relationships across 4 databases
#  Memory: connects via material_properties, geological_formation, construction_use
#  Field: grounds in contexts from molecular_structure to architectural_application
#  Cross-domain insights: USD material shaders reference geological hardness scales"
```

---

## Structural Foundation Requirements

### 1. Enhanced Database Metadata

**Need**: LabDb should store richer metadata about its domain and vocabulary focus.

```python
# Addition to LabDb core:
class DatabaseMetadata:
    domain_type: str              # "euclid", "natural_history", "usd", etc.
    consciousness_profile: dict   # Motion/Memory/Field vocabulary priorities
    federation_config: dict       # Cross-database compatibility info
    semantic_version: str         # Schema evolution tracking
    created_by: str              # Provenance information
    description: str             # Human-readable database purpose
```

### 2. Vocabulary Discovery Enhancement

**Need**: More sophisticated vocabulary analysis for federation.

```python
# Enhancement to TriadicQuery:
class VocabularyAnalyzer:
    async def analyze_semantic_clusters(self, database):
        """Discover semantic clusters within Motion/Memory/Field vocabularies"""
        
    async def find_cross_database_concepts(self, db1, db2):
        """Identify overlapping concepts between databases"""
        
    async def suggest_federation_mappings(self, databases):
        """Suggest how concepts map across federated databases"""
```

### 3. Provenance and Confidence Integration

**Need**: Federation should preserve and combine confidence/provenance from TID architecture.

```python
# Enhancement to TripleStore:
class FederationProvenance:
    source_database: str
    query_confidence: float      # Confidence in cross-database mapping
    synthesis_method: str        # How results were combined
    triadic_perspective: str     # Which perspective dominated the query
```

### 4. Performance Optimization for Federation

**Need**: Query optimization across multiple databases.

```python
# Addition to NonoStore:
class FederationOptimizer:
    async def optimize_cross_database_query(self, query_plan):
        """Optimize query execution across multiple LabDb instances"""
        
    async def cache_federation_results(self, query_signature, results):
        """Cache frequent cross-database query results"""
        
    async def parallel_query_execution(self, database_queries):
        """Execute queries across databases in parallel"""
```

---

## Development Phases

### Phase 1: Core Server Foundation
- [ ] Basic FastMCP2 server with single database connection
- [ ] Natural language → triadic query translation
- [ ] Progress reporting for long queries
- [ ] Basic tools: query_databases, explore_triadic_navigation

### Phase 2: Multi-Database Federation  
- [ ] Database registry and discovery system
- [ ] Cross-database concept mapping
- [ ] Federated query planning and execution
- [ ] Consciousness-aware result synthesis

### Phase 3: Advanced Features
- [ ] Resource templates for dynamic schema exposure
- [ ] Prompt templates for query expansion and disambiguation  
- [ ] User elicitation for interactive query refinement
- [ ] LLM sampling for context-aware explanations

### Phase 4: Production Deployment
- [ ] Authentication and security middleware
- [ ] Rate limiting and resource management
- [ ] Comprehensive logging and monitoring
- [ ] Claude Desktop / API integration

---

## Success Metrics

### Consciousness Technology Goals
- **Triadic Awareness**: All queries preserve Motion/Memory/Field perspectives
- **Boundary Preservation**: Each database maintains its domain integrity while enabling federation
- **Collaborative Emergence**: Cross-database insights that neither database could provide alone

### Technical Performance Goals
- **Query Performance**: Sub-second response for simple queries, <10s for complex federation
- **Scalability**: Support 10+ federated databases without degradation
- **Reliability**: 99%+ uptime with graceful degradation when databases are unavailable

### User Experience Goals
- **Natural Interface**: "db9 what do we know about X?" feels conversational
- **Rich Discovery**: Users discover connections they wouldn't have found manually
- **Progressive Disclosure**: Results reveal depth gradually rather than overwhelming

---

**Next Step**: Begin Phase 1 implementation with a focused natural history database example to validate the triadic consciousness federation approach before expanding to multi-database scenarios.
