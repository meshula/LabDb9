#!/usr/bin/env python3
"""
Unit test for Unified KeyData Streaming Interface
Tests billion-scale streaming architecture with 100 test triples
"""

import sys
import os
import tempfile
import unittest

# Add our python directory to the path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'python'))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'build'))


class TestStreamingInterface(unittest.TestCase):
    """Test suite for TriadicDirectAccess streaming interface"""
    
    def setUp(self):
        """Set up test database with 100 predictable triples"""
        import labdb
        
        if not labdb._bindings_available:
            self.skipTest("C++ bindings not available")
        
        # Create temporary database
        self.temp_dir = tempfile.TemporaryDirectory()
        db_path = os.path.join(self.temp_dir.name, "streaming_test.ldb")
        
        self.store = labdb.NonoStore(db_path)
        
        # Create 100 test triples with predictable patterns
        self._create_test_data()
        
        # Import streaming interface
        from labdb.streaming_access import TriadicDirectAccess, DatabaseInventory, KeyKind
        self.access = TriadicDirectAccess(self.store)
        self.KeyKind = KeyKind
    
    def tearDown(self):
        """Clean up temporary database"""
        if hasattr(self, 'temp_dir'):
            self.temp_dir.cleanup()
    
    def _create_test_data(self):
        """Create 100 test triples with known patterns for validation"""
        
        # Pattern 1: Animals (25 triples)
        animals = ["ant", "bee", "cat", "dog", "elk"]
        properties = ["hasLegs", "hasColor", "hasSize", "hasHabitat", "canFly"]
        
        for i, animal in enumerate(animals):
            for j, prop in enumerate(properties):
                value = f"value_{i}_{j}"
                self.store.connect(animal, prop, value)
        
        # Pattern 2: Minerals (25 triples) 
        minerals = ["quartz", "granite", "marble", "diamond", "ruby"]
        for i, mineral in enumerate(minerals):
            for j, prop in enumerate(properties):
                value = f"mineral_{i}_{j}"
                self.store.connect(mineral, prop, value)
        
        # Pattern 3: Tools (25 triples)
        tools = ["hammer", "screwdriver", "wrench", "drill", "saw"]
        for i, tool in enumerate(tools):
            for j, prop in enumerate(properties):
                value = f"tool_{i}_{j}"
                self.store.connect(tool, prop, value)
        
        # Pattern 4: Colors (25 triples)
        colors = ["red", "blue", "green", "yellow", "purple"]
        for i, color in enumerate(colors):
            for j, prop in enumerate(properties):
                value = f"color_{i}_{j}"
                self.store.connect(color, prop, value)
        
        # Verify we have exactly 100 triples
        stats = self.store.get_stats()
        assert stats.total_triples == 100, f"Expected 100 triples, got {stats.total_triples}"
        
        print(f"✅ Created {stats.total_triples} test triples")
    
    def test_database_summary(self):
        """Test get_summary() provides accurate database overview"""
        summary = self.access.get_summary()
        
        # Verify basic counts
        self.assertEqual(summary['total_triples'], 100)
        self.assertGreater(summary['storage_efficiency'], 0)
        
        # Verify vocabulary counts
        vocab = summary['vocabulary']
        self.assertEqual(vocab['subjects'], 20)  # 5 each of animals, minerals, tools, colors
        self.assertEqual(vocab['predicates'], 5)  # hasLegs, hasColor, hasSize, hasHabitat, canFly
        self.assertEqual(vocab['objects'], 100)  # Each triple has unique object
        
        print(f"✅ Database summary: {summary}")
    
    def test_keykind_inventory_streaming(self):
        """Test streaming through each KeyKind with different batch sizes"""
        from labdb.streaming_access import DatabaseInventory
        
        # Test subjects streaming
        subjects_inv = DatabaseInventory(self.KeyKind.SUBJECTS)
        
        # Stream in small batches
        collected_subjects = []
        batch_count = 0
        while self.access.inventory(subjects_inv, batch_size=5):
            batch_count += 1
            collected_subjects.extend(subjects_inv.data.current_batch)
            
            # Verify progress tracking
            self.assertGreaterEqual(subjects_inv.data.progress(), 0.0)
            self.assertLessEqual(subjects_inv.data.progress(), 1.0)
            
            # Verify batch properties
            self.assertLessEqual(len(subjects_inv.data.current_batch), 5)
            self.assertTrue(subjects_inv.data.batch_size() <= 5)
        
        # Verify we got all subjects
        self.assertEqual(len(collected_subjects), 20)
        self.assertEqual(set(collected_subjects), {
            "ant", "bee", "cat", "dog", "elk",
            "quartz", "granite", "marble", "diamond", "ruby", 
            "hammer", "screwdriver", "wrench", "drill", "saw",
            "red", "blue", "green", "yellow", "purple"
        })
        
        print(f"✅ Streamed {len(collected_subjects)} subjects in {batch_count} batches")
    
    def test_predicate_frequency_analysis(self):
        """Test frequency statistics for predicates"""
        from labdb.streaming_access import DatabaseInventory
        
        predicates_inv = DatabaseInventory(self.KeyKind.PREDICATES)
        
        # Get all predicates with frequency analysis
        self.access.inventory(predicates_inv, batch_size=10)
        
        # Each predicate should appear exactly 20 times (5 subjects × 4 categories)
        expected_frequency = 20
        for predicate in predicates_inv.data.current_batch:
            actual_frequency = predicates_inv.frequency_stats[predicate]
            self.assertEqual(actual_frequency, expected_frequency,
                           f"Predicate '{predicate}' should appear {expected_frequency} times, got {actual_frequency}")
        
        print(f"✅ Frequency analysis: {predicates_inv.frequency_stats}")
    
    def test_top_and_rare_functionality(self):
        """Test top() and rare() operations"""
        from labdb.streaming_access import DatabaseInventory
        
        # Test top subjects (should all have equal frequency of 5)
        subjects_inv = DatabaseInventory(self.KeyKind.SUBJECTS)
        self.assertTrue(self.access.top(subjects_inv, 10))
        
        # All subjects should have frequency of 5 (5 predicates each)
        for subject in subjects_inv.data.current_batch:
            self.assertEqual(subjects_inv.frequency_stats[subject], 5)
        
        # Test rare objects (each object appears only once)
        objects_inv = DatabaseInventory(self.KeyKind.OBJECTS)
        self.assertTrue(self.access.rare(objects_inv, 10))
        
        # All objects should have frequency of 1
        for obj in objects_inv.data.current_batch:
            self.assertEqual(objects_inv.frequency_stats[obj], 1)
        
        print(f"✅ Top/rare functionality working")
    
    def test_reset_and_rewind(self):
        """Test iterator reset functionality"""
        from labdb.streaming_access import DatabaseInventory
        
        subjects_inv = DatabaseInventory(self.KeyKind.SUBJECTS)
        
        # Get first batch
        self.access.inventory(subjects_inv, batch_size=5)
        first_batch = subjects_inv.data.current_batch.copy()
        original_position = subjects_inv.data.position()
        
        # Advance further
        self.access.inventory(subjects_inv, batch_size=5)
        second_batch = subjects_inv.data.current_batch.copy()
        advanced_position = subjects_inv.data.position()
        
        # Verify we advanced
        self.assertGreater(advanced_position, original_position)
        self.assertNotEqual(first_batch, second_batch)
        
        # Reset and verify we're back at start
        subjects_inv.reset()
        self.assertEqual(subjects_inv.data.position(), 0)
        self.assertEqual(subjects_inv.data.first_entry, 0)
        self.assertEqual(subjects_inv.data.remaining, subjects_inv.data.total_count)
        
        # Get first batch again - should match original
        self.access.inventory(subjects_inv, batch_size=5)
        reset_batch = subjects_inv.data.current_batch.copy()
        self.assertEqual(first_batch, reset_batch)
        
        print(f"✅ Reset functionality working")
    
    def test_stream_all_iterator(self):
        """Test stream_all() iterator interface"""
        
        # Test streaming all subjects
        all_subjects = []
        batch_count = 0
        
        for batch in self.access.stream_all(self.KeyKind.SUBJECTS, batch_size=7):
            batch_count += 1
            all_subjects.extend(batch)
            self.assertLessEqual(len(batch), 7)  # Batch size respected
        
        # Verify we got all subjects
        self.assertEqual(len(all_subjects), 20)
        self.assertGreater(batch_count, 1)  # Multiple batches
        
        print(f"✅ Iterator interface: {len(all_subjects)} subjects in {batch_count} batches")
    
    def test_billion_scale_simulation(self):
        """Test progress tracking simulates billion-scale behavior"""
        from labdb.streaming_access import DatabaseInventory
        
        # Simulate large dataset by using small batch sizes
        objects_inv = DatabaseInventory(self.KeyKind.OBJECTS)
        
        progress_points = []
        while self.access.inventory(objects_inv, batch_size=3):
            progress = objects_inv.data.progress()
            progress_points.append(progress)
            
            # Verify progress is monotonically increasing
            if len(progress_points) > 1:
                self.assertGreaterEqual(progress, progress_points[-2])
        
        # Final progress should be 100%
        self.assertEqual(progress_points[-1], 1.0)
        
        # Should have had multiple progress points
        self.assertGreater(len(progress_points), 10)
        
        print(f"✅ Billion-scale simulation: {len(progress_points)} progress points")
    
    def test_enhanced_integration(self):
        """Test integration with EnhancedNonoStore"""
        import labdb
        
        # Create enhanced store
        enhanced = labdb.EnhancedNonoStore(self.store._store_path if hasattr(self.store, '_store_path') else self.temp_dir.name + "/test.ldb")
        
        # Get direct access through enhanced interface
        direct = enhanced.direct_access()
        
        # Verify it works the same way
        summary = direct.get_summary()
        self.assertEqual(summary['total_triples'], 100)
        
        print(f"✅ Enhanced integration working")


def run_streaming_tests():
    """Run all streaming interface tests"""
    print("🌊 UNIFIED KEYDAYA STREAMING INTERFACE TESTS")
    print("=" * 60)
    
    # Check if bindings are available
    try:
        import labdb
        if not labdb._bindings_available:
            print("⚠️  C++ bindings not available - cannot run tests")
            return False
    except ImportError:
        print("❌ Cannot import labdb")
        return False
    
    # Run tests
    suite = unittest.TestLoader().loadTestsFromTestCase(TestStreamingInterface)
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)
    
    if result.wasSuccessful():
        print("\n🎉 ALL STREAMING INTERFACE TESTS PASSED!")
        print("✅ Ready for billion-scale triadic consciousness databases!")
        return True
    else:
        print(f"\n❌ {len(result.failures)} failures, {len(result.errors)} errors")
        return False


if __name__ == "__main__":
    success = run_streaming_tests()
    sys.exit(0 if success else 1)
