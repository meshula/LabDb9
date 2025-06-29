#!/usr/bin/env python3
"""
Basic test for LabDb Python integration
Tests both the Python module structure and (when available) C++ bindings
"""

import sys
import os
import tempfile

# Add our python directory to the path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'python'))

def test_module_import():
    """Test that the Python module can be imported"""
    print("🧪 Testing module import...")
    
    try:
        import labdb
        print(f"✅ Successfully imported labdb version {labdb.__version__}")
        
        # Test that perspective constants work
        assert labdb.Perspective.MOTION == "Motion"
        assert labdb.Perspective.MEMORY == "Memory" 
        assert labdb.Perspective.FIELD == "Field"
        print("✅ Triadic perspective constants working")
        
        # Check if C++ bindings are available
        if hasattr(labdb, '_bindings_available') and labdb._bindings_available:
            print("✅ C++ bindings are available")
            return test_cpp_bindings(labdb)
        else:
            print("⚠️  C++ bindings not available (expected until built)")
            return test_graceful_fallback(labdb)
            
    except Exception as e:
        print(f"❌ Failed to import labdb: {e}")
        return False

def test_graceful_fallback(labdb):
    """Test that graceful fallback works when C++ bindings aren't available"""
    print("🧪 Testing graceful fallback...")
    
    try:
        # Should be able to access perspective constants
        assert labdb.Perspective.MOTION == "Motion"
        print("✅ Can access Python-only components")
        
        # Trying to use C++ classes should give helpful error
        try:
            store = labdb.NonoStore("/tmp/test")
            print("❌ Expected error for missing C++ bindings")
            return False
        except RuntimeError as e:
            if "C++ bindings not available" in str(e):
                print("✅ Helpful error message for missing C++ bindings")
                return True
            else:
                print(f"❌ Unexpected error: {e}")
                return False
                
    except Exception as e:
        print(f"❌ Graceful fallback test failed: {e}")
        return False

def test_cpp_bindings(labdb):
    """Test C++ bindings functionality (if available)"""
    print("🧪 Testing C++ bindings...")
    
    try:
        # Create temporary database
        with tempfile.TemporaryDirectory() as temp_dir:
            db_path = os.path.join(temp_dir, "test.ldb")
            
            # Test basic NonoStore creation
            store = labdb.NonoStore(db_path)
            print("✅ NonoStore creation successful")
            
            # Test basic operations
            store.connect("granite", "isA", "rock")
            store.connect("granite", "hasColor", "gray")
            print("✅ Basic connect operations successful")
            
            # Test query
            results = store.query("granite", "*", "*")
            assert len(results) == 2
            print(f"✅ Query successful: found {len(results)} triples")
            
            # Test vocabulary discovery
            subjects = store.all_subjects()
            assert "granite" in subjects
            print(f"✅ Vocabulary discovery: {len(subjects)} subjects")
            
            # Test perspective constants integration
            print(f"✅ Motion perspective: {labdb.Perspective.MOTION}")
            print(f"✅ Memory perspective: {labdb.Perspective.MEMORY}")  
            print(f"✅ Field perspective: {labdb.Perspective.FIELD}")
            
            return True
            
    except Exception as e:
        print(f"❌ C++ bindings test failed: {e}")
        return False

def main():
    """Main test function"""
    print("=" * 60)
    print("🚀 LabDb Python Integration Test")
    print("=" * 60)
    
    success = test_module_import()
    
    print("\n" + "=" * 60)
    if success:
        print("✅ All tests passed!")
        print("\n🎯 Next development steps:")
        print("   1. Install pybind11: pip install pybind11")
        print("   2. Configure build: cmake -DBUILD_PYTHON_BINDINGS=ON build")
        print("   3. Build bindings: make pylabdb")
        print("   4. Test full integration: python test_python_integration.py")
        print("   5. Continue with TriadicQuery Python bindings")
    else:
        print("❌ Some tests failed - check output above")
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
