#pragma once

#include "LabDb/NonoStore.h"
#include "LabDb/TriadicQuery.h"
#include <chrono>
#include <vector>
#include <string>
#include <functional>
#include <map>
#include <memory>

namespace LabDb {

/// Performance benchmarking suite for LabDb operations
class PerformanceBenchmark {
public:
    /// Benchmark result for a single test
    struct BenchmarkResult {
        std::string test_name;
        std::string description;
        size_t iterations;
        double total_time_ms;
        double avg_time_ms;
        double min_time_ms;
        double max_time_ms;
        double ops_per_second;
        size_t memory_used_bytes;
        std::map<std::string, double> custom_metrics;
    };
    
    /// Dataset configuration for benchmarks
    struct DatasetConfig {
        size_t num_triples;
        size_t num_subjects;
        size_t num_predicates;
        size_t num_objects;
        double relationship_density;  // 0.0 to 1.0
        std::string pattern_type;     // "random", "hierarchical", "clustered"
    };
    
    /// Query pattern for testing
    struct QueryPattern {
        std::string name;
        std::string subject_pattern;
        std::string predicate_pattern;
        std::string object_pattern;
        std::string description;
    };
    
    explicit PerformanceBenchmark(const std::string& benchmark_db_path);
    ~PerformanceBenchmark();
    
    /// Core operation benchmarks
    BenchmarkResult benchmark_insertions(const DatasetConfig& config, size_t iterations = 1000);
    BenchmarkResult benchmark_queries(const std::vector<QueryPattern>& patterns, 
                                       const DatasetConfig& dataset, 
                                       size_t iterations = 1000);
    BenchmarkResult benchmark_vocabulary_discovery(const DatasetConfig& dataset, 
                                                    size_t iterations = 100);
    
    /// Triadic operation benchmarks
    BenchmarkResult benchmark_motion_queries(const DatasetConfig& dataset, size_t iterations = 500);
    BenchmarkResult benchmark_memory_queries(const DatasetConfig& dataset, size_t iterations = 500);
    BenchmarkResult benchmark_field_queries(const DatasetConfig& dataset, size_t iterations = 500);
    BenchmarkResult benchmark_perspective_shifts(const DatasetConfig& dataset, size_t iterations = 100);
    BenchmarkResult benchmark_triadic_traversals(const DatasetConfig& dataset, size_t iterations = 50);
    
    /// Scalability benchmarks
    std::vector<BenchmarkResult> benchmark_scaling(const std::vector<DatasetConfig>& configs,
                                                    const std::string& operation_type);
    
    /// Comparison benchmarks
    BenchmarkResult benchmark_vs_simple_storage(const DatasetConfig& dataset);
    BenchmarkResult benchmark_index_efficiency(const DatasetConfig& dataset);
    
    /// Memory and resource benchmarks
    BenchmarkResult benchmark_memory_usage(const DatasetConfig& dataset);
    BenchmarkResult benchmark_concurrent_reads(const DatasetConfig& dataset, 
                                                size_t num_threads = 4);
    
    /// Comprehensive benchmark suite
    std::vector<BenchmarkResult> run_comprehensive_suite();
    
    /// Reporting
    void print_result(const BenchmarkResult& result);
    void print_comparison(const std::vector<BenchmarkResult>& results);
    void export_results_csv(const std::vector<BenchmarkResult>& results, 
                             const std::string& filename);
    void generate_performance_report(const std::vector<BenchmarkResult>& results,
                                     const std::string& filename);
    
    /// Utilities
    static std::vector<DatasetConfig> get_standard_dataset_configs();
    static std::vector<QueryPattern> get_standard_query_patterns();
    
private:
    std::string _benchmark_db_path;
    std::unique_ptr<NonoStore> _store;
    std::unique_ptr<TriadicQuery> _triadic;
    
    /// Dataset generation
    void generate_dataset(const DatasetConfig& config);
    void generate_random_dataset(const DatasetConfig& config);
    void generate_hierarchical_dataset(const DatasetConfig& config);
    void generate_clustered_dataset(const DatasetConfig& config);
    
    /// Timing utilities
    template<typename Func>
    BenchmarkResult time_operation(const std::string& name, 
                                   const std::string& description,
                                   size_t iterations, 
                                   Func&& operation);
    
    double get_current_time_ms();
    size_t get_memory_usage();
    
    /// Simple storage for comparison
    class SimpleStorage {
    public:
        struct Triple { std::string s, p, o; };
        
        void insert(const std::string& s, const std::string& p, const std::string& o);
        std::vector<Triple> query(const std::string& s, const std::string& p, const std::string& o);
        std::vector<std::string> all_subjects();
        std::vector<std::string> all_predicates();
        std::vector<std::string> all_objects();
        
    private:
        std::vector<Triple> _triples;
    };
};

} // namespace LabDb
