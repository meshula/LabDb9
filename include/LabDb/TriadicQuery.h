#pragma once

#include "LabDb/NonoStore.h"
#include <string>
#include <vector>
#include <memory>

namespace LabDb {

/// Triadic query interface providing conscious navigation through Motion/Memory/Field perspectives
/// Implements त्रित्रयम् (tritrayam) - triadic consciousness navigation patterns
class TriadicQuery {
public:
    /// Triadic perspectives for conscious navigation
    enum class Perspective {
        Motion,  // स्पन्द (spanda) - Subject-driven action and expression
        Memory,  // स्मृति (smriti) - Predicate-driven connection and pattern  
        Field    // क्षेत्र (kshetra) - Object-driven context and grounding
    };
    
    /// Result structure for triadic queries
    struct TriadicResult {
        std::string entity;
        std::string relation;
        std::string context;
        Perspective discovered_through;
    };
    
    /// Statistics for triadic analysis
    struct TriadicStats {
        size_t motion_entities;     // Unique subjects expressing action
        size_t memory_relations;    // Unique predicates connecting entities
        size_t field_contexts;      // Unique objects grounding relationships
        size_t total_connections;   // Total relationship instances
        double connectivity_ratio;  // Average connections per entity
        double vocabulary_density;  // Vocabulary richness measure
    };
    
    /// Construct triadic query interface around a NonoStore
    explicit TriadicQuery(std::shared_ptr<NonoStore> store);
    
    /// Motion-driven queries (स्पन्द perspective)
    /// What does this entity express, act upon, or initiate?
    std::vector<TriadicResult> motion_from(const std::string& entity);
    
    /// What entities express through this relationship?
    std::vector<TriadicResult> motion_through(const std::string& relation);
    
    /// What are all the ways this entity expresses itself?
    std::vector<std::string> entity_expressions(const std::string& entity);
    
    /// Memory-driven queries (स्मृति perspective)  
    /// What connections exist for this relationship type?
    std::vector<TriadicResult> memory_relations(const std::string& predicate);
    
    /// What relationship patterns connect these entities?
    std::vector<TriadicResult> memory_between(const std::string& entity1, const std::string& entity2);
    
    /// What are the most frequent relationship patterns?
    std::vector<std::pair<std::string, size_t>> relation_frequencies();
    
    /// Field-driven queries (क्षेत्र perspective)
    /// What receives into this context or grounding?
    std::vector<TriadicResult> field_contexts(const std::string& object);
    
    /// What contexts ground this type of relationship?
    std::vector<TriadicResult> field_for_relation(const std::string& relation);
    
    /// What are the primary grounding contexts?
    std::vector<std::string> primary_contexts();
    
    /// Cube architecture navigation
    /// Switch perspective while maintaining query focus
    std::vector<TriadicResult> perspective_shift(const std::vector<TriadicResult>& results, 
                                                  Perspective new_perspective);
    
    /// Traverse the triadic cube from any starting point
    std::vector<TriadicResult> triadic_traverse(const std::string& starting_point, 
                                                 int max_depth = 3);
    
    /// Explore the crown structure around a focal point
    std::vector<TriadicResult> crown_exploration(const std::string& focal_entity,
                                                  const std::string& focal_relation = "*",
                                                  const std::string& focal_context = "*");
    
    /// Analytics and insights
    /// Get comprehensive triadic statistics
    TriadicStats get_triadic_stats();
    
    /// Find entities that bridge different domains
    std::vector<std::string> bridge_entities(double connectivity_threshold = 2.0);
    
    /// Detect relationship clusters and patterns
    std::vector<std::vector<std::string>> detect_relationship_clusters();
    
    /// Vocabulary boundary analysis
    struct VocabularyBoundary {
        std::string domain_name;
        std::vector<std::string> entities;
        std::vector<std::string> relations;
        std::vector<std::string> contexts;
        double coherence_score;
    };
    std::vector<VocabularyBoundary> detect_vocabulary_boundaries();
    
    /// Utility methods
    /// Convert perspective to human-readable string
    static std::string perspective_name(Perspective p);
    
    /// Convert perspective to Sanskrit term
    static std::string perspective_sanskrit(Perspective p);
    
    /// Get optimal query perspective for a given pattern
    static Perspective optimal_perspective(const std::string& subject_pattern,
                                           const std::string& predicate_pattern, 
                                           const std::string& object_pattern);
    
    /// Entity Discovery & Sampling Interface
    /// Sample N entities from Motion perspective (subjects)
    std::vector<std::string> sample_motion_entities(size_t count, size_t offset = 0);
    
    /// Sample N entities from Memory perspective (predicates)
    std::vector<std::string> sample_memory_relations(size_t count, size_t offset = 0);
    
    /// Sample N entities from Field perspective (objects/contexts)
    std::vector<std::string> sample_field_contexts(size_t count, size_t offset = 0);
    
    /// Get paginated random sampling across all three perspectives
    struct SamplingResult {
        std::vector<std::string> motion_entities;
        std::vector<std::string> memory_relations;
        std::vector<std::string> field_contexts;
        size_t total_motion_count;
        size_t total_memory_count;
        size_t total_field_count;
    };
    SamplingResult sample_triadic_entities(size_t motion_count = 5, 
                                           size_t memory_count = 5,
                                           size_t field_count = 10,
                                           size_t offset = 0);
    
    /// Browse all entities by type with pagination
    std::vector<std::string> browse_entities_by_type(Perspective perspective, 
                                                     size_t limit = 20, 
                                                     size_t offset = 0);
                                                     
    /// Random sampling from each perspective (useful for exploration)
    std::vector<std::string> random_sample_motion(size_t count);
    std::vector<std::string> random_sample_memory(size_t count);
    std::vector<std::string> random_sample_field(size_t count);
    
    /// Access to underlying store for advanced usage
    std::shared_ptr<NonoStore> get_store() const { return _store; }
    
private:
    std::shared_ptr<NonoStore> _store;
    
    /// Internal helpers
    std::vector<TriadicResult> convert_triples(const std::vector<NonoStore::Triple>& triples,
                                               Perspective perspective);
    
    double calculate_connectivity_ratio();
    double calculate_vocabulary_density();
    
    /// Clustering algorithms for pattern detection
    std::vector<std::vector<std::string>> cluster_by_relations(const std::vector<std::string>& entities);
    double calculate_coherence(const std::vector<std::string>& entities,
                              const std::vector<std::string>& relations);
};

} // namespace LabDb
