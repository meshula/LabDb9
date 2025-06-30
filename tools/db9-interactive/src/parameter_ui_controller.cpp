//
// Parameter UI Controller Implementation
//

#include "parameter_ui_controller.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace consciousness {

// UIParameterSet implementation

HHNeuron::Parameters UIParameterSet::toHHParameters() const {
    HHNeuron::Parameters params;
    params.C_m = C_m;
    params.g_Na = g_Na;
    params.g_K = g_K;
    params.g_L = g_L;
    params.E_Na = E_Na;
    params.E_K = E_K;
    params.E_L = E_L;
    return params;
}

void UIParameterSet::fromHHParameters(const HHNeuron::Parameters& params) {
    C_m = params.C_m;
    g_Na = params.g_Na;
    g_K = params.g_K;
    g_L = params.g_L;
    E_Na = params.E_Na;
    E_K = params.E_K;
    E_L = params.E_L;
}

bool UIParameterSet::validate() {
    parameters_valid = true;
    validation_message.clear();
    
    // Validate conductances (must be positive)
    if (g_Na <= 0.0f || g_K <= 0.0f || g_L <= 0.0f) {
        parameters_valid = false;
        validation_message = "All conductances must be positive";
        return false;
    }
    
    // Validate capacitance (must be positive)
    if (C_m <= 0.0f) {
        parameters_valid = false;
        validation_message = "Membrane capacitance must be positive";
        return false;
    }
    
    // Validate reversal potential ordering (typical biological constraints)
    if (E_Na <= E_K) {
        parameters_valid = false;
        validation_message = "Sodium reversal potential should be higher than potassium";
        return false;
    }
    
    return true;
}

void UIParameterSet::resetToDefaults() {
    C_m = 1.0f;
    g_Na = 120.0f;
    g_K = 36.0f;
    g_L = 0.3f;
    E_Na = 50.0f;
    E_K = -77.0f;
    E_L = -54.387f;
    background_current = 0.0f;
    parameters_valid = true;
    validation_message.clear();
}

// ParameterUIController implementation

ParameterUIController::ParameterUIController() 
    : has_unsaved_changes_(false) {
    ui_params_.resetToDefaults();
}

bool ParameterUIController::renderParameterPanel(const char* panel_title) {
    bool changed = false;
    
    if (ImGui::CollapsingHeader(panel_title, ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("parameter_controller");
        
        // Membrane parameters
        if (ImGui::TreeNode("Membrane Properties")) {
            changed |= renderParameterSlider("Capacitance (μF/cm²)", &ui_params_.C_m, 0.1f, 10.0f, "%.2f");
            changed |= renderParameterSlider("Background Current", &ui_params_.background_current, -10.0f, 10.0f, "%.2f");
            ImGui::TreePop();
        }
        
        // Conductances
        if (ImGui::TreeNode("Conductances (mS/cm²)")) {
            changed |= renderParameterSlider("Sodium (g_Na)", &ui_params_.g_Na, 1.0f, 200.0f, "%.1f");
            changed |= renderParameterSlider("Potassium (g_K)", &ui_params_.g_K, 1.0f, 100.0f, "%.1f");
            changed |= renderParameterSlider("Leak (g_L)", &ui_params_.g_L, 0.01f, 5.0f, "%.3f");
            ImGui::TreePop();
        }
        
        // Reversal potentials
        if (ImGui::TreeNode("Reversal Potentials (mV)")) {
            changed |= renderParameterSlider("Sodium (E_Na)", &ui_params_.E_Na, 30.0f, 70.0f, "%.1f");
            changed |= renderParameterSlider("Potassium (E_K)", &ui_params_.E_K, -100.0f, -50.0f, "%.1f");
            changed |= renderParameterSlider("Leak (E_L)", &ui_params_.E_L, -80.0f, -40.0f, "%.3f");
            ImGui::TreePop();
        }
        
        // Validation status
        if (!ui_params_.parameters_valid) {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
            ImGui::Text("⚠ %s", ui_params_.validation_message.c_str());  
            ImGui::PopStyleColor();
        }
        
        // Action buttons
        if (ImGui::Button("Reset to Defaults")) {
            ui_params_.resetToDefaults();
            changed = true;
        }
        
        ImGui::PopID();
    }
    
    if (changed) {
        has_unsaved_changes_ = true;
        ui_params_.validate();
    }
    
    return changed;
}

bool ParameterUIController::renderParameterSlider(const char* label, float* value, float min_val, float max_val, const char* format) {
    return ImGui::SliderFloat(label, value, min_val, max_val, format);
}

void ParameterUIController::setParameters(const HHNeuron::Parameters& params) {
    ui_params_.fromHHParameters(params);
    ui_params_.validate();
    has_unsaved_changes_ = false;
}

HHNeuron::Parameters ParameterUIController::getParameters() const {
    return ui_params_.toHHParameters();
}

} // namespace consciousness
