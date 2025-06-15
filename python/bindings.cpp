#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include "LabDb/NonoStore.h"
#include "LabDb/TriadicQuery.h"

namespace py = pybind11;

PYBIND11_MODULE(pylabdb, m) {
    m.doc() = "LabDb Python Bindings - Triadic Consciousness Database";
    
    // Core NonoStore class
    py::class_<LabDb::NonoStore>(m, "NonoStore")
        .def(py::init<const std::string&>(), 
             "Create NonoStore instance with database path",
             py::arg("db_path"))
        
        // Core operations
        .def("connect", &LabDb::NonoStore::connect,
             "Connect subject to object via predicate",
             py::arg("subject"), py::arg("predicate"), py::arg("object"))
        
        .def("disconnect", &LabDb::NonoStore::disconnect,
             "Disconnect specific triple",
             py::arg("subject"), py::arg("predicate"), py::arg("object"))
        
        .def("exists", &LabDb::NonoStore::exists,
             "Check if triple exists",
             py::arg("subject"), py::arg("predicate"), py::arg("object"))
        
        .def("count", &LabDb::NonoStore::count,
             "Count triples matching pattern (* for wildcard)",
             py::arg("subject"), py::arg("predicate"), py::arg("object"))
        
        .def("query", &LabDb::NonoStore::query,
             "Query triples matching pattern (* for wildcard)",
             py::arg("subject"), py::arg("predicate"), py::arg("object"))
        
        // Vocabulary discovery - ontological awareness
        .def("all_subjects", &LabDb::NonoStore::all_subjects,
             "Get all subjects (Motion vocabulary)")
        
        .def("all_predicates", &LabDb::NonoStore::all_predicates,
             "Get all predicates (Memory vocabulary)")
        
        .def("all_objects", &LabDb::NonoStore::all_objects,
             "Get all objects (Field vocabulary)")
        
        // Convenience methods for common patterns
        .def("properties_of", &LabDb::NonoStore::properties_of,
             "Get all properties of an entity",
             py::arg("subject"))
        
        .def("entities_with_relation", &LabDb::NonoStore::entities_with_relation,
             "Get all entities using specific relation",
             py::arg("predicate"))
        
        .def("connections_to", &LabDb::NonoStore::connections_to,
             "Get all connections to specific object",
             py::arg("object"))
        
        // Statistics and health
        .def("get_stats", &LabDb::NonoStore::get_stats,
             "Get database statistics")
        
        // Bulk operations
        .def("begin_bulk_update", &LabDb::NonoStore::begin_bulk_update,
             "Begin bulk update transaction")
        
        .def("commit_bulk_update", &LabDb::NonoStore::commit_bulk_update,
             "Commit bulk update transaction")
        
        .def("rollback_bulk_update", &LabDb::NonoStore::rollback_bulk_update,
             "Rollback bulk update transaction");
    
    // TriadicQuery class for conscious navigation
    py::class_<LabDb::TriadicQuery>(m, "TriadicQuery")
        .def(py::init<std::shared_ptr<LabDb::NonoStore>>(),
             "Create TriadicQuery instance with NonoStore",
             py::arg("store"))
        
        // Motion-driven queries (स्पन्द perspective)
        .def("motion_from", &LabDb::TriadicQuery::motion_from,
             "What does this entity express, act upon, or initiate?",
             py::arg("entity"))
        
        .def("motion_through", &LabDb::TriadicQuery::motion_through,
             "What entities express through this relationship?",
             py::arg("relation"))
        
        .def("entity_expressions", &LabDb::TriadicQuery::entity_expressions,
             "What are all the ways this entity expresses itself?",
             py::arg("entity"))
        
        // Memory-driven queries (स्मृति perspective)  
        .def("memory_relations", &LabDb::TriadicQuery::memory_relations,
             "What connections exist for this relationship type?",
             py::arg("predicate"))
        
        .def("memory_between", &LabDb::TriadicQuery::memory_between,
             "What relationship patterns connect these entities?",
             py::arg("entity1"), py::arg("entity2"))
        
        .def("relation_frequencies", &LabDb::TriadicQuery::relation_frequencies,
             "What are the most frequent relationship patterns?")
        
        // Field-driven queries (क्षेत्र perspective)
        .def("field_contexts", &LabDb::TriadicQuery::field_contexts,
             "What receives into this context or grounding?",
             py::arg("object"))
        
        .def("field_for_relation", &LabDb::TriadicQuery::field_for_relation,
             "What contexts ground this type of relationship?",
             py::arg("relation"))
        
        .def("primary_contexts", &LabDb::TriadicQuery::primary_contexts,
             "What are the primary grounding contexts?")
        
        // Cube architecture navigation
        .def("perspective_shift", &LabDb::TriadicQuery::perspective_shift,
             "Switch perspective while maintaining query focus",
             py::arg("results"), py::arg("new_perspective"))
        
        .def("triadic_traverse", &LabDb::TriadicQuery::triadic_traverse,
             "Traverse the triadic cube from any starting point",
             py::arg("starting_point"), py::arg("max_depth") = 3)
        
        .def("crown_exploration", &LabDb::TriadicQuery::crown_exploration,
             "Explore the crown structure around a focal point",
             py::arg("focal_entity"), 
             py::arg("focal_relation") = "*",
             py::arg("focal_context") = "*")
        
        // Analytics and insights
        .def("get_triadic_stats", &LabDb::TriadicQuery::get_triadic_stats,
             "Get comprehensive triadic statistics")
        
        .def("bridge_entities", &LabDb::TriadicQuery::bridge_entities,
             "Find entities that bridge different domains",
             py::arg("connectivity_threshold") = 2.0)
        
        .def("detect_relationship_clusters", &LabDb::TriadicQuery::detect_relationship_clusters,
             "Detect relationship clusters and patterns")
        
        .def("detect_vocabulary_boundaries", &LabDb::TriadicQuery::detect_vocabulary_boundaries,
             "Vocabulary boundary analysis")
        
        // Access to underlying store
        .def("get_store", &LabDb::TriadicQuery::get_store,
             "Access to underlying store for advanced usage");
    
    // Triadic perspective enumeration
    py::enum_<LabDb::TriadicQuery::Perspective>(m, "Perspective")
        .value("Motion", LabDb::TriadicQuery::Perspective::Motion, "स्पन्द - Subject-driven action and expression")
        .value("Memory", LabDb::TriadicQuery::Perspective::Memory, "स्मृति - Predicate-driven connection and pattern")
        .value("Field", LabDb::TriadicQuery::Perspective::Field, "क्षेत्र - Object-driven context and grounding")
        .export_values();
    
    // TriadicResult structure
    py::class_<LabDb::TriadicQuery::TriadicResult>(m, "TriadicResult")
        .def_readonly("entity", &LabDb::TriadicQuery::TriadicResult::entity)
        .def_readonly("relation", &LabDb::TriadicQuery::TriadicResult::relation)
        .def_readonly("context", &LabDb::TriadicQuery::TriadicResult::context)
        .def_readonly("discovered_through", &LabDb::TriadicQuery::TriadicResult::discovered_through)
        .def("__repr__", [](const LabDb::TriadicQuery::TriadicResult& tr) {
            return "TriadicResult('" + tr.entity + "', '" + tr.relation + "', '" + tr.context + 
                   "', discovered_through=" + LabDb::TriadicQuery::perspective_name(tr.discovered_through) + ")";
        });
    
    // TriadicStats structure
    py::class_<LabDb::TriadicQuery::TriadicStats>(m, "TriadicStats")
        .def_readonly("motion_entities", &LabDb::TriadicQuery::TriadicStats::motion_entities)
        .def_readonly("memory_relations", &LabDb::TriadicQuery::TriadicStats::memory_relations)
        .def_readonly("field_contexts", &LabDb::TriadicQuery::TriadicStats::field_contexts)
        .def_readonly("total_connections", &LabDb::TriadicQuery::TriadicStats::total_connections)
        .def_readonly("connectivity_ratio", &LabDb::TriadicQuery::TriadicStats::connectivity_ratio)
        .def_readonly("vocabulary_density", &LabDb::TriadicQuery::TriadicStats::vocabulary_density)
        .def("__repr__", [](const LabDb::TriadicQuery::TriadicStats& ts) {
            return "TriadicStats(motion=" + std::to_string(ts.motion_entities) + 
                   ", memory=" + std::to_string(ts.memory_relations) + 
                   ", field=" + std::to_string(ts.field_contexts) + ")";
        });
    
    // VocabularyBoundary structure
    py::class_<LabDb::TriadicQuery::VocabularyBoundary>(m, "VocabularyBoundary")
        .def_readonly("domain_name", &LabDb::TriadicQuery::VocabularyBoundary::domain_name)
        .def_readonly("entities", &LabDb::TriadicQuery::VocabularyBoundary::entities)
        .def_readonly("relations", &LabDb::TriadicQuery::VocabularyBoundary::relations)
        .def_readonly("contexts", &LabDb::TriadicQuery::VocabularyBoundary::contexts)
        .def_readonly("coherence_score", &LabDb::TriadicQuery::VocabularyBoundary::coherence_score);
    
    // Triple result structure
    py::class_<LabDb::NonoStore::Triple>(m, "Triple")
        .def_readonly("subject", &LabDb::NonoStore::Triple::subject)
        .def_readonly("predicate", &LabDb::NonoStore::Triple::predicate)
        .def_readonly("object", &LabDb::NonoStore::Triple::object)
        .def("__repr__", [](const LabDb::NonoStore::Triple& t) {
            return "Triple('" + t.subject + "', '" + t.predicate + "', '" + t.object + "')";
        });
    
    // Database statistics
    py::class_<LabDb::NonoStore::DatabaseStats>(m, "DatabaseStats")
        .def_readonly("lmdb_stats", &LabDb::NonoStore::DatabaseStats::lmdb_stats);
    
    py::class_<LabDb::NonoStore::LmdbStats>(m, "LmdbStats")
        .def_readonly("page_size", &LabDb::NonoStore::LmdbStats::page_size)
        .def_readonly("depth", &LabDb::NonoStore::LmdbStats::depth)
        .def_readonly("branch_pages", &LabDb::NonoStore::LmdbStats::branch_pages)
        .def_readonly("leaf_pages", &LabDb::NonoStore::LmdbStats::leaf_pages)
        .def_readonly("overflow_pages", &LabDb::NonoStore::LmdbStats::overflow_pages)
        .def_readonly("entries", &LabDb::NonoStore::LmdbStats::entries);
    
    // Utility functions
    m.def("perspective_name", &LabDb::TriadicQuery::perspective_name,
          "Convert perspective to human-readable string",
          py::arg("perspective"));
    
    m.def("perspective_sanskrit", &LabDb::TriadicQuery::perspective_sanskrit,
          "Convert perspective to Sanskrit term",
          py::arg("perspective"));
    
    m.def("optimal_perspective", &LabDb::TriadicQuery::optimal_perspective,
          "Get optimal query perspective for a given pattern",
          py::arg("subject_pattern"), py::arg("predicate_pattern"), py::arg("object_pattern"));
}
