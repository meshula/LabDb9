#include "LabDb/TripleStore.h"
#include "LabDb/TermDictionary.h"
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
    const std::string test_db_path = "/tmp/test_triple_store";
    
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
        
        rc = mdb_env_set_maxdbs(env, 20); // Allow multiple databases
        assert(rc == 0);
        
        rc = mdb_env_open(env, test_db_path.c_str(), 0, 0664);
        assert(rc == 0);
        
        std::cout << "✓ LMDB environment initialized\n";
        
        // Create components
        LabDb::TermDictionary term_dict(env);
        LabDb::TIDSequenceGenerator tid_gen(env);
        LabDb::TripleStore triple_store(env, term_dict, tid_gen);
        
        std::cout << "✓ TripleStore and dependencies created\n";
        
        // Test basic triple storage operations in a transaction
        MDB_txn* txn = nullptr;
        rc = mdb_txn_begin(env, nullptr, 0, &txn);
        assert(rc == 0);
        
        try {
            // Test storing triples with strings
            auto tid1 = triple_store.store_triple(txn, "granite", "isA", "rock", "test_source", 0.95f);
            auto tid2 = triple_store.store_triple(txn, "granite", "hasColor", "gray", "test_source", 0.90f);
            auto tid3 = triple_store.store_triple(txn, "granite", "hasHardness", "6", "test_source", 1.0f);
            
            std::cout << "✓ Stored triples: TID1=" << tid1 << ", TID2=" << tid2 << ", TID3=" << tid3 << "\n";
            
            // Test retrieving triples
            auto triple1 = triple_store.get_triple(txn, tid1);
            assert(triple1.has_value());
            std::cout << "✓ Retrieved triple data for TID1\n";
            
            // Test retrieving as strings
            auto string_triple1 = triple_store.get_triple_as_strings(txn, tid1);
            assert(string_triple1.has_value());
            assert(string_triple1->subject == "granite");
            assert(string_triple1->predicate == "isA");
            assert(string_triple1->object == "rock");
            assert(string_triple1->confidence == 0.95f);
            std::cout << "✓ Retrieved string triple: " << string_triple1->subject 
                      << " " << string_triple1->predicate 
                      << " " << string_triple1->object << "\n";
            
            // Commit transaction
            rc = mdb_txn_commit(txn);
            assert(rc == 0);
            std::cout << "✓ Transaction committed successfully\n";
            
        } catch (...) {
            mdb_txn_abort(txn);
            throw;
        }
        
        // Clean up environment
        std::cout << "✓ Cleaning up environment\n";
        mdb_env_close(env);
        
        cleanup_test_db(test_db_path);
        
        std::cout << "\n🎉 All TripleStore tests PASSED!\n";
        std::cout << "✨ Phase 2.2 TripleStore implementation COMPLETE!\n";
        
        return 0;
        
    } catch (const LabDb::TripleStoreException& e) {
        std::cerr << "❌ TripleStore error: " << e.what() << "\n";
        cleanup_test_db(test_db_path);
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test error: " << e.what() << "\n";
        cleanup_test_db(test_db_path);
        return 1;
    }
}
