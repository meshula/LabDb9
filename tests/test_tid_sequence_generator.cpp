#include "LabDb/TIDSequenceGenerator.h"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <vector>
#include <lmdb.h>

namespace fs = std::filesystem;

void cleanup_test_db(const std::string& path) {
    if (fs::exists(path)) {
        fs::remove_all(path);
    }
}

int main() {
    const std::string test_db_path = "/tmp/test_tid_sequence_generator";
    
    try {
        // Clean up any existing test database
        cleanup_test_db(test_db_path);
        fs::create_directories(test_db_path);
        
        // Initialize LMDB environment
        MDB_env* env = nullptr;
        int rc = mdb_env_create(&env);
        assert(rc == 0);
        
        rc = mdb_env_set_mapsize(env, 1UL * 1024 * 1024 * 1024); // 1GB
        assert(rc == 0);
        
        rc = mdb_env_set_maxdbs(env, 10); // Allow multiple databases
        assert(rc == 0);
        
        rc = mdb_env_open(env, test_db_path.c_str(), 0, 0664);
        assert(rc == 0);
        
        std::cout << "✓ LMDB environment initialized\n";
        
        // Create TIDSequenceGenerator
        LabDb::TIDSequenceGenerator generator(env);
        std::cout << "✓ TIDSequenceGenerator created\n";
        
        // Test basic sequence operations in a transaction
        MDB_txn* txn = nullptr;
        rc = mdb_txn_begin(env, nullptr, 0, &txn);
        assert(rc == 0);
        
        try {
            // Test initial state
            auto initial_current = generator.current_sequence(txn);
            auto initial_next = generator.peek_next_sequence(txn);
            std::cout << "✓ Initial state: current=" << initial_current 
                      << ", next=" << initial_next << "\n";
            assert(initial_current == LabDb::TIDSequenceGenerator::FIRST_VALID_TID);
            assert(initial_next == LabDb::TIDSequenceGenerator::FIRST_VALID_TID + 1);
            
            // Test next_sequence operations
            auto tid1 = generator.next_sequence(txn);
            auto tid2 = generator.next_sequence(txn);
            auto tid3 = generator.next_sequence(txn);
            
            std::cout << "✓ Generated TIDs: " << tid1 << ", " << tid2 << ", " << tid3 << "\n";
            assert(tid1 == LabDb::TIDSequenceGenerator::FIRST_VALID_TID);
            assert(tid2 == tid1 + 1);
            assert(tid3 == tid2 + 1);
            
            // Test current sequence after generation
            auto current_after = generator.current_sequence(txn);
            std::cout << "✓ Current sequence after generation: " << current_after << "\n";
            assert(current_after == tid3 + 1);
            
            // Test TID validation
            assert(generator.is_valid_tid(txn, tid1));
            assert(generator.is_valid_tid(txn, tid2));
            assert(generator.is_valid_tid(txn, tid3));
            assert(!generator.is_valid_tid(txn, LabDb::TIDSequenceGenerator::INVALID_TID));
            assert(!generator.is_valid_tid(txn, current_after + 10)); // Future TID
            std::cout << "✓ TID validation works correctly\n";
            
            // Test total allocated
            auto total_allocated = generator.total_allocated(txn);
            std::cout << "✓ Total allocated: " << total_allocated << "\n";
            assert(total_allocated == 4); // FIRST_VALID_TID + 3 generated
            
            // Test batch allocation
            auto batch_start = generator.allocate_batch(txn, 5);
            std::cout << "✓ Batch allocation: start=" << batch_start << ", count=5\n";
            
            // Verify batch allocation worked
            for (size_t i = 0; i < 5; ++i) {
                assert(generator.is_valid_tid(txn, batch_start + i));
            }
            assert(!generator.is_valid_tid(txn, batch_start + 5)); // Beyond batch
            std::cout << "✓ Batch allocation validation passed\n";
            
            // Test range validation
            assert(generator.is_valid_tid_range(txn, batch_start, 5));
            assert(!generator.is_valid_tid_range(txn, batch_start, 6)); // Extends beyond allocated
            assert(!generator.is_valid_tid_range(txn, batch_start + 10, 1)); // Future range
            std::cout << "✓ Range validation works correctly\n";
            
            // Test statistics
            auto stats = generator.get_stats(txn);
            std::cout << "✓ Statistics: current=" << stats.current_sequence 
                      << ", next=" << stats.next_available 
                      << ", total=" << stats.total_allocated 
                      << ", healthy=" << stats.sequence_healthy << "\n";
            assert(stats.sequence_healthy);
            assert(stats.total_allocated > 0);
            
            // Test sequence validation
            assert(generator.validate_sequence(txn));
            std::cout << "✓ Sequence validation passed\n";
            
            // Commit transaction
            rc = mdb_txn_commit(txn);
            assert(rc == 0);
            std::cout << "✓ Transaction committed successfully\n";
            
        } catch (...) {
            mdb_txn_abort(txn);
            throw;
        }
        
        // Test persistence in a new transaction
        txn = nullptr;
        rc = mdb_txn_begin(env, nullptr, MDB_RDONLY, &txn);
        assert(rc == 0);
        
        try {
            // Verify sequence persisted
            auto persistent_current = generator.current_sequence(txn);
            std::cout << "✓ Persistence verified: current=" << persistent_current << "\n";
            assert(persistent_current > LabDb::TIDSequenceGenerator::FIRST_VALID_TID);
            
            // Verify statistics persist
            auto persistent_stats = generator.get_stats(txn);
            assert(persistent_stats.sequence_healthy);
            assert(persistent_stats.total_allocated > 0);
            std::cout << "✓ Statistics persistence verified\n";
            
            mdb_txn_commit(txn);
            
        } catch (...) {
            mdb_txn_abort(txn);
            throw;
        }
        
        // Clean up environment
        std::cout << "✓ Cleaning up environment\n";
        mdb_env_close(env);
        
        cleanup_test_db(test_db_path);
        
        std::cout << "\n🎉 All TIDSequenceGenerator tests PASSED!\n";
        std::cout << "✨ Phase 2.1 TID sequence generator COMPLETE!\n";
        
        return 0;
        
    } catch (const LabDb::TIDSequenceException& e) {
        std::cerr << "❌ TIDSequenceGenerator error: " << e.what() << "\n";
        cleanup_test_db(test_db_path);
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test error: " << e.what() << "\n";
        cleanup_test_db(test_db_path);
        return 1;
    }
}
