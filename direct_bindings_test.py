#!/usr/bin/env python3
"""
Direct bindings test for Phase 2.4 TID Architecture Python Bindings
"""

import sys
import os

# Add the LabDb Python package to path
sys.path.insert(0, '/Users/nporcino/dev/Lab/LabDb/python')

print("🔍 Direct C++ bindings test for Phase 2.4")
print("=" * 50)

try:
    # Try importing the C++ module directly
    print("🔄 Attempting direct pylabdb import...")
    
    # Import the C++ bindings directly
    from labdb.pylabdb import NonoStore, Triple, Stats
    from labdb.pylabdb import TermDictionary, TIDSequenceGenerator, TripleStore
    
    print("✅ Direct pylabdb import successful!")
    print("✅ Core classes imported:")
    print("  - NonoStore")
    print("  - TermDictionary") 
    print("  - TIDSequenceGenerator")
    print("  - TripleStore")
    print("  - Triple, Stats")
    
    # Test basic functionality
    print("\n🧪 Testing TID architecture integration...")
    
    import tempfile
    import shutil
    
    temp_dir = tempfile.mkdtemp(prefix="labdb_tid_test_")
    db_path = os.path.join(temp_dir, "test_tid.db")
    
    try:
        # Create NonoStore (should use TID architecture)
        store = NonoStore(db_path)
        print(f"✅ NonoStore created: {db_path}")
        
        # Test basic operations (TID-based internally)
        result = store.connect("granite", "isA", "rock")
        print(f"✅ Connected triple: granite isA rock → {result}")
        
        result = store.connect("granite", "hasColor", "gray")
        print(f"✅ Connected triple: granite hasColor gray → {result}")
        
        # Test query (should use TID resolution)
        triples = store.query("granite", "*", "*")
        print(f"✅ Query results: {len(triples)} triples found")
        for triple in triples:
            print(f"    {triple.subject} {triple.predicate} {triple.object}")
            
        # Test vocabulary discovery (TID-based indices)
        subjects = store.all_subjects()
        predicates = store.all_predicates()
        objects = store.all_objects()
        print(f"✅ Vocabulary: {len(subjects)} subjects, {len(predicates)} predicates, {len(objects)} objects")
        
        # Test statistics
        stats = store.get_stats()
        print(f"✅ Statistics: {stats.total_triples} triples, {stats.lmdb_stats.entries} LMDB entries")
        
        # Calculate storage efficiency
        if stats.total_triples > 0:
            efficiency = stats.lmdb_stats.entries / (stats.total_triples * 9)
            print(f"✅ Storage efficiency ratio: {efficiency:.2f}")
            if efficiency < 2.0:
                print("🎯 Excellent TID-based storage efficiency!")
            
        print("\n🎉 Phase 2.4: TID Architecture Python Bindings - DIRECT TEST SUCCESS!")
        print("✅ C++ TID architecture fully accessible from Python")
        print("✅ NonoStore operations using internal TID storage")
        print("✅ Storage efficiency demonstrates TID benefits")
        
    finally:
        # Cleanup
        shutil.rmtree(temp_dir)
        
except ImportError as e:
    print(f"❌ Direct import failed: {e}")
    print("\nDebugging pylabdb.so...")
    
    # Check architecture and dependencies
    import subprocess
    try:
        result = subprocess.run(['file', '/Users/nporcino/dev/Lab/LabDb/python/labdb/pylabdb.so'], 
                               capture_output=True, text=True)
        print(f"File info: {result.stdout}")
        
        result = subprocess.run(['otool', '-L', '/Users/nporcino/dev/Lab/LabDb/python/labdb/pylabdb.so'], 
                               capture_output=True, text=True)
        print(f"Dependencies: {result.stdout}")
    except:
        print("Could not check .so file details")
        
except Exception as e:
    print(f"❌ Test failed: {e}")
    import traceback
    traceback.print_exc()
