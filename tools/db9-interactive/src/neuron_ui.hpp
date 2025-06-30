//
// Neuron UI - Interface Layer (C++ with Dear ImGui)
// 
// This is the Interface Layer in the triadic architecture:
// - C++ ImGui interface management
// - Unified parameter control via ParameterUIController
// - State monitoring and analysis
//

#pragma once

#include "hh_neuron.hpp"
#include "neuron_viz_data.h"
#include "parameter_ui_controller.hpp"
#include <memory>

namespace consciousness {

/// UI State and Configuration
struct NeuronUIState {
    // Simulation control
    bool running = false;
    bool show_demo = false;
    bool show_analysis = true;
    bool show_parameters = true;
    bool show_stimulus = true;
    
    // Visualization settings
    float time_window_ms = 100.0f;
    bool show_gating_variables = true;
    bool show_currents = false;
    bool auto_scale = true;
    
    // Stimulus control
    float stimulus_current = 10.0f;
    float stimulus_duration = 2.0f;
    
    // Analysis window
    float analysis_window_ms = 1000.0f;
    
    // Export settings
    bool export_on_stop = false;
    char export_filename[256] = "neuron_data.json";
};

/// Neuron UI Manager - Interface Layer with DRY Parameter Management
class NeuronUI {
public:
    explicit NeuronUI(std::shared_ptr<HHNeuron> neuron);
    ~NeuronUI();
    
    /// Render the UI (call each frame)
    void render();
    
    /// Update simulation state
    void update(float dt);
    
    /// Get UI state
    const NeuronUIState& getState() const { return state_; }
    
    /// Reset neuron and UI state
    void reset();
    
    /// Export neuron data
    void exportData() const;

private:
    std::shared_ptr<HHNeuron> neuron_;
    NeuronUIState state_;
    
    // Unified parameter management (eliminates duplication)
    std::unique_ptr<ParameterUIController> parameter_controller_;
    
    // Analysis data
    float current_frequency_;
    float coherence_measure_;
    
    // UI panels
    void renderControlPanel();
    void renderStimulusPanel();
    void renderAnalysisPanel();
    void renderStatusPanel();
    
    // Helpers
    void updateAnalysisData();
    void syncParametersWithNeuron();
};

// C++ bridge functions (not in extern "C" block)
neuron_viz_data_t get_neuron_viz_data(void);
void set_neuron_viz_data(const neuron_viz_data_t* data);

} // namespace consciousness
