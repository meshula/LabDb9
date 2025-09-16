#include "LabDb/PerformanceBenchmark.h"
#include <iostream>
#include <filesystem>
#include <iomanip>
#include <fstream>
#include <format>
#include <cstring>
#include <cstdlib>
#include <chrono>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <numeric>

void benchmark_euclid() {
    std::cout << "Running Euclid benchmark...\n";
    
    // Structure to hold Euclid entry data
    struct EuclidEntry {
        size_t line_number;
        std::string text_content;
        std::string sequence_id;  // rope:{name}:{seq_id} pattern
    };
    
    std::vector<EuclidEntry> entries;
    
    // Read and parse the Euclid transliteration file
    std::ifstream file("testenv/euclid/sources/euclid_final-translit.txt");
    if (!file.is_open()) {
        std::cerr << "Failed to open Euclid transliteration file\n";
        return;
    }
    
    std::cout << "Parsing Euclid transliteration file...\n";
    std::string line;
    size_t current_line_number = 0;
    size_t sequence_counter = 0;
    
    while (std::getline(file, line) && entries.size() < 1000) {
        // Parse line markers: "# line {number}"
        if (line.starts_with("# line ")) {
            std::string line_num_str = line.substr(7);
            current_line_number = std::stoul(line_num_str);
        } else if (!line.empty() && current_line_number > 0) {
            // This is actual Greek text content
            EuclidEntry entry;
            entry.line_number = current_line_number;
            entry.text_content = line;
            // Generate sequence ID in rope pattern
            entry.sequence_id = std::format("{:06d}000", sequence_counter++);
            entries.push_back(entry);
        }
    }
    
    std::cout << "Parsed " << entries.size() << " Euclid entries\n";
    
    // Set up benchmark database
    std::string benchmark_db = "/tmp/euclid_rope_benchmark";
    std::filesystem::remove_all(benchmark_db);
    
    // Pre-allocate text buffer for memory optimization
    size_t total_text_size = 0;
    for (const auto& entry : entries) {
        total_text_size += entry.text_content.length() + 1;
    }
    
    char* text_buffer = static_cast<char*>(malloc(total_text_size));
    char* buffer_ptr = text_buffer;
    
    // Copy all text to pre-allocated buffer
    for (auto& entry : entries) {
        strcpy(buffer_ptr, entry.text_content.c_str());
        entry.text_content = std::string(buffer_ptr); // Point to buffer
        buffer_ptr += entry.text_content.length() + 1;
    }
    
    try {
        // Benchmark 1: Rope Storage Insertion (simulate rope triples)
        std::cout << "Benchmarking rope storage insertion...\n";
        
        auto insertion_start = std::chrono::high_resolution_clock::now();
        {
            LabDb::NonoStore store(benchmark_db + "_insert");
            for (size_t i = 0; i < entries.size(); ++i) {
                const auto& entry = entries[i];
                
                // Add entity for text content
                std::string entity_id = "entity_" + std::to_string(i);
                store.add_triple(entity_id, "text-content", entry.text_content);
                store.add_triple(entity_id, "xml-line", std::to_string(entry.line_number));
                
                // Add rope sequence triple: rope:euclid-complete-text:{seq_id} -> entity_id
                std::string rope_key = "rope:euclid-complete-text:" + entry.sequence_id;
                store.add_triple(rope_key, "points-to", entity_id);
                store.add_triple(rope_key, "sequence-position", std::to_string(i));
                
                // Add semantic markers for golden mean (simulate semantic discovery)
                if (entry.text_content.find("μέσον") != std::string::npos || 
                    entry.text_content.find("λόγον") != std::string::npos) {
                    store.add_triple(entity_id, "relates-to", "golden-mean");
                    store.add_triple(entity_id, "field-type", "definition");
                }
            }
        }
        auto insertion_end = std::chrono::high_resolution_clock::now();
        auto insertion_duration = std::chrono::duration_cast<std::chrono::milliseconds>(insertion_end - insertion_start);
        
        // Benchmark 2: Sequential Rope Retrieval
        std::cout << "Benchmarking sequential rope retrieval...\n";
        LabDb::NonoStore retrieval_store(benchmark_db + "_insert");
        
        auto retrieval_start = std::chrono::high_resolution_clock::now();
        double total_chars = 0;
        for (size_t i = 0; i < entries.size(); ++i) {
            // Query rope sequence: rope:euclid-complete-text:{seq_id}
            std::string rope_key = "rope:euclid-complete-text:" + entries[i].sequence_id;
            auto rope_results = retrieval_store.query(rope_key, "points-to", "*");
            
            if (!rope_results.empty()) {
                // Get the entity_id and fetch its content
                std::string entity_id = rope_results[0].object;
                auto content_results = retrieval_store.query(entity_id, "text-content", "*");
                if (!content_results.empty()) {
                    total_chars += content_results[0].object.length();
                }
            }
        }
        auto retrieval_end = std::chrono::high_resolution_clock::now();
        auto retrieval_duration = std::chrono::duration_cast<std::chrono::milliseconds>(retrieval_end - retrieval_start);
        
        // Benchmark 3: Bounded Chunk Retrieval (simulate 24-fragment chunks)
        std::cout << "Benchmarking bounded chunk retrieval...\n";
        
        auto chunk_start = std::chrono::high_resolution_clock::now();
        size_t total_chunks = 0;
        size_t chunk_size = 24;
        
        // Test chunks around different center positions
        for (size_t center = chunk_size/2; center < entries.size() - chunk_size/2; center += 100) {
            size_t start = center - chunk_size/2;
            size_t end = center + chunk_size/2;
            
            double chunk_chars = 0;
            for (size_t i = start; i < end && i < entries.size(); ++i) {
                std::string rope_key = "rope:euclid-complete-text:" + entries[i].sequence_id;
                auto rope_results = retrieval_store.query(rope_key, "points-to", "*");
                if (!rope_results.empty()) {
                    std::string entity_id = rope_results[0].object;
                    auto content_results = retrieval_store.query(entity_id, "text-content", "*");
                    if (!content_results.empty()) {
                        chunk_chars += content_results[0].object.length();
                    }
                }
            }
            total_chunks++;
        }
        auto chunk_end = std::chrono::high_resolution_clock::now();
        auto chunk_duration = std::chrono::duration_cast<std::chrono::milliseconds>(chunk_end - chunk_start);
        
        // Benchmark 4: Rope Index Population Strategies
        std::cout << "Benchmarking rope index population strategies...\n";
        
        // Strategy 1: std::map (RB-tree, suboptimal for ordered insertion)
        auto map_start = std::chrono::high_resolution_clock::now();
        std::map<int, std::string> map_index;
        
        // Get all subjects with rope prefix
        auto all_subjects = retrieval_store.all_subjects();
        for (const auto& subject : all_subjects) {
            if (subject.starts_with("rope:euclid-complete-text:") && 
                !subject.ends_with(":metadata")) {
                
                // Extract sequence ID from "rope:euclid-complete-text:000123000"
                size_t last_colon = subject.rfind(':');
                if (last_colon != std::string::npos) {
                    int seq_id = std::stoi(subject.substr(last_colon + 1));
                    
                    // Get entity_id for this sequence position
                    auto results = retrieval_store.query(subject, "points-to", "*");
                    if (!results.empty()) {
                        map_index[seq_id] = results[0].object; // RB-tree insertion
                    }
                }
            }
        }
        auto map_end = std::chrono::high_resolution_clock::now();
        auto map_duration = std::chrono::duration_cast<std::chrono::milliseconds>(map_end - map_start);
        
        // Strategy 2: std::vector (optimal for ordered insertion)
        auto vector_start = std::chrono::high_resolution_clock::now();
        std::vector<std::pair<int, std::string>> vector_index;
        vector_index.reserve(entries.size());
        
        // Collect all rope entries first
        std::vector<std::pair<int, std::string>> temp_entries;
        for (const auto& subject : all_subjects) {
            if (subject.starts_with("rope:euclid-complete-text:") && 
                !subject.ends_with(":metadata")) {
                
                size_t last_colon = subject.rfind(':');
                if (last_colon != std::string::npos) {
                    int seq_id = std::stoi(subject.substr(last_colon + 1));
                    auto results = retrieval_store.query(subject, "points-to", "*");
                    if (!results.empty()) {
                        temp_entries.emplace_back(seq_id, results[0].object);
                    }
                }
            }
        }
        
        // Sort by sequence ID (should already be ordered, but ensure it)
        std::sort(temp_entries.begin(), temp_entries.end());
        
        // Copy to final vector (ordered insertion)
        for (const auto& entry : temp_entries) {
            vector_index.emplace_back(entry);
        }
        auto vector_end = std::chrono::high_resolution_clock::now();
        auto vector_duration = std::chrono::duration_cast<std::chrono::milliseconds>(vector_end - vector_start);
        
        // Strategy 3: Hash map for reverse lookups (entity_id -> position)
        auto hash_start = std::chrono::high_resolution_clock::now();
        std::unordered_map<std::string, int> entity_to_position;
        entity_to_position.reserve(entries.size());
        
        for (const auto& entry : vector_index) {
            entity_to_position[entry.second] = entry.first;
        }
        auto hash_end = std::chrono::high_resolution_clock::now();
        auto hash_duration = std::chrono::duration_cast<std::chrono::milliseconds>(hash_end - hash_start);
        
        // Benchmark 5: Memex Navigation Scenarios
        std::cout << "Benchmarking Memex navigation scenarios...\n";
        
        // Scenario 1: "Quote the definition of the golden mean"
        auto semantic_start = std::chrono::high_resolution_clock::now();
        
        // Step 1: Semantic discovery (find golden mean entities)
        auto golden_mean_entities = retrieval_store.query("*", "relates-to", "golden-mean");
        std::cout << "  Found " << golden_mean_entities.size() << " golden mean references\n";
        
        std::string golden_mean_text;
        int golden_mean_position = -1;
        
        if (!golden_mean_entities.empty()) {
            // Step 2: Get the definition text
            std::string target_entity = golden_mean_entities[0].subject;
            auto content_results = retrieval_store.query(target_entity, "text-content", "*");
            if (!content_results.empty()) {
                golden_mean_text = content_results[0].object;
                // Step 3: Find rope position for this entity
                auto pos_it = entity_to_position.find(target_entity);
                if (pos_it != entity_to_position.end()) {
                    golden_mean_position = pos_it->second;
                }
            }
        }
        auto semantic_end = std::chrono::high_resolution_clock::now();
        auto semantic_duration = std::chrono::duration_cast<std::chrono::microseconds>(semantic_end - semantic_start);
        
        // Scenario 2: "Get context around the golden mean definition"
        auto context_start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::string> context_fragments;
        if (golden_mean_position >= 0) {
            // Get 24-fragment context window around the definition
            int start_pos = std::max(0, golden_mean_position - 12);
            int end_pos = std::min(static_cast<int>(vector_index.size()), golden_mean_position + 12);
            
            for (int pos = start_pos; pos < end_pos; ++pos) {
                if (pos < static_cast<int>(vector_index.size())) {
                    std::string entity_id = vector_index[pos].second;
                    auto content_results = retrieval_store.query(entity_id, "text-content", "*");
                    if (!content_results.empty()) {
                        context_fragments.push_back(content_results[0].object);
                    }
                }
            }
        }
        auto context_end = std::chrono::high_resolution_clock::now();
        auto context_duration = std::chrono::duration_cast<std::chrono::microseconds>(context_end - context_start);
        
        // Scenario 3: Navigation performance tests
        auto nav_start = std::chrono::high_resolution_clock::now();
        
        // Test 100 random entity->position lookups
        size_t successful_lookups = 0;
        for (int i = 0; i < 100; ++i) {
            std::string test_entity = "entity_" + std::to_string(i % entries.size());
            auto it = entity_to_position.find(test_entity);
            if (it != entity_to_position.end()) {
                successful_lookups++;
                // Test position->entity lookup using vector
                int position = it->second;
                if (position < static_cast<int>(vector_index.size())) {
                    std::string retrieved_entity = vector_index[position].second;
                    // Verify round-trip consistency
                    if (retrieved_entity == test_entity) {
                        // Success
                    }
                }
            }
        }
        auto nav_end = std::chrono::high_resolution_clock::now();
        auto nav_duration = std::chrono::duration_cast<std::chrono::microseconds>(nav_end - nav_start);
        
        // Report results
        std::cout << "\n=== Euclid Rope Benchmark Results ===\n";
        
        double insertion_ms = insertion_duration.count();
        double retrieval_ms = retrieval_duration.count();
        double chunk_ms = chunk_duration.count();
        
        std::cout << "Rope Storage Insertion:\n";
        std::cout << "  - Total time: " << std::fixed << std::setprecision(1) << insertion_ms << " ms\n";
        std::cout << "  - Entries processed: " << entries.size() << "\n";
        std::cout << "  - Throughput: " << std::fixed << std::setprecision(1) 
                  << (entries.size() * 1000.0 / insertion_ms) << " entries/sec\n";
        
        std::cout << "\nSequential Rope Retrieval:\n";
        std::cout << "  - Total time: " << std::fixed << std::setprecision(1) << retrieval_ms << " ms\n";
        std::cout << "  - Characters retrieved: " << static_cast<size_t>(total_chars) << "\n";
        std::cout << "  - Throughput: " << std::fixed << std::setprecision(1) 
                  << (entries.size() * 1000.0 / retrieval_ms) << " lookups/sec\n";
        
        std::cout << "\nBounded Chunk Retrieval:\n";
        std::cout << "  - Total time: " << std::fixed << std::setprecision(1) << chunk_ms << " ms\n";
        std::cout << "  - Chunks processed: " << total_chunks << "\n";
        std::cout << "  - Throughput: " << std::fixed << std::setprecision(1) 
                  << (total_chunks * 24 * 1000.0 / chunk_ms) << " fragments/sec\n";
        
        // Index Population Strategy Results
        std::cout << "\n=== Rope Index Population Strategy Results ===\n";
        std::cout << "std::map (RB-tree) Population:\n";
        std::cout << "  - Time: " << map_duration.count() << " ms\n";
        std::cout << "  - Entries: " << map_index.size() << "\n";
        std::cout << "  - Rate: " << std::fixed << std::setprecision(1) 
                  << (map_index.size() * 1000.0 / map_duration.count()) << " entries/sec\n";
        
        std::cout << "\nstd::vector (ordered) Population:\n";
        std::cout << "  - Time: " << vector_duration.count() << " ms\n";
        std::cout << "  - Entries: " << vector_index.size() << "\n";
        std::cout << "  - Rate: " << std::fixed << std::setprecision(1) 
                  << (vector_index.size() * 1000.0 / vector_duration.count()) << " entries/sec\n";
        
        std::cout << "\nHash Map (reverse lookup) Population:\n";
        std::cout << "  - Time: " << hash_duration.count() << " ms\n";
        std::cout << "  - Entries: " << entity_to_position.size() << "\n";
        std::cout << "  - Rate: " << std::fixed << std::setprecision(1) 
                  << (entity_to_position.size() * 1000.0 / hash_duration.count()) << " entries/sec\n";
        
        // Memex Navigation Scenario Results
        std::cout << "\n=== Memex Navigation Scenario Results ===\n";
        std::cout << "\"Quote the definition of the golden mean\":\n";
        std::cout << "  - Semantic discovery time: " << semantic_duration.count() << " μs\n";
        std::cout << "  - Golden mean position: " << golden_mean_position << "\n";
        std::cout << "  - Definition preview: \"" << golden_mean_text.substr(0, 50) << "...\"\n";
        
        std::cout << "\n\"Get context around the golden mean definition\":\n";
        std::cout << "  - Context retrieval time: " << context_duration.count() << " μs\n";
        std::cout << "  - Context fragments: " << context_fragments.size() << "\n";
        std::cout << "  - Total context chars: " << std::accumulate(context_fragments.begin(), context_fragments.end(), 0,
                     [](int sum, const std::string& s) { return sum + s.length(); }) << "\n";
        
        std::cout << "\nNavigation Performance (100 lookups):\n";
        std::cout << "  - Round-trip navigation time: " << nav_duration.count() << " μs\n";
        std::cout << "  - Average per lookup: " << std::fixed << std::setprecision(2) 
                  << (nav_duration.count() / 100.0) << " μs\n";
        std::cout << "  - Successful lookups: " << successful_lookups << "/100\n";
        
        // Performance Analysis for Implementation
        std::cout << "\n=== Implementation Decision Matrix ===\n";
        double index_startup_cost = map_duration.count() + hash_duration.count();
        double per_navigation_benefit = (retrieval_ms / entries.size()) - (nav_duration.count() / 100000.0);
        
        std::cout << "Index Population Analysis:\n";
        std::cout << "  - Total startup cost: " << index_startup_cost << " ms\n";
        std::cout << "  - Per-navigation speedup: " << std::fixed << std::setprecision(3) 
                  << per_navigation_benefit << " ms\n";
        std::cout << "  - Break-even point: " << std::fixed << std::setprecision(1) 
                  << (index_startup_cost / per_navigation_benefit) << " navigation operations\n";
        
        if (index_startup_cost / per_navigation_benefit < 100) {
            std::cout << "  - RECOMMENDATION: Use startup indexing (low break-even)\n";
        } else {
            std::cout << "  - RECOMMENDATION: Use on-demand queries (high break-even)\n";
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Euclid benchmark failed: " << e.what() << "\n";
    }
    
    // Clean up
    free(text_buffer);
    std::cout << "Euclid benchmark completed.\n";
}

int main() {
    std::cout << "=== LabDb Performance Benchmark Suite ===\n";
    
    // Create benchmark instance
    std::string benchmark_db = "/tmp/labdb_benchmark";
    std::filesystem::remove_all(benchmark_db);
    
    LabDb::PerformanceBenchmark benchmark(benchmark_db);
    
    try {
        // Run comprehensive benchmark suite
        auto results = benchmark.run_comprehensive_suite();
        
        // Print summary
        std::cout << "\n=== Benchmark Results Summary ===\n";
        benchmark.print_comparison(results);
        
        // Print detailed results for key benchmarks
        for (const auto& result : results) {
            if (result.test_name.find("vs Simple") != std::string::npos ||
                result.test_name == "Insertions" ||
                result.test_name == "Queries" ||
                result.test_name == "Vocabulary Discovery") {
                benchmark.print_result(result);
            }
        }
        
        // Export results
        benchmark.export_results_csv(results, "labdb_benchmark_results.csv");
        benchmark.generate_performance_report(results, "labdb_performance_report.md");
        
        std::cout << "\nBenchmark suite completed successfully!\n";
        
        // Show key metrics
        double total_ops_per_sec = 0;
        size_t op_count = 0;
        for (const auto& result : results) {
            if (result.ops_per_second > 0) {
                total_ops_per_sec += result.ops_per_second;
                op_count++;
            }
        }
        
        if (op_count > 0) {
            double avg_ops_per_sec = total_ops_per_sec / op_count;
            std::cout << "\nAverage Operations/Second: " << std::fixed << std::setprecision(1) 
                      << avg_ops_per_sec << "\n";
        }
        
        // Look for speedup results
        for (const auto& result : results) {
            if (result.custom_metrics.count("speedup_factor") > 0) {
                std::cout << "NonoStore Speedup: " << std::fixed << std::setprecision(1)
                          << result.custom_metrics.at("speedup_factor") << "x faster than linear search\n";
            }
        }
        
        // Run Euclid rope benchmark
        std::cout << "\n" << std::string(50, '=') << "\n";
        benchmark_euclid();
        
    } catch (const std::exception& e) {
        std::cerr << "Benchmark failed: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
