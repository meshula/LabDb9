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
        bool result = store.add_triple("granite", "isA", "rock");
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
        assert(store.add_triple("granite", "isA", "rock") && "Failed to insert granite-isA-rock");
        assert(store.add_triple("granite", "hasColor", "gray") && "Failed to insert granite-hasColor-gray");
        assert(store.add_triple("marble", "isA", "rock") && "Failed to insert marble-isA-rock");
        assert(store.add_triple("sandstone", "isA", "rock") && "Failed to insert sandstone-isA-rock");
        assert(store.add_triple("marble", "hasColor", "white") && "Failed to insert marble-hasColor-white");
        
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
        store.add_triple("granite", "isA", "rock");
        store.add_triple("granite", "hasColor", "gray");
        store.add_triple("marble", "isA", "rock");
        store.add_triple("diamond", "isA", "mineral");
        store.add_triple("diamond", "hasHardness", "10");
        
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

void test_adding_triples() {
    std::cout << "Testing triple addition and vocabulary statistics...\n";
    
    std::string test_db = "/tmp/labdb_adding_test";
    std::filesystem::remove_all(test_db);
    
    try {
        LabDb::NonoStore store(test_db);
        
        // Check initial state
        auto initial_stats = store.get_stats();
        assert(initial_stats.total_triples == 0 && "Initial database should be empty");
        assert(initial_stats.unique_subjects == 0 && "Initial subjects should be 0");
        assert(initial_stats.unique_predicates == 0 && "Initial predicates should be 0");
        assert(initial_stats.unique_objects == 0 && "Initial objects should be 0");
        
        std::cout << "  Initial state: " << initial_stats.total_triples << " triples, "
                  << initial_stats.unique_subjects << " subjects, "
                  << initial_stats.unique_predicates << " predicates, "
                  << initial_stats.unique_objects << " objects\n";
        
        // Add first triple: beetle -> has_wings -> two_pairs
        bool result1 = store.add_triple("beetle", "has_wings", "two_pairs");
        assert(result1 && "First triple addition failed");
        
        auto stats_after_1 = store.get_stats();
        assert(stats_after_1.total_triples == 1 && "Should have 1 triple after first addition");
        assert(stats_after_1.unique_subjects == 1 && "Should have 1 unique subject");
        assert(stats_after_1.unique_predicates == 1 && "Should have 1 unique predicate");
        assert(stats_after_1.unique_objects == 1 && "Should have 1 unique object");
        
        std::cout << "  After 1st triple: " << stats_after_1.total_triples << " triples, "
                  << stats_after_1.unique_subjects << " subjects, "
                  << stats_after_1.unique_predicates << " predicates, "
                  << stats_after_1.unique_objects << " objects\n";
        
        // Add second triple: beetle -> is_type -> insect (reuses subject)
        bool result2 = store.add_triple("beetle", "is_type", "insect");
        assert(result2 && "Second triple addition failed");
        
        auto stats_after_2 = store.get_stats();
        assert(stats_after_2.total_triples == 2 && "Should have 2 triples after second addition");
        assert(stats_after_2.unique_subjects == 1 && "Should still have 1 unique subject (beetle reused)");
        assert(stats_after_2.unique_predicates == 2 && "Should have 2 unique predicates");
        assert(stats_after_2.unique_objects == 2 && "Should have 2 unique objects");
        
        std::cout << "  After 2nd triple: " << stats_after_2.total_triples << " triples, "
                  << stats_after_2.unique_subjects << " subjects, "
                  << stats_after_2.unique_predicates << " predicates, "
                  << stats_after_2.unique_objects << " objects\n";
        
        // Add third triple: cricket -> is_type -> insect (reuses predicate and object)
        bool result3 = store.add_triple("cricket", "is_type", "insect");
        assert(result3 && "Third triple addition failed");
        
        auto stats_after_3 = store.get_stats();
        assert(stats_after_3.total_triples == 3 && "Should have 3 triples after third addition");
        assert(stats_after_3.unique_subjects == 2 && "Should have 2 unique subjects");
        assert(stats_after_3.unique_predicates == 2 && "Should still have 2 unique predicates (is_type reused)");
        assert(stats_after_3.unique_objects == 2 && "Should still have 2 unique objects (insect reused)");
        
        std::cout << "  After 3rd triple: " << stats_after_3.total_triples << " triples, "
                  << stats_after_3.unique_subjects << " subjects, "
                  << stats_after_3.unique_predicates << " predicates, "
                  << stats_after_3.unique_objects << " objects\n";
        
        // Verify the triples exist
        assert(store.exists("beetle", "has_wings", "two_pairs") && "First triple should exist");
        assert(store.exists("beetle", "is_type", "insect") && "Second triple should exist");
        assert(store.exists("cricket", "is_type", "insect") && "Third triple should exist");
        
        // Verify vocabulary discovery works correctly
        auto subjects = store.all_subjects();
        auto predicates = store.all_predicates();
        auto objects = store.all_objects();
        
        assert(subjects.size() == 2 && "Should find 2 subjects in vocabulary");
        assert(predicates.size() == 2 && "Should find 2 predicates in vocabulary");
        assert(objects.size() == 2 && "Should find 2 objects in vocabulary");
        
        std::cout << "  ✅ Vocabulary statistics correctly updated after each triple addition\n";
        std::cout << "  ✅ Triple existence verification passed\n";
        std::cout << "  ✅ Vocabulary discovery matches statistics\n";
        
        std::cout << "✅ Adding triples test passed\n";
        
    } catch (const LabDb::LmdbException& e) {
        std::cerr << "❌ Adding triples test failed: " << e.what() << std::endl;
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
        store.add_triple("granite", "isA", "rock");
        store.add_triple("granite", "hasColor", "gray");
        store.add_triple("granite", "hasHardness", "6");
        store.add_triple("marble", "isA", "rock");
        store.add_triple("marble", "hasColor", "white");
        store.add_triple("diamond", "isA", "mineral");
        store.add_triple("diamond", "hasHardness", "10");
        store.add_triple("quartz", "isA", "mineral");
        
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
    test_adding_triples();
    
    demonstrate_triadic_crown();
    
    std::cout << "\n🎉 All NonoStore core tests passed!\n";
    std::cout << "Triadic consciousness database fully operational.\n";
    
    return 0;
}
