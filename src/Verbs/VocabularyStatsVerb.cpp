#include "VocabularyStatsVerb.h"
#include "LabDb/Db9Dispatcher.h"
#include "LabDb/DatabaseManager.h"
#include "LabDb/DatabaseVerbs.h"
#include "LabDb/Verbs.h"
#include "LabDb/EntityId.h"
#include "LabDb/LabText.hpp"
#include <algorithm>
#include <sstream>
#include <mutex>

namespace LabDb {

bool VocabularyStatsVerb::registered = false;
std::once_flag VocabularyStatsVerb::register_flag;

VocabularyStatsVerb::VocabularyStatsVerb() {
    // Constructor - verb setup if needed
}

std::string VocabularyStatsVerb::getVerbName() const {
    return "get-vocabulary-stats";
}

std::string VocabularyStatsVerb::getDescription() const {
    return R"(
Analyze vocabulary statistics for a database including predicate usage, entity counts, and distribution with EID resolution.
Usage:
```lisp
(get-vocabulary-stats :dbid database-id)
;; Returns: {
;;   "total_triples": 644,
;;   "unique_predicates": 33,
;;   "unique_subjects": 545,
;;   "unique_objects": 151,
;;   "predicate_list": ["isA", "eid:2", "eid:116", ...],
;;   "predicate_usage": [
;;     {"predicate": "isA", "count": 480}, 
;;     {"predicate": "eid:2", "value": "describes", "count": 120},
;;     {"predicate": "eid:116", "value": "hasAttribute", "count": 44}
;;   ]
;; }
```
    )";
}

Db9Response VocabularyStatsVerb::execute(const ::lab::Text::Sexpr& sexpr) {
    // Extract dbid parameter from S-expression
    // Expected format: (get-vocabulary-stats :dbid "db1")
    std::string dbid;
    
    // Parse parameters from S-expression
    for (size_t i = 0; i < sexpr.expr.size() - 1; ++i) {
        const auto& elem = sexpr.expr[i];
        
        // Look for atoms that match our parameter name
        if (elem.token == tsSexprAtom) {
            int stringIndex = elem.ref;
            if (stringIndex < static_cast<int>(sexpr.strings.size())) {
                const std::string& token = sexpr.strings[stringIndex];
            
                if (token == ":dbid") {
                    // Found parameter, get next value
                    if (i + 1 < sexpr.expr.size()) {
                        const auto& value_elem = sexpr.expr[i + 1];
                        if (value_elem.token == tsSexprAtom || value_elem.token == tsSexprString) {
                            int valueIndex = value_elem.ref;
                            if (valueIndex < static_cast<int>(sexpr.strings.size())) {
                                dbid = sexpr.strings[valueIndex];
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    
    if (dbid.empty()) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_code = "MISSING_PARAMETER";
        response.error_message = "Required parameter 'dbid' not provided";
        return response;
    }
    
    try {
        auto stats = analyzeDatabase(dbid);
        std::string response_json = formatResponse(stats);
        
        Db9Response response;
        response.status = Db9Response::Success;
        response.result = response_json;
        return response;
    }
    catch (const std::exception& e) {
        Db9Response response;
        response.status = Db9Response::Error;
        response.error_code = "ANALYSIS_FAILED";
        response.error_message = std::string("Vocabulary analysis failed: ") + e.what();
        return response;
    }
}

VocabularyStatsVerb::VocabularyStats VocabularyStatsVerb::analyzeDatabase(const std::string& dbid) {
    auto& dbManager = DatabaseManager::instance();
    auto database = dbManager.getDatabase(dbid);
    if (!database) {
        throw std::runtime_error("Database not found: " + dbid);
    }
    
    // Get NonoStore for EID resolution
    auto store = dbManager.getDatabase(dbid);
    if (!store) {
        throw std::runtime_error("Store not found for database: " + dbid);
    }

    VocabularyStats stats;

    // Use unordered_sets for O(1) unique counting and unordered_map for predicate analysis
    std::unordered_set<std::string> unique_subjects;
    std::unordered_set<std::string> unique_predicates;
    std::unordered_set<std::string> unique_objects;
    std::unordered_map<std::string, size_t> predicate_counts;
    std::unordered_map<std::string, std::string> predicate_resolutions; // EID -> resolved value

    // Single pass through all triples - O(N) algorithm
    auto triples = database->query("*", "*", "*");

    // Process each triple with EID resolution for predicates
    for (const auto& triple : triples) {
        std::string subject = triple.subject;
        std::string predicate = triple.predicate;
        std::string object = triple.object;

        // Update unique sets
        unique_subjects.insert(subject);
        unique_predicates.insert(predicate);
        unique_objects.insert(object);

        // Count predicate usage
        predicate_counts[predicate]++;
        
        // Resolve EID chain for predicate if not already resolved
        if (predicate_resolutions.find(predicate) == predicate_resolutions.end()) {
            if (predicate.starts_with("eid:")) {
                // This is already an EID, get the entity directly
                EntityId predicateEntity = EntityId::fromEid(predicate, *store);
                if (predicateEntity.isValid() && predicateEntity.exists()) {
                    std::string entity_value = predicateEntity.name();
                    
                    // Use base class EID resolution utility
                    auto resolved = resolveEidChain(predicate, entity_value, store, false);
                    predicate_resolutions[predicate] = resolved.final_value;
                } else {
                    // EID doesn't exist - use predicate as-is
                    predicate_resolutions[predicate] = predicate;
                }
            } else {
                // Not an EID - use predicate as-is
                predicate_resolutions[predicate] = predicate;
            }
        }

        stats.total_triples++;
    }

    // Set final counts
    stats.unique_subjects = unique_subjects.size();
    stats.unique_predicates = unique_predicates.size();
    stats.unique_objects = unique_objects.size();

    // Create sorted predicate list
    stats.predicate_list.assign(unique_predicates.begin(), unique_predicates.end());
    std::sort(stats.predicate_list.begin(), stats.predicate_list.end());

    // Create enhanced predicate usage list with EID resolution
    for (const auto& pair : predicate_counts) {
        VocabularyStats::PredicateUsage usage;
        usage.predicate = pair.first;
        usage.count = pair.second;
        usage.value = predicate_resolutions[pair.first];
        usage.had_resolution = (usage.value != usage.predicate);
        stats.predicate_usage.push_back(usage);
    }
    
    // Sort by count descending
    std::sort(stats.predicate_usage.begin(), stats.predicate_usage.end(),
              [](const auto& a, const auto& b) {
                  return a.count > b.count;
              });

    return stats;
}

std::string VocabularyStatsVerb::formatResponse(const VocabularyStats& stats) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"total_triples\": " << stats.total_triples << ",\n";
    json << "  \"unique_predicates\": " << stats.unique_predicates << ",\n";
    json << "  \"unique_subjects\": " << stats.unique_subjects << ",\n";
    json << "  \"unique_objects\": " << stats.unique_objects << ",\n";
    
    // Predicate list
    json << "  \"predicate_list\": [";
    for (size_t i = 0; i < stats.predicate_list.size(); ++i) {
        if (i > 0) json << ", ";
        json << "\"" << stats.predicate_list[i] << "\"";
    }
    json << "],\n";
    
    // Predicate usage (sorted by frequency)
    json << "  \"predicate_usage\": [";
    for (size_t i = 0; i < stats.predicate_usage.size(); ++i) {
        if (i > 0) json << ", ";
        const auto& usage = stats.predicate_usage[i];
        json << "{\"predicate\": \"" << usage.predicate << "\"";
        
        // Include resolved value if different from predicate (indicating EID resolution occurred)
        if (usage.had_resolution) {
            json << ", \"value\": \"" << usage.value << "\"";
        }
        
        json << ", \"count\": " << usage.count << "}";
    }
    json << "]\n";
    json << "}";
    
    return json.str();
}

void VocabularyStatsVerb::registerVerb(Db9Dispatcher& dispatcher) {
    std::call_once(register_flag, [&dispatcher]() {
        if (!registered) {
            dispatcher.registerVerb(std::make_unique<LabDb::VocabularyStatsVerb>());
            registered = true;
        }
    });
}

} // namespace LabDb
