#!/usr/bin/env python3
"""
Simple import test for Phase 2.4 TID Architecture Python Bindings
"""

import sys
import os

# Add the LabDb Python package to path
sys.path.insert(0, '/Users/nporcino/dev/Lab/LabDb/python')

print("🔍 Debugging Python import for Phase 2.4")
print("=" * 50)

try:
    print("Python path:", sys.path[0])
    print("Current working directory:", os.getcwd())
    
    # Check if the module directory exists
    module_path = '/Users/nporcino/dev/Lab/LabDb/python/labdb'
    print(f"Module directory exists: {os.path.exists(module_path)}")
    
    if os.path.exists(module_path):
        files = os.listdir(module_path)
        print(f"Files in module directory: {files}")
        
        # Check if pylabdb.so exists and has content
        so_file = os.path.join(module_path, 'pylabdb.so')
        if os.path.exists(so_file):
            size = os.path.getsize(so_file)
            print(f"pylabdb.so exists, size: {size} bytes")
        else:
            print("❌ pylabdb.so not found!")
    
    # Try importing the module
    print("\n🔄 Attempting to import labdb...")
    import labdb
    
    print(f"✅ labdb imported successfully!")
    print(f"Version: {labdb.__version__}")
    print(f"Bindings available: {labdb._bindings_available}")
    
    if labdb._bindings_available:
        print("✅ C++ bindings are available!")
        print("Available classes:", [name for name in dir(labdb) if not name.startswith('_')])
        
        # Try creating a simple NonoStore
        print("\n🧪 Testing basic NonoStore creation...")
        import tempfile
        temp_dir = tempfile.mkdtemp()
        db_path = os.path.join(temp_dir, "test.db")
        
        store = labdb.NonoStore(db_path)
        print(f"✅ NonoStore created at {db_path}")
        
        # Test TID architecture components if available
        print("\n🧪 Testing TID architecture classes...")
        if hasattr(labdb, 'TIDSequenceGenerator'):
            print("✅ TIDSequenceGenerator available")
        if hasattr(labdb, 'TripleStore'):
            print("✅ TripleStore available")
        if hasattr(labdb, 'TermDictionary'):
            print("✅ TermDictionary available")
            
        print("\n🎉 Phase 2.4: TID Architecture Python Bindings - SUCCESS!")
        
    else:
        print("❌ C++ bindings not available")
        
except ImportError as e:
    print(f"❌ Import error: {e}")
    print("\nDebugging import failure...")
    
    # Try importing the C++ module directly
    try:
        from labdb import pylabdb
        print("✅ Direct pylabdb import successful")
    except ImportError as e2:
        print(f"❌ Direct pylabdb import failed: {e2}")
        
        # Check if we can find the .so file
        import glob
        so_files = glob.glob('/Users/nporcino/dev/Lab/LabDb/**/*.so', recursive=True)
        print(f"Found .so files: {so_files}")
        
except Exception as e:
    print(f"❌ Unexpected error: {e}")
    import traceback
    traceback.print_exc()
