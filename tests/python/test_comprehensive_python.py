#!/usr/bin/env python3
"""
Comprehensive test for LabDb Python bindings
Tests both NonoStore and TriadicQuery functionality
"""

import sys
import os
import tempfile

# Add our python directory to the path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'python'))
# Add build directory for compiled bindings
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'build'))

def test_triadic_python_integration():
    """Test complete triadic functionality through Python"""
    print("🧪 Testing TriadicQuery Python integration...")
    
    try:
        import labdb
        
        if not labdb._bindings_available:
            print("⚠️  C++ bindings not available - cannot test TriadicQuery")
            return False
        
        # Create temporary database
        with tempfile.TemporaryDirectory() as temp_dir:
            db_path = os.path.join(temp_dir, "triadic_test.ldb")
            
            # Create NonoStore and populate with triadic data
            store = labdb.NonoStore(db_path)
            
            # Create some knowledge relationships
            store.connect("granite", "isA", "rock")
            store.connect("granite", "hasColor", "gray")
            store.connect("granite", "hasHardness", "6")
            store.connect("marble", "isA", "rock")
            store.connect("marble", "hasColor", "white")
            store.connect("diamond", "isA", "mineral")
            store.connect("diamond", "hasHardness", "10")
            
            print("✅ Knowledge base created with triadic relationships")
            
            # Create TriadicQuery instance using factory pattern (solves pybind11 holder type issues)
            triadic = store.create_triadic_query()
            print("✅ TriadicQuery instance created using factory pattern")
            
            # Test Motion queries (स्पन्द perspective)
            motion_results = triadic.motion_from("granite")
            print(f"✅ Motion from granite: {len(motion_results)} expressions")
            
            expressions = triadic.entity_expressions("granite")
            print(f"✅ Granite expressions: {expressions}")
            
            # Test Memory queries (स्मृति perspective)
            memory_results = triadic.memory_relations("isA")
            print(f"✅ Memory relations for 'isA': {len(memory_results)} connections")
            
            frequencies = triadic.relation_frequencies()
            print(f"✅ Relation frequencies: {len(frequencies)} patterns")
            
            # Test Field queries (क्षेत्र perspective)
            field_results = triadic.field_contexts("rock")
            print(f"✅ Field contexts for 'rock': {len(field_results)} groundings")
            
            primary_contexts = triadic.primary_contexts()
            print(f"✅ Primary contexts: {primary_contexts}")
            
            # Test cube navigation
            crown_results = triadic.crown_exploration("granite")
            print(f"✅ Crown exploration around granite: {len(crown_results)} connections")
            
            traversal_results = triadic.triadic_traverse("granite", 2)
            print(f"✅ Triadic traversal from granite: {len(traversal_results)} paths")
            
            # Test analytics
            stats = triadic.get_triadic_stats()
            print(f"✅ Triadic stats: {stats}")
            
            bridge_entities = triadic.bridge_entities(1.0)
            print(f"✅ Bridge entities: {bridge_entities}")
            
            # Test perspective constants
            print(f"✅ Motion perspective: {labdb.Perspective.Motion}")
            print(f"✅ Memory perspective: {labdb.Perspective.Memory}")
            print(f"✅ Field perspective: {labdb.Perspective.Field}")
            
            # Test utility functions
            perspective_name = labdb.perspective_name(labdb.Perspective.Motion)
            sanskrit_name = labdb.perspective_sanskrit(labdb.Perspective.Motion)
            print(f"✅ Perspective names: {perspective_name} / {sanskrit_name}")
            
            optimal_p = labdb.optimal_perspective("granite", "*", "*")
            print(f"✅ Optimal perspective for granite-*-*: {optimal_p}")
            
            return True
            
    except Exception as e:
        print(f"❌ TriadicQuery integration test failed: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_pythonic_interface():
    """Test that the Python interface feels natural"""
    print("🧪 Testing Pythonic interface design...")
    
    try:
        import labdb
        
        if not labdb._bindings_available:
            print("⚠️  C++ bindings not available - cannot test Pythonic interface")
            return False
        
        with tempfile.TemporaryDirectory() as temp_dir:
            db_path = os.path.join(temp_dir, "pythonic_test.ldb")
            store = labdb.NonoStore(db_path)
            
            # Test that results are iterable
            store.connect("python", "isA", "language")
            store.connect("python", "hasType", "interpreted")
            
            results = store.query("python", "*", "*")
            print(f"✅ Query results are iterable: {len(results)} results")
            
            for triple in results:
                print(f"  {triple}")
                # Test Triple repr
                assert "Triple(" in str(triple)
                assert "python" in str(triple)
            
            # Test vocabulary discovery as lists
            subjects = store.all_subjects()
            predicates = store.all_predicates() 
            objects = store.all_objects()
            
            assert isinstance(subjects, list)
            assert isinstance(predicates, list)
            assert isinstance(objects, list)
            print("✅ Vocabulary methods return Python lists")
            
            # Test triadic result iteration
            triadic = store.create_triadic_query()
            motion_results = triadic.motion_from("python")
            
            for result in motion_results:
                print(f"  TriadicResult: {result}")
                # Test that TriadicResult has proper attributes
                assert hasattr(result, 'entity')
                assert hasattr(result, 'relation')
                assert hasattr(result, 'context')
                assert hasattr(result, 'discovered_through')
            
            print("✅ TriadicResult iteration and attributes working")
            
            return True
            
    except Exception as e:
        print(f"❌ Pythonic interface test failed: {e}")
        import traceback
        traceback.print_exc()
        return False

def main():
    """Main test function"""
    print("=" * 70)
    print("🚀 LabDb Comprehensive Python Integration Test")
    print("=" * 70)
    
    # Import basic test
    try:
        import labdb
        print(f"✅ Successfully imported labdb version {labdb.__version__}")
        print(f"📋 Available components: {labdb.__all__}")
        print(f"🔧 Bindings available: {getattr(labdb, '_bindings_available', False)}")
    except Exception as e:
        print(f"❌ Failed to import labdb: {e}")
        return 1
    
    # Test perspective constants (always available)
    try:
        assert labdb.Perspective.MOTION == "Motion"
        assert labdb.Perspective.MEMORY == "Memory"
        assert labdb.Perspective.FIELD == "Field"
        print("✅ Triadic perspective constants working")
    except Exception as e:
        print(f"❌ Perspective constants failed: {e}")
        return 1
    
    success = True
    
    if hasattr(labdb, '_bindings_available') and labdb._bindings_available:
        print("\n🔌 C++ bindings detected - running full integration tests")
        
        if not test_triadic_python_integration():
            success = False
            
        if not test_pythonic_interface():
            success = False
    else:
        print("\n⚠️  C++ bindings not available - testing graceful fallback")
        
        try:
            store = labdb.NonoStore("/tmp/test")
            print("❌ Expected error for missing bindings")
            success = False
        except RuntimeError as e:
            if "C++ bindings not available" in str(e):
                print("✅ Graceful fallback working with helpful error")
            else:
                print(f"❌ Unexpected error: {e}")
                success = False
    
    print("\n" + "=" * 70)
    if success:
        print("✅ All tests passed!")
        
        if not (hasattr(labdb, '_bindings_available') and labdb._bindings_available):
            print("\n🎯 Next steps to enable C++ bindings:")
            print("   1. Install pybind11: pip install pybind11")
            print("   2. Configure build: cmake -DBUILD_PYTHON_BINDINGS=ON build")
            print("   3. Build bindings: make pylabdb")
            print("   4. Test full integration: python test_comprehensive_python.py")
        else:
            print("\n🎆 Full triadic consciousness Python integration working!")
            print("🎯 Ready for Inception facts system integration")
    else:
        print("❌ Some tests failed - check output above")
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
