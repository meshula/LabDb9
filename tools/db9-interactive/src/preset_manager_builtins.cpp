//
// Preset Manager Built-in Presets
// Contains built-in preset definitions for common use cases
//

#include "preset_manager.hpp"

namespace consciousness {

void PresetManager::initializeBuiltinPresets() {
    builtin_presets_["basic_ring"] = createBasicRingPreset();
    builtin_presets_["traveling_wave"] = createTravelingWavePreset();
    builtin_presets_["synchronization"] = createSynchronizationPreset();
    builtin_presets_["motor_oscillation"] = createMotorOscillationPreset();
    builtin_presets_["consciousness_ring"] = createConsciousnessRingPreset();
}

ExperimentalPreset PresetManager::createBasicRingPreset() const {
    ExperimentalPreset preset;
    
    preset.name = "Basic Ring Network";
    preset.description = "Simple 10-neuron ring with standard HH parameters for basic demonstrations";
    preset.author = "HH-Simulator";
    preset.created_date = PresetUtils::getCurrentTimestamp();
    preset.version = "1.0";
    
    // Topology: Small ring
    preset.topology.type = TopologyType::RING;
    preset.topology.neuron_count = 10;
    
    // Connectivity: Unidirectional, light coupling
    preset.connectivity.bidirectional = false;
    preset.connectivity.synaptic_weight = 0.5;
    preset.connectivity.synaptic_delay = 0.5;
    preset.connectivity.adjacent_only = true;
    
    // Standard HH parameters
    preset.base_parameters = HHNeuron::Parameters();
    
    // Simulation settings
    preset.simulation_dt = 0.01;
    preset.recording_duration = 500.0;
    preset.uniform_background_current = 0.0;
    
    return preset;
}

ExperimentalPreset PresetManager::createTravelingWavePreset() const {
    ExperimentalPreset preset;
    
    preset.name = "Traveling Wave Ring";
    preset.description = "20-neuron ring optimized for traveling wave propagation with stronger coupling";
    preset.author = "HH-Simulator";
    preset.created_date = PresetUtils::getCurrentTimestamp();
    preset.version = "1.0";
    
    // Topology: Larger ring for wave dynamics
    preset.topology.type = TopologyType::RING;
    preset.topology.neuron_count = 20;
    
    // Connectivity: Unidirectional with stronger coupling
    preset.connectivity.bidirectional = false;
    preset.connectivity.synaptic_weight = 1.5;
    preset.connectivity.synaptic_delay = 0.5;
    preset.connectivity.adjacent_only = true;
    
    // Standard HH parameters with slight modification for excitability
    preset.base_parameters = HHNeuron::Parameters();
    preset.base_parameters.g_Na = 130.0; // Slightly higher sodium conductance
    
    // Add a network stimulus for wave initiation
    NetworkStimulus wave_stimulus;
    wave_stimulus.pattern = NetworkStimulus::SINGLE_SITE;
    wave_stimulus.target_neurons = {0}; // Stimulate first neuron
    wave_stimulus.current = 15.0;
    wave_stimulus.duration = 2.0;
    preset.stimulus_protocols.push_back(wave_stimulus);
    
    preset.simulation_dt = 0.01;
    preset.recording_duration = 1000.0;
    
    return preset;
}

ExperimentalPreset PresetManager::createSynchronizationPreset() const {
    ExperimentalPreset preset;
    
    preset.name = "Synchronization Study";
    preset.description = "Bidirectional ring network for studying synchronization phenomena";
    preset.author = "HH-Simulator";
    preset.created_date = PresetUtils::getCurrentTimestamp();
    preset.version = "1.0";
    
    // Topology: Medium ring
    preset.topology.type = TopologyType::RING;
    preset.topology.neuron_count = 15;
    
    // Connectivity: Bidirectional for synchronization
    preset.connectivity.bidirectional = true;
    preset.connectivity.synaptic_weight = 1.0;
    preset.connectivity.synaptic_delay = 0.5;
    preset.connectivity.adjacent_only = true;
    
    // Standard parameters
    preset.base_parameters = HHNeuron::Parameters();
    
    // Random initial stimulation to break symmetry
    NetworkStimulus sync_stimulus;
    sync_stimulus.pattern = NetworkStimulus::RANDOM;
    sync_stimulus.current = 10.0;
    sync_stimulus.duration = 1.0;
    preset.stimulus_protocols.push_back(sync_stimulus);
    
    preset.simulation_dt = 0.01;
    preset.recording_duration = 1500.0;
    
    return preset;
}

ExperimentalPreset PresetManager::createMotorOscillationPreset() const {
    ExperimentalPreset preset;
    
    preset.name = "Motor Oscillation Pattern";
    preset.description = "Grid network configured for motor-like oscillatory patterns";
    preset.author = "HH-Simulator";
    preset.created_date = PresetUtils::getCurrentTimestamp();
    preset.version = "1.0";
    
    // Topology: Small grid
    preset.topology.type = TopologyType::GRID;
    preset.topology.width = 4;
    preset.topology.height = 4;
    
    // Connectivity: Bidirectional grid
    preset.connectivity.bidirectional = true;
    preset.connectivity.synaptic_weight = 0.8;
    preset.connectivity.synaptic_delay = 1.0; // Slightly longer delay
    preset.connectivity.adjacent_only = true;
    
    // Modified parameters for oscillations
    preset.base_parameters = HHNeuron::Parameters();
    preset.base_parameters.g_K = 30.0; // Reduced potassium for longer spikes
    
    // Background current for sustained activity
    preset.uniform_background_current = 5.0;
    
    preset.simulation_dt = 0.01;
    preset.recording_duration = 2000.0;
    
    return preset;
}

ExperimentalPreset PresetManager::createConsciousnessRingPreset() const {
    ExperimentalPreset preset;
    
    preset.name = "Consciousness Framework Ring";
    preset.description = "Large ring network for consciousness framework integration and vikalpa analysis";
    preset.author = "HH-Simulator";
    preset.created_date = PresetUtils::getCurrentTimestamp();
    preset.version = "1.0";
    
    // Topology: Large ring for complex dynamics
    preset.topology.type = TopologyType::RING;
    preset.topology.neuron_count = 50;
    
    // Connectivity: Bidirectional with moderate coupling
    preset.connectivity.bidirectional = true;
    preset.connectivity.synaptic_weight = 1.2;
    preset.connectivity.synaptic_delay = 0.8;
    preset.connectivity.adjacent_only = true;
    
    // Parameters tuned for rich dynamics
    preset.base_parameters = HHNeuron::Parameters();
    preset.base_parameters.g_Na = 125.0;
    preset.base_parameters.g_K = 35.0;
    
    // Spatial gradient for heterogeneity
    SpatialPattern gradient;
    gradient.type = SpatialPattern::GRADIENT;
    gradient.start_params = preset.base_parameters;
    gradient.end_params = preset.base_parameters;
    gradient.end_params.g_Na = 140.0; // Higher excitability at one end
    preset.spatial_patterns.push_back(gradient);
    preset.active_spatial_pattern = 0;
    
    preset.simulation_dt = 0.01;
    preset.recording_duration = 3000.0;
    preset.analysis_window_ms = 1000.0;
    preset.enable_phase_analysis = true;
    preset.enable_wave_detection = true;
    
    return preset;
}

} // namespace consciousness
