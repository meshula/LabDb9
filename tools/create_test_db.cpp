#include "LabDb/NonoStore.h"
#include "LabDb/TriadicQuery.h"
#include <iostream>

int main() {
    std::cout << "Creating test database with sample triadic data...\n";
    
    try {
        // Create a test database
        auto store = std::make_shared<LabDb::NonoStore>("test-data/sample_triadic.lmdb");
        
        // Add some sample triadic relationships
        std::cout << "Adding Motion/Memory/Field relationships...\n";
        
        // Motion-oriented entities (subjects that act)
        store->add_triple("granite", "isA", "rock");
        store->add_triple("granite", "hasColor", "gray");
        store->add_triple("granite", "hasHardness", "6");
        store->add_triple("quartz", "isA", "mineral");
        store->add_triple("quartz", "hasColor", "clear");
        store->add_triple("diamond", "isA", "mineral");
        store->add_triple("diamond", "hasHardness", "10");
        
        // More complex triadic relationships
        store->add_triple("consciousness", "manifests", "awareness");
        store->add_triple("awareness", "emerges", "through_attention");
        store->add_triple("attention", "focuses", "present_moment");
        store->add_triple("mind", "creates", "thoughts");
        store->add_triple("thoughts", "arise_in", "consciousness");
        
        // Field contexts (what receives and grounds)
        store->add_triple("wisdom", "flows_through", "understanding");
        store->add_triple("understanding", "bridges", "knowledge");
        store->add_triple("knowledge", "rests_in", "memory");
        
        // Get statistics
        LabDb::TriadicQuery query(store);
        auto stats = query.get_triadic_stats();
        
        std::cout << "\n=== Created Test Database ===\n";
        std::cout << "Motion Entities: " << stats.motion_entities << "\n";
        std::cout << "Memory Relations: " << stats.memory_relations << "\n";
        std::cout << "Field Contexts: " << stats.field_contexts << "\n";
        std::cout << "Total Connections: " << stats.total_connections << "\n";
        std::cout << "Database created successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
