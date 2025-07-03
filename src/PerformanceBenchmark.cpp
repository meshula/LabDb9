#include "LabDb/PerformanceBenchmark.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <random>
#include <algorithm>
#include <thread>
#include <future>
#include <filesystem>
#include <sstream>
#include <numeric>
#include <set>

#if defined(__APPLE__)
#include <mach/mach.h>
#elif defined(__linux__)
#include <sys/resource.h>
#elif defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#endif

namespace LabDb {

PerformanceBenchmark::PerformanceBenchmark(const std::string& benchmark_db_path) 
    : _benchmark_db_path(benchmark_db_path) {
    std::filesystem::remove_all(_benchmark_db_path);
    _store = std::make_unique<NonoStore>(_benchmark_db_path);
    _triadic = std::make_unique<TriadicQuery>(std::shared_ptr<NonoStore>(_store.get(), [](NonoStore*) {}));
}

PerformanceBenchmark::~PerformanceBenchmark() = default;

// Timing utility method - needs to be here due to template instantiation
template<typename Func>
PerformanceBenchmark::BenchmarkResult PerformanceBenchmark::time_operation(
    const std::string& name, 
    const std::string& description,
    size_t iterations, 
    Func&& operation) {
    
    BenchmarkResult result;
    result.test_name = name;
    result.description = description;
    result.iterations = iterations;
    
    std::vector<double> times;
    times.reserve(iterations);
    
    size_t initial_memory = get_memory_usage();
    
    for (size_t i = 0; i < iterations; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        operation();
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        times.push_back(duration.count() / 1000.0);  // Convert to milliseconds
    }
    
    result.memory_used_bytes = get_memory_usage() - initial_memory;
    
    // Calculate statistics
    result.total_time_ms = std::accumulate(times.begin(), times.end(), 0.0);
    result.avg_time_ms = result.total_time_ms / iterations;
    result.min_time_ms = *std::min_element(times.begin(), times.end());
    result.max_time_ms = *std::max_element(times.begin(), times.end());
    result.ops_per_second = 1000.0 / result.avg_time_ms;
    
    return result;
}

PerformanceBenchmark::BenchmarkResult PerformanceBenchmark::benchmark_insertions(
    const DatasetConfig& config, size_t iterations) {
    
    return time_operation(
        "Insertions",
        "Measuring triple insertion performance with " + std::to_string(config.num_triples) + " triples",
        iterations,
        [this, &config]() {
            std::filesystem::remove_all(_benchmark_db_path + "_insert_test");
            auto temp_store = std::make_unique<NonoStore>(_benchmark_db_path + "_insert_test");
            
            std::random_device rd;
            std::mt19937 gen(rd());
            
            for (size_t i = 0; i < config.num_triples; ++i) {
                std::string subject = "entity_" + std::to_string(gen() % config.num_subjects);
                std::string predicate = "relation_" + std::to_string(gen() % config.num_predicates);
                std::string object = "value_" + std::to_string(gen() % config.num_objects);
                temp_store->add_triple(subject, predicate, object);
            }
            
            auto stats = temp_store->get_stats();
            return static_cast<double>(stats.lmdb_stats.entries);
        }
    );
}

PerformanceBenchmark::BenchmarkResult PerformanceBenchmark::benchmark_queries(
    const std::vector<QueryPattern>& patterns, 
    const DatasetConfig& dataset,
    size_t iterations) {
    
    generate_dataset(dataset);
    
    return time_operation(
        "Queries",
        "Measuring query performance across " + std::to_string(patterns.size()) + " patterns",
        iterations,
        [this, &patterns]() {
            double total_results = 0;
            for (const auto& pattern : patterns) {
                auto results = _store->query(pattern.subject_pattern, 
                                              pattern.predicate_pattern, 
                                              pattern.object_pattern);
                total_results += results.size();
            }
            return total_results;
        }
    );
}

PerformanceBenchmark::BenchmarkResult PerformanceBenchmark::benchmark_vocabulary_discovery(
    const DatasetConfig& dataset, size_t iterations) {
    
    generate_dataset(dataset);
    
    return time_operation(
        "Vocabulary Discovery",
        "Measuring vocabulary discovery performance",
        iterations,
        [this]() {
            auto subjects = _store->all_subjects();
            auto predicates = _store->all_predicates();
            auto objects = _store->all_objects();
            return static_cast<double>(subjects.size() + predicates.size() + objects.size());
        }
    );
}

PerformanceBenchmark::BenchmarkResult PerformanceBenchmark::benchmark_vs_simple_storage(
    const DatasetConfig& dataset) {
    
    SimpleStorage simple;
    auto simple_time = time_operation(
        "Simple Storage",
        "Linear search baseline comparison",
        1,
        [&simple, &dataset]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            
            for (size_t i = 0; i < dataset.num_triples; ++i) {
                std::string subject = "entity_" + std::to_string(gen() % dataset.num_subjects);
                std::string predicate = "relation_" + std::to_string(gen() % dataset.num_predicates);
                std::string object = "value_" + std::to_string(gen() % dataset.num_objects);
                simple.insert(subject, predicate, object);
            }
            
            double total_results = 0;
            for (size_t i = 0; i < 100; ++i) {
                auto results = simple.query("entity_0", "*", "*");
                total_results += results.size();
            }
            return total_results;
        }
    );
    
    generate_dataset(dataset);
    auto nonostore_time = time_operation(
        "NonoStore",
        "Optimized nine-index performance",
        1,
        [this]() {
            double total_results = 0;
            for (size_t i = 0; i < 100; ++i) {
                auto results = _store->query("entity_0", "*", "*");
                total_results += results.size();
            }
            return total_results;
        }
    );
    
    BenchmarkResult comparison;
    comparison.test_name = "NonoStore vs Simple";
    comparison.description = "Performance comparison: NonoStore vs linear search";
    comparison.iterations = 1;
    comparison.total_time_ms = nonostore_time.total_time_ms;
    comparison.avg_time_ms = nonostore_time.avg_time_ms;
    comparison.custom_metrics["simple_time_ms"] = simple_time.avg_time_ms;
    comparison.custom_metrics["speedup_factor"] = simple_time.avg_time_ms / nonostore_time.avg_time_ms;
    
    return comparison;
}

std::vector<PerformanceBenchmark::BenchmarkResult> PerformanceBenchmark::run_comprehensive_suite() {
    std::vector<BenchmarkResult> results;
    
    std::cout << "Running comprehensive LabDb performance benchmark suite...\
";
    
    std::vector<DatasetConfig> demo_configs = {
        {1000, 100, 10, 50, 0.1, "random"},
        {5000, 200, 15, 100, 0.2, "random"}
    };
    
    auto patterns = get_standard_query_patterns();
    
    for (const auto& config : demo_configs) {
        std::cout << "Testing with " << config.num_triples << " triples...\
";
        
        results.push_back(benchmark_insertions(config, 3));
        results.push_back(benchmark_queries(patterns, config, 25));
        results.push_back(benchmark_vocabulary_discovery(config, 10));
        results.push_back(benchmark_vs_simple_storage(config));
    }
    
    std::cout << "Comprehensive benchmark suite completed.\
";
    return results;
}

// Dataset generation
void PerformanceBenchmark::generate_dataset(const DatasetConfig& config) {
    _store = std::make_unique<NonoStore>(_benchmark_db_path);
    _triadic = std::make_unique<TriadicQuery>(std::shared_ptr<NonoStore>(_store.get(), [](NonoStore*) {}));
    generate_random_dataset(config);
}

void PerformanceBenchmark::generate_random_dataset(const DatasetConfig& config) {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    for (size_t i = 0; i < config.num_triples; ++i) {
        std::string subject = "entity_" + std::to_string(gen() % config.num_subjects);
        std::string predicate = "relation_" + std::to_string(gen() % config.num_predicates);
        std::string object = "value_" + std::to_string(gen() % config.num_objects);
        _store->add_triple(subject, predicate, object);
    }
}

std::vector<PerformanceBenchmark::DatasetConfig> PerformanceBenchmark::get_standard_dataset_configs() {
    return {
        {1000, 100, 10, 50, 0.1, "random"},
        {5000, 200, 15, 100, 0.2, "random"}
    };
}

std::vector<PerformanceBenchmark::QueryPattern> PerformanceBenchmark::get_standard_query_patterns() {
    return {
        {"Subject Lookup", "entity_0", "*", "*", "Find all properties of specific entity"},
        {"Predicate Lookup", "*", "relation_0", "*", "Find all connections using specific relation"},
        {"Object Lookup", "*", "*", "value_0", "Find all entities with specific value"},
        {"Specific Triple", "entity_0", "relation_0", "value_0", "Exact triple lookup"}
    };
}

// Reporting methods
void PerformanceBenchmark::print_result(const BenchmarkResult& result) {
    std::cout << "\
=== " << result.test_name << " ===\
";
    std::cout << "Description: " << result.description << "\
";
    std::cout << "Average Time: " << std::fixed << std::setprecision(3) << result.avg_time_ms << " ms\
";
    std::cout << "Ops/Second: " << std::fixed << std::setprecision(1) << result.ops_per_second << "\
";
    
    for (const auto& [key, value] : result.custom_metrics) {
        std::cout << key << ": " << std::fixed << std::setprecision(3) << value << "\
";
    }
}

void PerformanceBenchmark::print_comparison(const std::vector<BenchmarkResult>& results) {
    std::cout << "\
=== Performance Comparison ===\
";
    std::cout << std::left << std::setw(25) << "Test Name" 
              << std::setw(15) << "Avg Time (ms)"
              << std::setw(15) << "Ops/Second" << "\
";
    std::cout << std::string(55, '-') << "\
";
    
    for (const auto& result : results) {
        std::cout << std::left << std::setw(25) << result.test_name
                  << std::setw(15) << std::fixed << std::setprecision(3) << result.avg_time_ms
                  << std::setw(15) << std::fixed << std::setprecision(1) << result.ops_per_second << "\
";
    }
}

void PerformanceBenchmark::export_results_csv(const std::vector<BenchmarkResult>& results, 
                                               const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    file << "test_name,avg_time_ms,ops_per_second\
";
    for (const auto& result : results) {
        file << result.test_name << "," << result.avg_time_ms << "," << result.ops_per_second << "\
";
    }
    std::cout << "Results exported to " << filename << "\
";
}

void PerformanceBenchmark::generate_performance_report(const std::vector<BenchmarkResult>& results,
                                                       const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    file << "# LabDb Performance Report\
\
";
    file << "## Summary\
\
Total benchmarks: " << results.size() << "\
\
";
    
    for (const auto& result : results) {
        file << "### " << result.test_name << "\
";
        file << "- Average time: " << result.avg_time_ms << " ms\
";
        file << "- Operations per second: " << result.ops_per_second << "\
\
";
    }
    
    std::cout << "Performance report generated: " << filename << "\
";
}

size_t PerformanceBenchmark::get_memory_usage() {
#if defined(__APPLE__)
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &infoCount) != KERN_SUCCESS)
        return 0;
    return info.resident_size;
#else
    return 0;
#endif
}

// Simple storage implementation
void PerformanceBenchmark::SimpleStorage::insert(const std::string& s, const std::string& p, const std::string& o) {
    _triples.push_back({s, p, o});
}

std::vector<PerformanceBenchmark::SimpleStorage::Triple> PerformanceBenchmark::SimpleStorage::query(
    const std::string& s, const std::string& p, const std::string& o) {
    
    std::vector<Triple> results;
    for (const auto& triple : _triples) {
        bool match = true;
        if (s != "*" && triple.s != s) match = false;
        if (p != "*" && triple.p != p) match = false;
        if (o != "*" && triple.o != o) match = false;
        if (match) results.push_back(triple);
    }
    return results;
}

// Triadic operation benchmarks - leveraging TriadicQuery functionality
PerformanceBenchmark::BenchmarkResult PerformanceBenchmark::benchmark_motion_queries(
    const DatasetConfig& dataset, size_t iterations) {
    
    generate_dataset(dataset);
    
    return time_operation(
        "Motion Queries",
        "Measuring motion-driven query performance",
        iterations,
        [this]() {
            double total_results = 0;
            
            // Test motion-driven queries (what entities express)
            auto motion_results = _triadic->motion_from("entity_0");
            total_results += motion_results.size();
            
            // Test entity expressions
            auto expressions = _triadic->entity_expressions("entity_0");
            total_results += expressions.size();
            
            // Test motion through relationships
            auto motion_through = _triadic->motion_through("relation_0");
            total_results += motion_through.size();
            
            return total_results;
        }
    );
}

PerformanceBenchmark::BenchmarkResult PerformanceBenchmark::benchmark_memory_queries(
    const DatasetConfig& dataset, size_t iterations) {
    
    generate_dataset(dataset);
    
    return time_operation(
        "Memory Queries",
        "Measuring memory-driven connection analysis performance",
        iterations,
        [this]() {
            double total_results = 0;
            
            // Test memory-driven relationship queries
            auto memory_relations = _triadic->memory_relations("relation_0");
            total_results += memory_relations.size();
            
            // Test entity-to-entity relationship patterns
            auto memory_between = _triadic->memory_between("entity_0", "entity_1");
            total_results += memory_between.size();
            
            // Test relation frequency analysis
            auto frequencies = _triadic->relation_frequencies();
            total_results += frequencies.size();
            
            return total_results;
        }
    );
}

PerformanceBenchmark::BenchmarkResult PerformanceBenchmark::benchmark_field_queries(
    const DatasetConfig& dataset, size_t iterations) {
    
    generate_dataset(dataset);
    
    return time_operation(
        "Field Queries",
        "Measuring field-driven context analysis performance",
        iterations,
        [this]() {
            double total_results = 0;
            
            // Test field-driven context queries
            auto field_contexts = _triadic->field_contexts("value_0");
            total_results += field_contexts.size();
            
            // Test relation grounding contexts
            auto field_for_relation = _triadic->field_for_relation("relation_0");
            total_results += field_for_relation.size();
            
            // Test primary context discovery
            auto primary_contexts = _triadic->primary_contexts();
            total_results += primary_contexts.size();
            
            return total_results;
        }
    );
}

PerformanceBenchmark::BenchmarkResult PerformanceBenchmark::benchmark_perspective_shifts(
    const DatasetConfig& dataset, size_t iterations) {
    
    generate_dataset(dataset);
    
    return time_operation(
        "Perspective Shifts",
        "Measuring triadic perspective transformation performance",
        iterations,
        [this]() {
            double total_operations = 0;
            
            // Test crown exploration around focal entities
            std::vector<std::string> entities = {"entity_0", "entity_1", "entity_2"};
            for (const auto& entity : entities) {
                auto crown_results = _triadic->crown_exploration(entity);
                total_operations += crown_results.size();
                
                // Test perspective shift from motion to memory
                auto motion_results = _triadic->motion_from(entity);
                auto shifted_results = _triadic->perspective_shift(motion_results, TriadicQuery::Perspective::Memory);
                total_operations += shifted_results.size();
            }
            
            return total_operations;
        }
    );
}

PerformanceBenchmark::BenchmarkResult PerformanceBenchmark::benchmark_triadic_traversals(
    const DatasetConfig& dataset, size_t iterations) {
    
    generate_dataset(dataset);
    
    return time_operation(
        "Triadic Traversals",
        "Measuring complete triadic cube navigation performance",
        iterations,
        [this]() {
            double total_paths = 0;
            
            // Test full triadic traversals from different starting points
            std::vector<std::string> starting_points = {"entity_0", "entity_1", "entity_2"};
            for (const auto& start : starting_points) {
                auto traversal_results = _triadic->triadic_traverse(start, 3);
                total_paths += traversal_results.size();
            }
            
            // Test bridge entity detection
            auto bridge_entities = _triadic->bridge_entities(2.0);
            total_paths += bridge_entities.size();
            
            // Test relationship cluster detection
            auto clusters = _triadic->detect_relationship_clusters();
            for (const auto& cluster : clusters) {
                total_paths += cluster.size();
            }
            
            return total_paths;
        }
    );
}

// Scalability benchmarks
std::vector<PerformanceBenchmark::BenchmarkResult> PerformanceBenchmark::benchmark_scaling(
    const std::vector<DatasetConfig>& configs, const std::string& operation_type) {
    
    std::vector<BenchmarkResult> results;
    
    for (const auto& config : configs) {
        if (operation_type == "insertions") {
            results.push_back(benchmark_insertions(config, 10));
        } else if (operation_type == "queries") {
            auto patterns = get_standard_query_patterns();
            results.push_back(benchmark_queries(patterns, config, 50));
        } else if (operation_type == "triadic") {
            results.push_back(benchmark_triadic_traversals(config, 10));
        }
    }
    
    // Add scaling analysis to the last result
    if (!results.empty()) {
        auto& last = results.back();
        last.custom_metrics["scaling_coefficient"] = 
            (results.back().avg_time_ms / results.front().avg_time_ms) / 
            (configs.back().num_triples / static_cast<double>(configs.front().num_triples));
    }
    
    return results;
}

// Memory and resource benchmarks
PerformanceBenchmark::BenchmarkResult PerformanceBenchmark::benchmark_memory_usage(
    const DatasetConfig& dataset) {
    
    size_t initial_memory = get_memory_usage();
    
    generate_dataset(dataset);
    
    size_t post_generation_memory = get_memory_usage();
    
    // Perform various operations to measure memory impact
    auto patterns = get_standard_query_patterns();
    benchmark_queries(patterns, dataset, 10);
    
    size_t post_operations_memory = get_memory_usage();
    
    BenchmarkResult result;
    result.test_name = "Memory Usage";
    result.description = "Memory consumption analysis";
    result.iterations = 1;
    result.memory_used_bytes = post_operations_memory - initial_memory;
    
    result.custom_metrics["generation_memory_mb"] = 
        (post_generation_memory - initial_memory) / (1024.0 * 1024.0);
    result.custom_metrics["operations_memory_mb"] = 
        (post_operations_memory - post_generation_memory) / (1024.0 * 1024.0);
    result.custom_metrics["bytes_per_triple"] = 
        result.memory_used_bytes / static_cast<double>(dataset.num_triples);
    
    return result;
}

PerformanceBenchmark::BenchmarkResult PerformanceBenchmark::benchmark_concurrent_reads(
    const DatasetConfig& dataset, size_t num_threads) {
    
    generate_dataset(dataset);
    
    auto patterns = get_standard_query_patterns();
    
    return time_operation(
        "Concurrent Reads",
        "Multi-threaded query performance with " + std::to_string(num_threads) + " threads",
        1,
        [this, &patterns, num_threads]() {
            std::vector<std::future<size_t>> futures;
            
            for (size_t i = 0; i < num_threads; ++i) {
                futures.push_back(std::async(std::launch::async, [this, &patterns]() {
                    size_t total_results = 0;
                    for (size_t j = 0; j < 100; ++j) {
                        for (const auto& pattern : patterns) {
                            auto results = _store->query(pattern.subject_pattern,
                                                        pattern.predicate_pattern,
                                                        pattern.object_pattern);
                            total_results += results.size();
                        }
                    }
                    return total_results;
                }));
            }
            
            double total_operations = 0;
            for (auto& future : futures) {
                total_operations += future.get();
            }
            
            return total_operations;
        }
    );
}

PerformanceBenchmark::BenchmarkResult PerformanceBenchmark::benchmark_index_efficiency(
    const DatasetConfig& dataset) {
    
    generate_dataset(dataset);
    
    BenchmarkResult result;
    result.test_name = "Index Efficiency";
    result.description = "Analysis of index utilization and efficiency";
    result.iterations = 1;
    
    auto stats = _store->get_stats();
    
    // Calculate index utilization metrics
    result.custom_metrics["total_entries"] = stats.lmdb_stats.entries;
    result.custom_metrics["index_overhead_ratio"] = 
        (stats.lmdb_stats.entries * 9.0) / dataset.num_triples; // 9 indices
    
    // Test query selectivity across different patterns
    auto selective_time = time_operation(
        "Selective Query", "High selectivity query", 100,
        [this]() {
            auto results = _store->query("entity_0", "relation_0", "*");
            return static_cast<double>(results.size());
        }
    );
    
    auto broad_time = time_operation(
        "Broad Query", "Low selectivity query", 100,
        [this]() {
            auto results = _store->query("*", "relation_0", "*");
            return static_cast<double>(results.size());
        }
    );
    
    result.custom_metrics["selective_query_ms"] = selective_time.avg_time_ms;
    result.custom_metrics["broad_query_ms"] = broad_time.avg_time_ms;
    result.custom_metrics["selectivity_advantage"] = 
        broad_time.avg_time_ms / selective_time.avg_time_ms;
    
    return result;
}

// Dataset generation implementations
void PerformanceBenchmark::generate_hierarchical_dataset(const DatasetConfig& config) {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Generate hierarchical structure: root -> level1 -> level2 -> leaves
    size_t nodes_per_level = static_cast<size_t>(std::sqrt(config.num_subjects));
    
    // Root level
    for (size_t i = 0; i < nodes_per_level; ++i) {
        std::string root = "root_" + std::to_string(i);
        
        // Level 1 children
        for (size_t j = 0; j < nodes_per_level; ++j) {
            std::string child1 = "level1_" + std::to_string(i * nodes_per_level + j);
            _store->add_triple(root, "hasChild", child1);
            _store->add_triple(child1, "parentOf", root);
            
            // Level 2 children
            for (size_t k = 0; k < nodes_per_level && 
                 _store->get_stats().lmdb_stats.entries < config.num_triples; ++k) {
                std::string child2 = "level2_" + std::to_string(j * nodes_per_level + k);
                _store->add_triple(child1, "hasChild", child2);
                _store->add_triple(child2, "parentOf", child1);
                
                // Add some cross-connections
                if (gen() % 10 < config.relationship_density * 10) {
                    std::string sibling = "level2_" + std::to_string((j * nodes_per_level + k + 1) % (nodes_per_level * nodes_per_level));
                    _store->add_triple(child2, "relatedTo", sibling);
                }
            }
        }
    }
}

void PerformanceBenchmark::generate_clustered_dataset(const DatasetConfig& config) {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    size_t num_clusters = config.num_subjects / 10; // ~10 entities per cluster
    size_t entities_per_cluster = config.num_subjects / num_clusters;
    
    for (size_t cluster = 0; cluster < num_clusters; ++cluster) {
        std::string cluster_name = "cluster_" + std::to_string(cluster);
        
        // Dense intra-cluster connections
        for (size_t i = 0; i < entities_per_cluster; ++i) {
            std::string entity1 = "entity_" + std::to_string(cluster * entities_per_cluster + i);
            _store->add_triple(entity1, "memberOf", cluster_name);
            
            for (size_t j = i + 1; j < entities_per_cluster; ++j) {
                if (gen() % 10 < config.relationship_density * 10) {
                    std::string entity2 = "entity_" + std::to_string(cluster * entities_per_cluster + j);
                    std::string relation = "intra_" + std::to_string(gen() % config.num_predicates);
                    _store->add_triple(entity1, relation, entity2);
                }
            }
        }
        
        // Sparse inter-cluster connections
        if (cluster > 0 && gen() % 10 < 2) { // 20% chance of inter-cluster connection
            std::string other_cluster = "cluster_" + std::to_string(gen() % cluster);
            _store->add_triple(cluster_name, "connectedTo", other_cluster);
        }
    }
}

// Utility implementations
double PerformanceBenchmark::get_current_time_ms() {
    auto now = std::chrono::high_resolution_clock::now();
    auto epoch = now.time_since_epoch();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
    return static_cast<double>(milliseconds.count());
}

// SimpleStorage vocabulary methods
std::vector<std::string> PerformanceBenchmark::SimpleStorage::all_subjects() {
    std::set<std::string> subjects;
    for (const auto& triple : _triples) {
        subjects.insert(triple.s);
    }
    return std::vector<std::string>(subjects.begin(), subjects.end());
}

std::vector<std::string> PerformanceBenchmark::SimpleStorage::all_predicates() {
    std::set<std::string> predicates;
    for (const auto& triple : _triples) {
        predicates.insert(triple.p);
    }
    return std::vector<std::string>(predicates.begin(), predicates.end());
}

std::vector<std::string> PerformanceBenchmark::SimpleStorage::all_objects() {
    std::set<std::string> objects;
    for (const auto& triple : _triples) {
        objects.insert(triple.o);
    }
    return std::vector<std::string>(objects.begin(), objects.end());
}

} // namespace LabDb
