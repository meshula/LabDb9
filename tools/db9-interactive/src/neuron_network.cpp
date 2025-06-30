//
// NeuronNetwork Implementation - Collective Consciousness Core
//

#include "neuron_network.hpp"
#include <algorithm>
#include <cmath>
#include <random>
#include <numeric>
#include <iostream>

namespace consciousness {

// NeuronNetwork Implementation

NeuronNetwork::NeuronNetwork() 
    : analyzer_(std::make_unique<NetworkAnalyzer>(this))
    , simulation_time_(0.0)
{
    // Initialize with default ring topology
    TopologyConfig default_config;
    default_config.type = TopologyType::RING;
    default_config.neuron_count = 10;
    
    initializeTopology(default_config);
}

NeuronNetwork::~NeuronNetwork() = default;

bool NeuronNetwork::initializeTopology(const TopologyConfig& config, 
                                     const ConnectivityConfig& connectivity) {
    if (!config.isValid()) {
        std::cerr << "Invalid topology configuration\n";
        return false;
    }
    
    // Store configurations
    topology_config_ = config;
    connectivity_config_ = connectivity;
    
    // Clear existing network
    neurons_.clear();
    connections_.clear();
    simulation_time_ = 0.0;
    
    // Create neurons
    int total_neurons = config.getTotalNeurons();
    neurons_.reserve(total_neurons);
    
    for (int i = 0; i < total_neurons; ++i) {
        neurons_.emplace_back(HHNeuron::Parameters());
    }
    
    // Build connectivity
    buildConnections();
    
    std::cout << "Network initialized: " << total_neurons << " neurons, "
              << connections_.size() << " connections\n";
    
    return true;
}

void NeuronNetwork::step(double dt) {
    // Update network stimulus
    updateNetworkStimulus();
    
    // Update synaptic inputs
    updateSynapticInputs();
    
    // Step all neurons
    for (auto& neuron : neurons_) {
        neuron.step(dt);
    }
    
    // Update simulation time
    simulation_time_ += dt;
    
    // Update analyzer
    analyzer_->update();
}

void NeuronNetwork::reset() {
    for (auto& neuron : neurons_) {
        neuron.reset();
    }
    simulation_time_ = 0.0;
    stopAllStimuli();
}

// Parameter Management

void NeuronNetwork::setAllParameters(const HHNeuron::Parameters& params) {
    for (auto& neuron : neurons_) {
        neuron.setParameters(params);
    }
}

void NeuronNetwork::setParametersForGroup(const std::set<int>& neuron_indices, 
                                        const HHNeuron::Parameters& params) {
    for (int index : neuron_indices) {
        if (index >= 0 && index < static_cast<int>(neurons_.size())) {
            neurons_[index].setParameters(params);
        }
    }
}

void NeuronNetwork::applySpatialPattern(const SpatialPattern& pattern) {
    for (int i = 0; i < static_cast<int>(neurons_.size()); ++i) {
        double factor = getSpatialFactor(i, pattern);
        
        HHNeuron::Parameters params;
        
        switch (pattern.type) {
            case SpatialPattern::GRADIENT:
                // Linear interpolation between start and end parameters
                params.C_m = pattern.start_params.C_m + factor * (pattern.end_params.C_m - pattern.start_params.C_m);
                params.g_Na = pattern.start_params.g_Na + factor * (pattern.end_params.g_Na - pattern.start_params.g_Na);
                params.g_K = pattern.start_params.g_K + factor * (pattern.end_params.g_K - pattern.start_params.g_K);
                params.g_L = pattern.start_params.g_L + factor * (pattern.end_params.g_L - pattern.start_params.g_L);
                params.E_Na = pattern.start_params.E_Na + factor * (pattern.end_params.E_Na - pattern.start_params.E_Na);
                params.E_K = pattern.start_params.E_K + factor * (pattern.end_params.E_K - pattern.start_params.E_K);
                params.E_L = pattern.start_params.E_L + factor * (pattern.end_params.E_L - pattern.start_params.E_L);
                break;
                
            case SpatialPattern::WAVE:
                // Sinusoidal modulation for traveling wave patterns
                {
                    double wave_phase = 2.0 * M_PI * factor + pattern.phase_offset;
                    double modulation = 0.5 * (1.0 + std::sin(wave_phase));
                    
                    params = pattern.start_params;
                    params.g_Na *= (1.0 + 0.2 * modulation);  // 20% modulation
                    params.g_K *= (1.0 + 0.2 * modulation);
                }
                break;
                
            case SpatialPattern::RANDOM:
                // Random variation around base parameters
                {
                    static std::random_device rd;
                    static std::mt19937 gen(rd());
                    std::uniform_real_distribution<> dis(-1.0, 1.0);
                    
                    double variation = pattern.variation_percent / 100.0;
                    
                    params = pattern.start_params;
                    params.g_Na *= (1.0 + variation * dis(gen));
                    params.g_K *= (1.0 + variation * dis(gen));
                    params.g_L *= (1.0 + variation * dis(gen));
                }
                break;
        }
        
        neurons_[i].setParameters(params);
    }
}

void NeuronNetwork::setAllBackgroundCurrent(double current) {
    for (auto& neuron : neurons_) {
        neuron.setBackgroundCurrent(current);
    }
}

void NeuronNetwork::applyBackgroundCurrentGradient(double start_current, double end_current) {
    for (int i = 0; i < static_cast<int>(neurons_.size()); ++i) {
        double factor = static_cast<double>(i) / (neurons_.size() - 1);
        double current = start_current + factor * (end_current - start_current);
        neurons_[i].setBackgroundCurrent(current);
    }
}

// Stimulus Control

void NeuronNetwork::applyNetworkStimulus(const NetworkStimulus& stimulus) {
    current_stimulus_ = stimulus;
    current_stimulus_.start_time = simulation_time_;
    current_stimulus_.active = true;
}

void NeuronNetwork::stimulateNeuron(int neuron_index, double current, double duration) {
    if (neuron_index >= 0 && neuron_index < static_cast<int>(neurons_.size())) {
        neurons_[neuron_index].stimulate(current, duration);
    }
}

void NeuronNetwork::stopAllStimuli() {
    current_stimulus_.active = false;
    // Individual neuron stimuli will expire naturally
}

// Network Analysis

double NeuronNetwork::getMeanFiringRate(double window_ms) const {
    double total_rate = 0.0;
    for (const auto& neuron : neurons_) {
        total_rate += neuron.getHistory().calculateFiringFrequency(window_ms);
    }
    return total_rate / neurons_.size();
}

double NeuronNetwork::getSynchronizationIndex() const {
    if (neurons_.size() < 2) return 1.0;
    
    // Calculate phase synchronization using Kuramoto order parameter
    double sum_cos = 0.0;
    double sum_sin = 0.0;
    
    for (const auto& neuron : neurons_) {
        double phase = neuron.calculateInstantaneousPhase();
        sum_cos += std::cos(phase);
        sum_sin += std::sin(phase);
    }
    
    double r = std::sqrt(sum_cos * sum_cos + sum_sin * sum_sin) / neurons_.size();
    return r;  // Returns value between 0 (no sync) and 1 (perfect sync)
}

NetworkWaveProperties NeuronNetwork::detectTravelingWave() const {
    NetworkWaveProperties wave;
    
    if (topology_config_.type != TopologyType::RING || neurons_.size() < 3) {
        return wave;  // Not detected
    }
    
    // Get recent spike times for phase analysis
    std::vector<double> phases;
    phases.reserve(neurons_.size());
    
    for (const auto& neuron : neurons_) {
        phases.push_back(neuron.calculateInstantaneousPhase());
    }
    
    // Calculate phase differences around the ring
    std::vector<double> phase_diffs;
    for (size_t i = 0; i < phases.size(); ++i) {
        size_t next = (i + 1) % phases.size();
        double diff = phases[next] - phases[i];
        
        // Wrap to [-π, π]
        while (diff > M_PI) diff -= 2.0 * M_PI;
        while (diff < -M_PI) diff += 2.0 * M_PI;
        
        phase_diffs.push_back(diff);
    }
    
    // Check for consistent phase progression (traveling wave)
    double mean_diff = std::accumulate(phase_diffs.begin(), phase_diffs.end(), 0.0) / phase_diffs.size();
    
    // Calculate coherence (how consistent the phase differences are)
    double variance = 0.0;
    for (double diff : phase_diffs) {
        variance += (diff - mean_diff) * (diff - mean_diff);
    }
    variance /= phase_diffs.size();
    
    wave.coherence = std::exp(-variance);  // High coherence = low variance
    
    // Detect wave if coherence is high enough and there's consistent progression
    if (wave.coherence > 0.7 && std::abs(mean_diff) > 0.1) {
        wave.detected = true;
        wave.direction = (mean_diff > 0) ? 1 : -1;
        wave.wavelength = 2.0 * M_PI / std::abs(mean_diff);
        
        // Estimate velocity (simplified)
        wave.velocity = wave.wavelength / 10.0;  // Rough estimate
    }
    
    return wave;
}

double NeuronNetwork::getPhaseCoherence() const {
    return getSynchronizationIndex();  // Same calculation for now
}

// Connectivity Management

void NeuronNetwork::addConnection(int from_neuron, int to_neuron, double weight, double delay) {
    if (from_neuron >= 0 && from_neuron < static_cast<int>(neurons_.size()) &&
        to_neuron >= 0 && to_neuron < static_cast<int>(neurons_.size()) &&
        from_neuron != to_neuron) {
        
        connections_.emplace_back(from_neuron, to_neuron, weight, delay);
    }
}

void NeuronNetwork::removeConnection(int from_neuron, int to_neuron) {
    connections_.erase(
        std::remove_if(connections_.begin(), connections_.end(),
            [from_neuron, to_neuron](const SynapticConnection& conn) {
                return conn.from_neuron == from_neuron && conn.to_neuron == to_neuron;
            }),
        connections_.end());
}

void NeuronNetwork::setConnectionWeight(int from_neuron, int to_neuron, double weight) {
    for (auto& conn : connections_) {
        if (conn.from_neuron == from_neuron && conn.to_neuron == to_neuron) {
            conn.weight = weight;
            break;
        }
    }
}

std::vector<NeuronPosition> NeuronNetwork::getTopologyLayout() const {
    std::vector<NeuronPosition> positions;
    positions.reserve(neurons_.size());
    
    switch (topology_config_.type) {
        case TopologyType::RING:
            {
                for (int i = 0; i < static_cast<int>(neurons_.size()); ++i) {
                    double angle = 2.0 * M_PI * i / neurons_.size();
                    positions.push_back({
                        0.5 + 0.4 * std::cos(angle),
                        0.5 + 0.4 * std::sin(angle),
                        i
                    });
                }
            }
            break;
            
        case TopologyType::GRID:
        case TopologyType::TORUS:
            {
                for (int i = 0; i < static_cast<int>(neurons_.size()); ++i) {
                    auto [x, y] = indexToGrid(i);
                    positions.push_back({
                        (x + 0.5) / topology_config_.width,
                        (y + 0.5) / topology_config_.height,
                        i
                    });
                }
            }
            break;
    }
    
    return positions;
}

NetworkVisualizationData NeuronNetwork::getVisualizationData() const {
    NetworkVisualizationData data;
    
    // Network structure
    data.neuron_positions = getTopologyLayout();
    data.connections = connections_;
    data.topology_type = topology_config_.type;
    
    // Current state
    data.voltages.reserve(neurons_.size());
    data.activity_levels.reserve(neurons_.size());
    data.spiking_states.reserve(neurons_.size());
    
    for (const auto& neuron : neurons_) {
        data.voltages.push_back(neuron.getVoltage());
        
        // Calculate activity level (0-1 based on voltage range)
        double activity = (neuron.getVoltage() + 100.0) / 150.0;  // Map -100mV to +50mV -> 0 to 1
        data.activity_levels.push_back(std::clamp(activity, 0.0, 1.0));
        
        data.spiking_states.push_back(neuron.detectSpike());
    }
    
    // Network metrics
    data.mean_firing_rate = getMeanFiringRate();
    data.synchronization_index = getSynchronizationIndex();
    data.phase_coherence = getPhaseCoherence();
    data.wave_properties = detectTravelingWave();
    data.motor_pattern = analyzer_->detectMotorPattern();
    
    data.current_time = simulation_time_;
    
    return data;
}

void NeuronNetwork::clearAllHistories() {
    for (auto& neuron : neurons_) {
        neuron.clearHistory();
    }
}

void NeuronNetwork::setHistoryCapacity(size_t capacity) {
    for (auto& neuron : neurons_) {
        neuron.setHistoryCapacity(capacity);
    }
}

// Private Methods

void NeuronNetwork::buildConnections() {
    connections_.clear();
    
    switch (topology_config_.type) {
        case TopologyType::RING:
            buildRingConnections();
            break;
        case TopologyType::GRID:
            buildGridConnections();
            break;
        case TopologyType::TORUS:
            buildTorusConnections();
            break;
    }
}

void NeuronNetwork::buildRingConnections() {
    int n = static_cast<int>(neurons_.size());
    
    std::cout << "Building ring connections for " << n << " neurons:\n";
    
    for (int i = 0; i < n; ++i) {
        int next = (i + 1) % n;
        
        // Forward connection
        addConnection(i, next, connectivity_config_.synaptic_weight, connectivity_config_.synaptic_delay);
        std::cout << "  " << i << " -> " << next << " (weight: " << connectivity_config_.synaptic_weight << ")\n";
        
        // Backward connection (if bidirectional)
        if (connectivity_config_.bidirectional) {
            int prev = (i - 1 + n) % n;
            addConnection(i, prev, connectivity_config_.synaptic_weight, connectivity_config_.synaptic_delay);
            std::cout << "  " << i << " -> " << prev << " (weight: " << connectivity_config_.synaptic_weight << ")\n";
        }
    }
}

void NeuronNetwork::buildGridConnections() {
    int width = topology_config_.width;
    int height = topology_config_.height;
    
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            int current = gridToIndex(x, y);
            
            // Connect to neighbors (4-connected)
            std::vector<std::pair<int, int>> neighbors = {
                {x + 1, y}, {x - 1, y}, {x, y + 1}, {x, y - 1}
            };
            
            for (auto [nx, ny] : neighbors) {
                if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                    int neighbor = gridToIndex(nx, ny);
                    addConnection(current, neighbor, connectivity_config_.synaptic_weight, 
                                connectivity_config_.synaptic_delay);
                }
            }
        }
    }
}

void NeuronNetwork::buildTorusConnections() {
    int width = topology_config_.width;
    int height = topology_config_.height;
    
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            int current = gridToIndex(x, y);
            
            // Connect to neighbors with wraparound
            std::vector<std::pair<int, int>> neighbors = {
                {(x + 1) % width, y}, {(x - 1 + width) % width, y},
                {x, (y + 1) % height}, {x, (y - 1 + height) % height}
            };
            
            for (auto [nx, ny] : neighbors) {
                int neighbor = gridToIndex(nx, ny);
                addConnection(current, neighbor, connectivity_config_.synaptic_weight,
                            connectivity_config_.synaptic_delay);
            }
        }
    }
}

void NeuronNetwork::updateSynapticInputs() {
    // New approach: Use neuron-centric synaptic dynamics
    // Each neuron manages its own "ionic bucket" with temporal dynamics
    
    for (const auto& conn : connections_) {
        if (conn.active && conn.from_neuron < static_cast<int>(neurons_.size()) && 
            conn.to_neuron < static_cast<int>(neurons_.size())) {
            
            const auto& source = neurons_[conn.from_neuron];
            auto& target = neurons_[conn.to_neuron];
            
            // When presynaptic neuron spikes, add conductance to postsynaptic "ionic bucket"
            if (source.detectSpike()) {
                // Add synaptic input with realistic kinetics
                double tau_decay = 2.0;  // 2ms decay time constant
                double E_reversal = 0.0; // Excitatory synapse (0mV)
                
                target.addSynapticInput(conn.weight, tau_decay, E_reversal);
                
                // Debug output for first few spikes
                static int spike_count = 0;
                if (spike_count < 10) {
                    std::cout << "Synaptic spike: neuron " << conn.from_neuron 
                             << " -> neuron " << conn.to_neuron 
                             << " (weight: " << conn.weight << ", tau: " << tau_decay << "ms)\n";
                    spike_count++;
                }
            }
        }
    }
    
    // Note: Synaptic decay happens automatically in each neuron's step() method
    // No need to manually set external currents - synaptic current is calculated
    // based on conductance and driving force in getSynapticCurrent()
}

void NeuronNetwork::updateNetworkStimulus() {
    if (!current_stimulus_.active) return;
    
    double elapsed = simulation_time_ - current_stimulus_.start_time;
    
    if (elapsed > current_stimulus_.duration) {
        current_stimulus_.active = false;
        return;
    }
    
    switch (current_stimulus_.pattern) {
        case NetworkStimulus::SINGLE_SITE:
            for (int neuron_idx : current_stimulus_.target_neurons) {
                stimulateNeuron(neuron_idx, current_stimulus_.current, 0.1);  // Short pulse
            }
            break;
            
        case NetworkStimulus::TRAVELING_WAVE:
            if (topology_config_.type == TopologyType::RING) {
                // Calculate which neuron should be stimulated based on wave progression
                double wave_position = current_stimulus_.wave_velocity * elapsed;
                int neuron_idx = static_cast<int>(wave_position) % static_cast<int>(neurons_.size());
                stimulateNeuron(neuron_idx, current_stimulus_.current, 0.1);
            }
            break;
            
        case NetworkStimulus::SPATIAL_GRADIENT:
            // Apply gradient across all neurons
            for (int i = 0; i < static_cast<int>(neurons_.size()); ++i) {
                double factor = static_cast<double>(i) / (neurons_.size() - 1);
                double current = current_stimulus_.current * factor;
                stimulateNeuron(i, current, 0.1);
            }
            break;
            
        case NetworkStimulus::RANDOM:
            // Random stimulation
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_real_distribution<> dis(0.0, 1.0);
            
            for (int i = 0; i < static_cast<int>(neurons_.size()); ++i) {
                if (dis(gen) < 0.1) {  // 10% chance per neuron
                    stimulateNeuron(i, current_stimulus_.current, 0.1);
                }
            }
            break;
    }
}

int NeuronNetwork::gridToIndex(int x, int y) const {
    return y * topology_config_.width + x;
}

std::pair<int, int> NeuronNetwork::indexToGrid(int index) const {
    int x = index % topology_config_.width;
    int y = index / topology_config_.width;
    return {x, y};
}

std::vector<int> NeuronNetwork::getNeighbors(int neuron_index) const {
    std::vector<int> neighbors;
    
    switch (topology_config_.type) {
        case TopologyType::RING:
            {
                int n = static_cast<int>(neurons_.size());
                neighbors.push_back((neuron_index + 1) % n);
                neighbors.push_back((neuron_index - 1 + n) % n);
            }
            break;
            
        case TopologyType::GRID:
        case TopologyType::TORUS:
            {
                auto [x, y] = indexToGrid(neuron_index);
                
                std::vector<std::pair<int, int>> neighbor_coords;
                if (topology_config_.type == TopologyType::GRID) {
                    neighbor_coords = {{x+1, y}, {x-1, y}, {x, y+1}, {x, y-1}};
                } else { // TORUS
                    int w = topology_config_.width;
                    int h = topology_config_.height;
                    neighbor_coords = {
                        {(x+1) % w, y}, {(x-1+w) % w, y},
                        {x, (y+1) % h}, {x, (y-1+h) % h}
                    };
                }
                
                for (auto [nx, ny] : neighbor_coords) {
                    if (topology_config_.type == TopologyType::GRID) {
                        if (nx >= 0 && nx < topology_config_.width && 
                            ny >= 0 && ny < topology_config_.height) {
                            neighbors.push_back(gridToIndex(nx, ny));
                        }
                    } else {
                        neighbors.push_back(gridToIndex(nx, ny));
                    }
                }
            }
            break;
    }
    
    return neighbors;
}

double NeuronNetwork::getSpatialFactor(int neuron_index, const SpatialPattern& pattern) const {
    switch (topology_config_.type) {
        case TopologyType::RING:
            return static_cast<double>(neuron_index) / (neurons_.size() - 1);
            
        case TopologyType::GRID:
        case TopologyType::TORUS:
            {
                auto [x, y] = indexToGrid(neuron_index);
                // Use diagonal distance for 2D gradients
                double dx = static_cast<double>(x) / (topology_config_.width - 1);
                double dy = static_cast<double>(y) / (topology_config_.height - 1);
                return std::sqrt(dx * dx + dy * dy) / std::sqrt(2.0);
            }
    }
    
    return 0.0;
}

// NetworkAnalyzer Implementation

NetworkAnalyzer::NetworkAnalyzer(const NeuronNetwork* network) 
    : network_(network), last_update_time_(0.0) {
}

void NetworkAnalyzer::update() {
    double current_time = network_->getTime();
    size_t neuron_count = network_->getNeuronCount();
    recent_spike_times_.resize(neuron_count);

    // Update spike detection
    for (size_t i = 0; i < network_->getNeuronCount(); ++i) {
        const auto& neuron = network_->getNeuron((int) i);
        if (neuron.detectSpike()) {
            recent_spike_times_[i].push_back(current_time);
            
            // Keep only recent spikes (last 2 seconds)
            auto& spikes = recent_spike_times_[i];
            spikes.erase(
                std::remove_if(spikes.begin(), spikes.end(),
                    [current_time](double spike_time) {
                        return (current_time - spike_time) > 2000.0;  // 2 seconds
                    }),
                spikes.end());
        }
    }
    
    last_update_time_ = current_time;
}

std::vector<std::vector<double>> NetworkAnalyzer::getAllSpikeTimes(double window_ms) const {
    std::vector<std::vector<double>> windowed_spikes;
    windowed_spikes.reserve(recent_spike_times_.size());
    
    double current_time = network_->getTime();
    double window_start = current_time - window_ms;
    
    for (const auto& neuron_spikes : recent_spike_times_) {
        std::vector<double> windowed;
        for (double spike_time : neuron_spikes) {
            if (spike_time >= window_start) {
                windowed.push_back(spike_time);
            }
        }
        windowed_spikes.push_back(windowed);
    }
    
    return windowed_spikes;
}

double NetworkAnalyzer::calculateCrossCorrelation(int neuron1, int neuron2, double window_ms) const {
    if (neuron1 >= static_cast<int>(recent_spike_times_.size()) || 
        neuron2 >= static_cast<int>(recent_spike_times_.size())) {
        return 0.0;
    }
    
    auto windowed_spikes = getAllSpikeTimes(window_ms);
    const auto& spikes1 = windowed_spikes[neuron1];
    const auto& spikes2 = windowed_spikes[neuron2];
    
    if (spikes1.empty() || spikes2.empty()) return 0.0;
    
    // Simple cross-correlation measure
    double correlation = 0.0;
    double time_window = 10.0;  // 10ms window for correlation
    
    for (double t1 : spikes1) {
        for (double t2 : spikes2) {
            if (std::abs(t1 - t2) < time_window) {
                correlation += 1.0;
            }
        }
    }
    
    // Normalize by geometric mean of spike counts
    double norm = std::sqrt(spikes1.size() * spikes2.size());
    return (norm > 0) ? correlation / norm : 0.0;
}

NetworkMotorPattern NetworkAnalyzer::detectMotorPattern() const {
    NetworkMotorPattern pattern;
    
    // Get recent firing rates
    std::vector<double> firing_rates;
    for (size_t i = 0; i < network_->getNeuronCount(); ++i) {
        const auto& neuron = network_->getNeuron(i);
        firing_rates.push_back(neuron.getHistory().calculateFiringFrequency(500.0));
    }
    
    // Calculate mean firing rate
    double mean_rate = std::accumulate(firing_rates.begin(), firing_rates.end(), 0.0) / firing_rates.size();
    
    if (mean_rate > 5.0) {  // Minimum activity threshold
        // Check for oscillations
        pattern.frequency = mean_rate;
        pattern.coherence = network_->getSynchronizationIndex();
        
        if (pattern.coherence > 0.8) {
            pattern.type = NetworkMotorPattern::OSCILLATION;
        } else if (pattern.coherence > 0.5) {
            // Check for traveling wave
            auto wave = network_->detectTravelingWave();
            if (wave.detected) {
                pattern.type = NetworkMotorPattern::TRAVELING_WAVE;
            } else {
                pattern.type = NetworkMotorPattern::STANDING_WAVE;
            }
        }
    }
    
    return pattern;
}

double NetworkAnalyzer::calculateNetworkEntropy() const {
    // Simple entropy measure based on voltage distribution
    std::vector<double> voltages;
    for (size_t i = 0; i < network_->getNeuronCount(); ++i) {
        voltages.push_back(network_->getNeuron(i).getVoltage());
    }
    
    // Bin voltages and calculate entropy
    const int num_bins = 20;
    std::vector<int> histogram(num_bins, 0);
    
    double min_v = *std::min_element(voltages.begin(), voltages.end());
    double max_v = *std::max_element(voltages.begin(), voltages.end());
    double bin_width = (max_v - min_v) / num_bins;
    
    if (bin_width == 0) return 0.0;
    
    for (double v : voltages) {
        int bin = std::min(num_bins - 1, static_cast<int>((v - min_v) / bin_width));
        histogram[bin]++;
    }
    
    // Calculate entropy
    double entropy = 0.0;
    double total = voltages.size();
    for (int count : histogram) {
        if (count > 0) {
            double p = count / total;
            entropy -= p * std::log2(p);
        }
    }
    
    return entropy;
}

double NetworkAnalyzer::calculateInformationFlow(int source_region, int target_region) const {
    // Placeholder for information flow calculation
    // In a full implementation, this would use transfer entropy or similar measures
    return calculateCrossCorrelation(source_region, target_region, 1000.0);
}

} // namespace consciousness
