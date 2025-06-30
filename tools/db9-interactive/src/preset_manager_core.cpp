//
// Preset Manager Core - Parameter Persistence System
// Core functionality for loading/saving presets
//

#include "preset_manager.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>

namespace consciousness {

// ExperimentalPreset Implementation

ExperimentalPreset ExperimentalPreset::fromNetwork(const NeuronNetwork& network, 
                                                  const std::string& name,
                                                  const std::string& description) {
    ExperimentalPreset preset;
    
    // Metadata
    preset.name = name;
    preset.description = description;
    preset.author = "HH-Simulator-User";
    preset.created_date = PresetUtils::getCurrentTimestamp();
    preset.last_modified = preset.created_date;
    
    // Network configuration
    preset.topology = network.getTopologyConfig();
    preset.connectivity = network.getConnectivityConfig();
    
    // Get base parameters from first neuron (assuming homogeneous network)
    if (network.getNeuronCount() > 0) {
        preset.base_parameters = network.getNeuron(0).getParameters();
    }
    
    // For now, assume uniform parameters
    preset.use_individual_parameters = false;
    preset.uniform_background_current = 0.0;
    preset.use_individual_background = false;
    
    // Default simulation settings
    preset.simulation_dt = 0.01;
    preset.recording_duration = 1000.0;
    
    return preset;
}

// PresetManager Implementation

PresetManager::PresetManager() 
    : preset_directory_("presets/")
    , auto_backup_enabled_(true) 
{
    createPresetDirectory();
    initializeBuiltinPresets();
    refreshPresetLibrary();
}

PresetManager::~PresetManager() = default;

bool PresetManager::savePreset(const ExperimentalPreset& preset, const std::string& filename) {
    if (!isValidPreset(preset)) {
        std::cerr << "Preset validation failed for: " << preset.name << std::endl;
        return false;
    }
    
    std::string file_path = filename.empty() ? 
        preset_directory_ + generatePresetFilename(preset.name) :
        preset_directory_ + filename;
    
    try {
        std::string json_data = serializePreset(preset);
        std::ofstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << file_path << std::endl;
            return false;
        }
        
        file << json_data;
        file.close();
        
        std::cout << "Preset saved: " << file_path << std::endl;
        refreshPresetLibrary();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving preset: " << e.what() << std::endl;
        return false;
    }
}

std::shared_ptr<ExperimentalPreset> PresetManager::loadPreset(const std::string& filename) {
    std::string file_path = preset_directory_ + filename;
    
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "Failed to open preset file: " << file_path << std::endl;
            return nullptr;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        
        auto preset = deserializePreset(buffer.str());
        if (preset && isValidPreset(*preset)) {
            std::cout << "Preset loaded: " << preset->name << std::endl;
            return preset;
        } else {
            std::cerr << "Invalid preset data in file: " << file_path << std::endl;
            return nullptr;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading preset: " << e.what() << std::endl;
        return nullptr;
    }
}

bool PresetManager::applyPresetToNetwork(const ExperimentalPreset& preset, NeuronNetwork& network) {
    if (!isValidPreset(preset)) {
        std::cerr << "Cannot apply invalid preset: " << preset.name << std::endl;
        return false;
    }
    
    try {
        bool topology_success = network.initializeTopology(preset.topology, preset.connectivity);
        if (!topology_success) {
            std::cerr << "Failed to apply topology configuration" << std::endl;
            return false;
        }
        
        // Apply parameters
        if (preset.use_individual_parameters && 
            preset.individual_parameters.size() == network.getNeuronCount()) {
            for (size_t i = 0; i < network.getNeuronCount(); ++i) {
                network.getNeuron(i).setParameters(preset.individual_parameters[i]);
            }
        } else {
            network.setAllParameters(preset.base_parameters);
        }
        
        network.setAllBackgroundCurrent(preset.uniform_background_current);
        
        // Apply spatial patterns if any
        if (preset.active_spatial_pattern >= 0 && 
            preset.active_spatial_pattern < static_cast<int>(preset.spatial_patterns.size())) {
            network.applySpatialPattern(preset.spatial_patterns[preset.active_spatial_pattern]);
        }
        
        network.reset();
        std::cout << "Preset applied successfully: " << preset.name << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error applying preset: " << e.what() << std::endl;
        return false;
    }
}

} // namespace consciousness
