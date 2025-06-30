#!/usr/bin/env python3
"""
LabDb Performance Benchmarking Suite - Python Edition
====================================================

Validates TID-based architecture efficiency using Euclid-inspired synthetic datasets.
Tests the predictions from analysis.md:
- "9× duplication explodes storage" → measure actual storage reduction
- "Prefix compression happy with binary keys" → validate LMDB optimization  
- "Mineral has 10k+ children" → test large fan-out scenarios

Usage:
    python3 benchmark_suite.py --quick     # Quick validation tests
    python3 benchmark_suite.py --full      # Comprehensive benchmarks
    python3 benchmark_suite.py --euclid    # Euclid-scale testing
"""

import sys
import os
import time
import random
import json
import argparse
import tempfile
from dataclasses import dataclass, asdict
from typing import List, Tuple, Dict, Optional
from pathlib import Path

# Add LabDb Python bindings to path
sys.path.insert(0, str(Path(__file__).parent.parent.parent / "python"))

try:
    import labdb
    LABDB_AVAILABLE = labdb._bindings_available if hasattr(labdb, '_bindings_available') else False
except ImportError:
    print("⚠️  LabDb Python bindings not available. Run benchmarks after compilation.")
    LABDB_AVAILABLE = False

@dataclass
class BenchmarkResult:
    """Result structure for individual benchmark runs"""
    test_name: str
    dataset_description: str
    triple_count: int
    insert_time_ms: float
    query_time_ms: float
    storage_bytes: int
    vocabulary_size: int
    triples_per_second: float
    queries_per_second: float
    memory_efficiency_ratio: float

class EuclidDatasetGenerator:
    """Generate Euclid-inspired synthetic datasets for benchmarking"""
    
    def __init__(self, seed: int = 42):
        random.seed(seed)
        
        # Euclid-inspired vocabulary (authentic ancient Greek mathematical terms)
        self.greek_terms = [
            "γραμμή", "σημεῖον", "κύκλος", "τρίγωνον", "τετράγωνον",
            "διάμετρος", "περίμετρος", "κέντρον", "γωνία", "πλευρά", 
            "ἴσος", "μεῖζον", "ἔλαττον", "παράλληλος", "κάθετος",
            "εὐθεῖα", "καμπύλη", "τομή", "συμμετρία", "ἀναλογία",
            "ἀπόδειξις", "θεώρημα", "λῆμμα", "ὅρος", "αἴτημα"
        ]
        
        self.english_terms = [
            "line", "point", "circle", "triangle", "square",
            "diameter", "perimeter", "center", "angle", "side",
            "equal", "greater", "less", "parallel", "perpendicular", 
            "straight", "curved", "section", "symmetry", "proportion",
            "proof", "theorem", "lemma", "definition", "postulate"
        ]
        
        self.predicates = [
            "defines", "part_of", "depends_on", "mentions", "proves",
            "constructs", "assumes", "demonstrates", "references", "extends",
            "isA", "hasProperty", "hasDefinition", "hasProof", "hasDiagram",
            "precedes", "follows", "requires", "establishes", "concludes"
        ]
    
    def generate_euclid_dataset(self, books: int, props_per_book: int, lines_per_prop: int) -> List[Tuple[str, str, str]]:
        """Generate Euclid-like hierarchical dataset: Books → Propositions → Lines"""
        triples = []
        
        for b in range(1, books + 1):
            book_id = f"b{b}"
            
            for p in range(1, props_per_book + 1):
                prop_id = f"{book_id}-p{p}"
                
                # Structural relationships
                triples.append((prop_id, "part_of", book_id))
                triples.append((prop_id, "source_line", str(b * 1000 + p)))
                triples.append((prop_id, "book_number", str(b)))
                
                # Add multilingual content (tri-language keys per Euclid MCP design)
                greek_content = self._generate_greek_text()
                english_content = self._generate_english_text()
                triples.append((f"{prop_id}#grc", "text", greek_content))
                triples.append((f"{prop_id}#eng", "text", english_content))
                
                # Add vocabulary relationships (simulates tokenization)
                for i, (greek_term, english_term) in enumerate(zip(self.greek_terms, self.english_terms)):
                    if random.random() < 0.3:  # 30% chance of term occurrence
                        triples.append((greek_term, "occurs_in", prop_id))
                        triples.append((greek_term, "english_gloss", english_term))
                        triples.append((prop_id, "mentions", greek_term))
                
                # Add dependency relationships (creates dependency graph)
                if p > 1:
                    dep_prop = random.randint(1, p - 1)
                    dep_id = f"{book_id}-p{dep_prop}"
                    triples.append((prop_id, "depends_on", dep_id))
                    
                # Cross-book dependencies (simulates "Prop. II.5" references)
                if b > 1 and random.random() < 0.2:
                    prev_book = random.randint(1, b - 1)
                    prev_prop = random.randint(1, props_per_book)
                    cross_ref = f"b{prev_book}-p{prev_prop}"
                    triples.append((prop_id, "references", cross_ref))
                
                # Add geometric concepts (simulates "defines sphere" relationships)
                if random.random() < 0.4:
                    pred = random.choice(self.predicates)
                    concept = random.choice(self.english_terms)
                    triples.append((prop_id, pred, concept))
                
                # Add proof structure
                triples.append((prop_id, "hasProof", f"proof_{prop_id}"))
                triples.append((f"proof_{prop_id}", "hasSteps", str(random.randint(3, 12))))
                
                # Add lines within proposition
                for l in range(1, lines_per_prop + 1):
                    line_id = f"{prop_id}-l{l}"
                    triples.append((line_id, "part_of", prop_id))
                    triples.append((line_id, "line_number", str(l)))
                    triples.append((line_id, "content", self._generate_line_content()))
        
        return triples
    
    def generate_high_fanout_dataset(self, root_concepts: int, children_per_root: int) -> List[Tuple[str, str, str]]:
        """Generate high fan-out dataset (tests 'mineral has 10k+ children' scenario)"""
        triples = []
        
        for r in range(1, root_concepts + 1):
            root = f"concept_{r}"
            root_name = random.choice(self.english_terms)
            triples.append((root, "name", root_name))
            triples.append((root, "isA", "root_concept"))
            
            for c in range(1, children_per_root + 1):
                child = f"instance_{r}_{c}"
                triples.append((child, "isA", root))
                triples.append((child, "hasId", str(c)))
                triples.append((child, "belongsTo", root))
                
                # Add properties to create richer graph structure
                prop = random.choice(self.predicates)
                value = random.choice(self.english_terms)
                triples.append((child, prop, value))
                
                # Add numeric properties
                triples.append((child, "hasValue", str(random.randint(1, 1000))))
                triples.append((child, "hasWeight", f"{random.uniform(0.1, 100.0):.2f}"))
                
                # Add cross-references within the same root
                if c > 10 and random.random() < 0.1:
                    sibling = f"instance_{r}_{random.randint(1, c-1)}"
                    triples.append((child, "relatedTo", sibling))
        
        return triples
    
    def generate_vocabulary_stress_dataset(self, vocab_terms: int, relations_per_term: int) -> List[Tuple[str, str, str]]:
        """Generate dataset that stresses vocabulary indices (S*/P*/O*)"""
        triples = []
        
        # Create large vocabulary
        terms = [f"term_{i}" for i in range(vocab_terms)]
        
        for term in terms:
            # Each term participates in multiple relationships
            for r in range(relations_per_term):
                predicate = f"relation_{r % 20}"  # Reuse predicates
                target = random.choice(terms)
                triples.append((term, predicate, target))
                
                # Add metadata
                triples.append((term, "hasIndex", str(terms.index(term))))
                triples.append((term, "category", f"category_{r % 5}"))
        
        return triples
    
    def _generate_greek_text(self) -> str:
        """Generate pseudo-Greek proposition text"""
        word_count = random.randint(15, 40)
        words = [random.choice(self.greek_terms) for _ in range(word_count)]
        return " ".join(words) + "."
    
    def _generate_english_text(self) -> str:
        """Generate English proposition text"""
        word_count = random.randint(12, 35)
        words = [random.choice(self.english_terms) for _ in range(word_count)]
        return " ".join(words) + "."
    
    def _generate_line_content(self) -> str:
        """Generate individual line content"""
        word_count = random.randint(5, 15)
        words = [random.choice(self.english_terms) for _ in range(word_count)]
        return " ".join(words)

if __name__ == "__main__":
    # For now, just test the generator
    print("🧪 LabDb Benchmark Suite - Dataset Generator Test")
    print("=" * 50)
    
    generator = EuclidDatasetGenerator()
    
    # Test small dataset generation
    print("\n📚 Testing Euclid dataset generation...")
    small_dataset = generator.generate_euclid_dataset(2, 5, 3)
    print(f"Generated {len(small_dataset)} triples for 2 books × 5 props × 3 lines")
    
    # Show sample triples
    print("\nSample triples:")
    for i, (s, p, o) in enumerate(small_dataset[:10]):
        print(f"  {i+1}. ({s}, {p}, {o})")
    
    print("\n📊 Testing high fan-out generation...")
    fanout_dataset = generator.generate_high_fanout_dataset(3, 100)
    print(f"Generated {len(fanout_dataset)} triples for 3 concepts × 100 children")
    
    print("\n📝 Testing vocabulary stress generation...")
    vocab_dataset = generator.generate_vocabulary_stress_dataset(50, 5)
    print(f"Generated {len(vocab_dataset)} triples for 50 terms × 5 relations")
    
    if LABDB_AVAILABLE:
        print("\n✅ LabDb bindings available - ready for full benchmarking!")
        print("Run with --quick, --full, or --euclid for performance testing")
    else:
        print("\n⚠️  LabDb bindings not available - compile first for full benchmarking")
        print("Current test validates dataset generation only")
    
    print("\n🎯 Dataset generation successful - ready for benchmarking!")
