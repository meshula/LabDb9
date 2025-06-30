//
// Preset Manager - Parameter Persistence System for HH Multi-Neuron Simulator
//
// This enables saving/loading of complete experimental configurations:
// - Individual neuron parameters and network-wide settings
// - Topology configurations and connectivity patterns  
// - Stimulus protocols and spatial patterns
// - Analysis settings and visualization preferences
//

#pragma once

#include "hh_neuron.hpp"
#include "neuron_network.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace consciousness {

/// Complete experimental configuration preset
struct ExperimentalPreset {
    // Metadata
    std::string name;
    std::string description;
    std::string author;
    std::string created_date;
    std::string last_modified;
    std::string version = "1.0";
    
    // Network configuration
    TopologyConfig topology;
    ConnectivityConfig connectivity;
    
    // Neuron parameters
    HHNeuron::Parameters base_parameters;
    std::vector<HHNeuron::Parameters> individual_parameters; ///< If per-neuron customization
    bool use_individual_parameters = false;
    
    // Spatial patterns
    std::vector<SpatialPattern> spatial_patterns;
    int active_spatial_pattern = -1; ///< -1 means no pattern active
    
    // Background currents
    double uniform_background_current = 0.0;
    std::vector<double> individual_background_currents;
    bool use_individual_background = false;
    
    // Stimulus protocols
    std::vector<NetworkStimulus> stimulus_protocols;
    
    // Simulation settings
    double simulation_dt = 0.01;
    double recording_duration = 1000.0; ///< Default recording time (ms)
    
    // Analysis settings
    double analysis_window_ms = 500.0;
    bool enable_spike_detection = true;
    bool enable_phase_analysis = true;
    bool enable_wave_detection = true;
    
    // Visualization settings
    struct VisualizationSettings {
        float time_window_ms = 200.0f;
        bool show_gating_variables = true;
        bool show_currents = false;
        bool auto_scale = true;
        bool show_topology = true;
        bool show_connections = true;
        std::string color_scheme = "default";
    } visualization;
    
    // Constructor
    ExperimentalPreset() = default;
    
    /// Create preset from current network state
    static ExperimentalPreset fromNetwork(const NeuronNetwork& network, 
                                        const std::string& name = "Untitled",
                                        const std::string& description = "");
};

/// Predefined preset categories for easy organization
enum class PresetCategory {
    BASIC_EXAMPLES,         ///< Simple demonstrations and tutorials
    CONSCIOUSNESS_MODELS,   ///< Consciousness framework experiments
    MOTOR_PATTERNS,         ///< Motor behavior and pattern studies
    WAVE_DYNAMICS,          ///< Traveling wave and synchronization
    PATHOLOGICAL_MODELS,    ///< Disease and dysfunction models
    CUSTOM_USER,            ///< User-created presets
    RESEARCH_PROTOCOLS      ///< Published research reproductions
};

/// Preset metadata for browsing and organization
struct PresetMetadata {
    std::string id;
    std::string name;
    std::string description;
    PresetCategory category;
    std::string author;
    std::string created_date;
    std::string tags;
    int neuron_count;
    std::string topology_type;
    double complexity_rating; ///< 1-5 complexity for user guidance
    
    PresetMetadata() = default;
};

/// Main preset management class
class PresetManager {
public:
    /// Constructor
    explicit PresetManager();
    
    /// Destructor
    ~PresetManager();
    
    // Preset loading/saving
    
    /// Save preset to file
    bool savePreset(const ExperimentalPreset& preset, const std::string& filename = "");
    
    /// Load preset from file
    std::shared_ptr<ExperimentalPreset> loadPreset(const std::string& filename);
    
    /// Load preset by ID from preset library
    std::shared_ptr<ExperimentalPreset> loadPresetById(const std::string& preset_id);
    
    /// Delete preset file
    bool deletePreset(const std::string& filename);
    
    // Preset library management
    
    /// Get all available preset metadata
    std::vector<PresetMetadata> getAvailablePresets() const;
    
    /// Get presets by category
    std::vector<PresetMetadata> getPresetsByCategory(PresetCategory category) const;
    
    /// Search presets by name or tags
    std::vector<PresetMetadata> searchPresets(const std::string& search_term) const;
    
    /// Refresh preset library (scan directory)
    void refreshPresetLibrary();
    
    // Network application
    
    /// Apply preset to network
    bool applyPresetToNetwork(const ExperimentalPreset& preset, NeuronNetwork& network);
    
    /// Create preset from current network state
    ExperimentalPreset createPresetFromNetwork(const NeuronNetwork& network,
                                             const std::string& name,
                                             const std::string& description = "") const;
    
    // Built-in presets
    
    /// Initialize built-in preset library
    void initializeBuiltinPresets();
    
    /// Get basic demonstration presets
    std::vector<ExperimentalPreset> getBasicPresets() const;
    
    /// Get consciousness model presets
    std::vector<ExperimentalPreset> getConsciousnessPresets() const;
    
    /// Get motor pattern presets
    std::vector<ExperimentalPreset> getMotorPatternPresets() const;
    
    // Configuration
    
    /// Set preset directory
    void setPresetDirectory(const std::string& directory);
    
    /// Get current preset directory
    const std::string& getPresetDirectory() const;
    
    /// Set auto-backup on apply
    void setAutoBackup(bool enabled) { auto_backup_enabled_ = enabled; }
    
    /// Get validation errors for preset
    std::vector<std::string> validatePreset(const ExperimentalPreset& preset) const;
    
private:
    // Internal state
    std::string preset_directory_;
    std::vector<PresetMetadata> preset_library_;
    bool auto_backup_enabled_;
    
    // Built-in presets storage
    std::map<std::string, ExperimentalPreset> builtin_presets_;
    
    // File I/O helpers
    std::string generatePresetFilename(const std::string& preset_name) const;
    std::string sanitizeFilename(const std::string& name) const;
    bool createPresetDirectory() const;
    
    // JSON serialization
    std::string serializePreset(const ExperimentalPreset& preset) const;
    std::shared_ptr<ExperimentalPreset> deserializePreset(const std::string& json_data) const;
    
    // Preset validation
    bool isValidPreset(const ExperimentalPreset& preset) const;
    
    // Built-in preset creators
    ExperimentalPreset createBasicRingPreset() const;
    ExperimentalPreset createTravelingWavePreset() const;
    ExperimentalPreset createSynchronizationPreset() const;
    ExperimentalPreset createMotorOscillationPreset() const;
    ExperimentalPreset createConsciousnessRingPreset() const;
    ExperimentalPreset createPathologicalSeizurePreset() const;
};

/// Utility functions for preset manipulation
namespace PresetUtils {
    /// Generate timestamp string
    std::string getCurrentTimestamp();
    
    /// Create backup filename
    std::string createBackupFilename(const std::string& original);
    
    /// Convert category to string
    std::string categoryToString(PresetCategory category);
    
    /// Convert string to category
    PresetCategory stringToCategory(const std::string& category_str);
    
    /// Generate unique preset ID
    std::string generatePresetId(const std::string& name);
    
    /// Validate neuron parameters
    bool validateNeuronParameters(const HHNeuron::Parameters& params);
    
    /// Validate topology configuration
    bool validateTopologyConfig(const TopologyConfig& config);
}

} // namespace consciousness
