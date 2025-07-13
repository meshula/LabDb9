#include "LabDb/EnhancedDatabaseVerbs.h"
#include "LabDb/DatabaseManager.h"
#include "LabDb/DatabaseVerbs.h"
#include "LabDb/Db9Dispatcher.h"
#include "LabDb/NonoStore.h"
#include "LabDb/EntityId.h"
#include <iostream>
#include <cassert>

// Direct test of EID chain resolution
int main() {
    std::cout << "🔧 EID Chain Resolution Debug Test\n";
    std::cout << "====================================\n\n";

    try {
        // Initialize verb registrations
        // (Verbs auto-register via getGlobalDb9Dispatcher)
        
        auto& dispatcher = LabDb::getGlobalDb9Dispatcher();
        
        // Open the test database (now using relative path to testenv)
        std::string openCmd = "(open-database :path \"testenv/test_chains.db9\" :dbid \"test_chains\")";
        auto response = dispatcher.executeCommand(openCmd);
        
        if (response.status != LabDb::Db9Response::Success) {
            std::cerr << "❌ Failed to open test database: " << response.result << std::endl;
            return 1;
        }
        
        std::cout << "✅ Opened test database: " << response.result << std::endl;
        
        // Extract the DBID
        std::string dbid;
        size_t pos = response.result.find("\"dbid\": \"");
        if (pos != std::string::npos) {
            pos += 9; // Length of "\"dbid\": \""
            size_t end = response.result.find("\"", pos);
            if (end != std::string::npos) {
                dbid = response.result.substr(pos, end - pos);
            }
        }
        
        if (dbid.empty()) {
            std::cerr << "❌ Could not extract DBID from response" << std::endl;
            return 1;
        }
        
        std::cout << "📋 Using DBID: " << dbid << std::endl << std::endl;
        
        // Test 1: Get entity eid:1 (should be DirectValue - no chain)
        std::cout << "🧪 Test 1: Get entity eid:1 (should be DirectValue)\n";
        std::cout << "----------------------------------------------------\n";
        
        std::string getEntityCmd = "(get-entity-enhanced :dbid " + dbid + " :eid \"eid:1\")";
        response = dispatcher.executeCommand(getEntityCmd);
        
        std::cout << "Response: " << response.result << std::endl << std::endl;
        
        // Test 1b: Get entity eid:4 (should chain: eid:4 -> eid:1 -> DirectValue)
        std::cout << "🧪 Test 1b: Get entity eid:4 (should chain to DirectValue)\n";
        std::cout << "-----------------------------------------------------------\n";
        
        std::string getEntity4Cmd = "(get-entity-enhanced :dbid " + dbid + " :eid \"eid:4\")";
        response = dispatcher.executeCommand(getEntity4Cmd);
        
        std::cout << "Response: " << response.result << std::endl << std::endl;
        
        // Test 1c: Get entity eid:7 (should chain: eid:7 -> eid:5 -> ChainEnd)
        std::cout << "🧪 Test 1c: Get entity eid:7 (should chain to ChainEnd)\n";
        std::cout << "--------------------------------------------------------\n";
        
        std::string getEntity7Cmd = "(get-entity-enhanced :dbid " + dbid + " :eid \"eid:7\")";
        response = dispatcher.executeCommand(getEntity7Cmd);
        
        std::cout << "Response: " << response.result << std::endl << std::endl;
        
        // Test 2: Test a non-existent triple (our test db has entities, not complex triples)
        std::cout << "🧪 Test 2: Find triple test (should be empty - our test db has simple entities)\n";
        std::cout << "-------------------------------------------------------------------------------\n";
        
        std::string findTripleCmd = "(find-triple-enhanced :subject \"eid:1\" :predicate \"*\" :object \"*\" :dbid " + dbid + ")";
        response = dispatcher.executeCommand(findTripleCmd);
        
        std::cout << "Response: " << response.result << std::endl << std::endl;
        
        // Test 3: Direct NonoStore access to test EntityId resolution
        std::cout << "🧪 Test 3: Direct EntityId resolution test\n";
        std::cout << "-------------------------------------------\n";
        
        // Get the DatabaseManager and NonoStore
        auto& manager = LabDb::DatabaseManager::instance();
        auto store = manager.getDatabase(dbid);
        
        if (store) {
            std::cout << "✅ Got NonoStore for " << dbid << std::endl;
            
            // Try to resolve eid:1 directly (no chain expected)
            try {
                LabDb::EntityId entity = LabDb::EntityId::fromEid("eid:1", *store);
                std::cout << "EntityId for eid:1: valid=" << entity.isValid() 
                         << " exists=" << entity.exists() << std::endl;
                
                if (entity.isValid() && entity.exists()) {
                    std::string name = entity.name();
                    std::cout << "eid:1 resolves to: '" << name << "'" << std::endl;
                    
                    // Check if this creates a chain (if name is also an EID)
                    if (name.starts_with("eid:")) {
                        std::cout << "🔗 Found EID chain! " << name << " is also an EID" << std::endl;
                        
                        // Try to resolve the next level
                        LabDb::EntityId nextEntity = LabDb::EntityId::fromEid(name, *store);
                        if (nextEntity.isValid() && nextEntity.exists()) {
                            std::string finalName = nextEntity.name();
                            std::cout << "Final resolution: " << name << " -> '" << finalName << "'" << std::endl;
                        }
                    } else {
                        std::cout << "ℹ️  eid:1 resolves directly (no chain)" << std::endl;
                    }
                } else {
                    std::cout << "❌ eid:1 does not resolve to a valid entity" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cout << "❌ Exception during EntityId resolution: " << e.what() << std::endl;
            }
            
            std::cout << std::endl;
            
            // Try to resolve eid:4 directly (should have EID chain: eid:4 -> eid:1 -> DirectValue)
            try {
                LabDb::EntityId entity4 = LabDb::EntityId::fromEid("eid:4", *store);
                std::cout << "EntityId for eid:4: valid=" << entity4.isValid() 
                         << " exists=" << entity4.exists() << std::endl;
                
                if (entity4.isValid() && entity4.exists()) {
                    std::string name4 = entity4.name();
                    std::cout << "eid:4 resolves to: '" << name4 << "'" << std::endl;
                    
                    // Check if this creates a chain (if name is also an EID)
                    if (name4.starts_with("eid:")) {
                        std::cout << "🔗 Found EID chain! eid:4 -> " << name4 << " (this is an EID)" << std::endl;
                        
                        // Try to resolve the next level
                        LabDb::EntityId nextEntity = LabDb::EntityId::fromEid(name4, *store);
                        if (nextEntity.isValid() && nextEntity.exists()) {
                            std::string finalName = nextEntity.name();
                            std::cout << "Final resolution: " << name4 << " -> '" << finalName << "'" << std::endl;
                            std::cout << "🎯 COMPLETE CHAIN: eid:4 -> " << name4 << " -> '" << finalName << "'" << std::endl;
                        } else {
                            std::cout << "❌ " << name4 << " does not resolve to a valid entity" << std::endl;
                        }
                    } else {
                        std::cout << "ℹ️  eid:4 resolves directly (no chain)" << std::endl;
                    }
                } else {
                    std::cout << "❌ eid:4 does not resolve to a valid entity" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cout << "❌ Exception during eid:4 resolution: " << e.what() << std::endl;
            }
        } else {
            std::cout << "❌ Could not get NonoStore for " << dbid << std::endl;
        }
        
        std::cout << "\n🔧 Debug test complete!\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "💥 Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}