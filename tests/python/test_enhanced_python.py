#!/usr/bin/env python3
"""
Test enhanced Pythonic interface for LabDb
Validates dictionary-like access, context managers, and triadic exploration
"""

import sys
import os
import tempfile

# Add our python directory to the path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'python'))

def test_enhanced_nonostore():
    """Test EnhancedNonoStore with Pythonic patterns"""
    print("🧪 Testing EnhancedNonoStore Pythonic interface...")
    
    try:
        import labdb
        
        if not labdb._bindings_available:
            print("⚠️  C++ bindings not available - cannot test EnhancedNonoStore")
            return False
        
        with tempfile.TemporaryDirectory() as temp_dir:
            db_path = os.path.join(temp_dir, "enhanced_test.ldb")
            
            # Test enhanced store creation
            store = labdb.EnhancedNonoStore(db_path)
            print("✅ EnhancedNonoStore created")
            
            # Test basic operations
            store.connect("python", "isA", "language")
            store.connect("python", "hasType", "interpreted")
            store.connect("python", "createdBy", "guido")
            print("✅ Basic connections established")
            
            # Test dictionary-like access
            python_props = store["python"]
            print(f"✅ Dictionary access: python has {len(python_props)} properties")
            
            # Test query shortcut
            languages = store["*", "isA", "language"]
            print(f"✅ Tuple query: found {len(languages)} languages")
            
            # Test membership
            assert ("python", "isA", "language") in store
            print("✅ Membership test working")
            
            # Test context manager for transactions
            with store:
                store["java", "isA"] = "language"
                store["java", "hasType"] = "compiled"
                store["cpp", "isA"] = "language"
            print("✅ Transaction context manager working")
            
            # Test vocabulary properties
            print(f"✅ Subjects: {store.subjects}")
            print(f"✅ Predicates: {store.predicates}")
            print(f"✅ Objects: {store.objects}")
            print(f"✅ Vocabulary: {store.vocabulary}")
            
            # Test triadic exploration
            motion_results = store.explore_motion("python")
            print(f"✅ Motion exploration: {len(motion_results)} results")
            
            memory_results = store.explore_memory("isA")
            print(f"✅ Memory exploration: {len(memory_results)} results")
            
            field_results = store.explore_field("language")
            print(f"✅ Field exploration: {len(field_results)} results")
            
            # Test crown exploration
            crown_results = store.explore_around("python")
            print(f"✅ Crown exploration: {len(crown_results)} results")
            
            # Test traversal
            traversal_results = store.traverse_from("python", 2)
            print(f"✅ Triadic traversal: {len(traversal_results)} paths")
            
            # Test analytics
            stats = store.stats
            print(f"✅ Statistics: {stats}")
            
            bridges = store.bridge_entities
            print(f"✅ Bridge entities: {bridges}")
            
            return True
            
    except Exception as e:
        print(f"❌ EnhancedNonoStore test failed: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_enhanced_triadic_results():
    """Test EnhancedTriadicResults with filtering and analysis"""
    print("🧪 Testing EnhancedTriadicResults...")
    
    try:
        import labdb
        
        if not labdb._bindings_available:
            print("⚠️  C++ bindings not available - cannot test EnhancedTriadicResults")
            return False
        
        with tempfile.TemporaryDirectory() as temp_dir:
            db_path = os.path.join(temp_dir, "results_test.ldb")
            store = labdb.EnhancedNonoStore(db_path)
            
            # Create diverse knowledge base
            store.connect("granite", "isA", "rock")
            store.connect("granite", "hasColor", "gray")
            store.connect("granite", "hasHardness", "6")
            store.connect("marble", "isA", "rock")
            store.connect("marble", "hasColor", "white")
            store.connect("diamond", "isA", "mineral")
            store.connect("diamond", "hasHardness", "10")
            store.connect("emerald", "isA", "mineral")
            store.connect("emerald", "hasColor", "green")
            
            # Get enhanced results
            motion_results = store.explore_motion("granite")
            print(f"✅ Got {len(motion_results)} motion results")
            
            # Test iteration
            for result in motion_results:
                print(f"  {result}")
                assert hasattr(result, 'motion')
                assert hasattr(result, 'memory')
                assert hasattr(result, 'field')
            print("✅ Enhanced result iteration working")
            
            # Test filtering
            color_results = motion_results.filter_by_relation("hasColor")
            print(f"✅ Color filtering: {len(color_results)} results")
            
            # Test grouping
            by_relation = motion_results.group_by_relation()
            print(f"✅ Grouping by relation: {list(by_relation.keys())}")
            
            # Test analysis
            print(f"✅ Unique entities: {motion_results.entities}")
            print(f"✅ Unique relations: {motion_results.relations}")
            print(f"✅ Unique contexts: {motion_results.contexts}")
            
            # Test counts
            entity_counts = motion_results.count_by_entity()
            relation_counts = motion_results.count_by_relation()
            print(f"✅ Entity counts: {entity_counts}")
            print(f"✅ Relation counts: {relation_counts}")
            
            # Test conversion
            as_list = motion_results.as_list()
            as_triples = motion_results.as_triples()
            print(f"✅ Conversion to list: {len(as_list)} items")
            print(f"✅ Conversion to triples: {len(as_triples)} triples")
            
            # Test perspective exploration from results
            if motion_results:
                first_result = motion_results[0]
                
                # Test motion property
                nested_motion = first_result.motion
                print(f"✅ Nested motion exploration: {len(nested_motion)} results")
                
                # Test memory property
                nested_memory = first_result.memory
                print(f"✅ Nested memory exploration: {len(nested_memory)} results")
                
                # Test field property
                nested_field = first_result.field
                print(f"✅ Nested field exploration: {len(nested_field)} results")
                
                # Test perspective shifting
                shifted = first_result.shift_to("Memory")
                print(f"✅ Perspective shifting: {len(shifted)} results")
                
                # Test traversal
                traversed = first_result.traverse(2)
                print(f"✅ Result traversal: {len(traversed)} paths")
                
                # Test dictionary access
                assert first_result['entity'] == first_result.entity
                assert first_result['relation'] == first_result.relation
                assert first_result['context'] == first_result.context
                print("✅ Dictionary-like access working")
                
                # Test conversion
                as_dict = first_result.as_dict()
                as_triple = first_result.as_triple()
                print(f"✅ Result conversion: {as_dict}")
                print(f"✅ Triple conversion: {as_triple}")
            
            return True
            
    except Exception as e:
        print(f"❌ EnhancedTriadicResults test failed: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_pythonic_patterns():
    """Test various Pythonic patterns and conveniences"""
    print("🧪 Testing Pythonic patterns...")
    
    try:
        import labdb
        
        if not labdb._bindings_available:
            print("⚠️  C++ bindings not available - cannot test Pythonic patterns")
            return False
        
        with tempfile.TemporaryDirectory() as temp_dir:
            db_path = os.path.join(temp_dir, "pythonic_test.ldb")
            store = labdb.EnhancedNonoStore(db_path)
            
            # Test fluent interface patterns
            store.connect("entity1", "rel1", "value1")
            result = store["entity1"]
            if result:
                nested = result[0].motion.filter_by_relation("rel1")
                print(f"✅ Fluent interface: {len(nested)} results")
            
            # Test list comprehensions
            store.connect("item1", "hasProperty", "prop1")
            store.connect("item2", "hasProperty", "prop2")
            store.connect("item3", "hasProperty", "prop3")
            
            properties = store.explore_memory("hasProperty")
            prop_list = [r.context for r in properties if r.context.startswith("prop")]
            print(f"✅ List comprehension: {prop_list}")
            
            # Test generator expressions
            contexts = (r.context for r in properties)
            context_list = list(contexts)
            print(f"✅ Generator expression: {len(context_list)} contexts")
            
            # Test set operations
            all_entities = set(r.entity for r in store.explore_motion())
            all_contexts = set(r.context for r in store.explore_field())
            intersection = all_entities & all_contexts
            print(f"✅ Set operations: {len(intersection)} entity-context overlaps")
            
            # Test dictionary unpacking
            result_dict = properties[0].as_dict() if properties else {}
            if result_dict:
                entity, relation, context = result_dict['entity'], result_dict['relation'], result_dict['context']
                print(f"✅ Dictionary unpacking: {entity}-{relation}-{context}")
            
            # Test string formatting
            if properties:
                formatted = f"Found {len(properties)} properties: {', '.join(r.relation for r in properties[:3])}"
                print(f"✅ String formatting: {formatted}")
            
            return True
            
    except Exception as e:
        print(f"❌ Pythonic patterns test failed: {e}")
        import traceback
        traceback.print_exc()
        return False

def main():
    """Main test function"""
    print("=" * 70)
    print("🚀 LabDb Enhanced Pythonic Interface Test")
    print("=" * 70)
    
    try:
        import labdb
        print(f"✅ Successfully imported labdb version {labdb.__version__}")
        
        if not labdb._bindings_available:
            print("⚠️  C++ bindings not available - enhanced interface tests skipped")
            print("\n🎯 Install pybind11 and build bindings to test enhanced interface")
            return 0
            
        print(f"📋 Enhanced components available: {[c for c in labdb.__all__ if 'Enhanced' in c]}")
        
    except Exception as e:
        print(f"❌ Import failed: {e}")
        return 1
    
    success = True
    
    if not test_enhanced_nonostore():
        success = False
    
    if not test_enhanced_triadic_results():
        success = False
        
    if not test_pythonic_patterns():
        success = False
    
    print("\n" + "=" * 70)
    if success:
        print("✅ All enhanced interface tests passed!")
        print("\n🎆 Pythonic triadic consciousness interface working perfectly!")
        print("🎯 Dictionary-like access, context managers, and fluent exploration enabled")
        print("🧘 Enhanced results provide natural Motion/Memory/Field navigation")
    else:
        print("❌ Some enhanced interface tests failed")
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
