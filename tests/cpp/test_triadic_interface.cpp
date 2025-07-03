#include "LabDb/TriadicQuery.h"
#include <iostream>
#include <cassert>
#include <filesystem>
#include <set>

void test_motion_perspective() {
    std::cout << "Testing Motion (स्पन्द) perspective...\n";
    
    std::string test_db = "/tmp/labdb_triadic_motion_test";
    std::filesystem::remove_all(test_db);
    
    auto store = std::make_shared<LabDb::NonoStore>(test_db);
    LabDb::TriadicQuery triadic(store);
    
    // Build test knowledge graph
    store->add_triple("granite", "isA", "rock");
    store->add_triple("granite", "hasColor", "gray");
    store->add_triple("granite", "hasHardness", "6");
    store->add_triple("marble", "isA", "rock");
    store->add_triple("marble", "hasColor", "white");
    
    // Test motion_from: What does granite express?
    auto granite_motion = triadic.motion_from("granite");
    assert(granite_motion.size() == 3 && "Granite should have 3 expressions");
    
    // Verify all results are from Motion perspective
    for (const auto& result : granite_motion) {
        assert(result.discovered_through == LabDb::TriadicQuery::Perspective::Motion);
        assert(result.entity == "granite");
    }
    
    // Test entity_expressions: How does granite express itself?
    auto granite_expressions = triadic.entity_expressions("granite");
    std::set<std::string> expr_set(granite_expressions.begin(), granite_expressions.end());
    assert(expr_set.count("isA") == 1 && "Should express through isA");
    assert(expr_set.count("hasColor") == 1 && "Should express through hasColor");
    assert(expr_set.count("hasHardness") == 1 && "Should express through hasHardness");
    
    // Test motion_through: What entities express through isA?
    auto isa_motion = triadic.motion_through("isA");
    assert(isa_motion.size() == 2 && "Two entities express through isA");
    
    std::cout << "✅ Motion perspective test passed\n";
}

void test_memory_perspective() {
    std::cout << "Testing Memory (स्मृति) perspective...\n";
    
    std::string test_db = "/tmp/labdb_triadic_memory_test";
    std::filesystem::remove_all(test_db);
    
    auto store = std::make_shared<LabDb::NonoStore>(test_db);
    LabDb::TriadicQuery triadic(store);
    
    // Build test knowledge graph
    store->add_triple("granite", "isA", "rock");
    store->add_triple("marble", "isA", "rock");
    store->add_triple("sandstone", "isA", "rock");
    store->add_triple("diamond", "isA", "mineral");
    store->add_triple("granite", "hasColor", "gray");
    store->add_triple("marble", "hasColor", "white");
    
    // Test memory_relations: What connects through isA?
    auto isa_memory = triadic.memory_relations("isA");
    assert(isa_memory.size() == 4 && "Four connections through isA");
    
    // Verify all results are from Memory perspective
    for (const auto& result : isa_memory) {
        assert(result.discovered_through == LabDb::TriadicQuery::Perspective::Memory);
        assert(result.relation == "isA");
    }
    
    // Test relation_frequencies: What are the most common relations?
    auto frequencies = triadic.relation_frequencies();
    assert(frequencies.size() == 2 && "Should have 2 relation types");
    assert(frequencies[0].first == "isA" && frequencies[0].second == 4 && "isA should be most frequent");
    assert(frequencies[1].first == "hasColor" && frequencies[1].second == 2 && "hasColor should be second");
    
    // Test memory_between: What connects granite and rock?
    auto granite_rock_memory = triadic.memory_between("granite", "rock");
    assert(granite_rock_memory.size() == 1 && "One connection between granite and rock");
    assert(granite_rock_memory[0].relation == "isA");
    
    std::cout << "✅ Memory perspective test passed\n";
}

void test_field_perspective() {
    std::cout << "Testing Field (क्षेत्र) perspective...\n";
    
    std::string test_db = "/tmp/labdb_triadic_field_test";
    std::filesystem::remove_all(test_db);
    
    auto store = std::make_shared<LabDb::NonoStore>(test_db);
    LabDb::TriadicQuery triadic(store);
    
    // Build test knowledge graph
    store->add_triple("granite", "isA", "rock");
    store->add_triple("marble", "isA", "rock");
    store->add_triple("sandstone", "isA", "rock");
    store->add_triple("diamond", "isA", "mineral");
    store->add_triple("granite", "hasColor", "gray");
    store->add_triple("coal", "hasColor", "gray");
    
    // Test field_contexts: What receives into 'rock' context?
    auto rock_field = triadic.field_contexts("rock");
    assert(rock_field.size() == 3 && "Three entities ground into rock context");
    
    // Verify all results are from Field perspective
    for (const auto& result : rock_field) {
        assert(result.discovered_through == LabDb::TriadicQuery::Perspective::Field);
        assert(result.context == "rock");
    }
    
    // Test field_for_relation: What contexts ground hasColor relations?
    auto hascolor_field = triadic.field_for_relation("hasColor");
    assert(hascolor_field.size() == 2 && "Two contexts for hasColor");
    
    // Test primary_contexts: What are the main grounding contexts?
    auto primary = triadic.primary_contexts();
    assert(primary.size() >= 2 && "Should have multiple contexts");
    assert(primary[0] == "rock" && "Rock should be the primary context");
    
    std::cout << "✅ Field perspective test passed\n";
}

void test_cube_navigation() {
    std::cout << "Testing Cube Architecture Navigation...\n";
    
    std::string test_db = "/tmp/labdb_triadic_cube_test";
    std::filesystem::remove_all(test_db);
    
    auto store = std::make_shared<LabDb::NonoStore>(test_db);
    LabDb::TriadicQuery triadic(store);
    
    // Build rich knowledge graph
    store->add_triple("granite", "isA", "rock");
    store->add_triple("granite", "hasColor", "gray");
    store->add_triple("granite", "hasHardness", "6");
    store->add_triple("marble", "isA", "rock");
    store->add_triple("marble", "hasColor", "white");
    store->add_triple("coal", "hasColor", "gray");
    store->add_triple("slate", "hasColor", "gray");
    
    // Test perspective_shift: Start with Motion, shift to Memory
    auto granite_motion = triadic.motion_from("granite");
    auto shifted_to_memory = triadic.perspective_shift(granite_motion, LabDb::TriadicQuery::Perspective::Memory);
    assert(shifted_to_memory.size() >= granite_motion.size() && "Memory shift should expand perspective");
    
    // Test triadic_traverse: Deep exploration from granite
    auto traversal = triadic.triadic_traverse("granite", 2);
    assert(traversal.size() >= 3 && "Traversal should discover multiple connections");
    
    // Test crown_exploration: Comprehensive view around granite
    auto crown = triadic.crown_exploration("granite", "*", "*");
    assert(crown.size() >= 3 && "Crown exploration should reveal comprehensive connections");
    
    // Test optimal_perspective utility
    auto motion_optimal = LabDb::TriadicQuery::optimal_perspective("granite", "*", "*");
    assert(motion_optimal == LabDb::TriadicQuery::Perspective::Motion);
    
    auto memory_optimal = LabDb::TriadicQuery::optimal_perspective("*", "isA", "*");
    assert(memory_optimal == LabDb::TriadicQuery::Perspective::Memory);
    
    auto field_optimal = LabDb::TriadicQuery::optimal_perspective("*", "*", "rock");
    assert(field_optimal == LabDb::TriadicQuery::Perspective::Field);
    
    std::cout << "✅ Cube navigation test passed\n";
}

void test_triadic_analytics() {
    std::cout << "Testing Triadic Analytics...\n";
    
    std::string test_db = "/tmp/labdb_triadic_analytics_test";
    std::filesystem::remove_all(test_db);
    
    auto store = std::make_shared<LabDb::NonoStore>(test_db);
    LabDb::TriadicQuery triadic(store);
    
    // Build substantial knowledge graph
    store->add_triple("granite", "isA", "rock");
    store->add_triple("granite", "hasColor", "gray");
    store->add_triple("granite", "hasHardness", "6");
    store->add_triple("granite", "usedFor", "construction");
    store->add_triple("marble", "isA", "rock");
    store->add_triple("marble", "hasColor", "white");
    store->add_triple("marble", "usedFor", "sculpture");
    store->add_triple("diamond", "isA", "mineral");
    store->add_triple("diamond", "hasHardness", "10");
    
    // Test triadic statistics
    auto stats = triadic.get_triadic_stats();
    assert(stats.motion_entities == 3 && "Should have 3 motion entities");
    assert(stats.memory_relations == 4 && "Should have 4 memory relations");
    assert(stats.field_contexts == 8 && "Should have 8 field contexts");
    assert(stats.total_connections == 9 && "Should have 9 total connections");
    assert(stats.connectivity_ratio > 0.0 && "Should have positive connectivity");
    
    // Test bridge entity detection
    auto bridges = triadic.bridge_entities(2.0);
    assert(bridges.size() >= 1 && "Should find bridge entities");
    
    // Test relationship clustering
    auto clusters = triadic.detect_relationship_clusters();
    assert(clusters.size() >= 1 && "Should detect relationship clusters");
    
    // Test vocabulary boundary detection
    auto boundaries = triadic.detect_vocabulary_boundaries();
    assert(boundaries.size() >= 1 && "Should detect vocabulary boundaries");
    assert(boundaries[0].coherence_score >= 0.0 && "Should calculate coherence");
    
    std::cout << "✅ Triadic analytics test passed\n";
}

void demonstrate_triadic_consciousness() {
    std::cout << "\n=== Demonstrating Complete Triadic Consciousness ===\n";
    
    std::string test_db = "/tmp/labdb_triadic_demo";
    std::filesystem::remove_all(test_db);
    
    auto store = std::make_shared<LabDb::NonoStore>(test_db);
    LabDb::TriadicQuery triadic(store);
    
    // Build comprehensive knowledge graph
    store->add_triple("granite", "isA", "rock");
    store->add_triple("granite", "hasColor", "gray");
    store->add_triple("granite", "hasHardness", "6");
    store->add_triple("granite", "usedFor", "construction");
    store->add_triple("granite", "formedBy", "cooling");
    store->add_triple("marble", "isA", "rock");
    store->add_triple("marble", "hasColor", "white");
    store->add_triple("marble", "usedFor", "sculpture");
    store->add_triple("marble", "formedBy", "metamorphism");
    store->add_triple("diamond", "isA", "mineral");
    store->add_triple("diamond", "hasHardness", "10");
    store->add_triple("diamond", "hasColor", "clear");
    store->add_triple("quartz", "isA", "mineral");
    store->add_triple("quartz", "hasColor", "clear");
    
    std::cout << "Knowledge Graph Built:\n";
    std::cout << "  " << store->get_stats().total_triples << " triples across 9 indices\n\n";
    
    // Demonstrate Motion perspective (स्पन्द)
    std::cout << "MOTION (स्पन्द) - How does granite express itself?\n";
    auto granite_motion = triadic.motion_from("granite");
    for (const auto& result : granite_motion) {
        std::cout << "  " << result.entity << " " << result.relation << " " << result.context 
                  << " [discovered through " << LabDb::TriadicQuery::perspective_sanskrit(result.discovered_through) << "]\n";
    }
    
    // Demonstrate Memory perspective (स्मृति)
    std::cout << "\nMEMORY (स्मृति) - What patterns exist in isA relationships?\n";
    auto isa_memory = triadic.memory_relations("isA");
    auto frequencies = triadic.relation_frequencies();
    std::cout << "  Relationship frequency analysis:\n";
    for (const auto& [relation, count] : frequencies) {
        std::cout << "    " << relation << ": " << count << " connections\n";
    }
    
    // Demonstrate Field perspective (क्षेत्र)
    std::cout << "\nFIELD (क्षेत्र) - What receives into 'rock' context?\n";
    auto rock_field = triadic.field_contexts("rock");
    for (const auto& result : rock_field) {
        std::cout << "  " << result.entity << " " << result.relation << " " << result.context 
                  << " [grounded through " << LabDb::TriadicQuery::perspective_sanskrit(result.discovered_through) << "]\n";
    }
    
    // Demonstrate Cube Navigation
    std::cout << "\nCUBE NAVIGATION - Perspective shift from Motion to Memory:\n";
    auto shifted = triadic.perspective_shift(granite_motion, LabDb::TriadicQuery::Perspective::Memory);
    std::cout << "  Shifted from " << granite_motion.size() << " Motion results to " 
              << shifted.size() << " Memory connections\n";
    
    // Demonstrate Triadic Traversal
    std::cout << "\nTRIADIC TRAVERSAL - Deep exploration from granite:\n";
    auto traversal = triadic.triadic_traverse("granite", 2);
    std::cout << "  Discovered " << traversal.size() << " connections through 2-level traversal\n";
    
    // Demonstrate Crown Exploration
    std::cout << "\nCROWN EXPLORATION - Complete view around granite:\n";
    auto crown = triadic.crown_exploration("granite", "*", "*");
    std::cout << "  Crown reveals " << crown.size() << " total relationship patterns\n";
    
    // Demonstrate Analytics
    std::cout << "\nTRIADIC ANALYTICS:\n";
    auto stats = triadic.get_triadic_stats();
    std::cout << "  Motion entities (subjects): " << stats.motion_entities << "\n";
    std::cout << "  Memory relations (predicates): " << stats.memory_relations << "\n";
    std::cout << "  Field contexts (objects): " << stats.field_contexts << "\n";
    std::cout << "  Connectivity ratio: " << stats.connectivity_ratio << "\n";
    std::cout << "  Vocabulary density: " << stats.vocabulary_density << "\n";
    
    auto bridges = triadic.bridge_entities(2.0);
    std::cout << "  Bridge entities: ";
    for (size_t i = 0; i < bridges.size(); ++i) {
        std::cout << bridges[i];
        if (i < bridges.size() - 1) std::cout << ", ";
    }
    std::cout << "\n";
    
    std::cout << "\nThis demonstrates complete triadic consciousness:\n";
    std::cout << "- Conscious navigation through Motion/Memory/Field perspectives\n";
    std::cout << "- Dynamic perspective shifting and cube architecture traversal\n";
    std::cout << "- Analytical insights into knowledge structure and patterns\n";
    std::cout << "- Complete ontological awareness through vocabulary discovery\n";
    std::cout << "- त्रित्रयम् (tritrayam) principles fully operational in computational form\n";
}

int main() {
    std::cout << "=== LabDb Triadic Query Interface Tests ===\n";
    
    test_motion_perspective();
    test_memory_perspective();
    test_field_perspective();
    test_cube_navigation();
    test_triadic_analytics();
    
    demonstrate_triadic_consciousness();
    
    std::cout << "\n🎉 All Triadic Query Interface tests passed!\n";
    std::cout << "Triadic consciousness navigation fully operational.\n";
    
    return 0;
}
