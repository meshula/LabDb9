#pragma once

#include "LabDb/Verbs.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

namespace LabDb {

class Db9Dispatcher;

class VocabularyStatsVerb : public IDb9Verb {
public:
    VocabularyStatsVerb();
    virtual ~VocabularyStatsVerb() = default;

    std::string getVerbName() const override;
    std::string getDescription() const override;
    
    Db9Response execute(const ::lab::Text::Sexpr& sexpr) override;

    // Static registration function
    static void registerVerb(Db9Dispatcher& dispatcher);

private:
    struct VocabularyStats {
        size_t total_triples = 0;
        size_t unique_predicates = 0;
        size_t unique_subjects = 0;
        size_t unique_objects = 0;
        std::vector<std::string> predicate_list;
        
        // Enhanced predicate usage with EID resolution
        struct PredicateUsage {
            std::string predicate;      // Original predicate (may be EID)
            std::string value;          // Resolved human-readable value
            size_t count;               // Usage count
            bool had_resolution;        // True if EID chain resolution occurred
        };
        std::vector<PredicateUsage> predicate_usage;
    };

    VocabularyStats analyzeDatabase(const std::string& dbid);
    std::string formatResponse(const VocabularyStats& stats);
    
    static bool registered;
    static std::once_flag register_flag;
};

} // namespace LabDb