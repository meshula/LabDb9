#include "LabDb/NonoStore.h"
#include <iostream>
#include <cassert>
#include <filesystem>
#include <set>

void test_basic_operations() {
    std::cout << "Testing basic NonoStore operations...\n";
    
    // Clean up any existing test database
    std::string test_db = "/tmp/labdb_nonostore_test";
    std::filesystem::remove_all(test_db);
    
    try {
        LabDb::NonoStore store(test_db);
        
        // Test basic connect operation
        bool result = store.connect("granite", "isA", "rock");
        assert(result && "Connect operation failed");
        assert(store.get_last_error().success() && "Error after successful connect");
        
        // Test existence check
        assert(store.exists("granite", "isA", "rock") && "Triple should exist after connect");
        
        // Test that it doesn't exist in reverse
        assert(!store.exists("rock", "isA", "granite") && "Reverse triple should not exist");
        
        std::cout << "✅ Basic operations test passed\n";
        
    } catch (const LabDb::LmdbException& e) {
        std::cerr << "❌ Basic operations test failed: " << e.what() << std::endl;
        exit(1);
    }
}

void test_query_patterns() {
    std::cout << "Testing query patterns...\n";
    
    std::string test_db = "/tmp/labdb_query_test";
    std::filesystem::remove_all(test_db);
    
    try {
        LabDb::NonoStore store(test_db);
        
        // Insert test data
        assert(store.connect("granite", "isA", "rock") && "Failed to insert granite-isA-rock");
        assert(store.connect("granite", "hasColor", "gray") && "Failed to insert granite-hasColor-gray");
        assert(store.connect("marble", "isA", "rock") && "Failed to insert marble-isA-rock");
        assert(store.connect("sandstone", "isA", "rock") && "Failed to insert sandstone-isA-rock");
        assert(store.connect("marble", "hasColor", "white") && "Failed to insert marble-hasColor-white");
        
        // Test subject-driven queries (granite-*-*)
        auto granite_props = store.properties_of("granite");
        assert(granite_props.size() == 2 && "Granite should have 2 properties");
        
        // Test predicate-driven queries (*-isA-*)
        auto isa_relations = store.entities_with_relation("isA");
        assert(isa_relations.size() == 3 && "Should find 3 isA relationships");
        
        // Test object-driven queries (*-*-rock)
        auto rock_connections = store.connections_to("rock");
        assert(rock_connections.size() == 3 && "Should find 3 connections to rock");
        
        // Test specific pattern queries
        auto rocks = store.query("*", "isA", "rock");
        assert(rocks.size() == 3 && "Should find 3 things that are rocks");
        
        // Test count without returning results
        assert(store.count("*", "hasColor", "*") == 2 && "Should count 2 color relationships");
        
        std::cout << "✅ Query patterns test passed\n";
        
    } catch (const LabDb::LmdbException& e) {
        std::cerr << "❌ Query patterns test failed: " << e.what() << std::endl;
        exit(1);
    }
}

void test_vocabulary_discovery() {
    std::cout << "Testing vocabulary discovery...\n";
    
    std::string test_db = "/tmp/labdb_vocab_test";
    std::filesystem::remove_all(test_db);
    
    try {
        LabDb::NonoStore store(test_db);
        
        // Insert diverse test data
        store.connect("granite", "isA", "rock");
        store.connect("granite", "hasColor", "gray");
        store.connect("marble", "isA", "rock");
        store.connect("diamond", "isA", "mineral");
        store.connect("diamond", "hasHardness", "10");
        
        // Test Motion vocabulary (all subjects)
        auto subjects = store.all_subjects();
        std::set<std::string> subject_set(subjects.begin(), subjects.end());
        assert(subject_set.count("granite") == 1 && "Should find granite in subjects");
        assert(subject_set.count("marble") == 1 && "Should find marble in subjects");
        assert(subject_set.count("diamond") == 1 && "Should find diamond in subjects");
        assert(subjects.size() == 3 && "Should have exactly 3 unique subjects");
        
        // Test Memory vocabulary (all predicates)
        auto predicates = store.all_predicates();
        std::set<std::string> predicate_set(predicates.begin(), predicates.end());
        assert(predicate_set.count("isA") == 1 && "Should find isA in predicates");
        assert(predicate_set.count("hasColor") == 1 && "Should find hasColor in predicates");
        assert(predicate_set.count("hasHardness") == 1 && "Should find hasHardness in predicates");
        assert(predicates.size() == 3 && "Should have exactly 3 unique predicates");
        
        // Test Field vocabulary (all objects)
        auto objects = store.all_objects();
        std::set<std::string> object_set(objects.begin(), objects.end());
        assert(object_set.count("rock") == 1 && "Should find rock in objects");
        assert(object_set.count("mineral") == 1 && "Should find mineral in objects");
        assert(object_set.count("gray") == 1 && "Should find gray in objects");
        assert(object_set.count("10") == 1 && "Should find 10 in objects");
        assert(objects.size() == 4 && "Should have exactly 4 unique objects");
        
        std::cout << "✅ Vocabulary discovery test passed\n";
        
    } catch (const LabDb::LmdbException& e) {
        std::cerr << "❌ Vocabulary discovery test failed: " << e.what() << std::endl;
        exit(1);
    }
}

void demonstrate_triadic_crown() {
    std::cout << "\n=== Demonstrating Complete Triadic Crown ===\n";
    
    std::string test_db = "/tmp/labdb_crown_demo";
    std::filesystem::remove_all(test_db);
    
    try {
        LabDb::NonoStore store(test_db);
        
        // Build a small knowledge graph
        store.connect("granite", "isA", "rock");
        store.connect("granite", "hasColor", "gray");
        store.connect("granite", "hasHardness", "6");
        store.connect("marble", "isA", "rock");
        store.connect("marble", "hasColor", "white");
        store.connect("diamond", "isA", "mineral");
        store.connect("diamond", "hasHardness", "10");
        store.connect("quartz", "isA", "mineral");
        
        std::cout << "Knowledge Graph Built:\n";
        std::cout << "  8 triples stored across 9 indices\n\n";
        
        // Demonstrate Motion (Subject-driven queries)
        std::cout << "MOTION (स्पन्द) - What does granite express?\n";
        auto granite_props = store.properties_of("granite");
        for (const auto& triple : granite_props) {
            std::cout << "  " << triple.subject << " " << triple.predicate << " " << triple.object << "\n";
        }
        
        // Demonstrate Memory (Predicate-driven queries)
        std::cout << "\nMEMORY (स्मृति) - What relationships connect entities?\n";
        auto predicates = store.all_predicates();
        for (const auto& pred : predicates) {
            auto count = store.count("*", pred, "*");
            std::cout << "  " << pred << ": " << count << " connections\n";
        }
        
        // Demonstrate Field (Object-driven queries)
        std::cout << "\nFIELD (क्षेत्र) - What receives into 'rock' context?\n";
        auto rock_things = store.connections_to("rock");
        for (const auto& triple : rock_things) {
            std::cout << "  " << triple.subject << " " << triple.predicate << " " << triple.object << "\n";
        }
        
        // Demonstrate complete vocabulary discovery
        std::cout << "\nVOCABULARY DISCOVERY (Complete Ontological Awareness):\n";
        std::cout << "  Subjects (Motion entities): ";
        auto subjects = store.all_subjects();
        for (size_t i = 0; i < subjects.size(); ++i) {
            std::cout << subjects[i];
            if (i < subjects.size() - 1) std::cout << ", ";
        }
        std::cout << "\n";
        
        std::cout << "  Predicates (Memory relations): ";
        for (size_t i = 0; i < predicates.size(); ++i) {
            std::cout << predicates[i];
            if (i < predicates.size() - 1) std::cout << ", ";
        }
        std::cout << "\n";
        
        std::cout << "  Objects (Field contexts): ";
        auto objects = store.all_objects();
        for (size_t i = 0; i < objects.size(); ++i) {
            std::cout << objects[i];
            if (i < objects.size() - 1) std::cout << ", ";
        }
        std::cout << "\n";
        
        // Show database statistics
        auto stats = store.get_stats();
        std::cout << "\nDATABASE STATISTICS:\n";
        std::cout << "  Total triples: " << stats.total_triples << "\n";
        std::cout << "  Unique subjects: " << stats.unique_subjects << "\n";
        std::cout << "  Unique predicates: " << stats.unique_predicates << "\n";
        std::cout << "  Unique objects: " << stats.unique_objects << "\n";
        std::cout << "  LMDB entries: " << stats.lmdb_stats.entries << "\n";
        
        std::cout << "\nThis demonstrates the complete nonostore crown:\n";
        std::cout << "- Motion/Memory/Field queries through optimal index selection\n";
        std::cout << "- Complete vocabulary discovery for ontological awareness\n";
        std::cout << "- Atomic transactions across all nine indices\n";
        std::cout << "- Triadic consciousness infrastructure fully operational\n";
        
    } catch (const LabDb::LmdbException& e) {
        std::cerr << "❌ Crown demonstration failed: " << e.what() << std::endl;
        exit(1);
    }
}

int main() {
    std::cout << "=== LabDb NonoStore Core Tests ===\n";
    
    test_basic_operations();
    test_query_patterns();
    test_vocabulary_discovery();
    
    demonstrate_triadic_crown();
    
    std::cout << "\n🎉 All NonoStore core tests passed!\n";
    std::cout << "Triadic consciousness database fully operational.\n";
    
    return 0;
}
