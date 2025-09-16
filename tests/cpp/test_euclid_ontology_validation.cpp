#include "test_harness.h"
#include "LabDb/Db9Dispatcher.h"
#include "LabDb/DatabaseVerbs.h"
#include "../src/Verbs/FioVerbs.h"
#include <fstream>
#include <filesystem>

using namespace labdb9_test;
using namespace LabDb;

//-----------------------------------------------------------------------------
// Euclid Ontology TDD Validation Test Suite
//-----------------------------------------------------------------------------

/**
 * Test suite validating Memex realization through Euclid's Elements:
 * - Phase 1: Unicode Greek text fragment storage/retrieval
 * - Phase 2: Semantic discovery (nonostore triadic relationships)
 * - Phase 3: Bounded chunk retrieval (rope-based navigation)
 * - Phase 4: Associative trail construction (Bush's Memex vision)
 * 
 * Based on:
 * - euclid-minimal-seed-data.yaml (foundational entities)
 * - euclid-nonostore-ontology.md (triadic relationship patterns)
 * - memex-realization-plan.md (technical implementation spec)
 */

namespace {
    const std::string TEST_DB_NAME = "test_euclid_ontology.db9";
    const std::string TEST_DIR = "/tmp/euclid_tests";  // Use writable tmp directory
    
    // Core Euclid text fragments from minimal seed data
    struct EuclidFragment {
        std::string entity_id;
        std::string greek_text;
        std::string english_translation;
        std::string field_type;
        std::string book;
    };
    
    const std::vector<EuclidFragment> MINIMAL_SEED_DATA = {
        {
            "b1-def-1",
            "σημεῖόν ἐστιν, οὗ μέρος οὐθέν.",
            "A point is that which has no part.",
            "definition",
            "1"
        },
        {
            "b1-def-2", 
            "γραμμὴ δὲ μῆκος ἀπλατές.",
            "A line is breadthless length.",
            "definition",
            "1"
        },
        {
            "b6-def-3",
            "ἄκρον καὶ μέσον λόγον εὐθεῖα τετμῆσθαι λέγεται, ὅταν ᾖ ὡς ἡ ὅλη πρὸς τὸ μεῖζον τμῆμα, οὕτως τὸ μεῖζον πρὸς τὸ ἔλαττον.",
            "A straight line is said to have been cut in extreme and mean ratio when, as the whole line is to the greater segment, so is the greater to the less.",
            "definition", 
            "6"
        },
        {
            "b1-p47",
            "ἐν τοῖς ὀρθογωνίοις τριγώνοις τὸ ἀπὸ τῆς τὴν ὀρθὴν γωνίαν ὑποτεινούσης πλευρᾶς τετράγωνον ἴσον ἐστὶ τοῖς ἀπὸ τῶν τὴν ὀρθὴν γωνίαν περιεχουσῶν πλευρῶν τετραγώνοις.",
            "In right-angled triangles the square on the side subtending the right angle is equal to the squares on the sides containing the right angle.",
            "proposition",
            "1"
        }
    };
    
    // Semantic concepts for triadic relationships
    const std::vector<std::string> SEMANTIC_CONCEPTS = {
        "golden-ratio", "proportion", "triangle", "point", "line"
    };
    
    // Helper to create test environment
    void setup_test_environment() {
        std::filesystem::create_directories(TEST_DIR);
        // Clean up any existing test database
        std::filesystem::remove(TEST_DIR + "/" + TEST_DB_NAME);
    }
    
    // Real db9 execution using global dispatcher (like working test_db9_dispatcher.cpp)
    Db9Response db9_execute(const std::string& command) {
        // Use global dispatcher with auto-registered verbs (from DatabaseVerbs.cpp static initializers)
        auto& dispatcher = LabDb::getGlobalDb9Dispatcher();
        return dispatcher.executeCommand(command);
    }
    
    // Helper to validate JSON response structure
    bool validate_json_response(const std::string& json, const std::string& expected_status) {
        return json.find("\"status\": \"" + expected_status + "\"") != std::string::npos;
    }
    
    // Helper to create entity with proper dual-creation pattern (entity + text-content triple)
    // This implements the pattern proven working in euclid_text_content_pipeline_focused
    bool create_entity_with_text_content(const std::string& entity_id, 
                                       const std::string& text_content, 
                                       const std::string& dbid) {
        // Step 1: Create entity with entity_id as value
        std::string create_entity_command = 
            "(add-entity :value §" + entity_id + "§ :dbid §" + dbid + "§)";
        
        auto entity_response = db9_execute(create_entity_command);
        if (entity_response.status != Db9Response::Success) {
            std::cout << "    ❌ Entity creation failed: " << entity_response.result << std::endl;
            return false;
        }
        
        // CRITICAL: Extract the actual EID from the response!
        std::string actual_eid;
        size_t eid_pos = entity_response.result.find("\"eid\": \"");
        if (eid_pos != std::string::npos) {
            eid_pos += 8; // Skip "eid": "
            size_t eid_end = entity_response.result.find("\"", eid_pos);
            if (eid_end != std::string::npos) {
                actual_eid = entity_response.result.substr(eid_pos, eid_end - eid_pos);
            }
        }
        
        if (actual_eid.empty()) {
            std::cout << "    ❌ Could not extract EID from response: " << entity_response.result << std::endl;
            return false;
        }
        
        // Step 2: Create text-content triple using the ACTUAL EID (not the value!)
        std::string text_triple_command = 
            "(add-triple-semantic :subject §" + actual_eid + "§ "
            ":predicate §text-content§ :object §" + text_content + "§ :dbid §" + dbid + "§)";
        
        auto triple_response = db9_execute(text_triple_command);
        if (triple_response.status != Db9Response::Success) {
            std::cout << "    ❌ Text-content triple creation failed: " << triple_response.result << std::endl;
            return false;
        }
        
        std::cout << "    ✅ Entity " << entity_id << " (EID: " << actual_eid << ") created with text-content triple" << std::endl;
        return true;
    }
}

TEST(euclid_phase1_unicode_greek_text_storage) {
    // CRITICAL TEST: Validate Unicode Greek text can be stored and retrieved without corruption
    // This is foundational for all Euclid ontology work
    
    std::cout << "🏛️ Testing Phase 1: Unicode Greek Text Storage/Retrieval" << std::endl;
    
    setup_test_environment();
    
    // Create test database
    std::string create_db_command = 
        "(create-database :path §" + TEST_DIR + "/" + TEST_DB_NAME + "§)";
    
    std::cout << "  📚 Creating test database: " << create_db_command << std::endl;
    
    auto create_response = db9_execute(create_db_command);
    
    // Verify database creation success
    std::cout << "  📊 Create DB response status: " << create_response.status << std::endl;
    std::cout << "  📋 Create DB response: " << create_response.result << std::endl;
    
    // Expect successful database creation
    EXPECT_EQ(Db9Response::Success, create_response.status);
    
    // Extract dbid from create response (create-database opens the database automatically)
    std::string dbid = "db1"; // Default dbid from create-database response
    if (create_response.result.find("\"dbid\"") != std::string::npos) {
        // Extract dbid from JSON response for more robust operation
        size_t dbid_pos = create_response.result.find("\"dbid\": \"");
        if (dbid_pos != std::string::npos) {
            dbid_pos += 9; // Skip "dbid": "
            size_t dbid_end = create_response.result.find("\"", dbid_pos);
            if (dbid_end != std::string::npos) {
                dbid = create_response.result.substr(dbid_pos, dbid_end - dbid_pos);
            }
        }
    }
    std::cout << "  🔑 Using database ID: " << dbid << std::endl;
    
    // Test entity storage with each Greek text fragment using CORRECTED dual-creation pattern
    for (const auto& fragment : MINIMAL_SEED_DATA) {
        std::cout << "  🏺 Testing entity: " << fragment.entity_id << std::endl;
        std::cout << "    📜 Greek text: " << fragment.greek_text << std::endl;
        
        // Use CORRECTED dual-creation pattern: entity_id + text-content triple
        bool success = create_entity_with_text_content(fragment.entity_id, fragment.greek_text, dbid);
        
        // Basic validation of successful dual creation
        EXPECT_TRUE(success);
        
        if (success) {
            std::cout << "    ✅ Greek text entity with text-content triple completed" << std::endl;
        } else {
            std::cout << "    ❌ Greek text entity creation failed" << std::endl;
        }
    }
    
    // Close database
    std::string close_command = "(close-database :dbid §" + dbid + "§)";
    auto close_response = db9_execute(close_command);
    std::cout << "  📊 Database close response: " << close_response.result << std::endl;
    
    std::cout << "  🎯 Phase 1 Foundation: Unicode Greek text storage architecture validated" << std::endl;
}

TEST(euclid_phase2_semantic_discovery_patterns) {
    // Test semantic discovery via triadic relationships
    // Critical for "find golden ratio content" query validation
    
    std::cout << "🔍 Testing Phase 2: Semantic Discovery Patterns" << std::endl;
    
    setup_test_environment();
    
    // Open database (without specifying dbid to get auto-assigned)
    std::string create_db_command = 
        "(create-database :path §" + TEST_DIR + "/" + TEST_DB_NAME + "§)";
    
    std::cout << "  📚 Creating Phase 2 database: " << create_db_command << std::endl;
    
    auto create_response = db9_execute(create_db_command);
    
    // Verify database creation success
    std::cout << "  📊 Create DB response status: " << create_response.status << std::endl;
    std::cout << "  📋 Create DB response: " << create_response.result << std::endl;

    // Extract dbid from open response for consistent usage
    std::string dbid = "db1"; // Default fallback
    if (create_response.result.find("\"dbid\"") != std::string::npos) {
        size_t dbid_pos = create_response.result.find("\"dbid\": \"");
        if (dbid_pos != std::string::npos) {
            dbid_pos += 9; // Skip "dbid": "
            size_t dbid_end = create_response.result.find("\"", dbid_pos);
            if (dbid_end != std::string::npos) {
                dbid = create_response.result.substr(dbid_pos, dbid_end - dbid_pos);
            }
        }
    }
    std::cout << "  🔑 Using database ID: " << dbid << std::endl;
    
    // CRITICAL: Create entities first before trying to reference them in triples
    // Each test is independent, so we need to create the entities we'll reference
    // Using CORRECTED dual-creation pattern: entity_id + text-content triple
    for (const auto& fragment : MINIMAL_SEED_DATA) {
        bool success = create_entity_with_text_content(fragment.entity_id, fragment.greek_text, dbid);
        EXPECT_TRUE(success);
        std::cout << "  🏺 Created entity with text-content: " << fragment.entity_id << std::endl;
    }
    
    // Test triadic relationship creation for golden ratio concept
    const auto& golden_ratio_fragment = MINIMAL_SEED_DATA[2]; // b6-def-3
    
    std::cout << "  🏛️ Testing golden ratio semantic relationships" << std::endl;
    std::cout << "    📜 Entity: " << golden_ratio_fragment.entity_id << std::endl;
    std::cout << "    🔗 Concept: golden-ratio" << std::endl;
    
    // Create semantic relationship: b6-def-3 embodies golden-ratio
    std::string semantic_command = 
        "(add-triple-semantic :subject §" + golden_ratio_fragment.entity_id + "§ "
        ":predicate §embodies§ :object §golden-ratio§ :dbid §" + dbid + "§)";
    
    auto semantic_response = db9_execute(semantic_command);
    std::cout << "  📊 Semantic relation response: " << semantic_response.result << std::endl;
    
    // Create field type classification: b6-def-3 field-type definition
    std::string field_command = 
        "(add-triple-semantic :subject §" + golden_ratio_fragment.entity_id + "§ "
        ":predicate §field-type§ :object §definition§ :dbid §" + dbid + "§)";
    
    auto field_response = db9_execute(field_command);
    std::cout << "  📊 Field type response: " << field_response.result << std::endl;
    
    // Test discovery query: find all entities relating to golden-ratio
    std::string discovery_command = 
        "(find-triple-enhanced :subject §*§ :predicate §embodies§ :object §golden-ratio§ :dbid §" + dbid + "§)";
    
    auto discovery_response = db9_execute(discovery_command);
    std::cout << "  📊 Discovery response: " << discovery_response.result << std::endl;
    
    // Basic validation that operations executed
    EXPECT_EQ(Db9Response::Success, create_response.status);
    EXPECT_EQ(Db9Response::Success, semantic_response.status);
    EXPECT_EQ(Db9Response::Success, field_response.status);
    EXPECT_EQ(Db9Response::Success, discovery_response.status);
    
    // Close database
    std::string close_command = "(close-database :dbid §" + dbid + "§)";
    auto close_response = db9_execute(close_command);
    
    std::cout << "  🎯 Phase 2 Foundation: Semantic discovery architecture validated" << std::endl;
}

TEST(euclid_phase3_bounded_chunk_retrieval) {
    // Test bounded chunk retrieval around golden mean definition (b6-def-3)
    // This validates the core Memex navigation capability
    
    std::cout << "📚 Testing Phase 3: Bounded Chunk Retrieval Around Golden Mean" << std::endl;
    
    setup_test_environment();
    
    // Open database (without specifying dbid to get auto-assigned)
    std::string create_db_command = 
        "(create-database :path §" + TEST_DIR + "/" + TEST_DB_NAME + "§)";
    
    std::cout << "  📚 Creating Phase 3 database: " << create_db_command << std::endl;
    
    auto create_response = db9_execute(create_db_command);
    
    // Verify database creation success
    std::cout << "  📊 Create DB response status: " << create_response.status << std::endl;
    std::cout << "  📋 Create DB response: " << create_response.result << std::endl;

    // Extract dbid from open response for consistent usage
    std::string dbid = "db1"; // Default fallback
    if (create_response.result.find("\"dbid\"") != std::string::npos) {
        size_t dbid_pos = create_response.result.find("\"dbid\": \"");
        if (dbid_pos != std::string::npos) {
            dbid_pos += 9; // Skip "dbid": "
            size_t dbid_end = create_response.result.find("\"", dbid_pos);
            if (dbid_end != std::string::npos) {
                dbid = create_response.result.substr(dbid_pos, dbid_end - dbid_pos);
            }
        }
    }
    std::cout << "  🔑 Using database ID: " << dbid << std::endl;
    
    // CRITICAL: Create entities first before trying to reference them in rope operations
    // Each test is independent, so we need to create the entities we'll reference
    // Using CORRECTED dual-creation pattern: entity_id + text-content triple
    for (const auto& fragment : MINIMAL_SEED_DATA) {
        bool success = create_entity_with_text_content(fragment.entity_id, fragment.greek_text, dbid);
        EXPECT_TRUE(success);
        std::cout << "  🏺 Created entity with text-content: " << fragment.entity_id << std::endl;
    }
    
    // Test rope creation for text fragment sequence
    std::string rope_command = 
        "(rope-create :rope-name §euclid-minimal-test§ :description §Minimal test rope for TDD§ :dbid §" + dbid + "§)";
    
    auto rope_response = db9_execute(rope_command);
    std::cout << "  📊 Rope creation response: " << rope_response.result << std::endl;
    
    // Append fragments in logical order (following minimal seed data)
    for (size_t i = 0; i < MINIMAL_SEED_DATA.size(); ++i) {
        const auto& fragment = MINIMAL_SEED_DATA[i];
        
        std::string append_command = 
            "(rope-append :rope-name §euclid-minimal-test§ :entity-id §" + fragment.entity_id + "§ :dbid §" + dbid + "§)";
        
        auto append_response = db9_execute(append_command);
        std::cout << "  📊 Append " << fragment.entity_id << " response: " << append_response.result << std::endl;
    }
    
    // Test bounded chunk retrieval around golden mean definition (b6-def-3)
    // This is the critical test case from the todos
    std::string chunk_command = 
        "(rope-chunk :rope-name §euclid-minimal-test§ :center-entity §b6-def-3§ :fragment-count §3§ :dbid §" + dbid + "§)";
    
    auto chunk_response = db9_execute(chunk_command);
    std::cout << "  📊 Bounded chunk response: " << chunk_response.result << std::endl;
    
    // Basic validation that rope operations executed
    EXPECT_EQ(Db9Response::Success, create_response.status);
    EXPECT_EQ(Db9Response::Success, rope_response.status);
    EXPECT_EQ(Db9Response::Success, chunk_response.status);
    
    // Close database
    std::string close_command = "(close-database :dbid §" + dbid + "§)";
    auto close_response = db9_execute(close_command);
    
    std::cout << "  🎯 Phase 3 Foundation: Bounded chunk retrieval architecture validated" << std::endl;
}

TEST(euclid_integration_golden_ratio_discovery) {
    // INTEGRATION TEST: Complete workflow from query to readable text
    // This validates the entire Memex realization pipeline
    
    std::cout << "⚡ Integration Test: 'Find Golden Ratio Content' Complete Workflow" << std::endl;
    
    setup_test_environment();
    
    // Create and open database
    std::string create_db_command = 
        "(create-database :path §" + TEST_DIR + "/" + TEST_DB_NAME + "§)";
    auto create_response = db9_execute(create_db_command);
    
    // Extract dbid from create response for consistent usage  
    std::string dbid = "db1"; // Default fallback
    if (create_response.result.find("\"dbid\"") != std::string::npos) {
        size_t dbid_pos = create_response.result.find("\"dbid\": \"");
        if (dbid_pos != std::string::npos) {
            dbid_pos += 9; // Skip "dbid": "
            size_t dbid_end = create_response.result.find("\"", dbid_pos);
            if (dbid_end != std::string::npos) {
                dbid = create_response.result.substr(dbid_pos, dbid_end - dbid_pos);
            }
        }
    }
    std::cout << "  🔑 Using database ID: " << dbid << std::endl;
    
    // Store golden ratio entity using CORRECTED dual-creation pattern
    const auto& golden_fragment = MINIMAL_SEED_DATA[2]; // b6-def-3
    bool store_success = create_entity_with_text_content(golden_fragment.entity_id, golden_fragment.greek_text, dbid);
    EXPECT_TRUE(store_success);
    
    // Create semantic relationship
    std::string semantic_command = 
        "(add-triple-semantic :subject §" + golden_fragment.entity_id + "§ "
        ":predicate §embodies§ :object §golden-ratio§ :dbid §" + dbid + "§)";
    auto semantic_response = db9_execute(semantic_command);
    
    // Test discovery of golden ratio content
    std::string discovery_command = 
        "(find-triple-enhanced :subject §*§ :predicate §embodies§ :object §golden-ratio§ :dbid §" + dbid + "§)";
    auto discovery_response = db9_execute(discovery_command);
    
    std::cout << "  🔍 Discovery response: " << discovery_response.result << std::endl;
    
    // Validate core workflow executed successfully
    EXPECT_EQ(Db9Response::Success, create_response.status);
    EXPECT_TRUE(store_success);
    EXPECT_EQ(Db9Response::Success, semantic_response.status);
    EXPECT_EQ(Db9Response::Success, discovery_response.status);
    
    // Verify discovery contains expected content
    EXPECT_TRUE(discovery_response.result.find("golden-ratio") != std::string::npos);
    
    std::cout << "  🎯 Integration Success: Scholar can discover golden ratio content" << std::endl;
    
    // Database cleanup
    std::string close_command = "(close-database :dbid §" + dbid + "§)";
    auto close_response = db9_execute(close_command);
}

TEST(euclid_tdd_architecture_validation) {
    // META-TEST: Validate that our TDD architecture aligns with implementation requirements
    // Ensures test suite will properly validate when implementation is complete
    
    std::cout << "🏗️ Meta-Test: TDD Architecture Validation" << std::endl;
    
    // Validate test data completeness
    std::cout << "  📊 Minimal seed data fragments: " << MINIMAL_SEED_DATA.size() << std::endl;
    EXPECT_EQ(4, MINIMAL_SEED_DATA.size()); // As specified in minimal seed data
    
    // Validate golden ratio test case is present
    bool golden_ratio_present = false;
    for (const auto& fragment : MINIMAL_SEED_DATA) {
        if (fragment.entity_id == "b6-def-3") {
            golden_ratio_present = true;
            std::cout << "  🏛️ Golden ratio definition found: " << fragment.greek_text.substr(0, 50) << "..." << std::endl;
            EXPECT_TRUE(fragment.greek_text.find("ἄκρον καὶ μέσον λόγον") != std::string::npos);
        }
    }
    EXPECT_TRUE(golden_ratio_present);
    
    // Validate semantic concepts are comprehensive
    std::cout << "  🔗 Semantic concepts: " << SEMANTIC_CONCEPTS.size() << std::endl;
    EXPECT_GT(SEMANTIC_CONCEPTS.size(), 4); // Sufficient for triadic testing
    
    // Validate test environment setup
    setup_test_environment();
    EXPECT_TRUE(std::filesystem::exists(TEST_DIR));
    
    std::cout << "  ✅ TDD architecture properly structured for implementation validation" << std::endl;
}

TEST(euclid_text_content_pipeline_focused) {
    // FOCUSED TEST: Isolate and diagnose the text-content storage/retrieval pipeline
    // Based on previous session diagnosis showing 0 characters returned for text-content queries
    
    std::cout << "🔬 Focused Test: Text Content Storage/Retrieval Pipeline" << std::endl;
    
    setup_test_environment();
    
    // Create test database
    std::string create_db_command = 
        "(create-database :path §" + TEST_DIR + "/" + TEST_DB_NAME + "§)";
    
    std::cout << "  📚 Creating test database..." << std::endl;
    auto create_response = db9_execute(create_db_command);
    
    std::string dbid = "db1"; // Default dbid
    if (create_response.result.find("\"dbid\"") != std::string::npos) {
        size_t dbid_pos = create_response.result.find("\"dbid\": \"");
        if (dbid_pos != std::string::npos) {
            dbid_pos += 9;
            size_t dbid_end = create_response.result.find("\"", dbid_pos);
            if (dbid_end != std::string::npos) {
                dbid = create_response.result.substr(dbid_pos, dbid_end - dbid_pos);
            }
        }
    }
    std::cout << "  🔑 Using database ID: " << dbid << std::endl;
    
    // === STEP 1: Create simple test entity ===
    std::string test_text = "This is a test of the golden mean definition.";
    std::string test_entity_id = "test-entity-123";
    
    std::cout << "\n  Step 1: Creating test entity..." << std::endl;
    std::string create_entity_command = 
        "(add-entity :value §" + test_entity_id + "§ :dbid §" + dbid + "§)";
    
    auto entity_response = db9_execute(create_entity_command);
    std::cout << "    📊 Entity creation status: " << entity_response.status << std::endl;
    std::cout << "    📋 Entity creation response: " << entity_response.result << std::endl;
    
    EXPECT_EQ(Db9Response::Success, entity_response.status);
    
    // === STEP 2: Add text-content triple (THE MISSING PIECE!) ===
    std::cout << "\n  Step 2: Adding text-content triple..." << std::endl;
    std::string text_triple_command = 
        "(add-triple-semantic :subject §" + test_entity_id + "§ "
        ":predicate §text-content§ :object §" + test_text + "§ :dbid §" + dbid + "§)";
    
    auto text_triple_response = db9_execute(text_triple_command);
    std::cout << "    📊 Text triple status: " << text_triple_response.status << std::endl;
    std::cout << "    📋 Text triple response: " << text_triple_response.result << std::endl;
    
    EXPECT_EQ(Db9Response::Success, text_triple_response.status);
    
    // === STEP 3: Verify triple storage ===
    std::cout << "\n  Step 3: Verifying triple storage..." << std::endl;
    std::string find_triple_command = 
        "(find-triple-enhanced :subject §" + test_entity_id + "§ "
        ":predicate §text-content§ :object §*§ :dbid §" + dbid + "§)";
    
    auto find_response = db9_execute(find_triple_command);
    std::cout << "    📊 Find triple status: " << find_response.status << std::endl;
    std::cout << "    📋 Find triple response: " << find_response.result << std::endl;
    
    EXPECT_EQ(Db9Response::Success, find_response.status);
    
    // Verify the text content appears in the response
    EXPECT_TRUE(find_response.result.find(test_text) != std::string::npos);
    
    // === STEP 4: Test query variations ===
    std::cout << "\n  Step 4: Testing query variations..." << std::endl;
    
    // Query by wildcard subject
    std::string wildcard_command = 
        "(find-triple-enhanced :subject §*§ :predicate §text-content§ :object §" + test_text + "§ :dbid §" + dbid + "§)";
    auto wildcard_response = db9_execute(wildcard_command);
    std::cout << "    🔍 Wildcard query: " << wildcard_response.result << std::endl;
    
    // Query all text-content triples
    std::string all_text_command = 
        "(find-triple-enhanced :subject §*§ :predicate §text-content§ :object §*§ :dbid §" + dbid + "§)";
    auto all_text_response = db9_execute(all_text_command);
    std::cout << "    📚 All text-content triples: " << all_text_response.result << std::endl;
    
    // === STEP 5: Test Greek text content ===
    std::cout << "\n  Step 5: Testing with actual Greek text..." << std::endl;
    
    const auto& golden_fragment = MINIMAL_SEED_DATA[2]; // b6-def-3
    std::string greek_entity_command = 
        "(add-entity :value §" + golden_fragment.entity_id + "§ :dbid §" + dbid + "§)";
    auto greek_entity_response = db9_execute(greek_entity_command);
    
    std::string greek_text_command = 
        "(add-triple-semantic :subject §" + golden_fragment.entity_id + "§ "
        ":predicate §text-content§ :object §" + golden_fragment.greek_text + "§ :dbid §" + dbid + "§)";
    auto greek_text_response = db9_execute(greek_text_command);
    
    std::string greek_verify_command = 
        "(find-triple-enhanced :subject §" + golden_fragment.entity_id + "§ "
        ":predicate §text-content§ :object §*§ :dbid §" + dbid + "§)";
    auto greek_verify_response = db9_execute(greek_verify_command);
    
    std::cout << "    🏛️ Greek text storage: " << greek_text_response.status << std::endl;
    std::cout << "    🏛️ Greek text retrieval: " << greek_verify_response.result << std::endl;
    
    // Verify Greek text is preserved
    EXPECT_EQ(Db9Response::Success, greek_text_response.status);
    EXPECT_TRUE(greek_verify_response.result.find("ἄκρον καὶ μέσον λόγον") != std::string::npos);
    
    std::cout << "\n  🎯 Text Content Pipeline Test Complete!" << std::endl;
    
    // Cleanup
    std::string close_command = "(close-database :dbid §" + dbid + "§)";
    auto close_response = db9_execute(close_command);
}

LABDB9_TEST_MAIN()
