#include "LabDb/NonostoreKeys.h"
#include <iostream>
#include <cassert>
#include <set>
#include <algorithm>

void test_basic_key_generation() {
    std::cout << "Testing basic key generation...\n";
    
    using IndexType = LabDb::NonostoreKeys::IndexType;
    
    // Test a simple triple: granite-isA-rock
    std::string subject = "granite";
    std::string predicate = "isA";
    std::string object = "rock";
    
    // Generate all nine keys
    auto keys = LabDb::NonostoreKeys::generate_all_keys(subject, predicate, object);
    
    // Verify we got 9 unique keys
    std::set<std::string> unique_keys(keys.begin(), keys.end());
    assert(unique_keys.size() == 9 && "Should generate 9 unique keys");
    
    // Check specific key formats
    assert(keys[0] == "~spo~granite~isA~rock" && "SPO key format incorrect");
    assert(keys[1] == "~sop~granite~rock~isA" && "SOP key format incorrect");
    assert(keys[2] == "~pso~isA~granite~rock" && "PSO key format incorrect");
    assert(keys[3] == "~pos~isA~rock~granite" && "POS key format incorrect");
    assert(keys[4] == "~osp~rock~granite~isA" && "OSP key format incorrect");
    assert(keys[5] == "~ops~rock~isA~granite" && "OPS key format incorrect");
    
    // Check vocabulary keys
    assert(keys[6] == "~subjects~granite~1" && "Subjects vocabulary key incorrect");
    assert(keys[7] == "~predicates~isA~1" && "Predicates vocabulary key incorrect");
    assert(keys[8] == "~objects~rock~1" && "Objects vocabulary key incorrect");
    
    std::cout << "✅ Basic key generation test passed\n";
}

void test_escaping() {
    std::cout << "Testing special character escaping...\n";
    
    // Test terms with special characters
    std::string subject = "my~subject";
    std::string predicate = "has\\property";
    std::string object = "value~with~tildes";
    
    auto keys = LabDb::NonostoreKeys::generate_all_keys(subject, predicate, object);
    
    // Verify escaping worked - no raw tildes in the middle of terms
    for (const auto& key : keys) {
        // Count separators (should be exactly what we expect)
        size_t separator_count = std::count(key.begin(), key.end(), '~');
        // Each key should have the right number of separators for its format
        assert(separator_count >= 3 && "Should have at least 3 separators per key");
    }
    
    // Test round-trip: generate and parse
    auto parsed = LabDb::NonostoreKeys::parse_key(keys[0]); // SPO key
    assert(parsed.subject == subject && "Subject escaping round-trip failed");
    assert(parsed.predicate == predicate && "Predicate escaping round-trip failed");
    assert(parsed.object == object && "Object escaping round-trip failed");
    
    std::cout << "✅ Special character escaping test passed\n";
}

void test_query_prefix_generation() {
    std::cout << "Testing query prefix generation...\n";
    
    // Test different query patterns
    
    // granite-*-* (subject specified)
    auto prefix1 = LabDb::NonostoreKeys::generate_query_prefix("granite", "*", "*");
    assert(prefix1 == "~spo~granite~" && "Subject-only query prefix incorrect");
    
    // *-isA-* (predicate specified)
    auto prefix2 = LabDb::NonostoreKeys::generate_query_prefix("*", "isA", "*");
    assert(prefix2 == "~pso~isA~" && "Predicate-only query prefix incorrect");
    
    // *-*-rock (object specified)
    auto prefix3 = LabDb::NonostoreKeys::generate_query_prefix("*", "*", "rock");
    assert(prefix3 == "~osp~rock~" && "Object-only query prefix incorrect");
    
    // granite-isA-* (subject and predicate specified)
    auto prefix4 = LabDb::NonostoreKeys::generate_query_prefix("granite", "isA", "*");
    assert(prefix4 == "~spo~granite~isA~" && "Subject-predicate query prefix incorrect");
    
    // *-isA-rock (predicate and object specified)
    auto prefix5 = LabDb::NonostoreKeys::generate_query_prefix("*", "isA", "rock");
    assert(prefix5 == "~pos~isA~rock~" && "Predicate-object query prefix incorrect");
    
    // granite-*-rock (subject and object specified)
    auto prefix6 = LabDb::NonostoreKeys::generate_query_prefix("granite", "*", "rock");
    assert(prefix6 == "~sop~granite~rock~" && "Subject-object query prefix incorrect");
    
    std::cout << "✅ Query prefix generation test passed\n";
}

void test_vocabulary_keys() {
    std::cout << "Testing vocabulary key generation...\n";
    
    using IndexType = LabDb::NonostoreKeys::IndexType;
    
    // Test vocabulary key generation
    auto subj_key = LabDb::NonostoreKeys::generate_vocabulary_key(IndexType::SUBJECTS, "granite");
    auto pred_key = LabDb::NonostoreKeys::generate_vocabulary_key(IndexType::PREDICATES, "isA");
    auto obj_key = LabDb::NonostoreKeys::generate_vocabulary_key(IndexType::OBJECTS, "rock");
    
    assert(subj_key == "~subjects~granite~1" && "Subject vocabulary key incorrect");
    assert(pred_key == "~predicates~isA~1" && "Predicate vocabulary key incorrect");
    assert(obj_key == "~objects~rock~1" && "Object vocabulary key incorrect");
    
    // Test vocabulary prefixes
    auto subj_prefix = LabDb::NonostoreKeys::get_vocabulary_prefix(IndexType::SUBJECTS);
    auto pred_prefix = LabDb::NonostoreKeys::get_vocabulary_prefix(IndexType::PREDICATES);
    auto obj_prefix = LabDb::NonostoreKeys::get_vocabulary_prefix(IndexType::OBJECTS);
    
    assert(subj_prefix == "~subjects~" && "Subject vocabulary prefix incorrect");
    assert(pred_prefix == "~predicates~" && "Predicate vocabulary prefix incorrect");
    assert(obj_prefix == "~objects~" && "Object vocabulary prefix incorrect");
    
    std::cout << "✅ Vocabulary key generation test passed\n";
}

void test_key_parsing() {
    std::cout << "Testing key parsing...\n";
    
    using IndexType = LabDb::NonostoreKeys::IndexType;
    
    // Test parsing content keys
    auto parsed_spo = LabDb::NonostoreKeys::parse_key("~spo~granite~isA~rock");
    assert(parsed_spo.index_type == IndexType::SPO && "SPO index type parsing failed");
    assert(parsed_spo.subject == "granite" && "SPO subject parsing failed");
    assert(parsed_spo.predicate == "isA" && "SPO predicate parsing failed");
    assert(parsed_spo.object == "rock" && "SPO object parsing failed");
    assert(!parsed_spo.is_vocabulary && "SPO should not be vocabulary");
    
    // Test parsing vocabulary keys
    auto parsed_vocab = LabDb::NonostoreKeys::parse_key("~subjects~granite~1");
    assert(parsed_vocab.index_type == IndexType::SUBJECTS && "Subject vocabulary index parsing failed");
    assert(parsed_vocab.term == "granite" && "Subject vocabulary term parsing failed");
    assert(parsed_vocab.is_vocabulary && "Subject vocabulary flag parsing failed");
    
    std::cout << "✅ Key parsing test passed\n";
}

void demonstrate_nonostore_crown() {
    std::cout << "\n=== Demonstrating Nonostore Crown Architecture ===\n";
    
    // Show how a single triple generates the complete crown
    std::string subject = "granite";
    std::string predicate = "isA"; 
    std::string object = "rock";
    
    auto keys = LabDb::NonostoreKeys::generate_all_keys(subject, predicate, object);
    
    std::cout << "Triple: " << subject << " " << predicate << " " << object << "\n\n";
    
    std::cout << "Content Indices (Traditional Hexastore):\n";
    for (int i = 0; i < 6; ++i) {
        auto index_type = static_cast<LabDb::NonostoreKeys::IndexType>(i);
        std::cout << "  " << LabDb::NonostoreKeys::index_name(index_type) << "\n";
        std::cout << "    " << keys[i] << "\n";
    }
    
    std::cout << "\nVocabulary Indices (Ontological Completion):\n";
    for (int i = 6; i < 9; ++i) {
        auto index_type = static_cast<LabDb::NonostoreKeys::IndexType>(i);
        std::cout << "  " << LabDb::NonostoreKeys::index_name(index_type) << "\n";
        std::cout << "    " << keys[i] << "\n";
    }
    
    std::cout << "\nQuery Examples:\n";
    std::cout << "  What properties does granite have? (granite-*-*)\n";
    std::cout << "    Prefix: " << LabDb::NonostoreKeys::generate_query_prefix("granite", "*", "*") << "\n";
    
    std::cout << "  What is a rock? (*-*-rock)\n";
    std::cout << "    Prefix: " << LabDb::NonostoreKeys::generate_query_prefix("*", "*", "rock") << "\n";
    
    std::cout << "  What kinds of relationships exist? (vocabulary discovery)\n";
    std::cout << "    Prefix: " << LabDb::NonostoreKeys::get_vocabulary_prefix(LabDb::NonostoreKeys::IndexType::PREDICATES) << "\n";
    
    std::cout << "\nThis demonstrates the nonostore crown:\n";
    std::cout << "- Six content indices enable efficient queries from any angle\n";
    std::cout << "- Three vocabulary indices enable ontological discovery\n";
    std::cout << "- Together they form complete triadic consciousness infrastructure\n";
}

int main() {
    std::cout << "=== LabDb Nonostore Key Generation Tests ===\n";
    
    test_basic_key_generation();
    test_escaping();
    test_query_prefix_generation();
    test_vocabulary_keys();
    test_key_parsing();
    
    demonstrate_nonostore_crown();
    
    std::cout << "\n🎉 All key generation tests passed!\n";
    std::cout << "Nine-index nonostore key system is ready for implementation.\n";
    
    return 0;
}
