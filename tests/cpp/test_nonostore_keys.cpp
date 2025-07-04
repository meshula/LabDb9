#include "LabDb/NonoStore.h"
#include "LabDb/TIDSequenceGenerator.h"
#include "LabDb/TermDictionary.h"
#include <iostream>
#include <cassert>
#include <set>
#include <filesystem>

#define AXIOM(x, msg) \
    if (!(x)) { \
        std::cerr << "Assertion failed: " << msg << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

void test_tid_encoding_decoding() {
    std::cout << "Testing TID encoding/decoding...\n";
    
    // Test various TID values
    std::vector<uint64_t> test_tids = {0, 1, 42, 255, 65535, 4294967295ULL, 18446744073709551615ULL};
    
    for (uint64_t original_tid : test_tids) {
        // Encode TID to storage format (16-character hex string)
        std::string encoded = LabDb::TIDSequenceGenerator::encode_tid_for_storage(original_tid);
        
        // Verify encoding format
        AXIOM(encoded.length() == 16, "TID encoding should produce 16-character strings");
        
        // Verify all characters are hex
        for (char c : encoded) {
            AXIOM(std::isxdigit(c), "TID encoding should only contain hex digits");
        }
        
        // Decode back to TID
        uint64_t decoded_tid = LabDb::TIDSequenceGenerator::decode_tid_from_storage(encoded);
        
        // Verify round-trip integrity
        AXIOM(decoded_tid == original_tid, "TID round-trip encoding/decoding failed");
    }
    
    std::cout << "✅ TID encoding/decoding test passed\n";
}

void test_tid_based_crown_keys() {
    std::cout << "Testing TID-based crown key generation...\n";
    
    std::string test_db = "/tmp/labdb_tid_keys_test";
    std::filesystem::remove_all(test_db);
    
    try {
        LabDb::NonoStore store(test_db);
        
        // Add test triples to populate TID system
        AXIOM(store.add_triple("granite", "isA", "rock"), "Failed to add granite triple");
        AXIOM(store.add_triple("marble", "hasColor", "white"), "Failed to add marble triple");
        
        // Test that the TID-based system works with queries
        auto granite_props = store.entities_with_subject("granite");
        AXIOM(granite_props.size() == 1, "Should find 1 granite property");
        AXIOM(granite_props[0].subject == "granite", "Subject should be granite");
        AXIOM(granite_props[0].predicate == "isA", "Predicate should be isA");
        AXIOM(granite_props[0].object == "rock", "Object should be rock");
        
        auto marble_props = store.entities_with_subject("marble");
        AXIOM(marble_props.size() == 1, "Should find 1 marble property");
        AXIOM(marble_props[0].predicate == "hasColor", "Marble predicate should be hasColor");
        
        // Test cross-queries work
        auto isa_relations = store.entities_with_predicate("isA");
        AXIOM(isa_relations.size() == 1, "Should find 1 isA relationship");
        
        auto color_relations = store.entities_with_predicate("hasColor");
        AXIOM(color_relations.size() == 1, "Should find 1 hasColor relationship");
        
        std::cout << "✅ TID-based crown key generation test passed\n";
        
    } catch (const LabDb::LmdbException& e) {
        std::cerr << "❌ TID-based crown key test failed: " << e.what() << std::endl;
        exit(1);
    }
}

void test_tid_query_optimization() {
    std::cout << "Testing TID-based query optimization...\n";
    
    std::string test_db = "/tmp/labdb_tid_optimization_test";
    std::filesystem::remove_all(test_db);
    
    try {
        LabDb::NonoStore store(test_db);
        
        // Build a larger dataset to test optimal index selection
        store.add_triple("granite", "isA", "rock");
        store.add_triple("granite", "hasColor", "gray");
        store.add_triple("granite", "hasHardness", "6");
        store.add_triple("marble", "isA", "rock");
        store.add_triple("marble", "hasColor", "white");
        store.add_triple("sandstone", "isA", "rock");
        store.add_triple("diamond", "isA", "mineral");
        store.add_triple("diamond", "hasHardness", "10");
        
        // Test subject-driven queries (should use SPO index)
        auto granite_all = store.entities_with_subject("granite");
        AXIOM(granite_all.size() == 3, "Granite should have 3 properties");
        
        // Test predicate-driven queries (should use PSO index)
        auto isa_all = store.entities_with_predicate("isA");
        AXIOM(isa_all.size() == 4, "Should find 4 isA relationships");
        
        // Test object-driven queries (should use OSP index)
        auto rock_all = store.entities_with_object("rock");
        AXIOM(rock_all.size() == 3, "Should find 3 things that are rocks");
        
        // Test specific patterns work
        auto rocks = store.query("*", "isA", "rock");
        AXIOM(rocks.size() == 3, "Query for rocks should return 3 results");
        
        auto hard_things = store.query("*", "hasHardness", "*");
        AXIOM(hard_things.size() == 2, "Should find 2 things with hardness");
        
        // Verify no false positives
        auto nonexistent = store.entities_with_subject("nonexistent");
        AXIOM(nonexistent.size() == 0, "Nonexistent subject should return 0 results");
        
        std::cout << "✅ TID-based query optimization test passed\n";
        
    } catch (const LabDb::LmdbException& e) {
        std::cerr << "❌ TID-based optimization test failed: " << e.what() << std::endl;
        exit(1);
    }
}

void test_vocabulary_discovery_with_tids() {
    std::cout << "Testing vocabulary discovery in TID architecture...\n";
    
    std::string test_db = "/tmp/labdb_tid_vocab_test";
    std::filesystem::remove_all(test_db);
    
    try {
        LabDb::NonoStore store(test_db);
        
        // Add diverse vocabulary
        store.add_triple("granite", "isA", "rock");
        store.add_triple("granite", "hasColor", "gray");
        store.add_triple("marble", "isA", "rock");
        store.add_triple("diamond", "isA", "mineral");
        store.add_triple("diamond", "hasHardness", "10");
        store.add_triple("ruby", "isA", "gemstone");
        
        // Test Motion vocabulary (all subjects)
        auto subjects = store.all_subjects();
        std::set<std::string> subject_set(subjects.begin(), subjects.end());
        AXIOM(subject_set.count("granite") == 1, "Should find granite in subjects");
        AXIOM(subject_set.count("marble") == 1, "Should find marble in subjects");
        AXIOM(subject_set.count("diamond") == 1, "Should find diamond in subjects");
        AXIOM(subject_set.count("ruby") == 1, "Should find ruby in subjects");
        AXIOM(subjects.size() == 4, "Should have exactly 4 unique subjects");
        
        // Test Memory vocabulary (all predicates)
        auto predicates = store.all_predicates();
        std::set<std::string> predicate_set(predicates.begin(), predicates.end());
        AXIOM(predicate_set.count("isA") == 1, "Should find isA in predicates");
        AXIOM(predicate_set.count("hasColor") == 1, "Should find hasColor in predicates");
        AXIOM(predicate_set.count("hasHardness") == 1, "Should find hasHardness in predicates");
        AXIOM(predicates.size() == 3, "Should have exactly 3 unique predicates");
        
        // Test Field vocabulary (all objects)
        auto objects = store.all_objects();
        std::set<std::string> object_set(objects.begin(), objects.end());
        AXIOM(object_set.count("rock") == 1, "Should find rock in objects");
        AXIOM(object_set.count("mineral") == 1, "Should find mineral in objects");
        AXIOM(object_set.count("gemstone") == 1, "Should find gemstone in objects");
        AXIOM(object_set.count("gray") == 1, "Should find gray in objects");
        AXIOM(object_set.count("10") == 1, "Should find 10 in objects");
        AXIOM(objects.size() == 5, "Should have exactly 5 unique objects");
        
        std::cout << "✅ Vocabulary discovery with TIDs test passed\n";
        
    } catch (const LabDb::LmdbException& e) {
        std::cerr << "❌ Vocabulary discovery test failed: " << e.what() << std::endl;
        exit(1);
    }
}

void test_tid_count_operations() {
    std::cout << "Testing TID-based count operations...\n";
    
    std::string test_db = "/tmp/labdb_tid_count_test";
    std::filesystem::remove_all(test_db);
    
    try {
        LabDb::NonoStore store(test_db);
        
        // Add test data
        store.add_triple("granite", "isA", "rock");
        store.add_triple("granite", "hasColor", "gray");
        store.add_triple("marble", "isA", "rock");
        store.add_triple("marble", "hasColor", "white");
        store.add_triple("sandstone", "isA", "rock");
        
        // Test count operations
        AXIOM(store.count("*", "isA", "*") == 3, "Should count 3 isA relationships");
        AXIOM(store.count("*", "hasColor", "*") == 2, "Should count 2 hasColor relationships");
        AXIOM(store.count("*", "*", "rock") == 3, "Should count 3 things that are rocks");
        AXIOM(store.count("granite", "*", "*") == 2, "Should count 2 granite properties");
        AXIOM(store.count("*", "*", "*") == 5, "Should count all 5 triples");
        
        // Test non-existent counts
        AXIOM(store.count("nonexistent", "*", "*") == 0, "Should count 0 for nonexistent subject");
        AXIOM(store.count("*", "nonexistent", "*") == 0, "Should count 0 for nonexistent predicate");
        AXIOM(store.count("*", "*", "nonexistent") == 0, "Should count 0 for nonexistent object");
        
        std::cout << "✅ TID-based count operations test passed\n";
        
    } catch (const LabDb::LmdbException& e) {
        std::cerr << "❌ TID-based count test failed: " << e.what() << std::endl;
        exit(1);
    }
}

void demonstrate_tid_architecture() {
    std::cout << "\n=== Demonstrating TID-Based Triadic Architecture ===\n";
    
    std::string test_db = "/tmp/labdb_tid_demo";
    std::filesystem::remove_all(test_db);
    
    try {
        LabDb::NonoStore store(test_db);
        
        std::cout << "Building knowledge graph with TID architecture...\n";
        
        // Build knowledge graph
        store.add_triple("granite", "isA", "rock");
        store.add_triple("granite", "hasColor", "gray");
        store.add_triple("granite", "hasHardness", "6");
        store.add_triple("marble", "isA", "rock");
        store.add_triple("marble", "hasColor", "white");
        store.add_triple("diamond", "isA", "mineral");
        store.add_triple("diamond", "hasHardness", "10");
        
        // Demonstrate triadic queries
        std::cout << "\nMOTION (Motion/Subject queries):\n";
        auto granite_motion = store.entities_with_subject("granite");
        std::cout << "  Granite expresses:\n";
        for (const auto& triple : granite_motion) {
            std::cout << "    " << triple.subject << " " << triple.predicate << " " << triple.object << "\n";
        }
        
        std::cout << "\nMEMORY (Memory/Predicate queries):\n";
        auto isa_memory = store.entities_with_predicate("isA");
        std::cout << "  'isA' relationships:\n";
        for (const auto& triple : isa_memory) {
            std::cout << "    " << triple.subject << " " << triple.predicate << " " << triple.object << "\n";
        }
        
        std::cout << "\nFIELD (Field/Object queries):\n";
        auto rock_field = store.entities_with_object("rock");
        std::cout << "  Things in 'rock' context:\n";
        for (const auto& triple : rock_field) {
            std::cout << "    " << triple.subject << " " << triple.predicate << " " << triple.object << "\n";
        }
        
        // Show vocabulary discovery
        std::cout << "\nVOCABULARY DISCOVERY:\n";
        std::cout << "  Motion entities: ";
        auto subjects = store.all_subjects();
        for (size_t i = 0; i < subjects.size(); ++i) {
            std::cout << subjects[i];
            if (i < subjects.size() - 1) std::cout << ", ";
        }
        std::cout << "\n";
        
        std::cout << "  Memory relations: ";
        auto predicates = store.all_predicates();
        for (size_t i = 0; i < predicates.size(); ++i) {
            std::cout << predicates[i];
            if (i < predicates.size() - 1) std::cout << ", ";
        }
        std::cout << "\n";
        
        std::cout << "  Field contexts: ";
        auto objects = store.all_objects();
        for (size_t i = 0; i < objects.size(); ++i) {
            std::cout << objects[i];
            if (i < objects.size() - 1) std::cout << ", ";
        }
        std::cout << "\n";
        
        // Show stats
        auto stats = store.get_stats();
        std::cout << "\nTID ARCHITECTURE STATISTICS:\n";
        std::cout << "  Total triples: " << stats.total_triples << "\n";
        std::cout << "  Unique subjects: " << stats.unique_subjects << "\n";
        std::cout << "  Unique predicates: " << stats.unique_predicates << "\n";
        std::cout << "  Unique objects: " << stats.unique_objects << "\n";
        std::cout << "  LMDB entries: " << stats.lmdb_stats.entries << "\n";
        
        std::cout << "\nTID Architecture Benefits:\n";
        std::cout << "- Compact binary keys with TermIDs instead of strings\n";
        std::cout << "- Hex-encoded TID values for optimal LMDB prefix compression\n";
        std::cout << "- Separate TripleStore for TID→triple resolution\n";
        std::cout << "- Efficient crown index selection based on query patterns\n";
        std::cout << "- Complete vocabulary discovery through dedicated indices\n";
        std::cout << "- Triadic consciousness through Motion/Memory/Field queries\n";
        
    } catch (const LabDb::LmdbException& e) {
        std::cerr << "❌ TID architecture demo failed: " << e.what() << std::endl;
        exit(1);
    }
}

int main() {
    std::cout << "=== LabDb TID-Based Nonostore Architecture Tests ===\n";
    
    test_tid_encoding_decoding();
    test_tid_based_crown_keys();
    test_tid_query_optimization();
    test_vocabulary_discovery_with_tids();
    test_tid_count_operations();
    
    demonstrate_tid_architecture();
    
    std::cout << "\n🎉 All TID-based nonostore tests passed!\n";
    std::cout << "Triadic consciousness database with TID architecture is fully operational.\n";
    
    return 0;
}
