#include "LabDb/NonoStore.h"
#include "LabDb/TriadicQuery.h"
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>

void print_usage() {
    std::cout << "LabDb Explorer - Triadic Consciousness Database Explorer\n\n";
    std::cout << "Usage: labdb-explore <database_path> [command] [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  --stats                     Show database statistics\n";
    std::cout << "  --vocab                     Show vocabulary summary\n";
    std::cout << "  --sample motion N [offset]  Sample N motion entities (default: 10)\n";
    std::cout << "  --sample memory N [offset]  Sample N memory relations (default: 5)\n";
    std::cout << "  --sample field N [offset]   Sample N field contexts (default: 20)\n";
    std::cout << "  --sample all [M] [Mem] [F]  Sample from all perspectives (default: 5 5 10)\n";
    std::cout << "  --browse motion [limit]     Browse motion entities with pagination\n";
    std::cout << "  --browse memory [limit]     Browse memory relations with pagination\n";
    std::cout << "  --browse field [limit]      Browse field contexts with pagination\n";
    std::cout << "  --random motion N           Random sample N motion entities\n";
    std::cout << "  --random memory N           Random sample N memory relations\n";
    std::cout << "  --random field N            Random sample N field contexts\n";
    std::cout << "  --explore entity            Explore triadic relationships around entity\n";
    std::cout << "\nExamples:\n";
    std::cout << "  labdb-explore /path/to/db.lmdb --stats\n";
    std::cout << "  labdb-explore /path/to/db.lmdb --sample motion 10\n";
    std::cout << "  labdb-explore /path/to/db.lmdb --sample all 5 3 15\n";
    std::cout << "  labdb-explore /path/to/db.lmdb --browse field 25\n";
    std::cout << "  labdb-explore /path/to/db.lmdb --explore granite\n";
    std::cout << "\nत्रित्रयम् (tritrayam) - Motion/Memory/Field consciousness navigation\n";
}

void print_stats(LabDb::TriadicQuery& query) {
    auto stats = query.get_triadic_stats();
    auto store_stats = query.get_store()->get_stats();
    
    std::cout << "=== LabDb Triadic Statistics ===\n";
    std::cout << "Motion Entities (स्पन्द):     " << stats.motion_entities << " subjects\n";
    std::cout << "Memory Relations (स्मृति):     " << stats.memory_relations << " predicates\n";
    std::cout << "Field Contexts (क्षेत्र):       " << stats.field_contexts << " objects\n";
    std::cout << "Total Connections:           " << stats.total_connections << " triples\n";
    std::cout << "Connectivity Ratio:          " << std::fixed << std::setprecision(2) << stats.connectivity_ratio << "\n";
    std::cout << "Vocabulary Density:          " << std::fixed << std::setprecision(3) << stats.vocabulary_density << "\n";
    std::cout << "\n=== LMDB Statistics ===\n";
    std::cout << "Database Entries:            " << store_stats.lmdb_stats.entries << "\n";
    std::cout << "Page Size:                   " << store_stats.lmdb_stats.page_size << " bytes\n";
    std::cout << "Database Depth:              " << store_stats.lmdb_stats.depth << "\n";
    std::cout << "Branch Pages:                " << store_stats.lmdb_stats.branch_pages << "\n";
    std::cout << "Leaf Pages:                  " << store_stats.lmdb_stats.leaf_pages << "\n";
    std::cout << "Overflow Pages:              " << store_stats.lmdb_stats.overflow_pages << "\n";
}

void print_vocab_summary(LabDb::TriadicQuery& query) {
    auto motion_sample = query.sample_motion_entities(5);
    auto memory_sample = query.sample_memory_relations(5);
    auto field_sample = query.sample_field_contexts(10);
    
    std::cout << "=== Vocabulary Summary ===\n";
    std::cout << "Motion Entities (first 5):\n";
    for (const auto& entity : motion_sample) {
        std::cout << "  \"" << entity << "\"\n";
    }
    
    std::cout << "Memory Relations (first 5):\n";
    for (const auto& relation : memory_sample) {
        std::cout << "  \"" << relation << "\"\n";
    }
    
    std::cout << "Field Contexts (first 10):\n";
    for (const auto& context : field_sample) {
        std::cout << "  \"" << context << "\"\n";
    }
}

void sample_perspective(LabDb::TriadicQuery& query, const std::string& perspective, size_t count, size_t offset = 0) {
    std::vector<std::string> results;
    
    if (perspective == "motion") {
        results = query.sample_motion_entities(count, offset);
        std::cout << "=== Motion Entities (स्पन्द) - Sample " << count << " from offset " << offset << " ===\n";
    } else if (perspective == "memory") {
        results = query.sample_memory_relations(count, offset);
        std::cout << "=== Memory Relations (स्मृति) - Sample " << count << " from offset " << offset << " ===\n";
    } else if (perspective == "field") {
        results = query.sample_field_contexts(count, offset);
        std::cout << "=== Field Contexts (क्षेत्र) - Sample " << count << " from offset " << offset << " ===\n";
    } else {
        std::cout << "Unknown perspective: " << perspective << "\n";
        return;
    }
    
    for (size_t i = 0; i < results.size(); ++i) {
        std::cout << std::setw(3) << (offset + i + 1) << ": \"" << results[i] << "\"\n";
    }
    
    if (results.empty()) {
        std::cout << "No entities found.\n";
    }
}

void sample_all_perspectives(LabDb::TriadicQuery& query, size_t motion_count, size_t memory_count, size_t field_count, size_t offset = 0) {
    auto result = query.sample_triadic_entities(motion_count, memory_count, field_count, offset);
    
    std::cout << "=== Triadic Sampling (त्रित्रयम्) ===\n";
    std::cout << "Total entities: " << result.total_motion_count << " motion, " 
              << result.total_memory_count << " memory, " 
              << result.total_field_count << " field\n\n";
    
    std::cout << "Motion Entities (स्पन्द) - " << result.motion_entities.size() << " sampled:\n";
    for (size_t i = 0; i < result.motion_entities.size(); ++i) {
        std::cout << "  " << std::setw(2) << (i+1) << ": \"" << result.motion_entities[i] << "\"\n";
    }
    
    std::cout << "\nMemory Relations (स्मृति) - " << result.memory_relations.size() << " sampled:\n";
    for (size_t i = 0; i < result.memory_relations.size(); ++i) {
        std::cout << "  " << std::setw(2) << (i+1) << ": \"" << result.memory_relations[i] << "\"\n";
    }
    
    std::cout << "\nField Contexts (क्षेत्र) - " << result.field_contexts.size() << " sampled:\n";
    for (size_t i = 0; i < result.field_contexts.size(); ++i) {
        std::cout << "  " << std::setw(2) << (i+1) << ": \"" << result.field_contexts[i] << "\"\n";
    }
}

void random_sample_perspective(LabDb::TriadicQuery& query, const std::string& perspective, size_t count) {
    std::vector<std::string> results;
    
    if (perspective == "motion") {
        results = query.random_sample_motion(count);
        std::cout << "=== Random Motion Entities (स्पन्द) - " << count << " samples ===\n";
    } else if (perspective == "memory") {
        results = query.random_sample_memory(count);
        std::cout << "=== Random Memory Relations (स्मृति) - " << count << " samples ===\n";
    } else if (perspective == "field") {
        results = query.random_sample_field(count);
        std::cout << "=== Random Field Contexts (क्षेत्र) - " << count << " samples ===\n";
    } else {
        std::cout << "Unknown perspective: " << perspective << "\n";
        return;
    }
    
    for (size_t i = 0; i < results.size(); ++i) {
        std::cout << std::setw(3) << (i + 1) << ": \"" << results[i] << "\"\n";
    }
}

void explore_entity(LabDb::TriadicQuery& query, const std::string& entity) {
    std::cout << "=== Triadic Exploration of: \"" << entity << "\" ===\n";
    
    // Motion perspective - what does this entity express?
    auto motion_results = query.motion_from(entity);
    std::cout << "\nMotion (स्पन्द) - Entity expressions (" << motion_results.size() << "):\n";
    for (const auto& result : motion_results) {
        std::cout << "  " << result.entity << " --[" << result.relation << "]--> " << result.context << "\n";
    }
    
    // Find what contexts this entity appears in (as object)
    auto field_results = query.field_contexts(entity);
    std::cout << "\nField (क्षेत्र) - Entity as context (" << field_results.size() << "):\n";
    for (const auto& result : field_results) {
        std::cout << "  " << result.entity << " --[" << result.relation << "]--> " << result.context << "\n";
    }
    
    // Get entity expressions for memory analysis
    auto expressions = query.entity_expressions(entity);
    std::cout << "\nMemory (स्मृति) - Relationship types used (" << expressions.size() << "):\n";
    for (const auto& expr : expressions) {
        std::cout << "  \"" << expr << "\"\n";
    }
    
    if (motion_results.empty() && field_results.empty()) {
        std::cout << "\nEntity \"" << entity << "\" not found in database.\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage();
        return 1;
    }
    
    std::string db_path = argv[1];
    std::string command = argv[2];
    
    try {
        // Open database
        auto store = std::make_shared<LabDb::NonoStore>(db_path);
        LabDb::TriadicQuery query(store);
        
        if (command == "--stats") {
            print_stats(query);
        }
        else if (command == "--vocab") {
            print_vocab_summary(query);
        }
        else if (command == "--sample" && argc >= 4) {
            std::string perspective = argv[3];
            
            if (perspective == "all") {
                size_t motion_count = (argc > 4) ? std::stoul(argv[4]) : 5;
                size_t memory_count = (argc > 5) ? std::stoul(argv[5]) : 5;
                size_t field_count = (argc > 6) ? std::stoul(argv[6]) : 10;
                size_t offset = (argc > 7) ? std::stoul(argv[7]) : 0;
                sample_all_perspectives(query, motion_count, memory_count, field_count, offset);
            } else {
                size_t count = (argc > 4) ? std::stoul(argv[4]) : 10;
                size_t offset = (argc > 5) ? std::stoul(argv[5]) : 0;
                sample_perspective(query, perspective, count, offset);
            }
        }
        else if (command == "--browse" && argc >= 4) {
            std::string perspective = argv[3];
            size_t limit = (argc > 4) ? std::stoul(argv[4]) : 20;
            size_t offset = (argc > 5) ? std::stoul(argv[5]) : 0;
            sample_perspective(query, perspective, limit, offset);  // browse uses same logic as sample
        }
        else if (command == "--random" && argc >= 5) {
            std::string perspective = argv[3];
            size_t count = std::stoul(argv[4]);
            random_sample_perspective(query, perspective, count);
        }
        else if (command == "--explore" && argc >= 4) {
            std::string entity = argv[3];
            explore_entity(query, entity);
        }
        else {
            std::cout << "Unknown command or insufficient arguments.\n\n";
            print_usage();
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
