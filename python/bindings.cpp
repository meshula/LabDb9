#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <pybind11/functional.h>
#include <pybind11/chrono.h>
#include "LabDb/NonoStore.h"
#include "LabDb/TriadicQuery.h"

namespace py = pybind11;

PYBIND11_MODULE(pylabdb, m) {
    m.doc() = "LabDb Python Bindings - Phase 2.4: TID Architecture";
    
    // Core NonoStore class (TID-based architecture)
    py::class_<LabDb::NonoStore, std::shared_ptr<LabDb::NonoStore>>(m, "NonoStore")
        .def(py::init<const std::string&>(), 
             "Create NonoStore instance with TID-based architecture",
             py::arg("db_path"))
        
        // Factory method for TriadicQuery creation (solves pybind11 holder type issues)
        .def("create_triadic_query", &LabDb::NonoStore::create_triadic_query,
             "Create TriadicQuery with weak_ptr to avoid pybind11 holder type issues",
             py::return_value_policy::take_ownership)
        
        // Core operations (now using TID architecture internally)
        .def("add_triple", &LabDb::NonoStore::add_triple,
             "Add subject to object via predicate (TID-based)",
             py::arg("subject"), py::arg("predicate"), py::arg("object"))
        
        .def("remove_triple", &LabDb::NonoStore::remove_triple,
             "Remove specific triple (TID-based)",
             py::arg("subject"), py::arg("predicate"), py::arg("object"))
        
        .def("exists", &LabDb::NonoStore::exists,
             "Check if triple exists (TID-based lookup)",
             py::arg("subject"), py::arg("predicate"), py::arg("object"))
        
        .def("count", &LabDb::NonoStore::count,
             "Count triples matching pattern (TID-based)",
             py::arg("subject"), py::arg("predicate"), py::arg("object"))
        
        .def("query", &LabDb::NonoStore::query,
             "Query triples using TID-based indices",
             py::arg("subject"), py::arg("predicate"), py::arg("object"))
        
        // Vocabulary discovery - now TID-efficient
        .def("all_subjects", &LabDb::NonoStore::all_subjects,
             "Get all subjects (TID-based vocabulary)")
        
        .def("all_predicates", &LabDb::NonoStore::all_predicates,
             "Get all predicates (TID-based vocabulary)")
        
        .def("all_objects", &LabDb::NonoStore::all_objects,
             "Get all objects (TID-based vocabulary)")
        
        // Convenience methods
        .def("properties_of", &LabDb::NonoStore::properties_of,
             "Get all properties of an entity (TID-based)",
             py::arg("subject"))
        
        .def("entities_with_relation", &LabDb::NonoStore::entities_with_relation,
             "Get all entities using specific relation (TID-based)",
             py::arg("predicate"))
        
        .def("connections_to", &LabDb::NonoStore::connections_to,
             "Get all connections to specific object (TID-based)",
             py::arg("object"))
        
        // Statistics - should show TID efficiency
        .def("get_stats", &LabDb::NonoStore::get_stats,
             "Get TID-based database statistics")
        
        .def("get_tid_metrics", &LabDb::NonoStore::get_tid_metrics,
             "Get detailed TID architecture metrics");
    
    // TriadicQuery class for conscious navigation (TID-aware)
    // Note: Use NonoStore.create_triadic_query() factory method instead of direct construction
    py::class_<LabDb::TriadicQuery>(m, "TriadicQuery")
        
        // Motion-driven queries (स्पन्द perspective)
        .def("motion_from", &LabDb::TriadicQuery::motion_from,
             "What does this entity express? (TID-based)",
             py::arg("entity"))
        
        .def("motion_through", &LabDb::TriadicQuery::motion_through,
             "What entities express through this relationship? (TID-based)",
             py::arg("relation"))
        
        .def("entity_expressions", &LabDb::TriadicQuery::entity_expressions,
             "All ways this entity expresses itself (TID-based)",
             py::arg("entity"))
        
        // Memory-driven queries (स्मृति perspective)  
        .def("memory_relations", &LabDb::TriadicQuery::memory_relations,
             "What connections exist for this relationship? (TID-based)",
             py::arg("predicate"))
        
        .def("memory_between", &LabDb::TriadicQuery::memory_between,
             "Relationship patterns between entities (TID-based)",
             py::arg("entity1"), py::arg("entity2"))
        
        .def("relation_frequencies", &LabDb::TriadicQuery::relation_frequencies,
             "Most frequent relationship patterns (TID-based)")
        
        // Field-driven queries (क्षेत्र perspective)
        .def("field_contexts", &LabDb::TriadicQuery::field_contexts,
             "What receives into this context? (TID-based)",
             py::arg("object"))
        
        .def("field_for_relation", &LabDb::TriadicQuery::field_for_relation,
             "What contexts ground this relationship? (TID-based)",
             py::arg("relation"))
        
        .def("primary_contexts", &LabDb::TriadicQuery::primary_contexts,
             "Primary grounding contexts (TID-based)")
        
        // Triadic analytics
        .def("get_triadic_stats", &LabDb::TriadicQuery::get_triadic_stats,
             "Get triadic statistics (TID-based)")
        
        .def("bridge_entities", &LabDb::TriadicQuery::bridge_entities,
             "Find bridge entities (TID-based)",
             py::arg("connectivity_threshold") = 2.0)
        
        // Cube architecture navigation
        .def("crown_exploration", &LabDb::TriadicQuery::crown_exploration,
             "Explore crown structure around focal point (TID-based)",
             py::arg("focal_entity"), py::arg("focal_relation") = "*", py::arg("focal_context") = "*")
        
        .def("triadic_traverse", &LabDb::TriadicQuery::triadic_traverse,
             "Traverse triadic cube from starting point (TID-based)",
             py::arg("starting_point"), py::arg("max_depth") = 3)
        
        .def("perspective_shift", &LabDb::TriadicQuery::perspective_shift,
             "Switch perspective while maintaining query focus (TID-based)",
             py::arg("results"), py::arg("new_perspective"));
    
    // Triadic perspective enumeration
    py::enum_<LabDb::TriadicQuery::Perspective>(m, "Perspective")
        .value("Motion", LabDb::TriadicQuery::Perspective::Motion, "स्पन्द - Subject-driven")
        .value("Memory", LabDb::TriadicQuery::Perspective::Memory, "स्मृति - Predicate-driven")  
        .value("Field", LabDb::TriadicQuery::Perspective::Field, "क्षेत्र - Object-driven")
        .export_values();
    
    // Result structures
    py::class_<LabDb::TriadicQuery::TriadicResult>(m, "TriadicResult")
        .def_readonly("entity", &LabDb::TriadicQuery::TriadicResult::entity)
        .def_readonly("relation", &LabDb::TriadicQuery::TriadicResult::relation)
        .def_readonly("context", &LabDb::TriadicQuery::TriadicResult::context)
        .def_readonly("discovered_through", &LabDb::TriadicQuery::TriadicResult::discovered_through)
        .def("__repr__", [](const LabDb::TriadicQuery::TriadicResult& tr) {
            return "TriadicResult('" + tr.entity + "', '" + tr.relation + "', '" + tr.context + "')";
        });
    
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
    
    // Triple result structure  
    py::class_<LabDb::NonoStore::Triple>(m, "Triple")
        .def_readonly("subject", &LabDb::NonoStore::Triple::subject)
        .def_readonly("predicate", &LabDb::NonoStore::Triple::predicate)
        .def_readonly("object", &LabDb::NonoStore::Triple::object)
        .def("__repr__", [](const LabDb::NonoStore::Triple& t) {
            return "Triple('" + t.subject + "', '" + t.predicate + "', '" + t.object + "')";
        });
    
    // Database statistics
    py::class_<LabDb::NonoStore::Stats>(m, "Stats")
        .def_readonly("total_triples", &LabDb::NonoStore::Stats::total_triples)
        .def_readonly("unique_subjects", &LabDb::NonoStore::Stats::unique_subjects)
        .def_readonly("unique_predicates", &LabDb::NonoStore::Stats::unique_predicates)
        .def_readonly("unique_objects", &LabDb::NonoStore::Stats::unique_objects)
        .def_readonly("lmdb_stats", &LabDb::NonoStore::Stats::lmdb_stats)
        .def_readonly("term_dictionary_size", &LabDb::NonoStore::Stats::term_dictionary_size)
        .def_readonly("subject_vocabulary_size", &LabDb::NonoStore::Stats::subject_vocabulary_size)
        .def_readonly("predicate_vocabulary_size", &LabDb::NonoStore::Stats::predicate_vocabulary_size)
        .def_readonly("object_vocabulary_size", &LabDb::NonoStore::Stats::object_vocabulary_size)
        .def_readonly("hexastore_indices_size", &LabDb::NonoStore::Stats::hexastore_indices_size)
        .def_readonly("crown_indices_size", &LabDb::NonoStore::Stats::crown_indices_size);
    
    // TID architecture metrics
    py::class_<LabDb::NonoStore::TIDMetrics>(m, "TIDMetrics")
        .def_readonly("term_dict_entries", &LabDb::NonoStore::TIDMetrics::term_dict_entries)
        .def_readonly("subject_tid_range", &LabDb::NonoStore::TIDMetrics::subject_tid_range)
        .def_readonly("predicate_tid_range", &LabDb::NonoStore::TIDMetrics::predicate_tid_range)
        .def_readonly("object_tid_range", &LabDb::NonoStore::TIDMetrics::object_tid_range)
        .def_readonly("total_tids_allocated", &LabDb::NonoStore::TIDMetrics::total_tids_allocated)
        .def_readonly("storage_efficiency", &LabDb::NonoStore::TIDMetrics::storage_efficiency);
    
    // LMDB stats structure
    py::class_<LabDb::LmdbStore::Stats>(m, "LmdbStats")
        .def_readonly("entries", &LabDb::LmdbStore::Stats::entries)
        .def_readonly("page_size", &LabDb::LmdbStore::Stats::page_size)
        .def_readonly("depth", &LabDb::LmdbStore::Stats::depth)
        .def_readonly("branch_pages", &LabDb::LmdbStore::Stats::branch_pages)
        .def_readonly("leaf_pages", &LabDb::LmdbStore::Stats::leaf_pages)
        .def_readonly("overflow_pages", &LabDb::LmdbStore::Stats::overflow_pages);
    
    // Utility functions
    m.def("perspective_name", &LabDb::TriadicQuery::perspective_name,
          "Convert perspective to human-readable string",
          py::arg("perspective"));
    
    m.def("perspective_sanskrit", &LabDb::TriadicQuery::perspective_sanskrit,
          "Convert perspective to Sanskrit term",
          py::arg("perspective"));
    
    m.def("optimal_perspective", &LabDb::TriadicQuery::optimal_perspective,
          "Get optimal query perspective for pattern",
          py::arg("subject_pattern"), py::arg("predicate_pattern"), py::arg("object_pattern"));
}
