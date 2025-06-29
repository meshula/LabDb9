#!/usr/bin/env python3
"""
Test script for LabDb Python bindings
"""

import sys
import os

def test_basic_import():
    """Test that we can import the labdb module"""
    try:
        # Add the python directory to path for testing
        sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'python'))
        
        import labdb
        print(f"✅ Successfully imported labdb version {labdb.__version__}")
        print(f"📋 Available classes: {labdb.__all__}")
        
        # Test perspective constants
        print(f"🧘 Triadic perspectives available:")
        print(f"   Motion (स्पन्द): {labdb.Perspective.MOTION}")
        print(f"   Memory (स्मृति): {labdb.Perspective.MEMORY}")
        print(f"   Field (क्षेत्र): {labdb.Perspective.FIELD}")
        
        return True
        
    except ImportError as e:
        print(f"❌ Failed to import labdb: {e}")
        print("   Note: C++ bindings not yet built")
        return False

def test_module_structure():
    """Test the module structure without C++ bindings"""
    try:
        sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'python'))
        
        # Test individual module components that don't require C++
        import labdb
        
        # Verify perspective constants
        assert hasattr(labdb, 'Perspective')
        assert labdb.Perspective.MOTION == "Motion"
        assert labdb.Perspective.MEMORY == "Memory" 
        assert labdb.Perspective.FIELD == "Field"
        
        print("✅ Python module structure validated")
        print("✅ Triadic perspective constants working")
        return True
        
    except Exception as e:
        print(f"❌ Module structure test failed: {e}")
        return False

if __name__ == "__main__":
    print("🧪 Testing LabDb Python module structure...")
    
    structure_ok = test_module_structure()
    import_ok = test_basic_import()
    
    if structure_ok:
        print("\n🎯 Next steps:")
        print("   1. Install pybind11: pip install pybind11")
        print("   2. Configure with Python bindings: cmake -DBUILD_PYTHON_BINDINGS=ON ..")
        print("   3. Build with Python module: make pylabdb")
        print("   4. Test C++ integration")
    else:
        print("\n❌ Python module structure needs fixes before proceeding")
        sys.exit(1)
