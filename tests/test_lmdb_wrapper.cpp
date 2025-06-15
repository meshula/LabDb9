#include "LabDb/LmdbStore.h"
#include <iostream>
#include <cassert>
#include <filesystem>

void test_basic_operations() {
    std::cout << "Testing basic LMDB operations...\n";
    
    // Clean up any existing test database
    std::string test_db = "/tmp/labdb_test";
    std::filesystem::remove_all(test_db);
    
    try {
        LabDb::LmdbStore store(test_db);
        
        // Test basic put/get
        {
            LabDb::LmdbStore::Transaction txn(store);
            
            bool result = store.put(txn, "test_key", "test_value");
            assert(result && "Put operation failed");
            
            std::string value;
            result = store.get(txn, "test_key", value);
            assert(result && "Get operation failed");
            assert(value == "test_value" && "Retrieved value doesn't match");
            
            txn.commit();
        }
        
        // Test persistence across transactions
        {
            LabDb::LmdbStore::Transaction txn(store, true); // read-only
            
            std::string value;
            bool result = store.get(txn, "test_key", value);
            assert(result && "Get operation failed in new transaction");
            assert(value == "test_value" && "Value not persisted");
        }
        
        std::cout << "✅ Basic operations test passed\n";
        
    } catch (const LabDb::LmdbException& e) {
        std::cerr << "❌ LMDB test failed: " << e.what() << std::endl;
        exit(1);
    }
}

void test_prefix_queries() {
    std::cout << "Testing prefix queries...\n";
    
    std::string test_db = "/tmp/labdb_prefix_test";
    std::filesystem::remove_all(test_db);
    
    try {
        LabDb::LmdbStore store(test_db);
        
        // Insert test data for prefix queries
        {
            LabDb::LmdbStore::Transaction txn(store);
            
            // Insert some nonostore-style keys
            store.put(txn, "~spo~granite~isA~rock", "{}");
            store.put(txn, "~spo~granite~hasColor~gray", "{}");
            store.put(txn, "~spo~marble~isA~rock", "{}");
            store.put(txn, "~pos~isA~rock~granite", "{}");
            store.put(txn, "~pos~isA~rock~marble", "{}");
            store.put(txn, "~subjects~granite~1", "{}");
            store.put(txn, "~subjects~marble~1", "{}");
            
            txn.commit();
        }
        
        // Test prefix query for all granite properties
        {
            LabDb::LmdbStore::Transaction txn(store, true);
            
            auto results = store.query_prefix(txn, "~spo~granite~");
            assert(results.size() == 2 && "Should find 2 granite properties");
            
            // Check the keys
            bool found_isa = false, found_color = false;
            for (const auto& [key, value] : results) {
                if (key.find("isA") != std::string::npos) found_isa = true;
                if (key.find("hasColor") != std::string::npos) found_color = true;
            }
            assert(found_isa && found_color && "Should find both granite properties");
        }
        
        // Test vocabulary discovery
        {
            LabDb::LmdbStore::Transaction txn(store, true);
            
            auto subjects = store.query_prefix(txn, "~subjects~");
            assert(subjects.size() == 2 && "Should find 2 subjects");
        }
        
        std::cout << "✅ Prefix queries test passed\n";
        
    } catch (const LabDb::LmdbException& e) {
        std::cerr << "❌ Prefix query test failed: " << e.what() << std::endl;
        exit(1);
    }
}

void test_transaction_rollback() {
    std::cout << "Testing transaction rollback...\n";
    
    std::string test_db = "/tmp/labdb_rollback_test";
    std::filesystem::remove_all(test_db);
    
    try {
        LabDb::LmdbStore store(test_db);
        
        // Put some initial data
        {
            LabDb::LmdbStore::Transaction txn(store);
            store.put(txn, "persistent_key", "persistent_value");
            txn.commit();
        }
        
        // Start a transaction and abort it
        {
            LabDb::LmdbStore::Transaction txn(store);
            store.put(txn, "temp_key", "temp_value");
            store.put(txn, "persistent_key", "modified_value");
            txn.abort(); // Explicit abort
        }
        
        // Verify rollback worked
        {
            LabDb::LmdbStore::Transaction txn(store, true);
            
            // temp_key should not exist
            std::string value;
            bool result = store.get(txn, "temp_key", value);
            assert(!result && "temp_key should not exist after rollback");
            
            // persistent_key should have original value
            result = store.get(txn, "persistent_key", value);
            assert(result && "persistent_key should still exist");
            assert(value == "persistent_value" && "persistent_key should have original value");
        }
        
        std::cout << "✅ Transaction rollback test passed\n";
        
    } catch (const LabDb::LmdbException& e) {
        std::cerr << "❌ Transaction rollback test failed: " << e.what() << std::endl;
        exit(1);
    }
}

int main() {
    std::cout << "=== LabDb LMDB Wrapper Tests ===\n";
    
    test_basic_operations();
    test_prefix_queries();
    test_transaction_rollback();
    
    std::cout << "\n🎉 All LMDB wrapper tests passed!\n";
    std::cout << "Ready to build the nonostore on this foundation.\n";
    
    return 0;
}
