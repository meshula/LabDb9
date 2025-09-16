# Euclid Nonostore Ontology Design

**Version**: 1.0  
**Date**: September 2025  
**Purpose**: Foundational ontological framework for Euclid corpus in LabDb9 nonostore  
**TDD Environment**: `/Users/nick/dev/Lab/LabDb9/tests/cpp/testenv/euclid/`

## Triadic Consciousness Foundation

This ontology implements त्रित्रयम् (triadic consciousness) principles for scholarly navigation through Euclid's Elements, enabling Motion/Memory/Field exploration of mathematical knowledge.

### Motion (स्पन्द - Spanda): Knowledge Unfolding
**How mathematical understanding develops through logical progression**
- Definitional foundations → Postulates → Propositions → Applications
- Construction sequences: point → line → triangle → complex figures
- Proof methodologies: direct proof → reductio → construction

### Memory (स्मृति - Smriti): Conceptual Connections  
**How mathematical concepts relate and reference each other**
- Cross-proposition dependencies: "as shown in Proposition II.5"
- Thematic relationships: proportion theory across Books 5, 6, 12
- Methodological patterns: similar triangle proofs, circle theorems

### Field (क्षेत्र - Kshetra): Foundational Grounding
**How mathematical knowledge grounds in axiomatic foundations**
- Postulate foundations: straight line construction, circle construction
- Definitional scaffolding: point → line → plane → solid
- Book organization: plane geometry → arithmetic → solid geometry

## Entity Ontology

### Primary Entity Types

#### 1. Text Fragments
**Purpose**: Atomic units of Euclid's text with complete provenance and metadata

**Entity ID Pattern**: `{book}-{type}{number}`
- `b1-def-1` = Book 1, Definition 1 (point definition)
- `b6-def-3` = Book 6, Definition 3 (golden mean definition)  
- `b1-p47` = Book 1, Proposition 47 (Pythagorean theorem)
- `b1-post-1` = Book 1, Postulate 1
- `b1-cn-1` = Book 1, Common Notion 1

**Content Structure**:
```
Entity ID: b6-def-3
Content: "ἄκρον καὶ μέσον λόγον εὐθεῖα τετμῆσθαι λέγεται, ὅταν ᾖ ὡς ἡ ὅλη πρὸς τὸ μεῖζον τμῆμα, οὕτως τὸ μεῖζον πρὸς τὸ ἔλαττον."
```

#### 2. Semantic Concepts
**Purpose**: Mathematical concepts that cross-reference multiple text fragments

**Entity ID Pattern**: `concept-{name}`
- `concept-golden-ratio` = Golden ratio/extreme and mean ratio
- `concept-proportion` = Theory of proportions
- `concept-similar-triangles` = Similar triangle relationships
- `concept-circle-construction` = Circle construction methods

#### 3. Book Structures  
**Purpose**: Organizational entities for hierarchical navigation

**Entity ID Pattern**: `book-{number}`
- `book-1` = Book 1 (Fundamentals of plane geometry)
- `book-6` = Book 6 (Plane geometry and proportion)
- `book-13` = Book 13 (Regular solids)

#### 4. Field Classifications
**Purpose**: Logical role classification for discovery patterns

**Entity ID Pattern**: `field-{type}`
- `field-definition` = Definitional statements
- `field-proposition` = Theorems and constructions
- `field-proof` = Logical demonstrations  
- `field-postulate` = Fundamental assumptions
- `field-common-notion` = Basic logical principles

## Relationship Ontology

### Structural Relationships (Memory)

#### Containment Hierarchy
```lisp
;; Book organization
(b6-def-3 part-of book-6)
(b1-p47 part-of book-1)

;; Logical structure  
(b6-p30 builds-on b6-def-3)
(b1-p47 requires b1-p46)
```

#### Sequential Dependencies
```lisp
;; Logical progression
(b1-p2 follows b1-p1)
(b6-def-4 follows b6-def-3)

;; Proof dependencies
(b1-p47 depends-on b1-def-15)  ; uses circle definition
(b6-p30 depends-on b6-def-3)   ; constructs golden mean
```

### Semantic Relationships (Field)

#### Conceptual Associations
```lisp
;; Mathematical concepts
(b6-def-3 embodies concept-golden-ratio)
(b6-def-3 relates-to concept-proportion)
(b1-p47 embodies concept-pythagorean-theorem)

;; Methodological patterns
(b1-p4 exemplifies proof-by-superposition)
(b3-p1 exemplifies circle-construction)
```

#### Cross-Book Connections
```lisp
;; Thematic development
(b2-p11 develops-concept concept-golden-ratio)
(b6-def-3 defines-concept concept-golden-ratio)
(b13-p8 applies-concept concept-golden-ratio)

;; Methodological echoes
(b1-triangle-constructions similar-method b4-triangle-constructions)
```

### Functional Relationships (Motion)

#### Knowledge Development
```lisp
;; How understanding unfolds
(b1-definitions establishes foundation-for b1-propositions)
(b5-proportion-theory enables b6-similarity-theory)
(plane-geometry-books ground solid-geometry-books)
```

#### Discovery Pathways
```lisp
;; Study progression patterns  
(b1-basic-constructions leads-to b2-transformation-geometry)
(b6-similarity prepares-for b12-circle-area-theory)
(b10-incommensurables culminates-in b13-regular-solids)
```

## Vocabulary Architecture

### Subject Vocabulary (Motion)
**What entities express dynamic action**
- Text fragments: `b1-p1`, `b6-def-3`, `b13-p18`
- Concepts: `concept-golden-ratio`, `concept-circle-construction`
- Books: `book-1`, `book-6`, `book-13`

### Predicate Vocabulary (Memory)  
**What relationships connect entities**

#### Structural Predicates
- `part-of` - Hierarchical containment
- `follows` - Sequential ordering
- `depends-on` - Logical prerequisites
- `builds-on` - Conceptual development

#### Semantic Predicates  
- `embodies` - Core concept manifestation
- `relates-to` - Conceptual association
- `exemplifies` - Methodological pattern
- `develops-concept` - Conceptual progression

#### Functional Predicates
- `establishes` - Foundational grounding
- `enables` - Methodological preparation  
- `leads-to` - Discovery progression
- `culminates-in` - Conceptual completion

### Object Vocabulary (Field)
**What contexts ground understanding**
- Concepts: `concept-golden-ratio`, `concept-proportion`
- Field types: `field-definition`, `field-proposition`
- Books: `book-1`, `book-6`
- Methods: `proof-by-superposition`, `circle-construction`

## Memex Integration Patterns

### Discovery Queries
```lisp
;; Find all golden ratio content
(find-triple-enhanced :subject "*" :predicate "relates-to" :object "concept-golden-ratio")

;; Find definitional foundations
(find-triple-enhanced :subject "*" :predicate "field-type" :object "field-definition")

;; Find cross-book concept development
(find-triple-enhanced :subject "*" :predicate "develops-concept" :object "concept-golden-ratio")
```

### Bounded Navigation Patterns
```lisp
;; Entry point discovery → chunk retrieval
(memex-discover :query "golden ratio" :max-results 5)
;; Returns: [b6-def-3, b2-p11, b6-p30, b13-p8, ...]

(memex-chunk :center-entity "b6-def-3" :fragment-count 24)  
;; Returns: 24 fragments around golden mean definition with context
```

### Associative Trail Construction
```lisp
;; Create study trail for proportion theory
(memex-trail-create :name "proportion-theory-development"
                   :entry-points ["b5-def-3" "b6-def-3" "b6-p1" "b12-p2"]
                   :context-fragments 24)
```

## Minimal Seed Data Design

### Phase 1: Essential Triadic Foundation
**Purpose**: Establish minimal working nonostore for TDD validation

#### Core Entities (12 entities)
```lisp
;; Foundational definitions
(add-entity :value "σημεῖόν ἐστιν, οὗ μέρος οὐθέν." :dbid db1)  ; b1-def-1 (point)
(add-entity :value "γραμμὴ δὲ μῆκος ἀπλατές." :dbid db1)        ; b1-def-2 (line)

;; Golden ratio definition (key test case)
(add-entity :value "ἄκρον καὶ μέσον λόγον εὐθεῖα τετμῆσθαι λέγεται..." :dbid db1)  ; b6-def-3

;; Pythagorean theorem (iconic proposition)  
(add-entity :value "ἐν τοῖς ὀρθογωνίοις τριγώνοις..." :dbid db1)  ; b1-p47

;; Semantic concepts
(add-entity :value "golden-ratio" :dbid db1)      ; concept-golden-ratio
(add-entity :value "proportion" :dbid db1)        ; concept-proportion  
(add-entity :value "triangle" :dbid db1)          ; concept-triangle

;; Field classifications
(add-entity :value "definition" :dbid db1)        ; field-definition
(add-entity :value "proposition" :dbid db1)       ; field-proposition

;; Book organization
(add-entity :value "book-1" :dbid db1)            ; book-1
(add-entity :value "book-6" :dbid db1)            ; book-6
```

#### Core Relationships (15 triples)
```lisp
;; Field type classification
(add-triple-semantic :subject "b1-def-1" :predicate "field-type" :object "definition" :dbid db1)
(add-triple-semantic :subject "b6-def-3" :predicate "field-type" :object "definition" :dbid db1) 
(add-triple-semantic :subject "b1-p47" :predicate "field-type" :object "proposition" :dbid db1)

;; Book organization
(add-triple-semantic :subject "b1-def-1" :predicate "part-of" :object "book-1" :dbid db1)
(add-triple-semantic :subject "b1-p47" :predicate "part-of" :object "book-1" :dbid db1)
(add-triple-semantic :subject "b6-def-3" :predicate "part-of" :object "book-6" :dbid db1)

;; Semantic concepts  
(add-triple-semantic :subject "b6-def-3" :predicate "embodies" :object "golden-ratio" :dbid db1)
(add-triple-semantic :subject "b6-def-3" :predicate "relates-to" :object "proportion" :dbid db1)
(add-triple-semantic :subject "b1-p47" :predicate "embodies" :object "triangle" :dbid db1)

;; Sequential relationships
(add-triple-semantic :subject "b1-def-2" :predicate "follows" :object "b1-def-1" :dbid db1)
(add-triple-semantic :subject "b1-p47" :predicate "depends-on" :object "b1-def-1" :dbid db1)

;; Cross-book concept development  
(add-triple-semantic :subject "b6-def-3" :predicate "develops-concept" :object "golden-ratio" :dbid db1)
```

### Phase 2: Rope Integration (Minimal)
**Purpose**: Test bounded chunk retrieval with minimal rope

#### Global Text Rope (5 fragments)
```lisp
;; Create minimal rope for testing
(rope-create :name "euclid-minimal-test" :description "Minimal test rope for TDD")

;; Add fragments in logical order
(rope-append :rope "euclid-minimal-test" :entity "b1-def-1")    ; position 1
(rope-append :rope "euclid-minimal-test" :entity "b1-def-2")    ; position 2  
(rope-append :rope "euclid-minimal-test" :entity "b6-def-3")    ; position 3
(rope-append :rope "euclid-minimal-test" :entity "b1-p47")     ; position 4
```

#### Test Bounded Retrieval
```lisp
;; Test chunk retrieval around golden mean
(rope-chunk :rope "euclid-minimal-test" :center-entity "b6-def-3" :fragment-count 3)
;; Expected: [b1-def-2, b6-def-3, b1-p47] with navigation metadata
```

## TDD Validation Patterns

### Test 1: Entity Storage & Retrieval
```cpp
TEST_F(EuclidOntologyTest, StoreTextFragmentWithMetadata) {
    auto entity_id = store_entity("b6-def-3", golden_mean_definition_text);
    
    EXPECT_EQ(get_entity_content(entity_id), golden_mean_definition_text);
    EXPECT_TRUE(entity_exists("b6-def-3"));
}
```

### Test 2: Semantic Discovery  
```cpp
TEST_F(EuclidOntologyTest, DiscoverGoldenRatioContent) {
    auto results = find_triple_enhanced("*", "relates-to", "golden-ratio");
    
    EXPECT_GE(results.size(), 1);
    EXPECT_THAT(extract_subjects(results), Contains("b6-def-3"));
}
```

### Test 3: Bounded Navigation
```cpp  
TEST_F(EuclidOntologyTest, ChunkRetrievalAroundGoldenMean) {
    auto chunk = get_chunk_around_entity("b6-def-3", 3);
    
    EXPECT_EQ(chunk.center_entity, "b6-def-3");
    EXPECT_EQ(chunk.fragments.size(), 3);
    EXPECT_THAT(chunk.concatenated_text, HasSubstr("ἄκρον καὶ μέσον"));
}
```

### Test 4: Triadic Navigation
```cpp
TEST_F(EuclidOntologyTest, TriadicConceptExploration) {
    // Motion: How does golden ratio concept unfold?
    auto motion = motion_from("golden-ratio");
    EXPECT_THAT(motion.related_entities, Contains("b6-def-3"));
    
    // Memory: How do proportion concepts connect?
    auto memory = memory_relations("proportion");  
    EXPECT_GE(memory.connections.size(), 2);
    
    // Field: How does definition ground in book structure?
    auto field = field_contexts("b6-def-3");
    EXPECT_THAT(field.grounding_contexts, Contains("book-6"));
}
```

## Implementation Strategy

### Phase 1: Ontological Foundation (Week 1)
1. **Create minimal seed database** with 12 entities + 15 triples
2. **Validate triadic discovery** patterns work correctly  
3. **Test entity storage/retrieval** with Greek Unicode content
4. **Confirm vocabulary discovery** finds expected predicates/objects

### Phase 2: Rope Integration (Week 2)  
1. **Create minimal rope** with 5 text fragments
2. **Test bounded chunk retrieval** around golden mean definition
3. **Validate navigation metadata** for chunk extension
4. **Confirm text concatenation** preserves Unicode properly

### Phase 3: Scaling Preparation (Week 3)
1. **Performance test** with 100 entities + 200 triples
2. **Batch ingestion patterns** for full corpus processing
3. **Metadata extraction** from YAML source patterns
4. **Cross-reference detection** algorithms

### Phase 4: Full Corpus Ingestion (Week 4+)
1. **Complete entity ingestion** (~10,000 text fragments)
2. **Comprehensive relationship mapping** 
3. **Global rope construction** with XML line ordering
4. **Advanced semantic analysis** for cross-references

## Success Criteria

### Ontological Coherence ✅
- Triadic navigation patterns successfully discover related content
- Vocabulary architecture enables effective semantic exploration  
- Field type classification supports targeted discovery queries

### Technical Performance ✅  
- Unicode Greek text stored and retrieved without corruption
- Bounded chunk assembly maintains logical reading flow
- Response time targets met for discovery and navigation operations

### Scholarly Utility ✅
- "Find golden ratio content" returns relevant fragments across books
- Study trail construction enables coherent thematic exploration
- Cross-reference patterns support deep scholarly investigation

---

**Status**: Ontological framework complete and ready for TDD implementation. Minimal seed data designed for immediate testing and validation.
