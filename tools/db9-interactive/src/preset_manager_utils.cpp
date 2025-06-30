//
// Preset Manager Utilities
// Helper functions and validation for preset management
//

#include "preset_manager.hpp"
#include <chrono>
#include <filesystem>
#include <algorithm>
#include <regex>
#include <iomanip>
#include <sstream>
#include <iostream>

namespace consciousness {

// Utility methods for PresetManager

std::vector<std::string> PresetManager::validatePreset(const ExperimentalPreset& preset) const {
    std::vector<std::string> errors;
    
    // Check required fields
    if (preset.name.empty()) {
        errors.push_back("Preset name is required");
    }
    
    // Validate topology
    if (!preset.topology.isValid()) {
        errors.push_back("Invalid topology configuration");
    }
    
    // Validate parameters
    if (!PresetUtils::validateNeuronParameters(preset.base_parameters)) {
        errors.push_back("Invalid neuron parameters");
    }
    
    // Validate individual parameters if used
    if (preset.use_individual_parameters) {
        int expected_count = preset.topology.getTotalNeurons();
        if (preset.individual_parameters.size() != expected_count) {
            errors.push_back("Individual parameter count mismatch");
        }
        
        for (size_t i = 0; i < preset.individual_parameters.size(); ++i) {
            if (!PresetUtils::validateNeuronParameters(preset.individual_parameters[i])) {
                errors.push_back("Invalid individual parameters for neuron " + std::to_string(i));
            }
        }
    }
    
    // Validate simulation settings
    if (preset.simulation_dt <= 0.0 || preset.simulation_dt > 1.0) {
        errors.push_back("Invalid simulation time step");
    }
    
    if (preset.recording_duration <= 0.0) {
        errors.push_back("Invalid recording duration");
    }
    
    return errors;
}

bool PresetManager::isValidPreset(const ExperimentalPreset& preset) const {
    auto errors = validatePreset(preset);
    return errors.empty();
}

std::string PresetManager::generatePresetFilename(const std::string& preset_name) const {
    return sanitizeFilename(preset_name) + ".json";
}

std::string PresetManager::sanitizeFilename(const std::string& name) const {
    std::string sanitized = name;
    
    // Replace invalid characters with underscores
    std::regex invalid_chars("[^a-zA-Z0-9._-]");
    sanitized = std::regex_replace(sanitized, invalid_chars, "_");
    
    // Remove multiple consecutive underscores
    std::regex multiple_underscores("_{2,}");
    sanitized = std::regex_replace(sanitized, multiple_underscores, "_");
    
    // Trim leading/trailing underscores
    sanitized = std::regex_replace(sanitized, std::regex("^_+|_+$"), "");
    
    // Ensure not empty
    if (sanitized.empty()) {
        sanitized = "preset";
    }
    
    return sanitized;
}

bool PresetManager::createPresetDirectory() const {
    try {
        std::filesystem::create_directories(preset_directory_);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create preset directory: " << e.what() << std::endl;
        return false;
    }
}

void PresetManager::refreshPresetLibrary() {
    preset_library_.clear();
    
    // Add built-in presets
    for (const auto& [id, preset] : builtin_presets_) {
        PresetMetadata meta;
        meta.id = id;
        meta.name = preset.name;
        meta.description = preset.description;
        meta.category = PresetCategory::BASIC_EXAMPLES; // Default for built-ins
        meta.author = preset.author;
        meta.created_date = preset.created_date;
        meta.neuron_count = preset.topology.getTotalNeurons();
        meta.topology_type = (preset.topology.type == TopologyType::RING) ? "Ring" : 
                           (preset.topology.type == TopologyType::GRID) ? "Grid" : "Torus";
        meta.complexity_rating = 2.0; // Default complexity
        
        preset_library_.push_back(meta);
    }
    
    // Scan filesystem for user presets
    if (!std::filesystem::exists(preset_directory_)) {
        return;
    }
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(preset_directory_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                auto preset = loadPreset(entry.path().filename().string());
                if (preset) {
                    PresetMetadata meta;
                    meta.id = entry.path().stem().string();
                    meta.name = preset->name;
                    meta.description = preset->description;
                    meta.category = PresetCategory::CUSTOM_USER;
                    meta.author = preset->author;
                    meta.created_date = preset->created_date;
                    meta.neuron_count = preset->topology.getTotalNeurons();
                    meta.topology_type = (preset->topology.type == TopologyType::RING) ? "Ring" : 
                                       (preset->topology.type == TopologyType::GRID) ? "Grid" : "Torus";
                    meta.complexity_rating = 3.0; // User presets default to medium complexity
                    
                    preset_library_.push_back(meta);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning preset directory: " << e.what() << std::endl;
    }
}

// Additional search and library management methods
std::vector<PresetMetadata> PresetManager::getAvailablePresets() const {
    return preset_library_;
}

std::vector<PresetMetadata> PresetManager::getPresetsByCategory(PresetCategory category) const {
    std::vector<PresetMetadata> filtered;
    std::copy_if(preset_library_.begin(), preset_library_.end(), 
                 std::back_inserter(filtered),
                 [category](const PresetMetadata& meta) {
                     return meta.category == category;
                 });
    return filtered;
}

std::vector<PresetMetadata> PresetManager::searchPresets(const std::string& search_term) const {
    std::vector<PresetMetadata> results;
    std::string lower_search = search_term;
    std::transform(lower_search.begin(), lower_search.end(), lower_search.begin(), ::tolower);
    
    for (const auto& meta : preset_library_) {
        std::string lower_name = meta.name;
        std::string lower_desc = meta.description;
        std::string lower_tags = meta.tags;
        
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        std::transform(lower_desc.begin(), lower_desc.end(), lower_desc.begin(), ::tolower);
        std::transform(lower_tags.begin(), lower_tags.end(), lower_tags.begin(), ::tolower);
        
        if (lower_name.find(lower_search) != std::string::npos ||
            lower_desc.find(lower_search) != std::string::npos ||
            lower_tags.find(lower_search) != std::string::npos) {
            results.push_back(meta);
        }
    }
    
    return results;
}

std::shared_ptr<ExperimentalPreset> PresetManager::loadPresetById(const std::string& preset_id) {
    // Check built-in presets first
    auto builtin_it = builtin_presets_.find(preset_id);
    if (builtin_it != builtin_presets_.end()) {
        return std::make_shared<ExperimentalPreset>(builtin_it->second);
    }
    
    // Search in library
    for (const auto& metadata : preset_library_) {
        if (metadata.id == preset_id) {
            std::string filename = metadata.id + ".json";
            return loadPreset(filename);
        }
    }
    
    std::cerr << "Preset not found: " << preset_id << std::endl;
    return nullptr;
}

ExperimentalPreset PresetManager::createPresetFromNetwork(const NeuronNetwork& network,
                                                        const std::string& name,
                                                        const std::string& description) const {
    return ExperimentalPreset::fromNetwork(network, name, description);
}

void PresetManager::setPresetDirectory(const std::string& directory) {
    preset_directory_ = directory;
    if (!preset_directory_.empty() && preset_directory_.back() != '/') {
        preset_directory_ += '/';
    }
    createPresetDirectory();
    refreshPresetLibrary();
}

const std::string& PresetManager::getPresetDirectory() const {
    return preset_directory_;
}

// PresetUtils Implementation

namespace PresetUtils {

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string createBackupFilename(const std::string& original) {
    std::filesystem::path p(original);
    std::string stem = p.stem().string();
    std::string ext = p.extension().string();
    std::string dir = p.parent_path().string();
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream backup_name;
    backup_name << stem << "_backup_" 
                << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S")
                << ext;
    
    if (!dir.empty()) {
        return dir + "/" + backup_name.str();
    }
    return backup_name.str();
}

bool validateNeuronParameters(const HHNeuron::Parameters& params) {
    // Check for reasonable parameter ranges
    if (params.C_m <= 0.0 || params.C_m > 10.0) return false;
    if (params.g_Na <= 0.0 || params.g_Na > 1000.0) return false;
    if (params.g_K <= 0.0 || params.g_K > 1000.0) return false;
    if (params.g_L <= 0.0 || params.g_L > 10.0) return false;
    
    // Check reversal potentials
    if (params.E_Na < -100.0 || params.E_Na > 100.0) return false;
    if (params.E_K < -150.0 || params.E_K > 0.0) return false;
    if (params.E_L < -100.0 || params.E_L > 0.0) return false;
    
    return true;
}

bool validateTopologyConfig(const TopologyConfig& config) {
    return config.isValid();
}

std::string categoryToString(PresetCategory category) {
    switch (category) {
        case PresetCategory::BASIC_EXAMPLES: return "Basic Examples";
        case PresetCategory::CONSCIOUSNESS_MODELS: return "Consciousness Models";
        case PresetCategory::MOTOR_PATTERNS: return "Motor Patterns";
        case PresetCategory::WAVE_DYNAMICS: return "Wave Dynamics";
        case PresetCategory::PATHOLOGICAL_MODELS: return "Pathological Models";
        case PresetCategory::CUSTOM_USER: return "Custom User";
        case PresetCategory::RESEARCH_PROTOCOLS: return "Research Protocols";
        default: return "Unknown";
    }
}

PresetCategory stringToCategory(const std::string& category_str) {
    if (category_str == "Basic Examples") return PresetCategory::BASIC_EXAMPLES;
    if (category_str == "Consciousness Models") return PresetCategory::CONSCIOUSNESS_MODELS;
    if (category_str == "Motor Patterns") return PresetCategory::MOTOR_PATTERNS;
    if (category_str == "Wave Dynamics") return PresetCategory::WAVE_DYNAMICS;
    if (category_str == "Pathological Models") return PresetCategory::PATHOLOGICAL_MODELS;
    if (category_str == "Custom User") return PresetCategory::CUSTOM_USER;
    if (category_str == "Research Protocols") return PresetCategory::RESEARCH_PROTOCOLS;
    return PresetCategory::CUSTOM_USER;
}

std::string generatePresetId(const std::string& name) {
    std::string id = name;
    std::transform(id.begin(), id.end(), id.begin(), ::tolower);
    std::replace(id.begin(), id.end(), ' ', '_');
    std::regex invalid_chars("[^a-z0-9_]");
    id = std::regex_replace(id, invalid_chars, "");
    return id;
}

} // namespace PresetUtils

} // namespace consciousness
