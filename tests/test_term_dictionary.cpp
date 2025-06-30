#include "LabDb/TermDictionary.h"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <lmdb.h>

namespace fs = std::filesystem;

void cleanup_test_db(const std::string& path) {
    if (fs::exists(path)) {
        fs::remove_all(path);
    }
}

int main() {
    const std::string test_db_path = "/tmp/test_term_dictionary";
    
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
        
        // Create TermDictionary
        LabDb::TermDictionary dict(env);
        std::cout << "✓ TermDictionary created\n";
        
        // Test basic operations in a transaction
        MDB_txn* txn = nullptr;
        rc = mdb_txn_begin(env, nullptr, 0, &txn);
        assert(rc == 0);
        
        try {
            // Test intern operation
            auto granite_id = dict.intern(txn, "granite");
            auto color_id = dict.intern(txn, "hasColor");
            auto gray_id = dict.intern(txn, "gray");
            
            std::cout << "✓ Interned terms: granite=" << granite_id 
                      << ", hasColor=" << color_id 
                      << ", gray=" << gray_id << "\n";
            
            // Test that re-interning returns same ID
            auto granite_id2 = dict.intern(txn, "granite");
            assert(granite_id == granite_id2);
            std::cout << "✓ Re-interning returns same ID\n";
            
            // Test lookup
            auto lookup_result = dict.lookup(txn, "granite");
            assert(lookup_result && *lookup_result == granite_id);
            std::cout << "✓ Lookup works correctly\n";
            
            // Test resolve
            auto resolve_result = dict.resolve(txn, granite_id);
            assert(resolve_result && *resolve_result == "granite");
            std::cout << "✓ Resolve works correctly\n";
            
            // Test exists
            assert(dict.exists(txn, "granite"));
            assert(dict.exists(txn, granite_id));
            assert(!dict.exists(txn, "nonexistent"));
            assert(!dict.exists(txn, 99999));
            std::cout << "✓ Exists checks work correctly\n";
            
            // Test statistics
            auto stats = dict.get_stats(txn);
            assert(stats.total_terms == 3);
            assert(stats.min_term_id >= 1);
            assert(stats.max_term_id >= stats.min_term_id);
            std::cout << "✓ Statistics: " << stats.total_terms << " terms, "
                      << "next_id=" << stats.next_available_id << "\n";
            
            // Test iteration
            std::cout << "✓ Terms in dictionary:\n";
            LabDb::TermDictionary::Iterator iter(txn, dict);
            if (iter.first()) {
                do {
                    std::cout << "  " << iter.current_term() 
                              << " → " << iter.current_id() << "\n";
                } while (iter.next());
            }
            
            // Test batch operations
            std::vector<std::string> batch_terms = {"mineral", "isA", "hardness", "6"};
            auto batch_ids = dict.intern_batch(txn, batch_terms);
            assert(batch_ids.size() == 4);
            std::cout << "✓ Batch intern completed\n";
            
            // Test consistency validation
            assert(dict.validate_consistency(txn));
            std::cout << "✓ Dictionary consistency validated\n";
            
            // Commit transaction
            rc = mdb_txn_commit(txn);
            assert(rc == 0);
            std::cout << "✓ Transaction committed successfully\n";
            
        } catch (...) {
            mdb_txn_abort(txn);
            throw;
        }
        
        // Test in a new read-only transaction to ensure persistence
        txn = nullptr;
        rc = mdb_txn_begin(env, nullptr, MDB_RDONLY, &txn);
        assert(rc == 0);
        
        try {
            auto granite_lookup = dict.lookup(txn, "granite");
            assert(granite_lookup.has_value());
            
            auto granite_resolve = dict.resolve(txn, *granite_lookup);
            assert(granite_resolve && *granite_resolve == "granite");
            
            std::cout << "✓ Persistence verified - data survives transaction boundary\n";
            
            mdb_txn_commit(txn); // Properly commit read-only transaction
            txn = nullptr;
            
        } catch (...) {
            if (txn) {
                mdb_txn_abort(txn);
                txn = nullptr;
            }
            throw;
        }
        
        // Clean up
        mdb_env_close(env);
        cleanup_test_db(test_db_path);
        
        std::cout << "\n🎉 All TermDictionary tests PASSED!\n";
        std::cout << "✨ Ready for Phase 1.2: Python bindings\n";
        
        return 0;
        
    } catch (const LabDb::TermDictionaryException& e) {
        std::cerr << "❌ TermDictionary error: " << e.what() << "\n";
        cleanup_test_db(test_db_path);
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test error: " << e.what() << "\n";
        cleanup_test_db(test_db_path);
        return 1;
    }
}
