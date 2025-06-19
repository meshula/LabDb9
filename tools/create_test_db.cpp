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
        store->connect("granite", "isA", "rock");
        store->connect("granite", "hasColor", "gray");
        store->connect("granite", "hasHardness", "6");
        store->connect("quartz", "isA", "mineral");
        store->connect("quartz", "hasColor", "clear");
        store->connect("diamond", "isA", "mineral");
        store->connect("diamond", "hasHardness", "10");
        
        // More complex triadic relationships
        store->connect("consciousness", "manifests", "awareness");
        store->connect("awareness", "emerges", "through_attention");
        store->connect("attention", "focuses", "present_moment");
        store->connect("mind", "creates", "thoughts");
        store->connect("thoughts", "arise_in", "consciousness");
        
        // Field contexts (what receives and grounds)
        store->connect("wisdom", "flows_through", "understanding");
        store->connect("understanding", "bridges", "knowledge");
        store->connect("knowledge", "rests_in", "memory");
        
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
