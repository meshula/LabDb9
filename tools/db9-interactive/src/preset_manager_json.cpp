//
// Preset Manager JSON Serialization
// Handles JSON serialization/deserialization of presets
//

#include "preset_manager.hpp"
#include <sstream>
#include <regex>

namespace consciousness {

// Simple JSON serialization (basic implementation)
std::string PresetManager::serializePreset(const ExperimentalPreset& preset) const {
    std::ostringstream json;
    
    json << "{\n";
    json << "  \"name\": \"" << preset.name << "\",\n";
    json << "  \"description\": \"" << preset.description << "\",\n";
    json << "  \"author\": \"" << preset.author << "\",\n";
    json << "  \"created_date\": \"" << preset.created_date << "\",\n";
    json << "  \"version\": \"" << preset.version << "\",\n";
    
    // Topology
    json << "  \"topology\": {\n";
    json << "    \"type\": " << static_cast<int>(preset.topology.type) << ",\n";
    json << "    \"neuron_count\": " << preset.topology.neuron_count << ",\n";
    json << "    \"width\": " << preset.topology.width << ",\n";
    json << "    \"height\": " << preset.topology.height << "\n";
    json << "  },\n";
    
    // Connectivity
    json << "  \"connectivity\": {\n";
    json << "    \"bidirectional\": " << (preset.connectivity.bidirectional ? "true" : "false") << ",\n";
    json << "    \"synaptic_weight\": " << preset.connectivity.synaptic_weight << ",\n";
    json << "    \"synaptic_delay\": " << preset.connectivity.synaptic_delay << ",\n";
    json << "    \"adjacent_only\": " << (preset.connectivity.adjacent_only ? "true" : "false") << "\n";
    json << "  },\n";
    
    // Base parameters
    json << "  \"base_parameters\": {\n";
    json << "    \"C_m\": " << preset.base_parameters.C_m << ",\n";
    json << "    \"g_Na\": " << preset.base_parameters.g_Na << ",\n";
    json << "    \"g_K\": " << preset.base_parameters.g_K << ",\n";
    json << "    \"g_L\": " << preset.base_parameters.g_L << ",\n";
    json << "    \"E_Na\": " << preset.base_parameters.E_Na << ",\n";
    json << "    \"E_K\": " << preset.base_parameters.E_K << ",\n";
    json << "    \"E_L\": " << preset.base_parameters.E_L << "\n";
    json << "  },\n";
    
    // Simulation settings
    json << "  \"simulation_dt\": " << preset.simulation_dt << ",\n";
    json << "  \"recording_duration\": " << preset.recording_duration << ",\n";
    json << "  \"uniform_background_current\": " << preset.uniform_background_current << ",\n";
    json << "  \"use_individual_parameters\": " << (preset.use_individual_parameters ? "true" : "false") << "\n";
    
    json << "}";
    
    return json.str();
}

// Simple JSON deserialization (basic implementation)
std::shared_ptr<ExperimentalPreset> PresetManager::deserializePreset(const std::string& json_data) const {
    auto preset = std::make_shared<ExperimentalPreset>();
    
    // Simple regex-based parsing (in production, use proper JSON library)
    std::regex name_regex("\"name\"\\s*:\\s*\"([^\"]*)\"");
    std::regex desc_regex("\"description\"\\s*:\\s*\"([^\"]*)\"");
    std::regex author_regex("\"author\"\\s*:\\s*\"([^\"]*)\"");
    std::regex created_regex("\"created_date\"\\s*:\\s*\"([^\"]*)\"");
    
    std::smatch match;
    
    if (std::regex_search(json_data, match, name_regex)) {
        preset->name = match[1].str();
    }
    if (std::regex_search(json_data, match, desc_regex)) {
        preset->description = match[1].str();
    }
    if (std::regex_search(json_data, match, author_regex)) {
        preset->author = match[1].str();
    }
    if (std::regex_search(json_data, match, created_regex)) {
        preset->created_date = match[1].str();
    }
    
    // Parse numeric values
    std::regex topology_type_regex(R"("type":\s*(\d+))");
    std::regex neuron_count_regex(R"("neuron_count":\s*(\d+))");
    std::regex width_regex(R"("width":\s*(\d+))");
    std::regex height_regex(R"("height":\s*(\d+))");
    
    if (std::regex_search(json_data, match, topology_type_regex)) {
        preset->topology.type = static_cast<TopologyType>(std::stoi(match[1].str()));
    }
    if (std::regex_search(json_data, match, neuron_count_regex)) {
        preset->topology.neuron_count = std::stoi(match[1].str());
    }
    if (std::regex_search(json_data, match, width_regex)) {
        preset->topology.width = std::stoi(match[1].str());
    }
    if (std::regex_search(json_data, match, height_regex)) {
        preset->topology.height = std::stoi(match[1].str());
    }
    
    // Parse boolean values
    std::regex bidirectional_regex(R"("bidirectional":\s*(true|false))");
    if (std::regex_search(json_data, match, bidirectional_regex)) {
        preset->connectivity.bidirectional = (match[1].str() == "true");
    }
    
    // Parse double values for parameters
    std::regex c_m_regex(R"("C_m":\s*([\d\.]+))");
    std::regex g_na_regex(R"("g_Na":\s*([\d\.]+))");
    std::regex g_k_regex(R"("g_K":\s*([\d\.]+))");
    std::regex g_l_regex(R"("g_L":\s*([\d\.]+))");
    std::regex e_na_regex(R"("E_Na":\s*([\d\.-]+))");
    std::regex e_k_regex(R"("E_K":\s*([\d\.-]+))");
    std::regex e_l_regex(R"("E_L":\s*([\d\.-]+))");
    
    if (std::regex_search(json_data, match, c_m_regex)) {
        preset->base_parameters.C_m = std::stod(match[1].str());
    }
    if (std::regex_search(json_data, match, g_na_regex)) {
        preset->base_parameters.g_Na = std::stod(match[1].str());
    }
    if (std::regex_search(json_data, match, g_k_regex)) {
        preset->base_parameters.g_K = std::stod(match[1].str());
    }
    if (std::regex_search(json_data, match, g_l_regex)) {
        preset->base_parameters.g_L = std::stod(match[1].str());
    }
    if (std::regex_search(json_data, match, e_na_regex)) {
        preset->base_parameters.E_Na = std::stod(match[1].str());
    }
    if (std::regex_search(json_data, match, e_k_regex)) {
        preset->base_parameters.E_K = std::stod(match[1].str());
    }
    if (std::regex_search(json_data, match, e_l_regex)) {
        preset->base_parameters.E_L = std::stod(match[1].str());
    }
    
    return preset;
}

} // namespace consciousness
