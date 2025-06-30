#include "LabDb/MigrationTool.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

/**
 * LabDb Migration Tool CLI
 * 
 * Command-line interface for migrating LabDb databases from string-based
 * storage to TID-based architecture.
 * 
 * Usage:
 *   labdb-migrate [options] <database_path>
 *   labdb-migrate --batch [options] <directory>
 *   labdb-migrate --rollback <database_path>
 *   labdb-migrate --status <database_path>
 */

void print_usage() {
    std::cout << "LabDb Migration Tool - String→TID Architecture Migration\n";
    std::cout << "========================================================\n\n";
    std::cout << "Usage:\n";
    std::cout << "  labdb-migrate [options] <database_path>     # Migrate single database\n";
    std::cout << "  labdb-migrate --batch [options] <directory> # Migrate all databases in directory\n";
    std::cout << "  labdb-migrate --rollback <database_path>    # Rollback to pre-migration state\n";
    std::cout << "  labdb-migrate --status <database_path>      # Check migration status\n";
    std::cout << "  labdb-migrate --estimate <database_path>    # Estimate migration requirements\n\n";
    std::cout << "Options:\n";
    std::cout << "  --no-backup          Skip backup creation (NOT RECOMMENDED)\n";
    std::cout << "  --no-validate        Skip migration validation\n";
    std::cout << "  --remove-old         Remove old string-based indices after migration\n";
    std::cout << "  --quiet              Suppress progress output\n";
    std::cout << "  --batch-size N       Process N triples per batch (default: 1000)\n";
    std::cout << "  --backup-suffix S    Use custom backup suffix (default: .pre_tid_migration)\n\n";
    std::cout << "Examples:\n";
    std::cout << "  labdb-migrate myapp.db\n";
    std::cout << "  labdb-migrate --no-validate --batch-size 5000 myapp.db\n";
    std::cout << "  labdb-migrate --batch /path/to/databases/\n";
    std::cout << "  labdb-migrate --rollback myapp.db\n\n";
}

void print_migration_stats(const LabDb::MigrationStats& stats) {
    std::cout << "\n🎯 Migration Statistics:\n";
    std::cout << "==========================================\n";
    
    // Timing information
    std::cout << "⏱️  Timing:\n";
    std::cout << "   Total time: " << stats.total_time.count() << "ms\n";
    std::cout << "   Backup: " << stats.backup_time.count() << "ms\n";
    std::cout << "   Scan: " << stats.scan_time.count() << "ms\n";
    std::cout << "   Dictionary build: " << stats.dictionary_build_time.count() << "ms\n";
    std::cout << "   Migration: " << stats.migration_time.count() << "ms\n";
    std::cout << "   Validation: " << stats.validation_time.count() << "ms\n\n";
    
    // Data information
    std::cout << "📊 Data:\n";
    std::cout << "   Total triples: " << stats.total_triples << "\n";
    std::cout << "   Unique subjects: " << stats.unique_subjects << "\n";
    std::cout << "   Unique predicates: " << stats.unique_predicates << "\n";
    std::cout << "   Unique objects: " << stats.unique_objects << "\n";
    std::cout << "   Total unique terms: " << stats.total_unique_terms << "\n";
    std::cout << "   Migrated triples: " << stats.migrated_triples << "\n\n";
    
    // Performance metrics
    std::cout << "🚀 Performance:\n";
    std::cout << "   Migration rate: " << std::fixed << std::setprecision(1) 
              << stats.migration_rate_triples_per_sec << " triples/sec\n";
    std::cout << "   Storage reduction: " << std::fixed << std::setprecision(2) 
              << stats.storage_reduction_ratio << "× more efficient\n\n";
    
    // Status flags
    std::cout << "✅ Status:\n";
    std::cout << "   Backup created: " << (stats.backup_created ? "YES" : "NO") << "\n";
    std::cout << "   Dictionary built: " << (stats.dictionary_built ? "YES" : "NO") << "\n";
    std::cout << "   Migration completed: " << (stats.migration_completed ? "YES" : "NO") << "\n";
    std::cout << "   Validation passed: " << (stats.validation_passed ? "YES" : "NO") << "\n";
    std::cout << "   Rollback available: " << (stats.rollback_available ? "YES" : "NO") << "\n";
    
    if (stats.validation_errors > 0) {
        std::cout << "   Validation errors: " << stats.validation_errors << "\n";
    }
}

void progress_callback(const std::string& operation, double percent) {
    // Create a simple progress bar
    int bar_width = 40;
    int filled = static_cast<int>(bar_width * percent / 100.0);
    
    std::cout << "\r[";
    for (int i = 0; i < bar_width; ++i) {
        if (i < filled) {
            std::cout << "█";
        } else {
            std::cout << " ";
        }
    }
    std::cout << "] " << std::fixed << std::setprecision(1) << percent << "% " << operation;
    std::cout.flush();
    
    if (percent >= 100.0) {
        std::cout << "\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }
    
    std::cout << "🔧 LabDb Migration Tool - Phase 2.5\n";
    std::cout << "=====================================\n\n";
    std::cout << "This tool safely migrates LabDb databases from string-based\n";
    std::cout << "storage to the new TID-based architecture.\n\n";
    std::cout << "✅ Features:\n";
    std::cout << "  • Automatic backup creation\n";
    std::cout << "  • Data integrity validation\n";
    std::cout << "  • Rollback capability\n";
    std::cout << "  • Progress reporting\n";
    std::cout << "  • Batch processing\n\n";
    
    // For now, just show that the tool framework is ready
    std::cout << "🚧 Status: Migration tool framework complete!\n";
    std::cout << "📋 Ready for implementation of core migration logic.\n\n";
    
    std::cout << "Next steps:\n";
    std::cout << "1. Implement string-based crown index scanning\n";
    std::cout << "2. Build TermDictionary from unique strings\n";
    std::cout << "3. Migrate crown indices to TID-based keys\n";
    std::cout << "4. Validate migrated data integrity\n";
    std::cout << "5. Enable production migration workflows\n\n";
    
    print_usage();
    return 0;
}
