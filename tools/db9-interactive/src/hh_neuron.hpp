//
// HH Neuron - C++ Implementation for Toroidal Consciousness Framework
// Based on conventional Hodgkin-Huxley formulation
//
// This is the Application Layer in the triadic architecture:
// - Pure C++ neuron simulation logic
// - Consciousness framework integration
// - High-performance numerical computation
//

#pragma once

#include <vector>
#include <string>
#include <memory>

namespace consciousness {

// Forward declarations
struct ConsciousnessState;
class NeuronHistory;

/// Hodgkin-Huxley Neuron Implementation
/// Based on the conventional formulation described in the toroidal consciousness framework
class HHNeuron {
public:
    /// Standard HH parameters structure
    struct Parameters {
        double C_m = 1.0;           ///< Membrane capacitance (μF/cm²)
        double g_Na = 120.0;        ///< Sodium conductance (mS/cm²)
        double g_K = 36.0;          ///< Potassium conductance (mS/cm²)
        double g_L = 0.3;           ///< Leak conductance (mS/cm²)
        
        // Reversal potentials (mV)
        double E_Na = 50.0;         ///< Sodium reversal potential
        double E_K = -77.0;         ///< Potassium reversal potential
        double E_L = -54.387;       ///< Leak reversal potential
        
        // Default constructor
        Parameters() = default;
    };
    
    /// Neuron state variables
    struct State {
        double V = -65.0;           ///< Membrane potential (mV)
        double m = 0.0529;          ///< Sodium activation
        double h = 0.5961;          ///< Sodium inactivation  
        double n = 0.3177;          ///< Potassium activation
        double time = 0.0;          ///< Current simulation time (ms)
        
        // Default constructor
        State() = default;
    };
    
    /// External stimulation parameters
    struct Stimulus {
        double current = 0.0;       ///< Stimulus current (μA/cm²)
        double start_time = 0.0;    ///< Stimulus start time (ms)
        double duration = 0.0;      ///< Stimulus duration (ms)
        bool active = false;        ///< Whether stimulus is currently active
        
        // Default constructor
        Stimulus() = default;
    };
    
    /// Neuron phase classification
    enum class Phase {
        Resting,
        Threshold, 
        Depolarization,
        Overshoot,
        Repolarization,
        Hyperpolarization,
        Unknown
    };
    
    /// Synaptic input state for temporal dynamics
    struct SynapticState {
        double conductance = 0.0;        ///< Current synaptic conductance (mS/cm²)
        double decay_tau = 2.0;          ///< Decay time constant (ms)
        double reversal_potential = 0.0; ///< Synaptic reversal potential (mV)
        
        SynapticState() = default;
        SynapticState(double tau, double E_rev) : decay_tau(tau), reversal_potential(E_rev) {}
    };

public:
    /// Constructors
    explicit HHNeuron() : HHNeuron(Parameters()) {}
    explicit HHNeuron(const Parameters& params);
    
    /// Copy constructor (deleted - use clone if needed)
    HHNeuron(const HHNeuron&) = delete;
    
    /// Move constructor
    HHNeuron(HHNeuron&& other) noexcept;
    
    /// Copy assignment (deleted - use clone if needed)
    HHNeuron& operator=(const HHNeuron&) = delete;
    
    /// Move assignment
    HHNeuron& operator=(HHNeuron&& other) noexcept;
    
    /// Destructor
    ~HHNeuron();
    
    // Simulation methods
    
    /// Single integration step
    void step(double dt = 0.01);
    
    /// Reset to resting state
    void reset();
    
    /// Apply current stimulus
    void stimulate(double current, double duration);
    
    /// Set background current
    void setBackgroundCurrent(double current) { background_current_ = current; }
    
    /// Set external current (for synaptic input)
    void setExternalCurrent(double current) { external_current_ = current; }
    
    // Synaptic dynamics methods
    
    /// Add synaptic input (called when presynaptic neuron spikes)
    void addSynapticInput(double weight, double tau = 2.0, double E_rev = 0.0);
    
    /// Update synaptic conductances (exponential decay)
    void updateSynapticDynamics(double dt);
    
    /// Get total synaptic current
    double getSynapticCurrent() const;
    
    // Parameter access
    
    /// Get current parameters
    const Parameters& getParameters() const { return params_; }
    
    /// Set parameters
    void setParameters(const Parameters& params) { params_ = params; }
    
    /// Get current state
    const State& getState() const { return state_; }
    
    /// Get current stimulus
    const Stimulus& getStimulus() const { return stimulus_; }
    
    // Analysis methods
    
    /// Get current phase
    Phase getCurrentPhase() const;
    
    /// Get phase as string
    std::string getPhaseString() const;
    
    /// Detect action potential (spike)
    bool detectSpike() const;
    
    /// Get total external current (background + stimulus + synaptic)
    double getTotalExternalCurrent() const;
    
    /// Get membrane potential
    double getVoltage() const { return state_.V; }
    
    /// Get simulation time
    double getTime() const { return state_.time; }
    
    // History and analysis
    
    /// Get simulation history
    const NeuronHistory& getHistory() const { return *history_; }
    
    /// Clear history
    void clearHistory();
    
    /// Set history capacity
    void setHistoryCapacity(size_t capacity);
    
    // Consciousness framework integration
    
    /// Get consciousness state representation
    ConsciousnessState getConsciousnessState() const;
    
    /// Calculate instantaneous phase for vikalpa analysis
    double calculateInstantaneousPhase() const;
    
    /// Get grade level (0-brane for single neuron)
    int getGradeLevel() const { return 0; }

    // Ion currents
    double I_Na() const;
    double I_K() const;
    double I_L() const;    

private:
    // Rate constants for gating variables
    double alpha_m(double V) const;
    double beta_m(double V) const;
    double alpha_h(double V) const;
    double beta_h(double V) const;
    double alpha_n(double V) const;
    double beta_n(double V) const;
    
    // Internal state
    Parameters params_;
    State state_;
    State prev_state_;              ///< Previous state for spike detection
    Stimulus stimulus_;
    double background_current_;     ///< Background current (μA/cm²)
    double external_current_;       ///< External/synaptic current (μA/cm²)
    
    // Synaptic state - the "ionic bucket" for temporal dynamics
    SynapticState synaptic_state_;  ///< Synaptic conductance and kinetics
    
    // History tracking
    std::unique_ptr<NeuronHistory> history_;
    
    // Update stimulus state
    void updateStimulus();
    
    // Update history
    void updateHistory();
    
    // Update synaptic state (called internally during step)
    void updateSynapticState(double dt);
};

/// Neuron simulation history for analysis and visualization
class NeuronHistory {
public:
    struct DataPoint {
        double time;
        double V, m, h, n;
        double I_Na, I_K, I_L;
        double I_ext;
        HHNeuron::Phase phase;
    };
    
    explicit NeuronHistory(size_t capacity = 10000);
    
    /// Add data point
    void addPoint(const HHNeuron& neuron);
    
    /// Get all data points
    const std::vector<DataPoint>& getData() const { return data_; }
    
    /// Get data in time window
    std::vector<DataPoint> getTimeWindow(double current_time, double window_ms) const;
    
    /// Clear history
    void clear();
    
    /// Set capacity
    void setCapacity(size_t capacity);
    
    /// Get spike times
    std::vector<double> getSpikeTimes() const;
    
    /// Calculate firing frequency
    double calculateFiringFrequency(double window_ms = 1000.0) const;

private:
    std::vector<DataPoint> data_;
    size_t capacity_;
    size_t write_index_;
    bool wrapped_;
};

/// Consciousness state representation for framework integration
struct ConsciousnessState {
    double voltage;                 ///< Current membrane potential
    double phase;                   ///< Instantaneous phase
    HHNeuron::Phase discrete_phase; ///< Discrete phase classification
    double activity_level;          ///< Activity measure (0-1)
    double complexity;              ///< State complexity measure
    bool spiking;                   ///< Currently spiking
    double time;                    ///< Time stamp
    
    // 0-brane specific measures
    double membrane_coherence;      ///< Internal coherence measure
    double information_content;     ///< Information theoretic measure
};

} // namespace consciousness
