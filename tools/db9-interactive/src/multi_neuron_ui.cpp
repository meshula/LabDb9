//
// Multi-Neuron UI Implementation - Collective Consciousness Interface
//

#include "multi_neuron_ui.hpp"
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <iostream>

// Ensure M_PI is available
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// C++ Dear ImGui
#include "imgui.h"

namespace consciousness {

// Global visualization data bridge
namespace {
    NetworkVisualizationData g_network_viz_data;
}

// Constructor and basic setup
MultiNeuronUI::MultiNeuronUI(std::shared_ptr<NeuronNetwork> network)
    : network_(network)
    , network_firing_rate_(0.0f)
    , synchronization_index_(0.0f)
    , phase_coherence_(0.0f)
{
    syncParametersFromNetwork();
    updateTopologyLayout();
    
    // Initialize stimulus targets
    state_.stimulus_target_neurons.clear();
    state_.stimulus_target_neurons.push_back(0);  // Default to first neuron
}

MultiNeuronUI::~MultiNeuronUI() = default;

// Main render function - entry point for UI
void MultiNeuronUI::render() {
    updateAnalysisData();
    updateVisualizationData();
    
    // Create main docking space
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(viewport->Size, ImGuiCond_Always);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | 
                                   ImGuiWindowFlags_NoMove | 
                                   ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus;
    
    if (ImGui::Begin("HH Multi-Neuron Simulator", nullptr, window_flags)) {
        // Get available space
        ImVec2 content_region = ImGui::GetContentRegionAvail();
        
        // Define control panel width (make it a member variable if you want it adjustable)
        static float control_panel_width = 350.0f;
        
        // Left column - Control panels
        ImGui::BeginChild("ControlPanels", ImVec2(control_panel_width, content_region.y), true, 
                         ImGuiWindowFlags_HorizontalScrollbar);
        
        renderSimulationControlPanel();
        renderNetworkConfigurationPanel();
        renderParameterPanel();
        renderStimulusControlPanel();
        renderCurrentStatePanel();
        renderAnalysisPanel();
        
        ImGui::EndChild();
        
        // Splitter (optional - allows runtime resizing)
        ImGui::SameLine();
        ImGui::Button("##splitter", ImVec2(8.0f, content_region.y));
        if (ImGui::IsItemActive()) {
            float delta = ImGui::GetIO().MouseDelta.x;
            control_panel_width += delta;
            control_panel_width = std::max(200.0f, std::min(control_panel_width, content_region.x - 100.0f));
        }
        ImGui::SetItemTooltip("Drag to resize panels");
        
        // Right column - Visualization
        ImGui::SameLine();
        float viz_width = content_region.x - control_panel_width - 8.0f; // Account for splitter
        ImGui::BeginChild("Visualization", ImVec2(viz_width, content_region.y), true);
        
        renderVisualizationTabs();
        
        ImGui::EndChild();
    }
    ImGui::End();
    
    if (state_.show_demo) {
        ImGui::ShowDemoWindow(&state_.show_demo);
    }
}

// Update function for simulation
void MultiNeuronUI::update(float dt) {
    if (state_.running) {
        constexpr float SIMULATION_DT = 0.01f;
        
        // Run multiple simulation steps per UI frame for speed control
        for (int i = 0; i < state_.sim_steps_per_update; ++i) {
            network_->step(SIMULATION_DT);
        }
    }
}

// Placeholder implementations for now - will expand in chunks
void MultiNeuronUI::renderSimulationControlPanel() {
    if (ImGui::CollapsingHeader("Simulation Control", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* button_text = state_.running ? "Pause" : "Start";
        if (ImGui::Button(button_text, ImVec2(80, 0))) {
            state_.running = !state_.running;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Reset", ImVec2(60, 0))) {
            reset();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Step", ImVec2(50, 0))) {
            network_->step(0.01f);
        }
        
        ImGui::Separator();
        
        // Simulation speed control
        ImGui::Text("🎬 Simulation Speed:");
        if (ImGui::SliderInt("Steps/Frame", &state_.sim_steps_per_update, 1, 50, "%d")) {
            // Clamp to reasonable values
            state_.sim_steps_per_update = std::max(1, std::min(state_.sim_steps_per_update, 50));
        }
        
        // Speed presets
        ImGui::SameLine();
        if (ImGui::Button("Slow", ImVec2(40, 0))) {
            state_.sim_steps_per_update = 1;
        }
        ImGui::SameLine();
        if (ImGui::Button("Fast", ImVec2(40, 0))) {
            state_.sim_steps_per_update = 10;
        }
        ImGui::SameLine();
        if (ImGui::Button("Turbo", ImVec2(45, 0))) {
            state_.sim_steps_per_update = 25;
        }
        
        // Show effective time rate
        float effective_rate = state_.sim_steps_per_update * 0.01f * 60.0f; // ms per second at 60fps
        ImGui::Text("⏱️ Time Rate: %.1fx real-time", effective_rate);
        
        ImGui::Separator();
        
        ImGui::Text("Network: %zu neurons", network_->getNeuronCount());
        ImGui::Text("Time: %.2f ms", network_->getTime());
        ImGui::Text("Firing Rate: %.2f Hz", network_firing_rate_);
        
        ImGui::Separator();
        ImGui::Checkbox("Show ImGui Demo", &state_.show_demo);
    }
}

void MultiNeuronUI::renderNetworkConfigurationPanel() {
    if (ImGui::CollapsingHeader("Network Configuration", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool config_changed = false;
        
        // Topology selector
        const char* topology_items[] = { "Ring", "Grid", "Torus" };
        int current_topology = static_cast<int>(state_.topology_type);
        if (ImGui::Combo("Topology", &current_topology, topology_items, 3)) {
            state_.topology_type = static_cast<TopologyType>(current_topology);
            config_changed = true;
        }
        
        // Size controls based on topology
        switch (state_.topology_type) {
            case TopologyType::RING:
                config_changed |= ImGui::SliderInt("Neuron Count", &state_.ring_neuron_count, 3, 100);
                break;
            case TopologyType::GRID:
            case TopologyType::TORUS:
                config_changed |= ImGui::SliderInt("Width", &state_.grid_width, 2, 10);
                config_changed |= ImGui::SliderInt("Height", &state_.grid_height, 2, 10);
                ImGui::Text("Total: %d neurons", state_.grid_width * state_.grid_height);
                break;
        }
        
        ImGui::Separator();
        
        // Connectivity settings
        config_changed |= ImGui::Checkbox("Bidirectional", &state_.bidirectional_connections);
        config_changed |= ImGui::SliderFloat("Synaptic Weight", &state_.synaptic_weight, 0.0f, 5.0f, "%.2f");
        
        // Add note about directionality
        if (state_.bidirectional_connections) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "⚠ Bidirectional may cause standing waves");
        } else {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "→ Unidirectional for traveling waves");
        }
        
        if (config_changed) {
            applyTopologyConfiguration();
        }
        
        ImGui::Separator();
        
        // Quick presets
        ImGui::Text("Presets:");
        if (ImGui::Button("Motor Ring", ImVec2(80, 0))) {
            state_.topology_type = TopologyType::RING;
            state_.ring_neuron_count = 20;
            state_.bidirectional_connections = false;  // Unidirectional for clean waves
            state_.synaptic_weight = 1.0f;             // Reduced weight
            applyTopologyConfiguration();
        }
        ImGui::SameLine();
        if (ImGui::Button("Small Grid", ImVec2(80, 0))) {
            state_.topology_type = TopologyType::GRID;
            state_.grid_width = 5;
            state_.grid_height = 5;
            state_.bidirectional_connections = true;   // Grid can be bidirectional
            state_.synaptic_weight = 0.8f;             // Even smaller for grid
            applyTopologyConfiguration();
        }
    }
}

void MultiNeuronUI::renderParameterPanel() {
    if (ImGui::CollapsingHeader("HH Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderParameterApplyModeSelector();
        
        ImGui::Separator();
        
        bool params_changed = false;
        
        ImGui::Text("Membrane Capacitance:");
        params_changed |= ImGui::SliderFloat("C_m (μF/cm²)", &ui_C_m_, 0.1f, 5.0f, "%.2f");
        
        ImGui::Text("Conductances:");
        params_changed |= ImGui::SliderFloat("g_Na (mS/cm²)", &ui_g_Na_, 50.0f, 200.0f, "%.1f");
        params_changed |= ImGui::SliderFloat("g_K (mS/cm²)", &ui_g_K_, 10.0f, 80.0f, "%.1f");
        params_changed |= ImGui::SliderFloat("g_L (mS/cm²)", &ui_g_L_, 0.1f, 1.0f, "%.2f");
        
        ImGui::Text("Reversal Potentials:");
        params_changed |= ImGui::SliderFloat("E_Na (mV)", &ui_E_Na_, 30.0f, 70.0f, "%.1f");
        params_changed |= ImGui::SliderFloat("E_K (mV)", &ui_E_K_, -100.0f, -50.0f, "%.1f");
        params_changed |= ImGui::SliderFloat("E_L (mV)", &ui_E_L_, -70.0f, -40.0f, "%.1f");
        
        ImGui::Text("Background Current:");
        params_changed |= ImGui::SliderFloat("I_bg (μA/cm²)", &ui_background_current_, 0.0f, 20.0f, "%.1f");
        
        if (params_changed) {
            applyParametersToNetwork();
        }
        
        ImGui::Separator();
        
        if (ImGui::Button("Reset to Defaults")) {
            HHNeuron::Parameters defaults;
            ui_C_m_ = static_cast<float>(defaults.C_m);
            ui_g_Na_ = static_cast<float>(defaults.g_Na);
            ui_g_K_ = static_cast<float>(defaults.g_K);
            ui_g_L_ = static_cast<float>(defaults.g_L);
            ui_E_Na_ = static_cast<float>(defaults.E_Na);
            ui_E_K_ = static_cast<float>(defaults.E_K);
            ui_E_L_ = static_cast<float>(defaults.E_L);
            ui_background_current_ = 0.0f;
            applyParametersToNetwork();
        }
        
        if (ImGui::Button("Spatial Pattern...")) {
            applySpatialParameterPattern();
        }
    }
}

void MultiNeuronUI::renderStimulusControlPanel() {
    if (ImGui::CollapsingHeader("Stimulus Control", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Background Stimulus:");
        
        static float global_stimulus = 0.0f;
        if (ImGui::SliderFloat("Global I_stim (μA/cm²)", &global_stimulus, 0.0f, 20.0f, "%.1f")) {
            // Apply stimulus to all neurons
            for (size_t i = 0; i < network_->getNeuronCount(); ++i) {
                network_->getNeuron(static_cast<int>(i)).setBackgroundCurrent(global_stimulus);
            }
        }
        
        ImGui::Separator();
        
        ImGui::Text("Quick Stimulus Presets:");
        if (ImGui::Button("No Stimulus", ImVec2(80, 0))) {
            global_stimulus = 0.0f;
            for (size_t i = 0; i < network_->getNeuronCount(); ++i) {
                network_->getNeuron(static_cast<int>(i)).setBackgroundCurrent(0.0f);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Weak (3.2μA)", ImVec2(80, 0))) {
            global_stimulus = 3.2f;  // Optimal propagation threshold
            for (size_t i = 0; i < network_->getNeuronCount(); ++i) {
                network_->getNeuron(static_cast<int>(i)).setBackgroundCurrent(3.2f);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Strong (10μA)", ImVec2(85, 0))) {
            global_stimulus = 10.0f;
            for (size_t i = 0; i < network_->getNeuronCount(); ++i) {
                network_->getNeuron(static_cast<int>(i)).setBackgroundCurrent(10.0f);
            }
        }
        
        ImGui::Separator();
        
        // Single neuron stimulus for testing
        static int target_neuron = 0;
        static float single_stimulus = 0.0f;
        
        ImGui::Text("Single Neuron Stimulus:");
        ImGui::SliderInt("Target Neuron", &target_neuron, 0, static_cast<int>(network_->getNeuronCount()) - 1);
        
        if (ImGui::SliderFloat("Single I_stim (μA/cm²)", &single_stimulus, 0.0f, 20.0f, "%.1f")) {
            if (target_neuron >= 0 && target_neuron < static_cast<int>(network_->getNeuronCount())) {
                network_->getNeuron(target_neuron).setBackgroundCurrent(single_stimulus);
            }
        }
        
        if (ImGui::Button("Spike Neuron!", ImVec2(100, 0))) {
            if (target_neuron >= 0 && target_neuron < static_cast<int>(network_->getNeuronCount())) {
                // Temporary strong stimulus to force a spike
                network_->getNeuron(target_neuron).setBackgroundCurrent(15.0f);
            }
        }
        
        ImGui::Separator();
        ImGui::Text("Note: HH neurons need stimulus current to fire");
        ImGui::Text("Try 'Weak (3.2μA)' for optimal wave propagation!");
    }
}

void MultiNeuronUI::renderCurrentStatePanel() {
    if (ImGui::CollapsingHeader("Current State", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Motion: Real-time network dynamics
        ImGui::Text("🌊 Network Dynamics");
        ImGui::Separator();
        
        // Time and simulation info
        ImGui::Text("Simulation Time: %.2f ms", network_->getTime());
        ImGui::Text("Active Neurons: %zu", network_->getNeuronCount());
        
        // Network-level consciousness metrics
        ImGui::Spacing();
        ImGui::Text("🧠 Collective Consciousness");
        ImGui::Separator();
        
        // Firing rate with visual indicator
        ImGui::Text("Firing Rate: %.2f Hz", network_firing_rate_);
        ImGui::ProgressBar(network_firing_rate_ / 50.0f, ImVec2(-1, 0), "");
        
        // Synchronization index
        ImGui::Text("Synchronization: %.3f", synchronization_index_);
        ImGui::ProgressBar(synchronization_index_, ImVec2(-1, 0), "");
        
        // Phase coherence
        ImGui::Text("Phase Coherence: %.3f", phase_coherence_);
        ImGui::ProgressBar(phase_coherence_, ImVec2(-1, 0), "");
        
        ImGui::Spacing();
        ImGui::Text("🔬 Individual Neuron States");
        ImGui::Separator();
        
        // Memory: Sample neuron states (show first few neurons)
        size_t neurons_to_show = std::min(static_cast<size_t>(5), network_->getNeuronCount());
        for (size_t i = 0; i < neurons_to_show; ++i) {
            const auto& neuron = network_->getNeuron(static_cast<int>(i));
            const auto& state = neuron.getState();
            
            ImGui::Text("Neuron %zu:", i);
            ImGui::SameLine(80);
            ImGui::Text("V=%.1f mV", state.V);
            ImGui::SameLine(160);
            
            // Phase indicator
            const char* phase_str = [&]() {
                auto phase = neuron.getCurrentPhase();
                switch (phase) {
                    case consciousness::HHNeuron::Phase::Resting: return "Rest";
                    case consciousness::HHNeuron::Phase::Threshold: return "Thresh";
                    case consciousness::HHNeuron::Phase::Depolarization: return "Depol";
                    case consciousness::HHNeuron::Phase::Overshoot: return "Spike";
                    case consciousness::HHNeuron::Phase::Repolarization: return "Repol";
                    case consciousness::HHNeuron::Phase::Hyperpolarization: return "Hyper";
                    default: return "Unknown";
                }
            }();
            ImGui::Text("%s", phase_str);
            
            // Spike detection indicator
            if (neuron.detectSpike()) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "●");
            }
        }
        
        if (network_->getNeuronCount() > neurons_to_show) {
            ImGui::Text("... and %zu more neurons", network_->getNeuronCount() - neurons_to_show);
        }
        
        // Field: Network topology information
        ImGui::Spacing();
        ImGui::Text("🕸️ Network Field");
        ImGui::Separator();
        
        const auto& config = network_->getTopologyConfig();
        const char* topology_name = [&]() {
            switch (config.type) {
                case consciousness::TopologyType::RING: return "Ring";
                case consciousness::TopologyType::GRID: return "Grid";
                case consciousness::TopologyType::TORUS: return "Torus";
                default: return "Unknown";
            }
        }();
        ImGui::Text("Topology: %s", topology_name);
        
        if (config.type == consciousness::TopologyType::RING) {
            ImGui::Text("Ring Size: %d neurons", config.neuron_count);
        } else {
            ImGui::Text("Grid Size: %d × %d", config.width, config.height);
        }
        
        ImGui::Text("Connections: %zu", network_->getConnections().size());
        
        // Wave detection status
        if (current_wave_properties_.detected) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "🌊 Wave Detected!");
            ImGui::Text("Velocity: %.2f neurons/ms", current_wave_properties_.velocity);
            ImGui::Text("Direction: %s", current_wave_properties_.direction > 0 ? "Forward" : "Backward");
            ImGui::Text("Coherence: %.3f", current_wave_properties_.coherence);
        } else {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No traveling wave detected");
        }
    }
}

void MultiNeuronUI::renderAnalysisPanel() {
    if (ImGui::CollapsingHeader("Network Analysis", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Analysis panel will be implemented here");
    }
}

void MultiNeuronUI::renderVisualizationTabs() {
    if (ImGui::BeginTabBar("VisualizationTabs")) {
        if (ImGui::BeginTabItem("Membrane Dynamics")) {
            state_.active_tab = VisualizationTab::MEMBRANE_DYNAMICS;
            
            // Create controls for visualization
            static float time_window = 500.0f; // ms
            static int max_traces = 10;
            static bool show_gating = false;
            static bool show_currents = false;
            static float trace_height = 60.0f;
            
            // Control panel
            ImGui::Text("📊 Voltage Trace Controls");
            ImGui::SliderFloat("Time Window (ms)", &time_window, 100.0f, 2000.0f, "%.0f");
            ImGui::SliderInt("Max Traces", &max_traces, 5, 20);
            ImGui::SliderFloat("Trace Height (px)", &trace_height, 30.0f, 100.0f, "%.0f");
            ImGui::Checkbox("Show Gating Variables", &show_gating);
            ImGui::SameLine();
            ImGui::Checkbox("Show Currents", &show_currents);
            
            ImGui::Separator();
            
            // Create canvas for traces
            ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
            ImVec2 canvas_size = ImGui::GetContentRegionAvail();
            canvas_size.y = std::max(canvas_size.y, 300.0f);
            
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 canvas_max = ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y);
            
            // Draw canvas background
            draw_list->AddRectFilled(canvas_pos, canvas_max, IM_COL32(15, 15, 25, 255));
            draw_list->AddRect(canvas_pos, canvas_max, IM_COL32(80, 80, 80, 255));
            
            if (network_->getNeuronCount() > 0) {
                // Calculate display parameters
                int neuron_count = std::min(max_traces, static_cast<int>(network_->getNeuronCount()));
                double current_time = network_->getTime();
                
                // Calculate trace layout
                float trace_spacing = canvas_size.y / neuron_count;
                float actual_trace_height = std::min(trace_height, trace_spacing * 0.8f);
                float margin_x = 50.0f;
                float plot_width = canvas_size.x - 2 * margin_x;
                
                // Time axis setup
                double time_start = current_time - time_window;
                double time_end = current_time;
                
                // Draw time axis
                draw_list->AddLine(
                    ImVec2(canvas_pos.x + margin_x, canvas_max.y - 30),
                    ImVec2(canvas_max.x - margin_x, canvas_max.y - 30),
                    IM_COL32(150, 150, 150, 255), 1.0f
                );
                
                // Time labels
                char time_label[32];
                snprintf(time_label, sizeof(time_label), "%.0f ms", time_start);
                draw_list->AddText(ImVec2(canvas_pos.x + margin_x, canvas_max.y - 25), 
                                 IM_COL32(200, 200, 200, 255), time_label);
                
                snprintf(time_label, sizeof(time_label), "%.0f ms", time_end);
                ImVec2 end_label_size = ImGui::CalcTextSize(time_label);
                draw_list->AddText(ImVec2(canvas_max.x - margin_x - end_label_size.x, canvas_max.y - 25), 
                                 IM_COL32(200, 200, 200, 255), time_label);
                
                // Draw voltage traces for each neuron
                for (int i = 0; i < neuron_count; ++i) {
                    const auto& neuron = network_->getNeuron(i);
                    const auto& history = neuron.getHistory();
                    auto data_window = history.getTimeWindow(current_time, time_window);
                    
                    if (data_window.empty()) continue;
                    
                    // Calculate trace position
                    float trace_center_y = canvas_pos.y + (i + 0.5f) * trace_spacing;
                    float trace_top = trace_center_y - actual_trace_height * 0.5f;
                    float trace_bottom = trace_center_y + actual_trace_height * 0.5f;
                    
                    // Draw trace background
                    ImU32 bg_color = IM_COL32(25, 25, 35, 180);
                    draw_list->AddRectFilled(
                        ImVec2(canvas_pos.x + margin_x, trace_top),
                        ImVec2(canvas_max.x - margin_x, trace_bottom),
                        bg_color
                    );
                    
                    // Draw voltage baseline (-65mV)
                    float baseline_y = trace_center_y;
                    draw_list->AddLine(
                        ImVec2(canvas_pos.x + margin_x, baseline_y),
                        ImVec2(canvas_max.x - margin_x, baseline_y),
                        IM_COL32(100, 100, 100, 150), 1.0f
                    );
                    
                    // Neuron label
                    char neuron_label[16];
                    snprintf(neuron_label, sizeof(neuron_label), "N%d", i);
                    draw_list->AddText(ImVec2(canvas_pos.x + 5, trace_center_y - 8), 
                                     IM_COL32(200, 200, 200, 255), neuron_label);
                    
                    // Voltage scale labels
                    char voltage_label[16];
                    snprintf(voltage_label, sizeof(voltage_label), "+40");
                    draw_list->AddText(ImVec2(canvas_pos.x + 25, trace_top), 
                                     IM_COL32(180, 180, 180, 200), voltage_label);
                    snprintf(voltage_label, sizeof(voltage_label), "-80");
                    draw_list->AddText(ImVec2(canvas_pos.x + 25, trace_bottom - 15), 
                                     IM_COL32(180, 180, 180, 200), voltage_label);
                    
                    // Draw voltage trace
                    if (data_window.size() > 1) {
                        std::vector<ImVec2> trace_points;
                        trace_points.reserve(data_window.size());
                        
                        for (const auto& point : data_window) {
                            // Map time to x coordinate
                            float x = canvas_pos.x + margin_x + 
                                     (point.time - time_start) / (time_end - time_start) * plot_width;
                            
                            // Map voltage to y coordinate (-80mV to +40mV range)
                            float voltage_norm = (point.V + 80.f) / 120.f; // Normalize to 0-1
                            voltage_norm = std::clamp(voltage_norm, 0.f, 1.f);
                            float y = trace_bottom - voltage_norm * actual_trace_height;
                            
                            trace_points.push_back(ImVec2(x, y));
                        }
                        
                        // Draw the trace line
                        ImU32 trace_color = getVoltageTraceColor(i);
                        for (size_t j = 1; j < trace_points.size(); ++j) {
                            draw_list->AddLine(trace_points[j-1], trace_points[j], trace_color, 2.0f);
                        }
                        
                        // Highlight spikes
                        for (size_t j = 0; j < data_window.size(); ++j) {
                            if (data_window[j].V > 0.0) { // Spike threshold
                                float spike_x = trace_points[j].x;
                                draw_list->AddLine(
                                    ImVec2(spike_x, trace_top),
                                    ImVec2(spike_x, trace_bottom),
                                    IM_COL32(255, 100, 100, 200), 1.5f
                                );
                            }
                        }
                    }
                    
                    // Show current voltage value
                    const auto& current_state = neuron.getState();
                    char voltage_text[32];
                    snprintf(voltage_text, sizeof(voltage_text), "%.1f mV", current_state.V);
                    ImVec2 voltage_text_size = ImGui::CalcTextSize(voltage_text);
                    draw_list->AddText(
                        ImVec2(canvas_max.x - margin_x - voltage_text_size.x - 5, trace_center_y - 8),
                        IM_COL32(255, 255, 100, 255), voltage_text
                    );
                    
                    // Optional: Show gating variables as smaller traces
                    if (show_gating && actual_trace_height > 50.0f) {
                        float gating_height = actual_trace_height * 0.2f;
                        float gating_y = trace_bottom - gating_height;
                        
                        // Draw m, h, n traces
                        if (data_window.size() > 1) {
                            std::vector<ImU32> gating_colors = {
                                IM_COL32(255, 150, 150, 180), // m - red
                                IM_COL32(150, 255, 150, 180), // h - green  
                                IM_COL32(150, 150, 255, 180)  // n - blue
                            };
                            
                            for (int gate = 0; gate < 3; ++gate) {
                                for (size_t j = 1; j < data_window.size(); ++j) {
                                    float x1 = canvas_pos.x + margin_x + 
                                             (data_window[j-1].time - time_start) / (time_end - time_start) * plot_width;
                                    float x2 = canvas_pos.x + margin_x + 
                                             (data_window[j].time - time_start) / (time_end - time_start) * plot_width;
                                    
                                    double val1, val2;
                                    if (gate == 0) { val1 = data_window[j-1].m; val2 = data_window[j].m; }
                                    else if (gate == 1) { val1 = data_window[j-1].h; val2 = data_window[j].h; }
                                    else { val1 = data_window[j-1].n; val2 = data_window[j].n; }
                                    
                                    float y1 = gating_y - val1 * gating_height;
                                    float y2 = gating_y - val2 * gating_height;
                                    
                                    draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), 
                                                     gating_colors[gate], 1.0f);
                                }
                            }
                        }
                    }
                }
                
                // Draw legend
                ImVec2 legend_pos = ImVec2(canvas_pos.x + 10, canvas_pos.y + 10);
                draw_list->AddText(legend_pos, IM_COL32(255, 255, 255, 255), 
                                 "🧠 Stacked Membrane Traces");
                draw_list->AddText(ImVec2(legend_pos.x, legend_pos.y + 20), 
                                 IM_COL32(200, 200, 200, 255), 
                                 "Red spikes = Wave propagation");
                
                if (show_gating) {
                    draw_list->AddText(ImVec2(legend_pos.x, legend_pos.y + 40), 
                                     IM_COL32(255, 150, 150, 255), "m(red) h(green) n(blue)");
                }
                
            } else {
                // No network
                ImVec2 center = ImVec2(canvas_pos.x + canvas_size.x * 0.5f, 
                                     canvas_pos.y + canvas_size.y * 0.5f);
                draw_list->AddText(ImVec2(center.x - 60, center.y), 
                                 IM_COL32(150, 150, 150, 255), "No network configured");
            }
            
            // Advance layout
            ImGui::Dummy(ImVec2(canvas_size.x, 10.0f));
            
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Topological View")) {
            state_.active_tab = VisualizationTab::TOPOLOGICAL_VIEW;
            
            // Create a canvas for network visualization
            ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
            ImVec2 canvas_size = ImGui::GetContentRegionAvail();
            canvas_size.y = std::max(canvas_size.y, 400.0f); // Minimum height
            
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 canvas_max = ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y);
            
            // Draw canvas background
            draw_list->AddRectFilled(canvas_pos, canvas_max, IM_COL32(20, 20, 30, 255));
            draw_list->AddRect(canvas_pos, canvas_max, IM_COL32(100, 100, 100, 255));
            
            // Network topology visualization
            // Calculate canvas center (needed for both network and no-network cases)
            float center_x = canvas_pos.x + canvas_size.x * 0.5f;
            float center_y = canvas_pos.y + canvas_size.y * 0.5f;
            
            if (network_->getNeuronCount() > 0) {
                const auto& config = network_->getTopologyConfig();
                
                // Draw topology info header using draw list instead of cursor positioning
                const char* topology_name = [&]() {
                    switch (config.type) {
                        case consciousness::TopologyType::RING: return "Ring Topology";
                        case consciousness::TopologyType::GRID: return "Grid Topology";  
                        case consciousness::TopologyType::TORUS: return "Torus Topology";
                        default: return "Unknown Topology";
                    }
                }();
                draw_list->AddText(ImVec2(canvas_pos.x + 10, canvas_pos.y + 10), 
                                 IM_COL32(204, 204, 255, 255), topology_name);
                
                // Calculate neuron positions based on topology
                std::vector<ImVec2> neuron_screen_positions;
                float radius = std::min(canvas_size.x, canvas_size.y) * 0.35f;
                
                size_t neuron_count = network_->getNeuronCount();
                neuron_screen_positions.resize(neuron_count);
                
                // Position neurons based on topology type
                if (config.type == consciousness::TopologyType::RING) {
                    // Ring layout
                    for (size_t i = 0; i < neuron_count; ++i) {
                        float angle = (2.0f * M_PI * i) / neuron_count;
                        neuron_screen_positions[i] = ImVec2(
                            center_x + radius * cos(angle),
                            center_y + radius * sin(angle)
                        );
                    }
                } else {
                    // Grid/Torus layout
                    int width = config.width;
                    int height = config.height;
                    float spacing_x = (canvas_size.x * 0.6f) / std::max(width - 1, 1);
                    float spacing_y = (canvas_size.y * 0.6f) / std::max(height - 1, 1);
                    float start_x = center_x - spacing_x * (width - 1) * 0.5f;
                    float start_y = center_y - spacing_y * (height - 1) * 0.5f;
                    
                    for (int y = 0; y < height; ++y) {
                        for (int x = 0; x < width; ++x) {
                            int neuron_idx = y * width + x;
                            if (neuron_idx < static_cast<int>(neuron_count)) {
                                neuron_screen_positions[neuron_idx] = ImVec2(
                                    start_x + x * spacing_x,
                                    start_y + y * spacing_y
                                );
                            }
                        }
                    }
                }
                
                // Draw connections first (so they appear behind neurons)
                const auto& connections = network_->getConnections();
                for (const auto& conn : connections) {
                    if (conn.from_neuron < static_cast<int>(neuron_count) && 
                        conn.to_neuron < static_cast<int>(neuron_count)) {
                        
                        ImVec2 start = neuron_screen_positions[conn.from_neuron];
                        ImVec2 end = neuron_screen_positions[conn.to_neuron];
                        
                        // Get synaptic conductance from target neuron's "ionic bucket"
                        const auto& target_neuron = network_->getNeuron(conn.to_neuron);
                        double synaptic_conductance = target_neuron.getSynapticCurrent() != 0.0 ? 
                            std::abs(target_neuron.getSynapticCurrent() / (target_neuron.getVoltage() - 0.0)) : 0.0;
                        
                        // Scale line thickness based on synaptic conductance
                        float base_thickness = 1.0f + conn.weight * 1.0f;
                        float conductance_scale = static_cast<float>(synaptic_conductance * 5.0); // Scale factor for visibility
                        float thickness = base_thickness + conductance_scale;
                        thickness = std::max(1.0f, std::min(thickness, 8.0f)); // Clamp between 1-8 pixels
                        
                        // Connection color intensity based on conductance
                        ImU32 connection_color;
                        if (synaptic_conductance > 0.001) {
                            // Active synapse - bright and colorful
                            int intensity = static_cast<int>(std::min(255.0, 120.0 + conductance_scale * 30.0));
                            connection_color = IM_COL32(intensity, intensity/2, 80, 200);
                        } else {
                            // Inactive synapse - dim
                            connection_color = IM_COL32(80, 120, 80, 120);
                        }
                        
                        draw_list->AddLine(start, end, connection_color, thickness);
                    }
                }
                
                // Draw neurons
                for (size_t i = 0; i < neuron_count; ++i) {
                    const auto& neuron = network_->getNeuron(static_cast<int>(i));
                    const auto& state = neuron.getState();
                    ImVec2 pos = neuron_screen_positions[i];
                    
                    // Neuron color based on voltage and activity
                    float voltage_norm = (state.V + 80.0f) / 120.0f; // Normalize roughly -80mV to +40mV
                    voltage_norm = std::max(0.0f, std::min(1.0f, voltage_norm));
                    
                    ImU32 neuron_color;
                    if (neuron.detectSpike()) {
                        // Bright flash for spiking
                        neuron_color = IM_COL32(255, 100, 100, 255);
                    } else {
                        // Color gradient from blue (hyperpolarized) to red (depolarized)
                        int red = static_cast<int>(voltage_norm * 255);
                        int blue = static_cast<int>((1.0f - voltage_norm) * 255);
                        neuron_color = IM_COL32(red, 50, blue, 200);
                    }
                    
                    // Draw neuron as circle
                    float neuron_radius = topology_layout_.neuron_radius;
                    draw_list->AddCircleFilled(pos, neuron_radius, neuron_color);
                    draw_list->AddCircle(pos, neuron_radius, IM_COL32(255, 255, 255, 100), 0, 1.5f);
                    
                    // Draw neuron ID
                    char neuron_label[16];
                    snprintf(neuron_label, sizeof(neuron_label), "%zu", i);
                    ImVec2 text_size = ImGui::CalcTextSize(neuron_label);
                    draw_list->AddText(ImVec2(pos.x - text_size.x * 0.5f, pos.y - text_size.y * 0.5f), 
                                     IM_COL32(255, 255, 255, 200), neuron_label);
                }
                
                // Draw legend using draw list
                ImVec2 legend_pos = ImVec2(canvas_pos.x + canvas_size.x - 150, canvas_pos.y + 40);
                draw_list->AddRectFilled(legend_pos, ImVec2(legend_pos.x + 140, legend_pos.y + 95), 
                                       IM_COL32(0, 0, 0, 150));
                
                draw_list->AddText(ImVec2(legend_pos.x + 5, legend_pos.y + 5),
                                 IM_COL32(255, 255, 255, 255), "Legend:");
                draw_list->AddText(ImVec2(legend_pos.x + 5, legend_pos.y + 25),
                                 IM_COL32(128, 128, 255, 255), "Blue: Hyperpolarized");
                draw_list->AddText(ImVec2(legend_pos.x + 5, legend_pos.y + 40),
                                 IM_COL32(255, 128, 128, 255), "Red: Depolarized");
                draw_list->AddText(ImVec2(legend_pos.x + 5, legend_pos.y + 55),
                                 IM_COL32(255, 102, 102, 255), "Bright: Spiking");
                draw_list->AddText(ImVec2(legend_pos.x + 5, legend_pos.y + 70),
                                 IM_COL32(255, 200, 80, 255), "Thick lines: Active synapses");
                
                // Network statistics overlay using draw list
                ImVec2 stats_pos = ImVec2(canvas_pos.x + 10, canvas_pos.y + 35);
                char stats_buffer[256];
                snprintf(stats_buffer, sizeof(stats_buffer), "Neurons: %zu", neuron_count);
                draw_list->AddText(stats_pos, IM_COL32(204, 204, 204, 255), stats_buffer);
                
                snprintf(stats_buffer, sizeof(stats_buffer), "Connections: %zu", connections.size());
                draw_list->AddText(ImVec2(stats_pos.x, stats_pos.y + 20), 
                                 IM_COL32(204, 204, 204, 255), stats_buffer);
                
                snprintf(stats_buffer, sizeof(stats_buffer), "Sync: %.3f", synchronization_index_);
                draw_list->AddText(ImVec2(stats_pos.x, stats_pos.y + 40), 
                                 IM_COL32(204, 204, 204, 255), stats_buffer);
                
            } else {
                // No network - use draw list instead of cursor positioning
                draw_list->AddText(ImVec2(center_x - 50, center_y), 
                                 IM_COL32(153, 153, 153, 255), "No network configured");
            }
            
            // Use ImGui::Dummy to properly advance layout instead of SetCursorScreenPos
            ImGui::Dummy(ImVec2(canvas_size.x, 10.0f));
            
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
}

// Placeholder helper functions
void MultiNeuronUI::syncParametersFromNetwork() {
    // Sync parameters from first neuron as default
    if (network_->getNeuronCount() > 0) {
        const auto& params = network_->getNeuron(0).getParameters();
        ui_C_m_ = static_cast<float>(params.C_m);
        ui_g_Na_ = static_cast<float>(params.g_Na);
        ui_g_K_ = static_cast<float>(params.g_K);
        ui_g_L_ = static_cast<float>(params.g_L);
        ui_E_Na_ = static_cast<float>(params.E_Na);
        ui_E_K_ = static_cast<float>(params.E_K);
        ui_E_L_ = static_cast<float>(params.E_L);
        ui_background_current_ = 5.0f;  // Default to weak stimulus for activity
    }
}

void MultiNeuronUI::updateTopologyLayout() {
    auto positions = network_->getTopologyLayout();
    topology_layout_.neuron_positions = positions;
    
    // Build connection lines for visualization
    topology_layout_.connection_lines.clear();
    for (const auto& conn : network_->getConnections()) {
        topology_layout_.connection_lines.emplace_back(conn.from_neuron, conn.to_neuron);
    }
}

void MultiNeuronUI::updateAnalysisData() {
    network_firing_rate_ = static_cast<float>(network_->getMeanFiringRate(state_.analysis_window_ms));
    synchronization_index_ = static_cast<float>(network_->getSynchronizationIndex());
    phase_coherence_ = static_cast<float>(network_->getPhaseCoherence());

    // NetworkVisualizationData::WaveProperties = NeuronNetwork::WaveProperties
    
    current_wave_properties_ = network_->detectTravelingWave();
    current_motor_pattern_ = network_->getAnalyzer().detectMotorPattern();
}

void MultiNeuronUI::updateVisualizationData() {
    // Update the global visualization data for bridge functions
    g_network_viz_data = network_->getVisualizationData();
}

void MultiNeuronUI::reset() {
    network_->reset();
    state_.running = false;
}

void MultiNeuronUI::exportData() const {
    std::cout << "Export data functionality coming soon\n";
}

NetworkVisualizationData MultiNeuronUI::getNetworkVisualizationData() const {
    return network_->getVisualizationData();
}

// Bridge functions
NetworkVisualizationData get_network_viz_data(void) {
    return g_network_viz_data;
}

void set_network_viz_data(const NetworkVisualizationData* data) {
    if (data) {
        g_network_viz_data = *data;
    }
}

void updateNetworkVisualizationData(const NeuronNetwork& network, const MultiNeuronUIState& ui_state) {
    g_network_viz_data = network.getVisualizationData();
}

// Placeholder implementations for remaining functions
void MultiNeuronUI::renderMembraneDynamicsTab() {}
void MultiNeuronUI::renderTopologicalViewTab() {}
void MultiNeuronUI::renderMultiNeuronTraces() {}
void MultiNeuronUI::renderNetworkTopology() {}
void MultiNeuronUI::renderNeuronNodes() {}
void MultiNeuronUI::renderConnectionLines() {}
void MultiNeuronUI::renderActivityAnimations() {}
void MultiNeuronUI::renderWaveVisualization() {}
void MultiNeuronUI::applyParametersToNetwork() {
    HHNeuron::Parameters params;
    params.C_m = ui_C_m_;
    params.g_Na = ui_g_Na_;
    params.g_K = ui_g_K_;
    params.g_L = ui_g_L_;
    params.E_Na = ui_E_Na_;
    params.E_K = ui_E_K_;
    params.E_L = ui_E_L_;
    
    switch (state_.parameter_mode) {
        case ParameterApplyMode::INDIVIDUAL:
            if (state_.focus_neuron_id < static_cast<int>(network_->getNeuronCount())) {
                network_->getNeuron(state_.focus_neuron_id).setParameters(params);
                network_->getNeuron(state_.focus_neuron_id).setBackgroundCurrent(ui_background_current_);
            }
            break;
            
        case ParameterApplyMode::SELECTED_GROUP:
            network_->setParametersForGroup(state_.selected_neurons, params);
            for (int neuron_id : state_.selected_neurons) {
                if (neuron_id < static_cast<int>(network_->getNeuronCount())) {
                    network_->getNeuron(neuron_id).setBackgroundCurrent(ui_background_current_);
                }
            }
            break;
            
        case ParameterApplyMode::ALL_NEURONS:
            network_->setAllParameters(params);
            network_->setAllBackgroundCurrent(ui_background_current_);
            break;
            
        case ParameterApplyMode::SPATIAL_PATTERN:
            // This will be handled by applySpatialParameterPattern()
            break;
    }
}
void MultiNeuronUI::applySpatialParameterPattern() {}
void MultiNeuronUI::applyTopologyConfiguration() {
    TopologyConfig config;
    config.type = state_.topology_type;
    
    switch (state_.topology_type) {
        case TopologyType::RING:
            config.neuron_count = state_.ring_neuron_count;
            break;
        case TopologyType::GRID:
        case TopologyType::TORUS:
            config.width = state_.grid_width;
            config.height = state_.grid_height;
            break;
    }
    
    ConnectivityConfig connectivity;
    connectivity.bidirectional = state_.bidirectional_connections;
    connectivity.synaptic_weight = state_.synaptic_weight;
    
    if (network_->initializeTopology(config, connectivity)) {
        updateTopologyLayout();
        std::cout << "Network topology updated successfully\n";
    } else {
        std::cerr << "Failed to update network topology\n";
    }
}
void MultiNeuronUI::applyNetworkStimulus() {}
void MultiNeuronUI::selectStimulusTargets() {}
void MultiNeuronUI::updateNeuronSelection() {}
void MultiNeuronUI::handleTopologyInteraction() {}
void MultiNeuronUI::detectMotorBehavior() {}
void MultiNeuronUI::renderParameterApplyModeSelector() {
    const char* mode_items[] = { "Individual", "Selected Group", "All Neurons", "Spatial Pattern" };
    int current_mode = static_cast<int>(state_.parameter_mode);
    
    if (ImGui::Combo("Apply Mode", &current_mode, mode_items, 4)) {
        state_.parameter_mode = static_cast<ParameterApplyMode>(current_mode);
    }
    
    // Show additional controls based on mode
    switch (state_.parameter_mode) {
        case ParameterApplyMode::INDIVIDUAL:
            ImGui::SliderInt("Target Neuron", &state_.focus_neuron_id, 0, static_cast<int>(network_->getNeuronCount()) - 1);
            break;
        case ParameterApplyMode::SELECTED_GROUP:
            ImGui::Text("Selected: %zu neurons", state_.selected_neurons.size());
            ImGui::Text("(Select neurons in topology view)");
            break;
        case ParameterApplyMode::ALL_NEURONS:
            ImGui::Text("Will apply to all %zu neurons", network_->getNeuronCount());
            break;
        case ParameterApplyMode::SPATIAL_PATTERN:
            ImGui::Text("Will apply spatial gradient/pattern");
            break;
    }
}

bool MultiNeuronUI::renderParameterSlider(const char* label, float* value, float min_val, float max_val, const char* format) {
    return ImGui::SliderFloat(label, value, min_val, max_val, format);
}

ImU32 MultiNeuronUI::getNeuronColor(int neuron_id, float activity_level) const {
    return IM_COL32(100, 100, 200, 255);  // Placeholder blue
}

ImU32 MultiNeuronUI::getConnectionColor(const SynapticConnection& connection) const {
    return IM_COL32(150, 150, 150, 100);  // Placeholder gray
}

ImU32 MultiNeuronUI::getVoltageTraceColor(int neuron_id) const {
    return IM_COL32(100 + neuron_id * 20, 150, 255, 255);  // Placeholder colors
}

ImVec2 MultiNeuronUI::networkToScreenCoords(float x, float y, const ImVec2& canvas_pos, const ImVec2& canvas_size) const {
    return ImVec2(canvas_pos.x + x * canvas_size.x, canvas_pos.y + y * canvas_size.y);
}

std::pair<float, float> MultiNeuronUI::screenToNetworkCoords(const ImVec2& screen_pos, const ImVec2& canvas_pos, const ImVec2& canvas_size) const {
    float x = (screen_pos.x - canvas_pos.x) / canvas_size.x;
    float y = (screen_pos.y - canvas_pos.y) / canvas_size.y;
    return {x, y};
}

void MultiNeuronUI::renderWaveProperties(const NetworkWaveProperties& wave) {
    ImGui::Text("Wave detection: %s", wave.detected ? "YES" : "NO");
}

void MultiNeuronUI::renderMotorPattern(const NetworkMotorPattern& pattern) {
    ImGui::Text("Motor pattern: %d", static_cast<int>(pattern.type));
}

void MultiNeuronUI::renderSynchronizationMeter(float sync_index) {
    ImGui::Text("Sync: %.3f", sync_index);
    ImGui::ProgressBar(sync_index);
}

} // namespace consciousness
