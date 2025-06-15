#include "LabDb/PerformanceBenchmark.h"
#include <iostream>
#include <filesystem>
#include <iomanip>

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
        
    } catch (const std::exception& e) {
        std::cerr << "Benchmark failed: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
