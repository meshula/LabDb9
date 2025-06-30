//
// NeuronNetwork - Collective Consciousness Architecture
//
// This extends the triadic architecture to manage multiple HH neurons:
// - Network topology management (Ring, Grid, Torus)
// - Collective parameter control
// - Synaptic connectivity and coupling
// - Motor behavior analysis
//

#pragma once

#include "hh_neuron.hpp"
#include <vector>
#include <memory>
#include <set>
#include <functional>
#include <string>

namespace consciousness {

// Forward declarations
class NetworkAnalyzer;

/// Network topology types
enum class TopologyType {
    RING,           ///< Neurons arranged in ring with adjacent connections
    GRID,           ///< Rectangular grid topology  
    TORUS           ///< Grid with wraparound connections
};

/// Network connectivity configuration
struct ConnectivityConfig {
    bool bidirectional = false;         ///< Connections work both ways (default: unidirectional for clean waves)
    double synaptic_weight = 1.0;       ///< Default connection strength (reduced from 2.0)
    double synaptic_delay = 0.5;        ///< Synaptic delay (ms)
    bool adjacent_only = true;          ///< Only connect adjacent neurons
    
    ConnectivityConfig() = default;
};

/// Network topology configuration
struct TopologyConfig {
    TopologyType type = TopologyType::RING;
    int neuron_count = 20;              ///< For ring topology
    int width = 5;                      ///< For grid/torus
    int height = 5;                     ///< For grid/torus
    
    // Validation
    bool isValid() const {
        if (type == TopologyType::RING) {
            return neuron_count >= 3 && neuron_count <= 100;
        } else {
            return width >= 2 && height >= 2 && (width * height) <= 100;
        }
    }
    
    /// Get total neuron count
    int getTotalNeurons() const {
        return (type == TopologyType::RING) ? neuron_count : (width * height);
    }
};

/// Parameter application modes
enum class ParameterApplyMode {
    INDIVIDUAL,         ///< Apply to single focused neuron
    SELECTED_GROUP,     ///< Apply to selected neurons
    ALL_NEURONS,        ///< Apply to entire network
    SPATIAL_PATTERN     ///< Apply spatial gradient/pattern
};

/// Spatial parameter pattern for motor behavior generation
struct SpatialPattern {
    enum Type { GRADIENT, WAVE, RANDOM } type = GRADIENT;
    
    // Gradient parameters
    HHNeuron::Parameters start_params;
    HHNeuron::Parameters end_params;
    
    // Wave parameters  
    double wavelength = 10.0;           ///< For traveling wave patterns
    double phase_offset = 0.0;          ///< Starting phase
    
    // Random parameters
    double variation_percent = 10.0;    ///< Percentage variation from base
    
    SpatialPattern() = default;
};

/// Connection between two neurons
struct SynapticConnection {
    int from_neuron;                    ///< Source neuron index
    int to_neuron;                      ///< Target neuron index
    double weight;                      ///< Connection strength
    double delay;                       ///< Synaptic delay (ms)
    bool active;                        ///< Connection enabled
    
    SynapticConnection(int from, int to, double w = 1.0, double d = 0.5) 
        : from_neuron(from), to_neuron(to), weight(w), delay(d), active(true) {}
};

/// Network-wide stimulus patterns
struct NetworkStimulus {
    enum Pattern { SINGLE_SITE, TRAVELING_WAVE, SPATIAL_GRADIENT, RANDOM } pattern;
    
    // Site selection
    std::vector<int> target_neurons;
    
    // Stimulus parameters
    double current = 10.0;              ///< Stimulus current (\u03bcA/cm\u00b2)
    double duration = 5.0;              ///< Stimulus duration (ms)
    
    // Traveling wave parameters
    double wave_velocity = 5.0;         ///< mm/s (for spatial interpretation)
    int wave_direction = 1;             ///< Direction around ring/across grid
    
    // Timing
    double start_time = 0.0;
    bool active = false;
    
    NetworkStimulus() = default;
};

// Wave properties - simplified struct
struct NetworkWaveProperties {
    bool detected = false;
    double velocity = 0.0;          ///< Wave velocity (neurons/ms)
    double wavelength = 0.0;        ///< Wavelength (neuron spacing)
    int direction = 0;              ///< Direction (1 = forward, -1 = backward)
    double coherence = 0.0;         ///< Wave coherence measure
};

// Motor pattern - simplified struct
struct NetworkMotorPattern {
    enum Type { NONE, OSCILLATION, TRAVELING_WAVE, STANDING_WAVE } type = NONE;
    double frequency = 0.0;
    double coherence = 0.0;
    std::vector<double> phase_profile;  ///< Phase at each neuron position
};

/// Get topology layout for visualization
struct NeuronPosition {
    double x, y;                    ///< Normalized coordinates [0,1]
    int index;
};
/// Visualization data structure for UI layer (moved here to resolve forward declaration)
struct NetworkVisualizationData {
    std::vector<NeuronPosition> neuron_positions;
    std::vector<SynapticConnection> connections;
    TopologyType topology_type;
    
    // Current state
    std::vector<double> voltages;
    std::vector<double> activity_levels;    ///< [0,1] activity intensity
    std::vector<bool> spiking_states;
    
    // Network metrics
    double mean_firing_rate;
    double synchronization_index;
    double phase_coherence;
    
    NetworkWaveProperties wave_properties;
    NetworkMotorPattern motor_pattern;
    
    // Time
    double current_time;
};



/// Main network class managing collective neural dynamics
class NeuronNetwork {
public:
    /// Constructor
    explicit NeuronNetwork();
    
    /// Destructor
    ~NeuronNetwork();
    
    // Network configuration
    
    /// Initialize network with topology
    bool initializeTopology(const TopologyConfig& config, 
                          const ConnectivityConfig& connectivity = ConnectivityConfig());
    
    /// Get current topology configuration
    const TopologyConfig& getTopologyConfig() const { return topology_config_; }
    
    /// Get connectivity configuration
    const ConnectivityConfig& getConnectivityConfig() const { return connectivity_config_; }
    
    /// Get number of neurons
    size_t getNeuronCount() const { return neurons_.size(); }
    
    /// Get neuron by index
    HHNeuron& getNeuron(int index) { return neurons_[index]; }
    const HHNeuron& getNeuron(int index) const { return neurons_[index]; }
    
    // Simulation control
    
    /// Step entire network forward
    void step(double dt = 0.01);
    
    /// Reset entire network
    void reset();
    
    /// Get simulation time
    double getTime() const { return simulation_time_; }
    
    // Parameter management
    
    /// Set parameters for all neurons
    void setAllParameters(const HHNeuron::Parameters& params);
    
    /// Set parameters for selected neurons
    void setParametersForGroup(const std::set<int>& neuron_indices, 
                              const HHNeuron::Parameters& params);
    
    /// Apply spatial parameter pattern
    void applySpatialPattern(const SpatialPattern& pattern);
    
    /// Set background current for all neurons
    void setAllBackgroundCurrent(double current);
    
    /// Apply background current gradient
    void applyBackgroundCurrentGradient(double start_current, double end_current);
    
    // Stimulus control
    
    /// Apply network-wide stimulus pattern
    void applyNetworkStimulus(const NetworkStimulus& stimulus);
    
    /// Stimulate specific neuron
    void stimulateNeuron(int neuron_index, double current, double duration);
    
    /// Stop all active stimuli
    void stopAllStimuli();
    
    // Network analysis
    
    /// Get network analyzer
    const NetworkAnalyzer& getAnalyzer() const { return *analyzer_; }
    
    /// Calculate mean firing rate
    double getMeanFiringRate(double window_ms = 1000.0) const;
    
    /// Calculate network synchronization index
    double getSynchronizationIndex() const;
    
    NetworkWaveProperties detectTravelingWave() const;
    
    /// Calculate phase coherence across network
    double getPhaseCoherence() const;
    
    // Connectivity management
    
    /// Get connection matrix (sparse representation)
    const std::vector<SynapticConnection>& getConnections() const { return connections_; }
    
    /// Add custom connection
    void addConnection(int from_neuron, int to_neuron, double weight, double delay = 0.5);
    
    /// Remove connection
    void removeConnection(int from_neuron, int to_neuron);
    
    /// Set connection weight
    void setConnectionWeight(int from_neuron, int to_neuron, double weight);
    

    std::vector<NeuronPosition> getTopologyLayout() const;
    
    // Visualization data interface
    
    /// Get visualization data for UI
    NetworkVisualizationData getVisualizationData() const;
    
    /// Clear all neuron histories
    void clearAllHistories();
    
    /// Set history capacity for all neurons
    void setHistoryCapacity(size_t capacity);
    
private:
    // Core components
    std::vector<HHNeuron> neurons_;
    std::vector<SynapticConnection> connections_;
    std::unique_ptr<NetworkAnalyzer> analyzer_;
    
    // Configuration
    TopologyConfig topology_config_;
    ConnectivityConfig connectivity_config_;
    
    // Simulation state
    double simulation_time_;
    NetworkStimulus current_stimulus_;
    
    // Private methods
    
    /// Build connections for current topology
    void buildConnections();
    
    /// Build ring topology connections
    void buildRingConnections();
    
    /// Build grid topology connections  
    void buildGridConnections();
    
    /// Build torus topology connections
    void buildTorusConnections();
    
    /// Update synaptic inputs for all neurons
    void updateSynapticInputs();
    
    /// Update network stimulus
    void updateNetworkStimulus();
    
    /// Convert 2D grid coordinates to linear index
    int gridToIndex(int x, int y) const;
    
    /// Convert linear index to 2D grid coordinates
    std::pair<int, int> indexToGrid(int index) const;
    
    /// Get neuron neighbors for topology
    std::vector<int> getNeighbors(int neuron_index) const;
    
    /// Calculate spatial interpolation factor
    double getSpatialFactor(int neuron_index, const SpatialPattern& pattern) const;
};

/// Network analysis class for motor behavior and consciousness metrics
class NetworkAnalyzer {
public:
    explicit NetworkAnalyzer(const NeuronNetwork* network);
    
    /// Update analysis (called each simulation step)
    void update();
    
    /// Get recent spike times for all neurons
    std::vector<std::vector<double>> getAllSpikeTimes(double window_ms = 1000.0) const;
    
    /// Calculate cross-correlation between neurons
    double calculateCrossCorrelation(int neuron1, int neuron2, double window_ms = 1000.0) const;
    
    /// Detect motor patterns
    NetworkMotorPattern detectMotorPattern() const;
    
    /// Calculate network entropy
    double calculateNetworkEntropy() const;
    
    /// Measure information flow between regions
    double calculateInformationFlow(int source_region, int target_region) const;
    
private:
    const NeuronNetwork* network_;
    
    // Analysis state
    std::vector<std::vector<double>> recent_spike_times_;
    double last_update_time_;
    
    // Motor pattern detection
    std::vector<double> phase_history_;
    std::vector<double> coherence_history_;
};

} // namespace consciousness
