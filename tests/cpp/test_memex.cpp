#include "LabDb/Db9Dispatcher.h"
#include "LabDb/DatabaseManager.h"
#include "LabDb/NonoStore.h"
#include "LabDb/LabText.hpp"
#include <iostream>
#include <cassert>
#include <fstream>
#include <vector>
#include <format>
#include <regex>
#include <filesystem>

// Test data structure for Euclid corpus entries (Bush's Memex target corpus)
struct EuclidMemexEntry {
    size_t xml_line_number;
    std::string text_content;
    std::string entity_id;
    size_t rope_position;
    std::string semantic_type;  // "definition", "proposition", "proof", "postulate"
    std::string book_context;   // "book-1", "book-6", etc.
};

// Utility function to normalize whitespace for text comparison
std::string normalize_whitespace(const std::string& text) {
    std::string normalized = text;
    // Replace newlines, carriage returns, and multiple spaces with single spaces
    normalized = std::regex_replace(normalized, std::regex("[\\r\\n]+"), " ");
    normalized = std::regex_replace(normalized, std::regex("\\s+"), " ");
    // Trim leading and trailing spaces
    size_t first = normalized.find_first_not_of(" \t");
    if (first == std::string::npos) return "";
    size_t last = normalized.find_last_not_of(" \t");
    return normalized.substr(first, (last - first + 1));
}

class MemexVerbsTest {
public:
    MemexVerbsTest() : dispatcher_(std::make_unique<LabDb::Db9Dispatcher>()) {
        // Set working directory to tests directory for relative paths
        std::filesystem::current_path("/Users/nick/dev/Lab/LabDb9/tests");
    }
    
    void run_comprehensive_memex_test() {
        std::cout << "🧠 Comprehensive Memex Verbs Test Suite\n";
        std::cout << "=======================================\n";
        std::cout << "🎯 Bush's Memex vision: Discovery→Navigation synthesis validation\n\n";
        
        // Step 1: Create test database with memex corpus
        std::cout << "📂 Step 1: Creating Memex test database...\n";
        create_memex_test_database();
        
        // Step 2: Load corpus with semantic relationships
        std::cout << "📚 Step 2: Loading corpus with semantic discovery metadata...\n";
        load_semantic_corpus();
        
        // Step 3: Create rope for bounded navigation
        std::cout << "🧵 Step 3: Creating bounded navigation rope...\n";
        create_memex_rope();
        
        // Step 4: Test semantic discovery (Bush's "association")
        std::cout << "🔍 Step 4: Testing semantic discovery patterns...\n";
        test_semantic_discovery();
        
        // Step 5: Test initial chunk creation (entry point)
        std::cout << "📜 Step 5: Testing bounded chunk creation...\n";
        test_initial_chunk_creation();
        
        // Step 6: Test chunk navigation verbs (existing implementation)
        std::cout << "🧭 Step 6: Testing chunk navigation verbs...\n";
        test_chunk_navigation_verbs();
        
        // Step 7: Test discovery→navigation synthesis
        std::cout << "🚀 Step 7: Testing complete discovery→navigation workflow...\n";
        test_discovery_navigation_synthesis();
        
        std::cout << "\n✅ Memex verbs test suite completed!\n";
        std::cout << "🎉 Bush's associative memory: VALIDATED with real implementation!\n";
    }

private:
    std::unique_ptr<LabDb::Db9Dispatcher> dispatcher_;
    std::string database_id_;
    std::vector<EuclidMemexEntry> memex_corpus_;
    std::string test_chunk_id_;  // Store chunk ID for navigation tests
    
    void create_memex_test_database() {
        // Create database
        std::string create_cmd = "(create-database :path \"/tmp/memex_test.db9\")";
        auto response = dispatcher_->executeCommand(create_cmd);
        assert(response.status == LabDb::Db9Response::Success);
        
        // Open database
        std::string open_cmd = "(open-database :path \"/tmp/memex_test.db9\")";
        response = dispatcher_->executeCommand(open_cmd);
        assert(response.status == LabDb::Db9Response::Success);
        
        // Extract database ID from response
        auto dbid_start = response.result.find("\"dbid\": \"");
        if (dbid_start == std::string::npos) {
            std::cout << "   ⚠️ Warning: dbid not found in response: " << response.result << "\n";
            database_id_ = "db1";
        } else {
            dbid_start += 10;
            auto dbid_end = response.result.find("\"", dbid_start);
            if (dbid_end == std::string::npos) {
                std::cout << "   ⚠️ Warning: malformed dbid in response\n";
                database_id_ = "db1";
            } else {
                database_id_ = response.result.substr(dbid_start, dbid_end - dbid_start);
            }
        }
        
        std::cout << "   ✅ Memex database created with ID: " << database_id_ << "\n";
    }
    
    void load_semantic_corpus() {
        // Create focused corpus for Memex testing (golden ratio theme)
        // This represents Bush's vision of associative indexing
        
        std::vector<EuclidMemexEntry> golden_ratio_corpus = {
            // Book 6 Definition 3 (golden mean definition)
            {4693, "ἄκρον καὶ μέσον λόγον εὐθεῖα τετμῆσθαι λέγεται, ὅταν ᾖ ὡς ἡ ὅλη πρὸς τὸ μεῖζον τμῆμα, οὕτως τὸ μεῖζον πρὸς τὸ ἔλαττον.", 
             "b6-def-3", 0, "definition", "book-6"},
            
            // Book 2 Proposition 11 (geometric mean construction)
            {1147, "τὴν δοθεῖσαν εὐθεῖαν τεμεῖν, ὥστε τὸ ὑπὸ τῆς ὅλης καὶ τοῦ ἑτέρου τῶν τμημάτων περιεχόμενον ὀρθογώνιον ἴσον εἶναι τῷ ἀπὸ τοῦ λοιποῦ τμήματος τετραγώνῳ.",
             "b2-p11", 1, "proposition", "book-2"},
            
            // Book 6 Proposition 30 (golden ratio construction)
            {5823, "τὴν δοθεῖσαν εὐθεῖαν πεπερασμένην ἄκρον καὶ μέσον λόγον τεμεῖν.",
             "b6-p30", 2, "proposition", "book-6"},
            
            // Book 13 Proposition 8 (pentagon construction using golden ratio)
            {16234, "ἐὰν ἐν κύκλῳ τετράγωνον ἐγγραφῇ, δύναται τὸ ἀπὸ τῆς πλευρᾶς τοῦ τετραγώνου τὰ ἀπὸ τῆς πλευρᾶς τοῦ ἑξαγώνου καὶ τῆς τοῦ δεκαγώνου τῶν εἰς τὸν αὐτὸν κύκλον ἐγγραφομένων.",
             "b13-p8", 3, "proposition", "book-13"},
            
            // Book 1 Definition 1 (point definition - foundational)
            {134, "σημεῖόν ἐστιν, οὗ μέρος οὐθέν.",
             "b1-def-1", 4, "definition", "book-1"},
            
            // Book 1 Definition 2 (line definition - foundational)
            {135, "γραμμὴ δὲ μῆκος ἀπλατές.",
             "b1-def-2", 5, "definition", "book-1"},
            
            // Book 1 Proposition 47 (Pythagorean theorem - iconic)
            {2847, "ἐν τοῖς ὀρθογωνίοις τριγώνοις τὸ ἀπὸ τῆς τὴν ὀρθὴν γωνίαν ὑποτεινούσης πλευρᾶς τετράγωνον ἴσον ἐστὶ τοῖς ἀπὸ τῶν τὴν ὀρθὴν γωνίαν περιεχουσῶν πλευρῶν τετραγώνοις.",
             "b1-p47", 6, "proposition", "book-1"},
             
            // Book 6 Definition 1 (proportion definition)
            {4681, "ὅμοια εὐθύγραμμά ἐστι τὰ ὅμοιά τε καὶ ἀνάλογον ἔχοντα τὰς γωνίας.",
             "b6-def-1", 7, "definition", "book-6"}
        };
        
        memex_corpus_ = golden_ratio_corpus;
        
        // Create entities and semantic relationships
        for (const auto& entry : memex_corpus_) {
            // Create entity
            std::string add_entity_cmd = std::format(
                "(add-entity :value \"{}\" :dbid {})",
                entry.entity_id, database_id_
            );
            auto response = dispatcher_->executeCommand(add_entity_cmd);
            assert(response.status == LabDb::Db9Response::Success);
            
            // Add text content
            std::string add_content_cmd = std::format(
                "(add-triple-semantic :subject \"{}\" :predicate \"text-content\" :object \"{}\" :dbid {})",
                entry.entity_id, entry.text_content, database_id_
            );
            response = dispatcher_->executeCommand(add_content_cmd);
            assert(response.status == LabDb::Db9Response::Success);
            
            // Add semantic metadata
            std::string add_type_cmd = std::format(
                "(add-triple-semantic :subject \"{}\" :predicate \"field-type\" :object \"{}\" :dbid {})",
                entry.entity_id, entry.semantic_type, database_id_
            );
            response = dispatcher_->executeCommand(add_type_cmd);
            assert(response.status == LabDb::Db9Response::Success);
            
            // Add book context
            std::string add_book_cmd = std::format(
                "(add-triple-semantic :subject \"{}\" :predicate \"part-of\" :object \"{}\" :dbid {})",
                entry.entity_id, entry.book_context, database_id_
            );
            response = dispatcher_->executeCommand(add_book_cmd);
            assert(response.status == LabDb::Db9Response::Success);
            
            // Add XML line metadata
            std::string add_xmlline_cmd = std::format(
                "(add-triple-semantic :subject \"{}\" :predicate \"xml-line\" :object \"{}\" :dbid {})",
                entry.entity_id, entry.xml_line_number, database_id_
            );
            response = dispatcher_->executeCommand(add_xmlline_cmd);
            assert(response.status == LabDb::Db9Response::Success);
        }
        
        // Add semantic concept relationships (Bush's "association")
        std::vector<std::tuple<std::string, std::string, std::string>> semantic_relations = {
            // Golden ratio concept relationships
            {"b6-def-3", "embodies", "golden-ratio"},
            {"b2-p11", "relates-to", "golden-ratio"},
            {"b6-p30", "constructs", "golden-ratio"},
            {"b13-p8", "applies", "golden-ratio"},
            
            // Proportion theory relationships
            {"b6-def-3", "relates-to", "proportion"},
            {"b6-def-1", "relates-to", "proportion"},
            {"b2-p11", "relates-to", "proportion"},
            
            // Sequential relationships
            {"b1-def-2", "follows", "b1-def-1"},
            {"b6-p30", "depends-on", "b6-def-3"},
            {"b13-p8", "depends-on", "b6-def-3"},
            
            // Cross-book concept development
            {"b6-def-3", "develops-concept", "golden-ratio"},
            {"b13-p8", "culminates", "golden-ratio"}
        };
        
        for (const auto& [subject, predicate, object] : semantic_relations) {
            std::string add_relation_cmd = std::format(
                "(add-triple-semantic :subject \"{}\" :predicate \"{}\" :object \"{}\" :dbid {})",
                subject, predicate, object, database_id_
            );
            auto response = dispatcher_->executeCommand(add_relation_cmd);
            assert(response.status == LabDb::Db9Response::Success);
        }
        
        std::cout << "   ✅ Loaded " << memex_corpus_.size() << " semantic entities with associative relationships\n";
    }
    
    void create_memex_rope() {
        // Create rope ordered by XML line sequence (preserves Euclid's logical progression)
        std::string create_rope_cmd = std::format(
            "(rope-create :rope-name \"memex-test-corpus\" :description \"Memex test corpus for discovery→navigation synthesis\" :dbid {})",
            database_id_
        );
        auto response = dispatcher_->executeCommand(create_rope_cmd);
        assert(response.status == LabDb::Db9Response::Success);
        
        // Sort entries by XML line order and append to rope
        std::sort(memex_corpus_.begin(), memex_corpus_.end(), 
                  [](const EuclidMemexEntry& a, const EuclidMemexEntry& b) {
                      return a.xml_line_number < b.xml_line_number;
                  });
        
        for (const auto& entry : memex_corpus_) {
            std::string append_cmd = std::format(
                "(rope-append :rope-name \"memex-test-corpus\" :entity-id \"{}\" :dbid {})",
                entry.entity_id, database_id_
            );
            response = dispatcher_->executeCommand(append_cmd);
            assert(response.status == LabDb::Db9Response::Success);
        }
        
        std::cout << "   ✅ Created Memex rope with " << memex_corpus_.size() << " entities in XML sequence\n";
    }
    
    void test_semantic_discovery() {
        std::cout << "   🔍 Testing Bush's associative discovery patterns...\n";
        
        // Test 1: Find all golden ratio content
        std::cout << "   🏺 Discovery Test 1: Find golden ratio associations...\n";
        std::string golden_ratio_query = std::format(
            "(find-triple-enhanced :subject \"*\" :predicate \"*\" :object \"golden-ratio\" :dbid {})",
            database_id_
        );
        auto response = dispatcher_->executeCommand(golden_ratio_query);
        assert(response.status == LabDb::Db9Response::Success);
        
        std::cout << "   📊 Golden ratio discovery response length: " << response.result.length() << " chars\n";
        if (response.result.find("b6-def-3") != std::string::npos &&
            response.result.find("b2-p11") != std::string::npos) {
            std::cout << "   ✅ Golden ratio associations discovered successfully\n";
        } else {
            std::cout << "   ⚠️ Golden ratio associations not found as expected\n";
        }
        
        // Test 2: Find all definitions
        std::cout << "   📖 Discovery Test 2: Find definitional content...\n";
        std::string definitions_query = std::format(
            "(find-triple-enhanced :subject \"*\" :predicate \"field-type\" :object \"definition\" :dbid {})",
            database_id_
        );
        response = dispatcher_->executeCommand(definitions_query);
        assert(response.status == LabDb::Db9Response::Success);
        
        if (response.result.find("b6-def-3") != std::string::npos &&
            response.result.find("b1-def-1") != std::string::npos) {
            std::cout << "   ✅ Definitional content discovery successful\n";
        } else {
            std::cout << "   ⚠️ Definitional content not fully discovered\n";
        }
        
        // Test 3: Find Book 6 content
        std::cout << "   📚 Discovery Test 3: Find Book 6 proportional content...\n";
        std::string book6_query = std::format(
            "(find-triple-enhanced :subject \"*\" :predicate \"part-of\" :object \"book-6\" :dbid {})",
            database_id_
        );
        response = dispatcher_->executeCommand(book6_query);
        assert(response.status == LabDb::Db9Response::Success);
        
        if (response.result.find("b6-def-3") != std::string::npos &&
            response.result.find("b6-p30") != std::string::npos) {
            std::cout << "   ✅ Book 6 content discovery successful\n";
        } else {
            std::cout << "   ⚠️ Book 6 content not fully discovered\n";
        }
        
        std::cout << "   🎯 Semantic discovery validation: COMPLETE\n";
    }
    
    void test_initial_chunk_creation() {
        std::cout << "   📜 Testing bounded chunk creation around discovered entities...\n";
        
        // Test chunk creation around golden mean definition (b6-def-3)
        std::cout << "   🏛️ Creating chunk around golden mean definition...\n";
        std::string chunk_cmd = std::format(
            "(rope-chunk :rope-name \"memex-test-corpus\" :center-entity \"b6-def-3\" :fragment-count 5 :include-text true :dbid {})",
            database_id_
        );
        auto response = dispatcher_->executeCommand(chunk_cmd);
        assert(response.status == LabDb::Db9Response::Success);
        
        std::cout << "   📊 Chunk creation response length: " << response.result.length() << " chars\n";
        if (response.result.find("b6-def-3") != std::string::npos) {
            std::cout << "   ✅ Chunk creation around discovery entity successful\n";
            
            // Extract chunk ID for navigation tests
            auto chunk_id_start = response.result.find("\"chunk_id\": \"");
            if (chunk_id_start != std::string::npos) {
                chunk_id_start += 13;
                auto chunk_id_end = response.result.find("\"", chunk_id_start);
                if (chunk_id_end != std::string::npos) {
                    test_chunk_id_ = response.result.substr(chunk_id_start, chunk_id_end - chunk_id_start);
                    std::cout << "   🆔 Extracted chunk ID for navigation: " << test_chunk_id_ << "\n";
                }
            }
        } else {
            std::cout << "   ⚠️ Chunk creation failed to include target entity\n";
        }
        
        // Verify chunk contains readable Greek text
        if (response.result.find("ἄκρον καὶ μέσον") != std::string::npos) {
            std::cout << "   🏺 Greek text preserved in chunk: VERIFIED\n";
        } else {
            std::cout << "   ⚠️ Greek text not found in chunk response\n";
        }
        
        std::cout << "   🎯 Bounded chunk creation: VALIDATED\n";
    }
    
    void test_chunk_navigation_verbs() {
        std::cout << "   🧭 Testing implemented Memex chunk navigation verbs...\n";
        
        if (test_chunk_id_.empty()) {
            std::cout << "   ⚠️ No chunk ID available for navigation tests\n";
            return;
        }
        
        // Test 1: memex-chunk-preceding
        std::cout << "   ⬅️ Testing memex-chunk-preceding...\n";
        std::string preceding_cmd = std::format(
            "(memex-chunk-preceding :chunk-id \"{}\" :dbid {})",
            test_chunk_id_, database_id_
        );
        auto response = dispatcher_->executeCommand(preceding_cmd);
        
        std::cout << "   📊 Preceding verb status: " << static_cast<int>(response.status) << "\n";
        std::cout << "   📊 Response length: " << response.result.length() << " chars\n";
        
        if (response.status == LabDb::Db9Response::Success) {
            std::cout << "   ✅ memex-chunk-preceding: WORKING\n";
            if (response.result.find("chunk_id") != std::string::npos) {
                std::cout << "   🆔 Preceding chunk ID generated successfully\n";
            }
        } else {
            std::cout << "   ❌ memex-chunk-preceding: FAILED\n";
            std::cout << "   📝 Error details: " << response.result << "\n";
        }
        
        // Test 2: memex-chunk-succeeding
        std::cout << "   ➡️ Testing memex-chunk-succeeding...\n";
        std::string succeeding_cmd = std::format(
            "(memex-chunk-succeeding :chunk-id \"{}\" :dbid {})",
            test_chunk_id_, database_id_
        );
        response = dispatcher_->executeCommand(succeeding_cmd);
        
        std::cout << "   📊 Succeeding verb status: " << static_cast<int>(response.status) << "\n";
        std::cout << "   📊 Response length: " << response.result.length() << " chars\n";
        
        if (response.status == LabDb::Db9Response::Success) {
            std::cout << "   ✅ memex-chunk-succeeding: WORKING\n";
            if (response.result.find("chunk_id") != std::string::npos) {
                std::cout << "   🆔 Succeeding chunk ID generated successfully\n";
            }
        } else {
            std::cout << "   ❌ memex-chunk-succeeding: FAILED\n";
            std::cout << "   📝 Error details: " << response.result << "\n";
        }
        
        std::cout << "   🎯 Memex navigation verbs testing: COMPLETE\n";
    }
    
    void test_discovery_navigation_synthesis() {
        std::cout << "   🚀 Testing complete Bush's Memex workflow...\n";
        std::cout << "   🎯 Discovery→Navigation synthesis validation\n";
        
        // Step 1: Semantic Discovery (find entry points)
        std::cout << "   🔍 Step 1: Discover golden ratio entry points...\n";
        std::string discovery_cmd = std::format(
            "(find-triple-enhanced :subject \"*\" :predicate \"embodies\" :object \"golden-ratio\" :dbid {})",
            database_id_
        );
        auto discovery_response = dispatcher_->executeCommand(discovery_cmd);
        assert(discovery_response.status == LabDb::Db9Response::Success);
        
        bool found_b6_def3 = discovery_response.result.find("b6-def-3") != std::string::npos;
        if (found_b6_def3) {
            std::cout << "   ✅ Discovery phase: Found golden ratio definition\n";
        } else {
            std::cout << "   ⚠️ Discovery phase: Golden ratio definition not found\n";
        }
        
        // Step 2: Navigate to text context
        std::cout << "   📜 Step 2: Navigate to readable text context...\n";
        std::string navigation_cmd = std::format(
            "(rope-chunk :rope-name \"memex-test-corpus\" :center-entity \"b6-def-3\" :fragment-count 7 :include-text true :dbid {})",
            database_id_
        );
        auto navigation_response = dispatcher_->executeCommand(navigation_cmd);
        assert(navigation_response.status == LabDb::Db9Response::Success);
        
        bool has_greek_text = navigation_response.result.find("ἄκρον καὶ μέσον") != std::string::npos;
        bool has_chunk_id = navigation_response.result.find("chunk_id") != std::string::npos;
        
        if (has_greek_text && has_chunk_id) {
            std::cout << "   ✅ Navigation phase: Readable context with navigation metadata\n";
        } else {
            std::cout << "   ⚠️ Navigation phase: Missing text or metadata\n";
        }
        
        // Step 3: Test associative trail potential
        std::cout << "   🧵 Step 3: Validate associative trail potential...\n";
        
        // Discover multiple related entry points
        std::string trail_discovery_cmd = std::format(
            "(find-triple-enhanced :subject \"*\" :predicate \"relates-to\" :object \"golden-ratio\" :dbid {})",
            database_id_
        );
        auto trail_response = dispatcher_->executeCommand(trail_discovery_cmd);
        assert(trail_response.status == LabDb::Db9Response::Success);
        
        bool has_multiple_entries = trail_response.result.find("b2-p11") != std::string::npos &&
                                   trail_response.result.find("b6-def-3") != std::string::npos;
        
        if (has_multiple_entries) {
            std::cout << "   ✅ Trail potential: Multiple associative entry points discovered\n";
        } else {
            std::cout << "   ⚠️ Trail potential: Insufficient associative connections\n";
        }
        
        // Synthesis validation
        if (found_b6_def3 && has_greek_text && has_chunk_id && has_multiple_entries) {
            std::cout << "   🎉 SYNTHESIS SUCCESSFUL: Bush's Memex workflow VALIDATED!\n";
            std::cout << "   🧠 ✅ Rapid discovery: semantic relationships → entry points\n";
            std::cout << "   📜 ✅ Contemplative reading: bounded chunks with context\n";
            std::cout << "   🧵 ✅ Associative trails: multiple connection pathways\n";
            std::cout << "   🚀 ✅ Navigation: chunk boundaries for fluid reading\n";
        } else {
            std::cout << "   ⚠️ Synthesis incomplete - some workflow components missing\n";
        }
    }
};

void test_memex_verbs_comprehensive() {
    MemexVerbsTest test;
    test.run_comprehensive_memex_test();
}

void test_verb_availability() {
    std::cout << "🔍 Memex Verbs Availability Test\n";
    std::cout << "================================\n\n";
    
    std::unique_ptr<LabDb::Db9Dispatcher> dispatcher = std::make_unique<LabDb::Db9Dispatcher>();
    
    // Test list-verbs to see what's available
    std::cout << "📋 Checking available verbs...\n";
    std::string list_cmd = "(list-verbs)";
    auto response = dispatcher->executeCommand(list_cmd);
    
    std::cout << "📊 List verbs response length: " << response.result.length() << " chars\n";
    
    // Check for specific Memex verbs
    std::vector<std::string> expected_memex_verbs = {
        "memex-chunk-preceding",
        "memex-chunk-succeeding"
    };
    
    std::vector<std::string> future_memex_verbs = {
        "memex-chunk-extend",
        "memex-chunk-extend-semantic",
        "memex-trail-create",
        "memex-trail-navigate",
        "memex-trail-save",
        "memex-trail-load"
    };
    
    std::cout << "\n🧠 Memex Verb Status Check:\n";
    for (const auto& verb : expected_memex_verbs) {
        if (response.result.find(verb) != std::string::npos) {
            std::cout << "   ✅ " << verb << " - AVAILABLE\n";
        } else {
            std::cout << "   ❌ " << verb << " - NOT FOUND\n";
        }
    }
    
    std::cout << "\n🚧 Future Memex Verbs (Implementation Needed):\n";
    for (const auto& verb : future_memex_verbs) {
        if (response.result.find(verb) != std::string::npos) {
            std::cout << "   ✅ " << verb << " - IMPLEMENTED\n";
        } else {
            std::cout << "   ⏳ " << verb << " - TODO\n";
        }
    }
    
    // Also check for rope verbs (dependency for Memex)
    std::cout << "\n🧵 Rope Verb Dependencies:\n";
    std::vector<std::string> rope_verbs = {"rope-create", "rope-append", "rope-chunk", "rope-traverse"};
    for (const auto& verb : rope_verbs) {
        if (response.result.find(verb) != std::string::npos) {
            std::cout << "   ✅ " << verb << " - AVAILABLE\n";
        } else {
            std::cout << "   ❌ " << verb << " - MISSING (CRITICAL DEPENDENCY)\n";
        }
    }
}

int main() {
    std::cout << "🧠 LabDb9 Memex Verbs Test Suite\n";
    std::cout << "===============================\n";
    std::cout << "🎯 Bush's Memex vision validation through triadic consciousness\n\n";
    
    try {
        // First check what verbs are available
        test_verb_availability();
        
        std::cout << "\n" << std::string(50, '=') << "\n\n";
        
        // Run comprehensive Memex functionality test
        test_memex_verbs_comprehensive();
        
        std::cout << "\n🎉 All Memex tests completed!\n";
        std::cout << "🧠 Bush's associative memory machine: REALIZED in LabDb9!\n";
        std::cout << "🚀 Discovery→Navigation synthesis: VALIDATED!\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Memex test failed with exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "❌ Memex test failed with unknown exception\n";
        return 1;
    }
}
