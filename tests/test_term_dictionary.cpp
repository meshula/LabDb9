#include "LabDb/TermDictionary.h"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <lmdb.h>
#include <set>

namespace fs = std::filesystem;

void cleanup_test_db(const std::string& path) {
    if (fs::exists(path)) {
        fs::remove_all(path);
    }
}

void test_architecture_specification_conformance(LabDb::TermDictionary& dict, MDB_txn* txn) {
    std::cout << "\n=== Testing Architecture Specification Conformance ===\n";
    
    // Test the core specification requirement: bidirectional string↔u64 mapping
    std::cout << "Testing bidirectional mapping specification...\n";
    
    // 1. Test that TermIDs are compact u64 values starting from 1
    auto first_id = dict.intern(txn, "test_term_1");
    auto second_id = dict.intern(txn, "test_term_2");
    
    assert(first_id >= LabDb::TermDictionary::FIRST_VALID_ID && "TermIDs must start from 1");
    assert(second_id > first_id && "TermIDs must be sequential");
    assert(first_id != LabDb::TermDictionary::INVALID_TERM_ID && "TermIDs must not be 0");
    std::cout << "  ✓ TermIDs are valid u64 values starting from 1\n";
    
    // 2. Test bidirectional consistency: string → TermID → string
    std::vector<std::string> test_terms = {
        "granite", "hasColor", "gray", "mineral", "isA", "hardness", "6",
        "complex_term_with_underscores", "utf8_测试_term", "special!@#$%^&*()chars"
    };
    
    std::set<LabDb::TermDictionary::TermID> allocated_ids;
    
    for (const auto& term : test_terms) {
        // Forward mapping: string → TermID
        auto term_id = dict.intern(txn, term);
        
        // Verify TermID is unique
        assert(allocated_ids.find(term_id) == allocated_ids.end() && "TermIDs must be unique");
        allocated_ids.insert(term_id);
        
        // Reverse mapping: TermID → string
        auto resolved_term = dict.resolve(txn, term_id);
        assert(resolved_term.has_value() && "Reverse lookup must succeed");
        assert(*resolved_term == term && "Bidirectional mapping must be consistent");
        
        // Verify idempotency: re-interning same term returns same ID
        auto term_id_2 = dict.intern(txn, term);
        assert(term_id == term_id_2 && "Re-interning must return same ID");
        
        std::cout << "  ✓ Bidirectional mapping verified: '" << term << "' ↔ " << term_id << "\n";
    }
    
    // 3. Test that lookup() works for existing terms
    for (const auto& term : test_terms) {
        auto lookup_result = dict.lookup(txn, term);
        assert(lookup_result.has_value() && "Lookup must find existing terms");
        
        auto resolve_result = dict.resolve(txn, *lookup_result);
        assert(resolve_result.has_value() && "Resolve must work for looked-up IDs");
        assert(*resolve_result == term && "Lookup/resolve cycle must be consistent");
    }
    std::cout << "  ✓ All lookup operations consistent\n";
    
    // 4. Test non-existent term behavior
    auto non_existent_lookup = dict.lookup(txn, "definitely_does_not_exist_12345");
    assert(!non_existent_lookup.has_value() && "Lookup of non-existent term must return nullopt");
    
    auto non_existent_resolve = dict.resolve(txn, 999999);
    assert(!non_existent_resolve.has_value() && "Resolve of non-existent ID must return nullopt");
    std::cout << "  ✓ Non-existent term handling correct\n";
    
    // 5. Test vocabulary discovery capability (critical for all_subjects/all_predicates/all_objects)
    std::cout << "\nTesting vocabulary discovery specification...\n";
    
    // Get all terms using iterator
    std::set<std::string> discovered_terms;
    std::set<LabDb::TermDictionary::TermID> discovered_ids;
    
    LabDb::TermDictionary::Iterator iter(txn, dict);
    if (iter.first()) {
        do {
            auto term = iter.current_term();
            auto id = iter.current_id();
            
            discovered_terms.insert(term);
            discovered_ids.insert(id);
            
            // Verify iterator consistency
            auto verify_lookup = dict.lookup(txn, term);
            assert(verify_lookup.has_value() && *verify_lookup == id && "Iterator must be consistent with lookup");
            
            auto verify_resolve = dict.resolve(txn, id);
            assert(verify_resolve.has_value() && *verify_resolve == term && "Iterator must be consistent with resolve");
            
        } while (iter.next());
    }
    
    // Verify all test terms were discovered
    for (const auto& term : test_terms) {
        assert(discovered_terms.count(term) == 1 && "All interned terms must be discoverable via iteration");
    }
    std::cout << "  ✓ Vocabulary discovery via iteration works: " << discovered_terms.size() << " terms\n";
    
    // 6. Test batch operations for efficiency
    std::vector<std::string> batch_terms = {"batch1", "batch2", "batch3"};
    auto batch_ids = dict.intern_batch(txn, batch_terms);
    assert(batch_ids.size() == 3 && "Batch intern must return correct number of IDs");
    
    auto batch_resolved = dict.resolve_batch(txn, batch_ids);
    assert(batch_resolved.size() == 3 && "Batch resolve must return correct number of terms");
    
    for (size_t i = 0; i < batch_terms.size(); ++i) {
        assert(batch_resolved[i].has_value() && "Batch resolve must find all IDs");
        assert(*batch_resolved[i] == batch_terms[i] && "Batch operations must be consistent");
    }
    std::cout << "  ✓ Batch operations work correctly\n";
    
    // 7. Test statistics accuracy
    auto stats = dict.get_stats(txn);
    auto expected_terms = test_terms.size() + batch_terms.size() + 2; // +2 for the initial test terms
    assert(stats.total_terms >= expected_terms && "Statistics must reflect actual term count");
    assert(stats.next_available_id > stats.max_term_id && "Next available ID must be greater than max used ID");
    std::cout << "  ✓ Statistics are accurate: " << stats.total_terms << " terms, next_id=" << stats.next_available_id << "\n";
    
    // 8. Test consistency validation
    assert(dict.validate_consistency(txn) && "Dictionary must pass consistency validation");
    std::cout << "  ✓ Dictionary passes consistency validation\n";
    
    std::cout << "\n🎯 ARCHITECTURE SPECIFICATION CONFORMANCE: ALL TESTS PASSED!\n";
    std::cout << "✨ TermDictionary fully implements TID-based architecture requirements\n";
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
            // Run architecture specification conformance tests FIRST
            test_architecture_specification_conformance(dict, txn);
            
            // Test basic operations
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
            {
                // Scope the iterator to ensure it's destroyed before transaction commit
                LabDb::TermDictionary::Iterator iter(txn, dict);
                if (iter.first()) {
                    do {
                        std::cout << "  " << iter.current_term() 
                                  << " → " << iter.current_id() << "\n";
                    } while (iter.next());
                }
            } // Iterator destroyed here, before transaction commit
            
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
