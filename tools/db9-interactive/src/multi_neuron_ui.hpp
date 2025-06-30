//
// Multi-Neuron UI - Collective Consciousness Interface Layer (C++ with Dear ImGui)
// 
// This extends the triadic architecture to network-level visualization:
// - C++ ImGui interface for multiple neurons
// - Collective parameter control and individual focus
// - Network topology visualization and analysis
// - Motor behavior observation and pattern detection
//

#pragma once

#include "neuron_network.hpp"
#include "neuron_viz_data.h"
#include <memory>
#include <set>

// Forward declare ImGui types
typedef unsigned int ImU32;
struct ImVec2;

namespace consciousness {

/// Visualization tab selection
enum class VisualizationTab {
    MEMBRANE_DYNAMICS,
    TOPOLOGICAL_VIEW
};

/// Multi-neuron UI State and Configuration
struct MultiNeuronUIState {
    // Simulation control
    bool running = false;
    bool show_demo = false;
    int sim_steps_per_update = 10;  // Default to 10x speed for better visualization
    
    // Panel visibility
    bool show_network_config = true;
    bool show_parameters = true;
    bool show_stimulus = true;
    bool show_analysis = true;
    
    // Visualization settings
    VisualizationTab active_tab = VisualizationTab::MEMBRANE_DYNAMICS;
    float time_window_ms = 200.0f;
    bool show_gating_variables = true;
    bool show_currents = false;
    bool auto_scale = true;
    
    // Parameter control mode
    ParameterApplyMode parameter_mode = ParameterApplyMode::ALL_NEURONS;
    int focus_neuron_id = 0;
    std::set<int> selected_neurons;
    
    // Network configuration
    TopologyType topology_type = TopologyType::RING;
    int ring_neuron_count = 20;
    int grid_width = 5;
    int grid_height = 5;
    bool bidirectional_connections = true;
    float synaptic_weight = 1.0f;
    
    // Stimulus control
    NetworkStimulus::Pattern stimulus_pattern = NetworkStimulus::SINGLE_SITE;
    float stimulus_current = 10.0f;
    float stimulus_duration = 5.0f;
    float wave_velocity = 5.0f;
    std::vector<int> stimulus_target_neurons;
    
    // Analysis settings
    float analysis_window_ms = 1000.0f;
    bool show_wave_detection = true;
    bool show_phase_analysis = true;
    
    // Visualization filtering
    int primary_focus_neuron = 0;
    std::set<int> secondary_monitor_neurons;
    bool show_background_traces = true;
    float background_opacity = 0.2f;
    
    // Export settings
    bool export_on_stop = false;
    char export_filename[256] = "network_data.json";
};

/// Network topology layout for visualization  
struct TopologyLayout {
    std::vector<NeuronPosition> neuron_positions;
    std::vector<std::pair<int, int>> connection_lines;  // From/to neuron indices
    float display_scale = 1.0f;
    float neuron_radius = 8.0f;
    float connection_thickness = 2.0f;
};

/// Multi-Neuron UI Manager - Network Interface Layer
class MultiNeuronUI {
public:
    explicit MultiNeuronUI(std::shared_ptr<NeuronNetwork> network);
    ~MultiNeuronUI();
    
    /// Render the complete UI (call each frame)
    void render();
    
    /// Update simulation state
    void update(float dt);
    
    /// Get UI state
    const MultiNeuronUIState& getState() const { return state_; }
    
    /// Reset network and UI state
    void reset();
    
    /// Export network data
    void exportData() const;
    
    /// Get visualization data for bridge functions
    NetworkVisualizationData getNetworkVisualizationData() const;

private:
    std::shared_ptr<NeuronNetwork> network_;
    MultiNeuronUIState state_;
    TopologyLayout topology_layout_;
    
    // UI parameter buffers (for ImGui sliders)
    float ui_C_m_;
    float ui_g_Na_;
    float ui_g_K_;
    float ui_g_L_;
    float ui_E_Na_;
    float ui_E_K_;
    float ui_E_L_;
    float ui_background_current_;
    
    // Analysis data cache
    float network_firing_rate_;
    float synchronization_index_;
    float phase_coherence_;
    NetworkWaveProperties current_wave_properties_;
    NetworkMotorPattern current_motor_pattern_;
    
    // UI Layout - Left Column Panels
    void renderSimulationControlPanel();
    void renderNetworkConfigurationPanel();
    void renderParameterPanel();
    void renderStimulusControlPanel();
    void renderCurrentStatePanel();
    void renderAnalysisPanel();
    
    // UI Layout - Right Column Visualization Tabs
    void renderVisualizationTabs();
    void renderMembraneDynamicsTab();
    void renderTopologicalViewTab();
    
    // Visualization helpers
    void renderMultiNeuronTraces();
    void renderNetworkTopology();
    void renderNeuronNodes();
    void renderConnectionLines();
    void renderActivityAnimations();
    void renderWaveVisualization();
    
    // Parameter management
    void syncParametersFromNetwork();
    void applyParametersToNetwork();
    void applySpatialParameterPattern();
    
    // Network configuration
    void applyTopologyConfiguration();
    void updateTopologyLayout();
    
    // Stimulus control
    void applyNetworkStimulus();
    void selectStimulusTargets();
    
    // Selection and interaction
    void updateNeuronSelection();
    void handleTopologyInteraction();
    
    // Analysis updates
    void updateAnalysisData();
    void detectMotorBehavior();
    
    // Visualization data management
    void updateVisualizationData();
    
    // UI helpers
    bool renderParameterSlider(const char* label, float* value, float min_val, float max_val, const char* format = "%.2f");
    void renderParameterApplyModeSelector();
    
    // Color and style helpers
    ImU32 getNeuronColor(int neuron_id, float activity_level) const;
    ImU32 getConnectionColor(const SynapticConnection& connection) const;
    ImU32 getVoltageTraceColor(int neuron_id) const;
    
    // Coordinate transformations
    ImVec2 networkToScreenCoords(float x, float y, const ImVec2& canvas_pos, const ImVec2& canvas_size) const;
    std::pair<float, float> screenToNetworkCoords(const ImVec2& screen_pos, const ImVec2& canvas_pos, const ImVec2& canvas_size) const;
    
    // Motor behavior visualization
    void renderWaveProperties(const NetworkWaveProperties& wave);
    void renderMotorPattern(const NetworkMotorPattern& pattern);
    void renderSynchronizationMeter(float sync_index);
};

// Bridge functions for visualization data (C++ linkage)
NetworkVisualizationData get_network_viz_data(void);
void set_network_viz_data(const NetworkVisualizationData* data);

// Update visualization data bridge (called from UI layer)
void updateNetworkVisualizationData(const NeuronNetwork& network, const MultiNeuronUIState& ui_state);

} // namespace consciousness
