#define LABTEXT_ODR  // ODR instantiation for LabText single header library
#include "LabDb/LabText.hpp"
#include "LabDb/Db9Dispatcher.h"

#include <sstream>
#include <iomanip>
#include <mutex>

namespace LabDb {

//-----------------------------------------------------------------------------
// JSON utilities
//-----------------------------------------------------------------------------
namespace {
    // Escape JSON string
    std::string escapeJson(const std::string& input) {
        std::string escaped;
        escaped.reserve(input.size() + input.size() / 10); // Reserve extra space for escapes
        
        for (char c : input) {
            switch (c) {
                case '"':  escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\b': escaped += "\\b"; break;
                case '\f': escaped += "\\f"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default:
                    if (c < 32) {
                        escaped += "\\u";
                        escaped += std::to_string((unsigned char)c);
                    } else {
                        escaped += c;
                    }
                    break;
            }
        }
        return escaped;
    }
}

//-----------------------------------------------------------------------------
// Db9Response implementation
//-----------------------------------------------------------------------------
std::string Db9Response::toJson() const {
    std::ostringstream json;
    json << "{\n";
    
    // Status
    json << "  \"status\": \"";
    switch (status) {
        case Success: json << "success"; break;
        case Error:   json << "error"; break;
        case Warning: json << "warning"; break;
    }
    json << "\",\n";
    
    // Result
    json << "  \"result\": \"" << escapeJson(result) << "\",\n";
    
    // Error information (optional)
    if (!error_code.empty()) {
        json << "  \"error_code\": \"" << escapeJson(error_code) << "\",\n";
    }
    if (!error_message.empty()) {
        json << "  \"error_message\": \"" << escapeJson(error_message) << "\",\n";
    }
    
    // Auto-reflexive metrics
    json << "  \"auto_reflexive\": {\n";
    json << "    \"operation_time_ms\": " << auto_reflexive.operation_time_ms.count() << ",\n";
    json << "    \"items_processed\": " << auto_reflexive.items_processed << ",\n";
    json << "    \"cache_hit_ratio\": " << std::fixed << std::setprecision(3) << auto_reflexive.cache_hit_ratio << ",\n";
    json << "    \"tid_allocations\": " << auto_reflexive.tid_allocations << ",\n";
    json << "    \"memory_usage_kb\": " << auto_reflexive.memory_usage_kb << "\n";
    json << "  }\n";
    
    json << "}";
    return json.str();
}

//-----------------------------------------------------------------------------
// Db9Dispatcher implementation
//-----------------------------------------------------------------------------
class Db9Dispatcher::Impl {
public:
    std::unordered_map<std::string, std::unique_ptr<IDb9Verb>> verbs;
    std::mutex verbMutex; // Thread safety for verb registration
    
    Db9Response executeVerb(const std::string& verbName, const ::lab::Text::Sexpr& sexpr) {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        std::lock_guard<std::mutex> lock(verbMutex);
        auto it = verbs.find(verbName);
        if (it == verbs.end()) {
            Db9Response response;
            response.status = Db9Response::Error;
            response.error_code = "UNKNOWN_VERB";
            response.error_message = "Unknown verb: " + verbName;
            response.result = "";
            
            auto endTime = std::chrono::high_resolution_clock::now();
            response.auto_reflexive.operation_time_ms = 
                std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            
            return response;
        }
        
        try {
            Db9Response response = it->second->execute(sexpr);
            
            // Add timing if not already set
            if (response.auto_reflexive.operation_time_ms.count() == 0) {
                auto endTime = std::chrono::high_resolution_clock::now();
                response.auto_reflexive.operation_time_ms = 
                    std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            }
            
            return response;
        }
        catch (const std::exception& e) {
            Db9Response response;
            response.status = Db9Response::Error;
            response.error_code = "EXECUTION_ERROR";
            response.error_message = std::string("Verb execution failed: ") + e.what();
            response.result = "";
            
            auto endTime = std::chrono::high_resolution_clock::now();
            response.auto_reflexive.operation_time_ms = 
                std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            
            return response;
        }
    }
    
    Db9Response parseSexprAndExecute(const std::string& sexprCommand) {
        try {
            // Parse the S-expression
            ::lab::Text::Sexpr sexpr((::lab::Text::StrView(sexprCommand)));
            
            // Validate we have a proper S-expression
            if (sexpr.expr.empty() || sexpr.expr[0].token != tsSexprPushList) {
                Db9Response response;
                response.status = Db9Response::Error;
                response.error_code = "PARSE_ERROR";
                response.error_message = "Invalid S-expression: must start with '('";
                return response;
            }
            
            // Extract verb name (first atom after opening paren)
            std::string verbName;
            size_t exprIndex = 1; // Skip the initial PushList
            if (exprIndex < sexpr.expr.size() && sexpr.expr[exprIndex].token == tsSexprAtom) {
                int stringIndex = sexpr.expr[exprIndex].ref;
                if (stringIndex < sexpr.strings.size()) {
                    verbName = sexpr.strings[stringIndex];
                }
            }
            
            if (verbName.empty()) {
                Db9Response response;
                response.status = Db9Response::Error;
                response.error_code = "PARSE_ERROR";
                response.error_message = "No verb found in S-expression";
                return response;
            }
            
            // Execute the verb
            return executeVerb(verbName, sexpr);
        }
        catch (const std::exception& e) {
            Db9Response response;
            response.status = Db9Response::Error;
            response.error_code = "PARSE_ERROR";
            response.error_message = std::string("S-expression parsing failed: ") + e.what();
            return response;
        }
    }
};

//-----------------------------------------------------------------------------
// Db9Dispatcher public interface
//-----------------------------------------------------------------------------
Db9Dispatcher::Db9Dispatcher() : m_impl(std::make_unique<Impl>()) {
}

Db9Dispatcher::~Db9Dispatcher() = default;

Db9Response Db9Dispatcher::executeCommand(const std::string& sexprCommand) {
    return m_impl->parseSexprAndExecute(sexprCommand);
}

Db9Response Db9Dispatcher::executeCommands(const std::vector<std::string>& sexprCommands) {
    Db9Response aggregatedResponse;
    
    if (sexprCommands.empty()) {
        aggregatedResponse.status = Db9Response::Error;
        aggregatedResponse.error_code = "NO_COMMANDS";
        aggregatedResponse.error_message = "No S-expression commands provided";
        return aggregatedResponse;
    }
    
    std::ostringstream resultArray;
    resultArray << "[\n";
    
    // Track aggregate metrics
    std::chrono::milliseconds totalTime(0);
    int64_t totalItems = 0;
    int32_t totalTidAllocations = 0;
    int64_t totalMemoryUsage = 0;
    bool hasErrors = false;
    
    for (size_t i = 0; i < sexprCommands.size(); ++i) {
        if (i > 0) resultArray << ",\n";
        
        Db9Response response = m_impl->parseSexprAndExecute(sexprCommands[i]);
        resultArray << "  " << response.toJson();
        
        // Aggregate metrics
        totalTime += response.auto_reflexive.operation_time_ms;
        totalItems += response.auto_reflexive.items_processed;
        totalTidAllocations += response.auto_reflexive.tid_allocations;
        totalMemoryUsage += response.auto_reflexive.memory_usage_kb;
        
        if (response.status == Db9Response::Error) {
            hasErrors = true;
        }
    }
    
    resultArray << "\n]";
    
    // Set aggregated response
    aggregatedResponse.status = hasErrors ? Db9Response::Warning : Db9Response::Success;
    aggregatedResponse.result = resultArray.str();
    
    if (hasErrors) {
        aggregatedResponse.error_code = "PARTIAL_FAILURES";
        aggregatedResponse.error_message = "Some commands in the batch failed - see individual results";
    }
    
    // Set aggregate metrics
    aggregatedResponse.auto_reflexive.operation_time_ms = totalTime;
    aggregatedResponse.auto_reflexive.items_processed = totalItems;
    aggregatedResponse.auto_reflexive.cache_hit_ratio = sexprCommands.size() > 0 ? 
        (totalItems > 0 ? 1.0 : 0.0) : 0.0; // Simplified calculation
    aggregatedResponse.auto_reflexive.tid_allocations = totalTidAllocations;
    aggregatedResponse.auto_reflexive.memory_usage_kb = totalMemoryUsage;
    
    return aggregatedResponse;
}

void Db9Dispatcher::registerVerb(std::unique_ptr<IDb9Verb> verb) {
    if (!verb) return;
    
    std::lock_guard<std::mutex> lock(m_impl->verbMutex);
    std::string verbName = verb->getVerbName();
    m_impl->verbs[verbName] = std::move(verb);
}

std::vector<std::string> Db9Dispatcher::getAvailableVerbs() const {
    std::lock_guard<std::mutex> lock(m_impl->verbMutex);
    std::vector<std::string> verbs;
    
    for (const auto& pair : m_impl->verbs) {
        verbs.push_back(pair.first);
    }
    
    std::sort(verbs.begin(), verbs.end());
    return verbs;
}

std::string Db9Dispatcher::getSpecification() const {
    // For now, return a placeholder - this will be populated with the full
    // markdown specification from db9-tool-spec.md
    return "# db9 Tool Specification\n\n"
           "High-performance triadic consciousness database operations via S-expressions.\n\n"
           "Available verbs: " + std::to_string(m_impl->verbs.size()) + " registered\n\n"
           "Use S-expression format: (verb :param1 value1 :param2 value2)\n";
}

//-----------------------------------------------------------------------------
// Global dispatcher singleton
//-----------------------------------------------------------------------------
Db9Dispatcher& getGlobalDb9Dispatcher() {
    static Db9Dispatcher dispatcher;
    return dispatcher;
}

} // namespace LabDb
