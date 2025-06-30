// Neuron UI Implementation - Interface Layer
//

#include "neuron_ui.hpp"
#include "parameter_ui_controller.hpp"
#include <cstdio>
#include <algorithm>
#include <cmath>

// C++ Dear ImGui
#include "imgui.h"

namespace consciousness {

// Forward declaration for visualization bridge
void updateVisualizationData(const HHNeuron& neuron, const NeuronUIState& ui_state);

NeuronUI::NeuronUI(std::shared_ptr<HHNeuron> neuron)
    : neuron_(neuron)
    , parameter_controller_(std::make_unique<ParameterUIController>())
    , current_frequency_(0.0f)
    , coherence_measure_(1.0f)
{
    syncParametersWithNeuron();
}

NeuronUI::~NeuronUI() = default;

void NeuronUI::render() {
    updateAnalysisData();
    updateVisualizationData(*neuron_, state_);
    
    // Create main docking space
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(viewport->Size, ImGuiCond_Always);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | 
                                   ImGuiWindowFlags_NoMove | 
                                   ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus;
    
    if (ImGui::Begin("HH Neuron Simulator", nullptr, window_flags)) {
        if (ImGui::BeginTable("MainLayout", 2, ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthFixed, 350.0f);
            ImGui::TableSetupColumn("Visualization", ImGuiTableColumnFlags_WidthStretch);
            
            ImGui::TableNextRow();
            
            ImGui::TableSetColumnIndex(0);
            renderControlPanel();
            
            // Use unified parameter controller (eliminates duplication)
            if (parameter_controller_->renderParameterPanel("HH Parameters")) {
                syncParametersWithNeuron();
            }
            
            renderStimulusPanel();
            renderAnalysisPanel();
            
            ImGui::TableSetColumnIndex(1);
            renderStatusPanel();
            
            ImGui::EndTable();
        }
    }
    ImGui::End();
    
    if (state_.show_demo) {
        ImGui::ShowDemoWindow(&state_.show_demo);
    }
}

void NeuronUI::update(float dt) {
    if (state_.running) {
        constexpr float SIMULATION_DT = 0.01f;
        neuron_->step(SIMULATION_DT);
    }
}

void NeuronUI::renderControlPanel() {
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
            neuron_->step(0.01f);
        }
        
        ImGui::Separator();
        
        ImGui::Checkbox("Show Gating Variables", &state_.show_gating_variables);
        ImGui::Checkbox("Show Currents", &state_.show_currents);
        ImGui::Checkbox("Auto Scale", &state_.auto_scale);
        
        ImGui::Text("Time Window (ms):");
        ImGui::SliderFloat("##TimeWindow", &state_.time_window_ms, 10.0f, 500.0f, "%.1f");
        
        ImGui::Separator();
        ImGui::Checkbox("Show ImGui Demo", &state_.show_demo);
    }
}

void NeuronUI::renderStimulusPanel() {
    if (ImGui::CollapsingHeader("Stimulus Control", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Stimulus Parameters:");
        ImGui::SliderFloat("Current (μA/cm²)", &state_.stimulus_current, 0.0f, 50.0f, "%.1f");
        ImGui::SliderFloat("Duration (ms)", &state_.stimulus_duration, 0.1f, 20.0f, "%.1f");
        
        if (ImGui::Button("Apply Stimulus")) {
            neuron_->stimulate(state_.stimulus_current, state_.stimulus_duration);
        }
        
        ImGui::Separator();
        
        ImGui::Text("Quick Presets:");
        if (ImGui::Button("Threshold", ImVec2(80, 0))) {
            neuron_->stimulate(8.0f, 2.0f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Strong", ImVec2(70, 0))) {
            neuron_->stimulate(15.0f, 3.0f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Sustained", ImVec2(80, 0))) {
            neuron_->stimulate(10.0f, 100.0f);
        }
        
        const auto& stimulus = neuron_->getStimulus();
        if (stimulus.active) {
            ImGui::Text("Active: %.1f μA/cm² for %.1f ms", stimulus.current, stimulus.duration);
            const float elapsed = static_cast<float>(neuron_->getTime() - stimulus.start_time);
            const float remaining = static_cast<float>(stimulus.duration) - elapsed;
            ImGui::Text("Remaining: %.2f ms", std::max(0.0f, remaining));
        } else {
            ImGui::Text("No active stimulus");
        }
    }
}

void NeuronUI::renderAnalysisPanel() {
    if (ImGui::CollapsingHeader("Analysis", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Analysis Window (ms):");
        ImGui::SliderFloat("##AnalysisWindow", &state_.analysis_window_ms, 100.0f, 5000.0f, "%.0f");
        
        ImGui::Separator();
        
        ImGui::Text("Firing Frequency: %.2f Hz", current_frequency_);
        ImGui::Text("Membrane Coherence: %.3f", coherence_measure_);
        
        const auto cs = neuron_->getConsciousnessState();
        ImGui::Text("Activity Level: %.3f", cs.activity_level);
        ImGui::Text("Information Content: %.3f", cs.information_content);
        
        ImGui::Separator();
        
        const auto& history = neuron_->getHistory();
        ImGui::Text("History Points: %zu", history.getData().size());
        
        if (ImGui::Button("Clear History")) {
            neuron_->clearHistory();
        }
        ImGui::SameLine();
        if (ImGui::Button("Export Data")) {
            exportData();
        }
        
        ImGui::Checkbox("Auto-export on stop", &state_.export_on_stop);
        ImGui::InputText("Filename", state_.export_filename, sizeof(state_.export_filename));
    }
}

void NeuronUI::renderStatusPanel() {
    if (ImGui::CollapsingHeader("Current State", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto& state = neuron_->getState();
        
        ImGui::Text("Time: %.2f ms", state.time);
        ImGui::Text("Voltage: %.2f mV", state.V);
        ImGui::Text("Phase: %s", neuron_->getPhaseString().c_str());
        
        ImGui::Separator();
        
        ImGui::Text("Gating Variables:");
        ImGui::Text("  m (Na activation): %.4f", state.m);
        ImGui::Text("  h (Na inactivation): %.4f", state.h);
        ImGui::Text("  n (K activation): %.4f", state.n);
        
        ImGui::Separator();
        
        ImGui::Text("Currents (μA/cm²):");
        ImGui::Text("  I_Na: %.2f", neuron_->I_Na());
        ImGui::Text("  I_K: %.2f", neuron_->I_K());
        ImGui::Text("  I_L: %.2f", neuron_->I_L());
        ImGui::Text("  I_ext: %.2f", neuron_->getTotalExternalCurrent());
        
        ImGui::Separator();
        
        // Membrane potential indicator
        ImGui::Text("Membrane Potential:");
        const float normalized_V = (static_cast<float>(state.V) + 100.0f) / 200.0f;
        const float clamped_V = std::clamp(normalized_V, 0.0f, 1.0f);
        
        ImGui::ProgressBar(clamped_V, ImVec2(-1, 0));
    }
    
    // Large visualization area integrated with sokol rendering
    if (ImGui::CollapsingHeader("Membrane Dynamics", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
        ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
        if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;
        if (canvas_sz.y < 50.0f) canvas_sz.y = 300.0f;
        
        ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);
        
        // Get the current visualization data
        extern neuron_viz_data_t get_neuron_viz_data(void);
        neuron_viz_data_t viz_data = get_neuron_viz_data();
        
        // Create a custom draw list for the canvas
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        
        // Background
        draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(15, 15, 25, 255));
        draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(100, 100, 120, 255));
        
        // If we have voltage history data, draw it
        if (viz_data.voltage_history && viz_data.time_history && viz_data.history_count > 1) {
            // Calculate time and voltage ranges
            float time_min = viz_data.time - viz_data.time_window_ms;
            float time_max = viz_data.time;
            float voltage_min = -100.0f;
            float voltage_max = 50.0f;
            
            // Draw voltage trace
            for (int i = 1; i < viz_data.history_count; i++) {
                // Map data coordinates to screen coordinates
                float norm_x1 = (viz_data.time_history[i-1] - time_min) / (time_max - time_min);
                float norm_y1 = (viz_data.voltage_history[i-1] - voltage_min) / (voltage_max - voltage_min);
                float norm_x2 = (viz_data.time_history[i] - time_min) / (time_max - time_min);
                float norm_y2 = (viz_data.voltage_history[i] - voltage_min) / (voltage_max - voltage_min);
                
                // Clamp to valid range
                norm_x1 = std::clamp(norm_x1, 0.0f, 1.0f);
                norm_y1 = std::clamp(norm_y1, 0.0f, 1.0f);
                norm_x2 = std::clamp(norm_x2, 0.0f, 1.0f);
                norm_y2 = std::clamp(norm_y2, 0.0f, 1.0f);
                
                // Convert to screen coordinates (flip Y for screen space)
                ImVec2 p1(canvas_p0.x + norm_x1 * canvas_sz.x, 
                         canvas_p0.y + (1.0f - norm_y1) * canvas_sz.y);
                ImVec2 p2(canvas_p0.x + norm_x2 * canvas_sz.x,
                         canvas_p0.y + (1.0f - norm_y2) * canvas_sz.y);
                
                // Color based on voltage
                float voltage_color_intensity = (viz_data.voltage_history[i] + 100.0f) / 150.0f;
                voltage_color_intensity = std::clamp(voltage_color_intensity, 0.0f, 1.0f);
                
                ImU32 voltage_color = IM_COL32(
                    (int)(74 + voltage_color_intensity * 181),   // Blue to red gradient
                    (int)(158 - voltage_color_intensity * 158),
                    (int)(255 - voltage_color_intensity * 255),
                    255
                );
                
                draw_list->AddLine(p1, p2, voltage_color, 2.0f);
            }
            
            // Draw gating variables if enabled
            if (viz_data.show_gating_variables && viz_data.m_history && viz_data.h_history && viz_data.n_history) {
                float gate_area_y = canvas_p0.y + canvas_sz.y * 0.7f;
                float gate_area_h = canvas_sz.y * 0.25f;
                
                for (int i = 1; i < viz_data.history_count; i++) {
                    float norm_x1 = (viz_data.time_history[i-1] - time_min) / (time_max - time_min);
                    float norm_x2 = (viz_data.time_history[i] - time_min) / (time_max - time_min);
                    
                    if (norm_x1 >= 0.0f && norm_x1 <= 1.0f && norm_x2 >= 0.0f && norm_x2 <= 1.0f) {
                        // m gate (red)
                        ImVec2 m1(canvas_p0.x + norm_x1 * canvas_sz.x,
                                 gate_area_y + (1.0f - viz_data.m_history[i-1]) * gate_area_h);
                        ImVec2 m2(canvas_p0.x + norm_x2 * canvas_sz.x,
                                 gate_area_y + (1.0f - viz_data.m_history[i]) * gate_area_h);
                        draw_list->AddLine(m1, m2, IM_COL32(255, 107, 107, 255), 1.5f);
                        
                        // h gate (cyan)
                        ImVec2 h1(canvas_p0.x + norm_x1 * canvas_sz.x,
                                 gate_area_y + (1.0f - viz_data.h_history[i-1]) * gate_area_h);
                        ImVec2 h2(canvas_p0.x + norm_x2 * canvas_sz.x,
                                 gate_area_y + (1.0f - viz_data.h_history[i]) * gate_area_h);
                        draw_list->AddLine(h1, h2, IM_COL32(79, 204, 199, 255), 1.5f);
                        
                        // n gate (light blue)
                        ImVec2 n1(canvas_p0.x + norm_x1 * canvas_sz.x,
                                 gate_area_y + (1.0f - viz_data.n_history[i-1]) * gate_area_h);
                        ImVec2 n2(canvas_p0.x + norm_x2 * canvas_sz.x,
                                 gate_area_y + (1.0f - viz_data.n_history[i]) * gate_area_h);
                        draw_list->AddLine(n1, n2, IM_COL32(69, 184, 209, 255), 1.5f);
                    }
                }
                
                // Gate variable labels
                draw_list->AddText(ImVec2(canvas_p0.x + 5, gate_area_y + 5), 
                                  IM_COL32(255, 107, 107, 255), "m");
                draw_list->AddText(ImVec2(canvas_p0.x + 20, gate_area_y + 5), 
                                  IM_COL32(79, 204, 199, 255), "h");
                draw_list->AddText(ImVec2(canvas_p0.x + 35, gate_area_y + 5), 
                                  IM_COL32(69, 184, 209, 255), "n");
            }
            
            // Draw grid lines
            ImU32 grid_color = IM_COL32(255, 255, 255, 25);
            
            // Vertical grid lines (time)
            for (int i = 1; i < 10; i++) {
                float x = canvas_p0.x + (float)i * canvas_sz.x / 10.0f;
                draw_list->AddLine(ImVec2(x, canvas_p0.y), ImVec2(x, canvas_p1.y), grid_color);
            }
            
            // Horizontal grid lines (voltage)
            for (int i = 1; i < 10; i++) {
                float y = canvas_p0.y + (float)i * canvas_sz.y / 10.0f;
                draw_list->AddLine(ImVec2(canvas_p0.x, y), ImVec2(canvas_p1.x, y), grid_color);
            }
            
            // Axes labels
            char time_label[32];
            snprintf(time_label, sizeof(time_label), "%.1f ms", viz_data.time);
            draw_list->AddText(ImVec2(canvas_p1.x - 50, canvas_p1.y - 15), 
                              IM_COL32(200, 200, 200, 255), time_label);
            
            char voltage_label[32];
            snprintf(voltage_label, sizeof(voltage_label), "%.1f mV", viz_data.voltage);
            draw_list->AddText(ImVec2(canvas_p0.x + 5, canvas_p0.y + 5), 
                              IM_COL32(200, 200, 200, 255), voltage_label);
            
            // Stimulus indicator
            if (viz_data.stimulus_active) {
                ImVec2 stim_p0(canvas_p0.x + 10, canvas_p0.y + 25);
                ImVec2 stim_p1(canvas_p0.x + 110, canvas_p0.y + 45);
                
                // Pulsing effect
                float pulse = sinf((float)viz_data.time * 0.1f) * 0.5f + 0.5f;
                ImU32 stim_color = IM_COL32(255, 204, 51, (int)(128 + pulse * 127));
                
                draw_list->AddRectFilled(stim_p0, stim_p1, stim_color);
                draw_list->AddRect(stim_p0, stim_p1, IM_COL32(255, 204, 51, 255));
                draw_list->AddText(ImVec2(stim_p0.x + 5, stim_p0.y + 2), 
                                  IM_COL32(0, 0, 0, 255), "STIMULUS");
            }
            
        } else {
            // No data available yet
            ImVec2 text_pos = ImVec2(canvas_p0.x + 10, canvas_p0.y + 10);
            draw_list->AddText(text_pos, IM_COL32(200, 200, 200, 255), 
                              "Membrane dynamics visualization");
            draw_list->AddText(ImVec2(text_pos.x, text_pos.y + 20), IM_COL32(150, 150, 150, 255), 
                              "Start simulation to see voltage traces");
        }
        
        // Reserve space for the canvas
        ImGui::InvisibleButton("membrane_canvas", canvas_sz);
        
        // Voltage scale indicator
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Membrane Potential: %.2f mV\nTime: %.2f ms", 
                             viz_data.voltage, viz_data.time);
        }
    }
}

void NeuronUI::updateAnalysisData() {
    const auto& history = neuron_->getHistory();
    current_frequency_ = static_cast<float>(history.calculateFiringFrequency(state_.analysis_window_ms));
    coherence_measure_ = 1.0f;
}

void NeuronUI::syncParametersWithNeuron() {
    // Sync from neuron to parameter controller
    parameter_controller_->setParameters(neuron_->getParameters());
    
    // Apply any changes from controller back to neuron
    if (parameter_controller_->hasUnsavedChanges()) {
        neuron_->setParameters(parameter_controller_->getParameters());
        parameter_controller_->markSaved();
    }
}

void NeuronUI::reset() {
    state_.running = false;
    neuron_->reset();
    
    // Reset parameters through controller
    parameter_controller_->setParameters(neuron_->getParameters());
    
    current_frequency_ = 0.0f;
}

void NeuronUI::exportData() const {
    printf("Export data to %s (not yet implemented)\
", state_.export_filename);
}

} // namespace consciousness`,
