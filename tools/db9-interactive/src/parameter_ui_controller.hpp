//
// Parameter UI Controller - Unified Parameter Management for Consciousness Framework
//
// This provides DRY-compliant parameter management across single neuron and network modes:
// - Templated parameter synchronization (HHNeuron vs NeuronNetwork)
// - Unified UI rendering and validation
// - Spatial parameter pattern support for topology programming
// - Analysis data computation and caching
//

#pragma once

#include "hh_neuron.hpp"
#include <memory>
#include <functional>
#include <vector>
#include <string>

namespace consciousness {

// Forward declarations
class NeuronNetwork;
enum class ParameterApplyMode;
struct SpatialPattern;

/// UI Parameter Set - manages ImGui parameter buffers and validation
struct UIParameterSet {
    // HH Parameters (UI buffers)
    float C_m = 1.0f;           ///< Membrane capacitance (μF/cm²)
    float g_Na = 120.0f;        ///< Sodium conductance (mS/cm²)
    float g_K = 36.0f;          ///< Potassium conductance (mS/cm²)
    float g_L = 0.3f;           ///< Leak conductance (mS/cm²)
    
    // Reversal potentials (mV)
    float E_Na = 50.0f;         ///< Sodium reversal potential
    float E_K = -77.0f;         ///< Potassium reversal potential
    float E_L = -54.387f;       ///< Leak reversal potential
    
    // Background current
    float background_current = 0.0f;    ///< Background current (μA/cm²)
    
    // Validation state
    bool parameters_valid = true;
    std::string validation_message;
    
    /// Convert to HHNeuron::Parameters
    HHNeuron::Parameters toHHParameters() const;
    
    /// Load from HHNeuron::Parameters
    void fromHHParameters(const HHNeuron::Parameters& params);
    
    /// Validate parameter ranges
    bool validate();
    
    /// Reset to default values
    void resetToDefaults();
};

/// Simplified Parameter UI Controller - eliminates duplication
class ParameterUIController {
public:
    explicit ParameterUIController();
    ~ParameterUIController() = default;
    
    /// Render complete parameter panel
    bool renderParameterPanel(const char* panel_title = "Parameters");
    
    /// Get UI parameter set
    const UIParameterSet& getUIParameters() const { return ui_params_; }
    
    /// Set parameters
    void setParameters(const HHNeuron::Parameters& params);
    
    /// Get parameters
    HHNeuron::Parameters getParameters() const;
    
    /// Check if parameters have been modified
    bool hasUnsavedChanges() const { return has_unsaved_changes_; }
    
    /// Mark parameters as saved
    void markSaved() { has_unsaved_changes_ = false; }

private:
    UIParameterSet ui_params_;
    bool has_unsaved_changes_;
    
    // Private helpers
    bool renderParameterSlider(const char* label, float* value, float min_val, float max_val, 
                              const char* format = "%.3f");
};

} // namespace consciousness
