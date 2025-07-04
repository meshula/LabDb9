#include "LabDb/TriadicQuery.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <random>

namespace LabDb {

TriadicQuery::TriadicQuery(std::weak_ptr<NonoStore> store) : _store(store) {
}

std::shared_ptr<NonoStore> TriadicQuery::get_store_safe() const {
    auto store = _store.lock();
    if (!store) {
        // Store has been destroyed - this is expected graceful degradation
        // Methods calling this will return empty results
    }
    return store;
}

// Motion-driven queries (स्पन्द perspective)
std::vector<TriadicQuery::TriadicResult> TriadicQuery::motion_from(const std::string& entity) {
    auto store = get_store_safe();
    if (!store) {
        return {}; // Graceful degradation if store destroyed
    }
    
    // Query all properties and relationships where entity is the subject
    auto properties = store->entities_with_subject(entity);
    return convert_triples(properties, Perspective::Motion);
}

std::vector<TriadicQuery::TriadicResult> TriadicQuery::motion_through(const std::string& relation) {
    auto store = get_store_safe();
    if (!store) {
        return {}; // Graceful degradation if store destroyed
    }
    
    // Query all entities that express through this relationship
    auto entities = store->entities_with_predicate(relation);
    return convert_triples(entities, Perspective::Motion);
}

std::vector<std::string> TriadicQuery::entity_expressions(const std::string& entity) {
    auto store = get_store_safe();
    if (!store) {
        return {}; // Graceful degradation
    }
    
    // Get all unique predicates that this entity uses to express itself
    auto properties = store->entities_with_subject(entity);
    std::unordered_set<std::string> unique_predicates;
    
    for (const auto& triple : properties) {
        unique_predicates.insert(triple.predicate);
    }
    
    return std::vector<std::string>(unique_predicates.begin(), unique_predicates.end());
}

// Memory-driven queries (स्मृति perspective)
std::vector<TriadicQuery::TriadicResult> TriadicQuery::memory_relations(const std::string& predicate) {
    auto store = get_store_safe();
    if (!store) {
        return {}; // Graceful degradation
    }
    
    // Query all connections using this predicate
    auto connections = store->query("*", predicate, "*");
    return convert_triples(connections, Perspective::Memory);
}

std::vector<TriadicQuery::TriadicResult> TriadicQuery::memory_between(const std::string& entity1, 
                                                                       const std::string& entity2) {
    auto store = get_store_safe();
    if (!store) {
        return {}; // Graceful degradation
    }
    
    std::vector<TriadicResult> results;
    
    // Find direct relationships entity1 -> entity2
    auto forward = store->query(entity1, "*", entity2);
    auto forward_results = convert_triples(forward, Perspective::Memory);
    results.insert(results.end(), forward_results.begin(), forward_results.end());
    
    // Find reverse relationships entity2 -> entity1
    auto reverse = store->query(entity2, "*", entity1);
    auto reverse_results = convert_triples(reverse, Perspective::Memory);
    results.insert(results.end(), reverse_results.begin(), reverse_results.end());
    
    return results;
}

std::vector<std::pair<std::string, size_t>> TriadicQuery::relation_frequencies() {
    auto store = get_store_safe();
    if (!store) {
        return {}; // Graceful degradation
    }
    
    auto predicates = store->all_predicates();
    std::vector<std::pair<std::string, size_t>> frequencies;
    
    for (const auto& predicate : predicates) {
        size_t count = store->count("*", predicate, "*");
        frequencies.emplace_back(predicate, count);
    }
    
    // Sort by frequency (descending)
    std::sort(frequencies.begin(), frequencies.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    return frequencies;
}

// Field-driven queries (क्षेत्र perspective)
std::vector<TriadicQuery::TriadicResult> TriadicQuery::field_contexts(const std::string& object) {
    auto store = get_store_safe();
    if (!store) {
        return {}; // Graceful degradation
    }
    
    // Query all relationships that ground into this object/context
    auto connections = store->entities_with_object(object);
    return convert_triples(connections, Perspective::Field);
}

std::vector<TriadicQuery::TriadicResult> TriadicQuery::field_for_relation(const std::string& relation) {
    auto store = get_store_safe();
    if (!store) {
        return {}; // Graceful degradation
    }
    
    // Query all contexts that ground this type of relationship
    auto contexts = store->query("*", relation, "*");
    return convert_triples(contexts, Perspective::Field);
}

std::vector<std::string> TriadicQuery::primary_contexts() {
    auto store = get_store_safe();
    if (!store) {
        return {}; // Graceful degradation
    }
    
    // Find the most frequently used objects/contexts
    auto objects = store->all_objects();
    
    std::vector<std::pair<std::string, size_t>> object_frequencies;
    for (const auto& object : objects) {
        size_t count = store->count("*", "*", object);
        object_frequencies.emplace_back(object, count);
    }
    
    // Sort by frequency and return top contexts
    std::sort(object_frequencies.begin(), object_frequencies.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::vector<std::string> primary;
    for (const auto& [object, count] : object_frequencies) {
        primary.push_back(object);
    }
    
    return primary;
}

// Cube architecture navigation
std::vector<TriadicQuery::TriadicResult> TriadicQuery::perspective_shift(
    const std::vector<TriadicResult>& results, 
    Perspective new_perspective) {
    
    std::vector<TriadicResult> shifted_results;
    
    for (const auto& result : results) {
        std::vector<TriadicResult> perspective_results;
        
        switch (new_perspective) {
            case Perspective::Motion:
                perspective_results = motion_from(result.entity);
                break;
            case Perspective::Memory:
                perspective_results = memory_relations(result.relation);
                break;
            case Perspective::Field:
                perspective_results = field_contexts(result.context);
                break;
        }
        
        shifted_results.insert(shifted_results.end(), 
                              perspective_results.begin(), perspective_results.end());
    }
    
    return shifted_results;
}

std::vector<TriadicQuery::TriadicResult> TriadicQuery::triadic_traverse(
    const std::string& starting_point, int max_depth) {
    
    std::vector<TriadicResult> all_results;
    std::unordered_set<std::string> visited;
    
    // Start with motion perspective from the starting point
    auto current_results = motion_from(starting_point);
    all_results.insert(all_results.end(), current_results.begin(), current_results.end());
    visited.insert(starting_point);
    
    for (int depth = 1; depth < max_depth; ++depth) {
        std::vector<TriadicResult> next_level;
        
        for (const auto& result : current_results) {
            // Explore through memory perspective
            if (visited.find(result.relation) == visited.end()) {
                auto memory_results = memory_relations(result.relation);
                next_level.insert(next_level.end(), memory_results.begin(), memory_results.end());
                visited.insert(result.relation);
            }
            
            // Explore through field perspective
            if (visited.find(result.context) == visited.end()) {
                auto field_results = field_contexts(result.context);
                next_level.insert(next_level.end(), field_results.begin(), field_results.end());
                visited.insert(result.context);
            }
        }
        
        all_results.insert(all_results.end(), next_level.begin(), next_level.end());
        current_results = next_level;
    }
    
    return all_results;
}

std::vector<TriadicQuery::TriadicResult> TriadicQuery::crown_exploration(
    const std::string& focal_entity,
    const std::string& focal_relation,
    const std::string& focal_context) {
    
    std::vector<TriadicResult> crown_results;
    
    // Motion crown: What does the focal entity express?
    if (focal_entity != "*") {
        auto motion_results = motion_from(focal_entity);
        crown_results.insert(crown_results.end(), motion_results.begin(), motion_results.end());
    }
    
    // Memory crown: What connects through this relationship?
    if (focal_relation != "*") {
        auto memory_results = memory_relations(focal_relation);
        crown_results.insert(crown_results.end(), memory_results.begin(), memory_results.end());
    }
    
    // Field crown: What grounds in this context?
    if (focal_context != "*") {
        auto field_results = field_contexts(focal_context);
        crown_results.insert(crown_results.end(), field_results.begin(), field_results.end());
    }
    
    return crown_results;
}

// Analytics and insights
TriadicQuery::TriadicStats TriadicQuery::get_triadic_stats() {
    auto store = get_store_safe();
    if (!store) {
        return {}; // Graceful degradation - return default-constructed stats
    }
    
    TriadicStats stats;
    
    stats.motion_entities = store->all_subjects().size();
    stats.memory_relations = store->all_predicates().size();
    stats.field_contexts = store->all_objects().size();
    
    auto store_stats = store->get_stats();
    stats.total_connections = store_stats.total_triples;
    
    stats.connectivity_ratio = calculate_connectivity_ratio();
    stats.vocabulary_density = calculate_vocabulary_density();
    
    return stats;
}

std::vector<std::string> TriadicQuery::bridge_entities(double connectivity_threshold) {
    auto store = get_store_safe();
    if (!store) {
        return {}; // Graceful degradation
    }
    
    auto subjects = store->all_subjects();
    std::vector<std::string> bridges;
    
    for (const auto& subject : subjects) {
        auto properties = store->entities_with_subject(subject);
        double connectivity = static_cast<double>(properties.size());
        
        if (connectivity >= connectivity_threshold) {
            bridges.push_back(subject);
        }
    }
    
    return bridges;
}

std::vector<std::vector<std::string>> TriadicQuery::detect_relationship_clusters() {
    auto store = get_store_safe();
    if (!store) {
        return {}; // Graceful degradation
    }
    auto subjects = store->all_subjects();
    return cluster_by_relations(subjects);
}

std::vector<TriadicQuery::VocabularyBoundary> TriadicQuery::detect_vocabulary_boundaries() {
    auto store = get_store_safe();
    if (!store) {
        return {}; // Graceful degradation
    }

    // Simplified implementation - in practice this would use more sophisticated clustering
    std::vector<VocabularyBoundary> boundaries;
    
    auto subjects = store->all_subjects();
    auto predicates = store->all_predicates();
    auto objects = store->all_objects();
    
    // Create a simple boundary based on relationship types
    VocabularyBoundary general_domain;
    general_domain.domain_name = "General";
    general_domain.entities = subjects;
    general_domain.relations = predicates;
    general_domain.contexts = objects;
    general_domain.coherence_score = calculate_coherence(subjects, predicates);
    
    boundaries.push_back(general_domain);
    
    return boundaries;
}

// Utility methods
std::string TriadicQuery::perspective_name(Perspective p) {
    switch (p) {
        case Perspective::Motion: return "Motion";
        case Perspective::Memory: return "Memory";
        case Perspective::Field: return "Field";
        default: return "Unknown";
    }
}

std::string TriadicQuery::perspective_sanskrit(Perspective p) {
    switch (p) {
        case Perspective::Motion: return "स्पन्द (spanda)";
        case Perspective::Memory: return "स्मृति (smriti)";
        case Perspective::Field: return "क्षेत्र (kshetra)";
        default: return "अज्ञात (ajnata)";
    }
}

TriadicQuery::Perspective TriadicQuery::optimal_perspective(
    const std::string& subject_pattern,
    const std::string& predicate_pattern,
    const std::string& object_pattern) {
    
    bool subj_wild = (subject_pattern == "*");
    bool pred_wild = (predicate_pattern == "*");
    bool obj_wild = (object_pattern == "*");
    
    // Determine optimal perspective based on what's specified
    if (!subj_wild && pred_wild && obj_wild) {
        return Perspective::Motion;  // Subject-focused
    } else if (subj_wild && !pred_wild && obj_wild) {
        return Perspective::Memory;  // Predicate-focused
    } else if (subj_wild && pred_wild && !obj_wild) {
        return Perspective::Field;   // Object-focused
    } else if (!subj_wild) {
        return Perspective::Motion;  // Default to motion for subject-specified queries
    } else if (!pred_wild) {
        return Perspective::Memory;  // Default to memory for predicate-specified queries
    } else {
        return Perspective::Field;   // Default to field for object-specified queries
    }
}

// Private implementation methods
std::vector<TriadicQuery::TriadicResult> TriadicQuery::convert_triples(
    const std::vector<NonoStore::Triple>& triples,
    Perspective perspective) {
    
    std::vector<TriadicResult> results;
    results.reserve(triples.size());
    
    for (const auto& triple : triples) {
        TriadicResult result;
        result.entity = triple.subject;
        result.relation = triple.predicate;
        result.context = triple.object;
        result.discovered_through = perspective;
        results.push_back(result);
    }
    
    return results;
}

double TriadicQuery::calculate_connectivity_ratio() {
    auto store = get_store_safe();
    if (!store) {
        return {}; // Graceful degradation
    }

    auto subjects = store->all_subjects();
    if (subjects.empty()) return 0.0;
    
    double total_connections = 0.0;
    for (const auto& subject : subjects) {
        auto properties = store->entities_with_subject(subject);
        total_connections += properties.size();
    }
    
    return total_connections / subjects.size();
}

double TriadicQuery::calculate_vocabulary_density() {
    auto store = get_store_safe();
    if (!store) {
        return {}; // Graceful degradation
    }

    auto stats = store->get_stats();
    if (stats.total_triples == 0) return 0.0;
    
    double total_vocabulary = stats.unique_subjects + stats.unique_predicates + stats.unique_objects;
    return total_vocabulary / stats.total_triples;
}

std::vector<std::vector<std::string>> TriadicQuery::cluster_by_relations(
    const std::vector<std::string>& entities) {
    
    // Simplified clustering based on shared relationships
    std::unordered_map<std::string, std::vector<std::string>> predicate_groups;
    
    for (const auto& entity : entities) {
        auto expressions = entity_expressions(entity);
        for (const auto& predicate : expressions) {
            predicate_groups[predicate].push_back(entity);
        }
    }
    
    std::vector<std::vector<std::string>> clusters;
    for (const auto& [predicate, group_entities] : predicate_groups) {
        if (group_entities.size() > 1) {  // Only include clusters with multiple entities
            clusters.push_back(group_entities);
        }
    }
    
    return clusters;
}

double TriadicQuery::calculate_coherence(
    const std::vector<std::string>& entities,
    const std::vector<std::string>& relations) {
    auto store = get_store_safe();
    if (!store) {
        return {}; // Graceful degradation
    }
    
    if (entities.empty() || relations.empty()) return 0.0;
    
    // Simple coherence measure: ratio of actual connections to possible connections
    double total_possible = entities.size() * relations.size();
    double actual_connections = 0.0;
    
    for (const auto& entity : entities) {
        for (const auto& relation : relations) {
            if (store->count(entity, relation, "*") > 0) {
                actual_connections += 1.0;
            }
        }
    }
    
    return actual_connections / total_possible;
}

// Entity Discovery & Sampling Interface Implementation
std::vector<std::string> TriadicQuery::sample_motion_entities(size_t count, size_t offset) {
    auto store = get_store_safe();
    if (!store) {
        return {}; // Graceful degradation
    }
    auto all_entities = store->all_subjects();
    
    // Apply offset and limit
    std::vector<std::string> sampled;
    for (size_t i = offset; i < all_entities.size() && sampled.size() < count; ++i) {
        sampled.push_back(all_entities[i]);
    }
    
    return sampled;
}

std::vector<std::string> TriadicQuery::sample_memory_relations(size_t count, size_t offset) {
    auto store = get_store_safe();
    if (!store) {
        return {}; // Graceful degradation
    }

    auto all_relations = store->all_predicates();
    
    // Apply offset and limit
    std::vector<std::string> sampled;
    for (size_t i = offset; i < all_relations.size() && sampled.size() < count; ++i) {
        sampled.push_back(all_relations[i]);
    }
    
    return sampled;
}

std::vector<std::string> TriadicQuery::sample_field_contexts(size_t count, size_t offset) {
    auto store = get_store_safe();
    if (!store) {
        return {}; // Graceful degradation
    }
    auto all_contexts = store->all_objects();
    
    // Apply offset and limit
    std::vector<std::string> sampled;
    for (size_t i = offset; i < all_contexts.size() && sampled.size() < count; ++i) {
        sampled.push_back(all_contexts[i]);
    }
    
    return sampled;
}

TriadicQuery::SamplingResult TriadicQuery::sample_triadic_entities(
    size_t motion_count, size_t memory_count, size_t field_count, size_t offset) {
    auto store = get_store_safe();
    if (!store) {
        return {}; // Graceful degradation
    }
    
    SamplingResult result;
    
    // Sample from each perspective
    result.motion_entities = sample_motion_entities(motion_count, offset);
    result.memory_relations = sample_memory_relations(memory_count, offset);
    result.field_contexts = sample_field_contexts(field_count, offset);
    
    // Get total counts for pagination info
    result.total_motion_count = store->all_subjects().size();
    result.total_memory_count = store->all_predicates().size();
    result.total_field_count = store->all_objects().size();
    
    return result;
}

std::vector<std::string> TriadicQuery::browse_entities_by_type(
    Perspective perspective, size_t limit, size_t offset) {
    
    switch (perspective) {
        case Perspective::Motion:
            return sample_motion_entities(limit, offset);
        case Perspective::Memory:
            return sample_memory_relations(limit, offset);
        case Perspective::Field:
            return sample_field_contexts(limit, offset);
        default:
            return {};
    }
}

std::vector<std::string> TriadicQuery::random_sample_motion(size_t count) {
    auto store = get_store_safe();
    if (!store) {
        return {}; // Graceful degradation
    }
    auto all_entities = store->all_subjects();
    
    if (all_entities.size() <= count) {
        return all_entities;
    }
    
    // Modern random sampling using std::shuffle
    std::vector<std::string> shuffled = all_entities;
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(shuffled.begin(), shuffled.end(), g);
    
    std::vector<std::string> sampled;
    for (size_t i = 0; i < count && i < shuffled.size(); ++i) {
        sampled.push_back(shuffled[i]);
    }
    
    return sampled;
}

std::vector<std::string> TriadicQuery::random_sample_memory(size_t count) {
    auto store = get_store_safe();
    if (!store) {
        return {}; // Graceful degradation
    }
    auto all_relations = store->all_predicates();
    
    if (all_relations.size() <= count) {
        return all_relations;
    }
    
    // Modern random sampling using std::shuffle
    std::vector<std::string> shuffled = all_relations;
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(shuffled.begin(), shuffled.end(), g);
    
    std::vector<std::string> sampled;
    for (size_t i = 0; i < count && i < shuffled.size(); ++i) {
        sampled.push_back(shuffled[i]);
    }
    
    return sampled;
}

std::vector<std::string> TriadicQuery::random_sample_field(size_t count) {
    auto store = get_store_safe();
    if (!store) {
        return {}; // Graceful degradation
    }
    auto all_contexts = store->all_objects();
    
    if (all_contexts.size() <= count) {
        return all_contexts;
    }
    
    // Modern random sampling using std::shuffle
    std::vector<std::string> shuffled = all_contexts;
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(shuffled.begin(), shuffled.end(), g);
    
    std::vector<std::string> sampled;
    for (size_t i = 0; i < count && i < shuffled.size(); ++i) {
        sampled.push_back(shuffled[i]);
    }
    
    return sampled;
}

} // namespace LabDb
