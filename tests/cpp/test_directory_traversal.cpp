#include "test_harness.h"
#include "LabDb/DirectoryTraversal.h"
#include <iostream>
#include <filesystem>

using namespace labdb9_test;
using namespace LabDb::FileSearch;

// Helper function to filter by extension
std::vector<FileInfo> filterByExtension(const std::vector<FileInfo>& files, const std::string& ext) {
    std::vector<FileInfo> filtered;
    for (const auto& file : files) {
        if (file.extension == ext) {
            filtered.push_back(file);
        }
    }
    return filtered;
}

TEST(basic_directory_listing) {
    std::string test_dir = "testenv";
    
    TraversalConfig config;
    config.max_depth = 0; // Only list immediate directory
    DirectoryTraversal traversal(config);
    
    auto all_files = traversal.traverse(test_dir);
    auto md_files = filterByExtension(all_files, ".md");
    
    std::cout << "Basic listing found " << md_files.size() << " .md files" << std::endl;
    EXPECT_GT(md_files.size(), 0);
}

TEST(recursive_traversal_depth_1) {
    std::string test_dir = "testenv";
    
    TraversalConfig config;
    config.max_depth = 1; // Go one level deep
    DirectoryTraversal traversal(config);
    
    auto all_files = traversal.traverse(test_dir);
    auto md_files = filterByExtension(all_files, ".md");
    
    std::cout << "Depth 1 traversal found " << md_files.size() << " .md files" << std::endl;
    for (const auto& file : md_files) {
        std::cout << "  Found: " << file.relative_path << std::endl;
    }
    
    // Should find files in subdirectories
    EXPECT_GT(md_files.size(), 3); // At least adventure/, scifi/, fantasy/ files
}

TEST(recursive_traversal_full_depth) {
    std::string test_dir = "testenv";
    
    TraversalConfig config;
    config.max_depth = 3; // Full traversal
    DirectoryTraversal traversal(config);
    
    auto all_files = traversal.traverse(test_dir);
    auto md_files = filterByExtension(all_files, ".md");
    
    std::cout << "Full traversal found " << md_files.size() << " .md files" << std::endl;
    
    // Count files by directory
    int adventure_count = 0, scifi_count = 0, fantasy_count = 0, unicode_count = 0;
    for (const auto& file : md_files) {
        std::cout << "  Found: " << file.relative_path << " (" << file.file_size << " bytes)" << std::endl;
        if (file.relative_path.find("adventure/") != std::string::npos) adventure_count++;
        if (file.relative_path.find("scifi/") != std::string::npos) scifi_count++;
        if (file.relative_path.find("fantasy/") != std::string::npos) fantasy_count++;
        if (file.relative_path.find("unicode_test/") != std::string::npos) unicode_count++;
    }
    
    std::cout << "Directory breakdown:" << std::endl;
    std::cout << "  adventure/: " << adventure_count << " files" << std::endl;
    std::cout << "  scifi/: " << scifi_count << " files" << std::endl;
    std::cout << "  fantasy/: " << fantasy_count << " files" << std::endl;
    std::cout << "  unicode_test/: " << unicode_count << " files" << std::endl;
    
    // Validate expected structure
    EXPECT_EQ(adventure_count, 3); // ark, grail, falcon
    EXPECT_EQ(scifi_count, 3);     // briefcase, rabbit, tesseract
    EXPECT_EQ(fantasy_count, 3);   // ring, wand, staff
    EXPECT_EQ(unicode_count, 3);   // sanskrit, mixed, normalization
    EXPECT_EQ(md_files.size(), 14);  // Total McGuffin files
}

TEST(performance_metrics) {
    std::string test_dir = "testenv";
    
    TraversalConfig config;
    config.max_depth = 3;
    DirectoryTraversal traversal(config);
    
    auto start = std::chrono::high_resolution_clock::now();
    auto all_files = traversal.traverse(test_dir);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto md_files = filterByExtension(all_files, ".md");
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Performance metrics:" << std::endl;
    std::cout << "  .md files found: " << md_files.size() << std::endl;
    std::cout << "  All files found: " << all_files.size() << std::endl;
    std::cout << "  Traversal time: " << duration.count() << " microseconds" << std::endl;
    std::cout << "  Files processed: " << traversal.getFilesFound() << std::endl;
    std::cout << "  Traversal time (internal): " << traversal.getTraversalTimeMs() << " ms" << std::endl;
    
    // Performance should be sub-millisecond for our small test corpus
    EXPECT_LT(duration.count(), 10000); // Less than 10ms
    EXPECT_EQ(md_files.size(), 14);
}

int main() {
    std::cout << "=== DirectoryTraversal McGuffin Test Suite ===" << std::endl;
    
    // Tests now use relative paths from build directory
    return labdb9_test::run_all_tests();
}
