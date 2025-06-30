//
// HH Neuron Implementation - Application Layer
//

#include "hh_neuron.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace consciousness {

// HHNeuron Implementation

HHNeuron::HHNeuron(const Parameters& params) 
    : params_(params)
    , background_current_(0.0)
    , external_current_(0.0)
    , history_(std::make_unique<NeuronHistory>())
{
    reset();
}

HHNeuron::HHNeuron(HHNeuron&& other) noexcept
    : params_(std::move(other.params_))
    , state_(std::move(other.state_))
    , prev_state_(std::move(other.prev_state_))
    , stimulus_(std::move(other.stimulus_))
    , background_current_(other.background_current_)
    , external_current_(other.external_current_)
    , history_(std::move(other.history_))
{
    // other is now in a valid but unspecified state
}

HHNeuron& HHNeuron::operator=(HHNeuron&& other) noexcept {
    if (this != &other) {
        params_ = std::move(other.params_);
        state_ = std::move(other.state_);
        prev_state_ = std::move(other.prev_state_);
        stimulus_ = std::move(other.stimulus_);
        background_current_ = other.background_current_;
        external_current_ = other.external_current_;
        history_ = std::move(other.history_);
        // other is now in a valid but unspecified state
    }
    return *this;
}

HHNeuron::~HHNeuron() = default;

void HHNeuron::step(double dt) {
    // Store previous state for spike detection
    prev_state_ = state_;
    
    // Update stimulus state
    updateStimulus();
    
    // Update synaptic dynamics
    updateSynapticDynamics(dt);
    
    // Get current voltage for rate constant calculations
    const double V = state_.V;
    
    // Calculate ion currents
    const double i_Na = I_Na();
    const double i_K = I_K();
    const double i_L = I_L();
    
    // Get total external current (background + stimulus + synaptic)
    const double I_ext = getTotalExternalCurrent() + getSynapticCurrent();
    
    // Update membrane potential
    const double dV_dt = (I_ext - i_Na - i_K - i_L) / params_.C_m;
    state_.V += dV_dt * dt;
    
    // Update gating variables
    const double am = alpha_m(V);
    const double bm = beta_m(V);
    const double ah = alpha_h(V);
    const double bh = beta_h(V);
    const double an = alpha_n(V);
    const double bn = beta_n(V);
    
    const double dm_dt = am * (1.0 - state_.m) - bm * state_.m;
    const double dh_dt = ah * (1.0 - state_.h) - bh * state_.h;
    const double dn_dt = an * (1.0 - state_.n) - bn * state_.n;
    
    state_.m += dm_dt * dt;
    state_.h += dh_dt * dt;
    state_.n += dn_dt * dt;
    
    // Clamp gating variables to [0,1]
    state_.m = std::clamp(state_.m, 0.0, 1.0);
    state_.h = std::clamp(state_.h, 0.0, 1.0);
    state_.n = std::clamp(state_.n, 0.0, 1.0);
    
    // Update time
    state_.time += dt;
    
    // Update history
    updateHistory();
}

void HHNeuron::reset() {
    // Reset to resting state
    state_.V = -65.0;
    state_.m = 0.0529;
    state_.h = 0.5961;
    state_.n = 0.3177;
    state_.time = 0.0;
    
    prev_state_ = state_;
    
    // Reset stimulus
    stimulus_.active = false;
    stimulus_.current = 0.0;
    stimulus_.start_time = 0.0;
    stimulus_.duration = 0.0;
    
    background_current_ = 0.0;
    external_current_ = 0.0;
    
    // Reset synaptic state - clear the "ionic bucket"
    synaptic_state_.conductance = 0.0;
    synaptic_state_.decay_tau = 2.0;
    synaptic_state_.reversal_potential = 0.0;
    
    // Clear history
    history_->clear();
}

void HHNeuron::stimulate(double current, double duration) {
    stimulus_.current = current;
    stimulus_.start_time = state_.time;
    stimulus_.duration = duration;
    stimulus_.active = true;
}

// Rate constants for gating variables (Hodgkin-Huxley 1952)
double HHNeuron::alpha_m(double V) const {
    if (std::abs(V + 40.0) < 1e-6) {
        return 1.0; // Limit as V approaches -40
    }
    return 0.1 * (V + 40.0) / (1.0 - std::exp(-(V + 40.0) / 10.0));
}

double HHNeuron::beta_m(double V) const {
    return 4.0 * std::exp(-(V + 65.0) / 18.0);
}

double HHNeuron::alpha_h(double V) const {
    return 0.07 * std::exp(-(V + 65.0) / 20.0);
}

double HHNeuron::beta_h(double V) const {
    return 1.0 / (1.0 + std::exp(-(V + 35.0) / 10.0));
}

double HHNeuron::alpha_n(double V) const {
    if (std::abs(V + 55.0) < 1e-6) {
        return 0.1; // Limit as V approaches -55
    }
    return 0.01 * (V + 55.0) / (1.0 - std::exp(-(V + 55.0) / 10.0));
}

double HHNeuron::beta_n(double V) const {
    return 0.125 * std::exp(-(V + 65.0) / 80.0);
}

// Ion currents
double HHNeuron::I_Na() const {
    return params_.g_Na * std::pow(state_.m, 3) * state_.h * (state_.V - params_.E_Na);
}

double HHNeuron::I_K() const {
    return params_.g_K * std::pow(state_.n, 4) * (state_.V - params_.E_K);
}

double HHNeuron::I_L() const {
    return params_.g_L * (state_.V - params_.E_L);
}

double HHNeuron::getTotalExternalCurrent() const {
    double total = background_current_ + external_current_;
    
    // Add stimulus if active
    if (stimulus_.active) {
        const double elapsed = state_.time - stimulus_.start_time;
        if (elapsed >= 0.0 && elapsed <= stimulus_.duration) {
            total += stimulus_.current;
        }
    }
    
    return total;
}

HHNeuron::Phase HHNeuron::getCurrentPhase() const {
    const double V = state_.V;
    
    if (V < -60.0) return Phase::Resting;
    if (V >= -60.0 && V < -40.0) return Phase::Threshold;
    if (V >= -40.0 && V < 20.0) return Phase::Depolarization;
    if (V >= 20.0) return Phase::Overshoot;
    if (V < -60.0 && V >= -80.0) return Phase::Repolarization;
    if (V < -80.0) return Phase::Hyperpolarization;
    
    return Phase::Unknown;
}

std::string HHNeuron::getPhaseString() const {
    switch (getCurrentPhase()) {
        case Phase::Resting: return "Resting";
        case Phase::Threshold: return "Threshold";
        case Phase::Depolarization: return "Depolarization";
        case Phase::Overshoot: return "Overshoot";
        case Phase::Repolarization: return "Repolarization";
        case Phase::Hyperpolarization: return "Hyperpolarization";
        default: return "Unknown";
    }
}

bool HHNeuron::detectSpike() const {
    // Upward zero crossing detection
    return prev_state_.V <= 0.0 && state_.V > 0.0;
}

double HHNeuron::calculateInstantaneousPhase() const {
    // Map voltage to phase for consciousness analysis
    // Normalize voltage to [0,1] range, then to [-π, π]
    const double normalized_V = (state_.V + 65.0) / 130.0; // Assuming -65 to +65 mV range
    const double clamped_V = std::clamp(normalized_V, 0.0, 1.0);
    return 2.0 * M_PI * clamped_V - M_PI;
}

ConsciousnessState HHNeuron::getConsciousnessState() const {
    ConsciousnessState cs;
    cs.voltage = state_.V;
    cs.phase = calculateInstantaneousPhase();
    cs.discrete_phase = getCurrentPhase();
    cs.activity_level = std::abs(state_.V + 65.0) / 130.0; // Normalized activity
    cs.complexity = 0.5; // Simple measure for single neuron
    cs.spiking = detectSpike();
    cs.time = state_.time;
    
    // 0-brane specific measures
    cs.membrane_coherence = 1.0; // Single neuron is always coherent with itself
    cs.information_content = std::abs(state_.V + 65.0) / 130.0; // Activity-based measure
    
    return cs;
}

void HHNeuron::clearHistory() {
    history_->clear();
}

void HHNeuron::setHistoryCapacity(size_t capacity) {
    history_->setCapacity(capacity);
}

void HHNeuron::updateStimulus() {
    // Check if stimulus should be deactivated
    if (stimulus_.active) {
        const double elapsed = state_.time - stimulus_.start_time;
        if (elapsed > stimulus_.duration) {
            stimulus_.active = false;
        }
    }
}

void HHNeuron::updateHistory() {
    history_->addPoint(*this);
}

// Synaptic dynamics implementation - the "ionic bucket" model

void HHNeuron::addSynapticInput(double weight, double tau, double E_rev) {
    // Add conductance increment to the "ionic bucket"
    synaptic_state_.conductance += weight;
    
    // Update synaptic parameters if different from current
    if (tau != synaptic_state_.decay_tau) {
        synaptic_state_.decay_tau = tau;
    }
    if (E_rev != synaptic_state_.reversal_potential) {
        synaptic_state_.reversal_potential = E_rev;
    }
}

void HHNeuron::updateSynapticDynamics(double dt) {
    // Exponential decay of synaptic conductance
    // g(t) = g0 * exp(-t/tau)
    // Discrete approximation: g(t+dt) = g(t) * exp(-dt/tau)
    if (synaptic_state_.conductance > 1e-6) { // Only update if significant conductance
        double decay_factor = std::exp(-dt / synaptic_state_.decay_tau);
        synaptic_state_.conductance *= decay_factor;
        
        // Prevent underflow
        if (synaptic_state_.conductance < 1e-6) {
            synaptic_state_.conductance = 0.0;
        }
    }
}

double HHNeuron::getSynapticCurrent() const {
    // I_syn = g_syn * (V - E_syn)
    // This is the realistic synaptic current based on driving force
    return synaptic_state_.conductance * (state_.V - synaptic_state_.reversal_potential);
}

void HHNeuron::updateSynapticState(double dt) {
    // This is an alias for updateSynapticDynamics for internal use
    updateSynapticDynamics(dt);
}

// NeuronHistory Implementation

NeuronHistory::NeuronHistory(size_t capacity) 
    : capacity_(capacity)
    , write_index_(0)
    , wrapped_(false)
{
    data_.reserve(capacity);
}

void NeuronHistory::addPoint(const HHNeuron& neuron) {
    DataPoint point;
    point.time = neuron.getState().time;
    point.V = neuron.getState().V;
    point.m = neuron.getState().m;
    point.h = neuron.getState().h;
    point.n = neuron.getState().n;
    point.I_Na = neuron.I_Na();
    point.I_K = neuron.I_K();
    point.I_L = neuron.I_L();
    point.I_ext = neuron.getTotalExternalCurrent();
    point.phase = neuron.getCurrentPhase();
    
    if (data_.size() < capacity_) {
        data_.push_back(point);
    } else {
        data_[write_index_] = point;
        write_index_ = (write_index_ + 1) % capacity_;
        if (write_index_ == 0) {
            wrapped_ = true;
        }
    }
}

std::vector<NeuronHistory::DataPoint> NeuronHistory::getTimeWindow(double current_time, double window_ms) const {
    std::vector<DataPoint> result;
    const double start_time = current_time - window_ms;
    
    for (const auto& point : data_) {
        if (point.time >= start_time && point.time <= current_time) {
            result.push_back(point);
        }
    }
    
    return result;
}

void NeuronHistory::clear() {
    data_.clear();
    write_index_ = 0;
    wrapped_ = false;
}

void NeuronHistory::setCapacity(size_t capacity) {
    capacity_ = capacity;
    if (data_.size() > capacity) {
        data_.resize(capacity);
    }
    data_.reserve(capacity);
    write_index_ = 0;
    wrapped_ = false;
}

std::vector<double> NeuronHistory::getSpikeTimes() const {
    std::vector<double> spike_times;
    
    for (size_t i = 1; i < data_.size(); ++i) {
        // Detect spikes (upward zero crossing)
        if (data_[i-1].V <= 0.0 && data_[i].V > 0.0) {
            spike_times.push_back(data_[i].time);
        }
    }
    
    return spike_times;
}

double NeuronHistory::calculateFiringFrequency(double window_ms) const {
    if (data_.empty()) return 0.0;
    
    const double current_time = data_.back().time;
    const auto spikes = getSpikeTimes();
    
    // Count spikes in the window
    int spike_count = 0;
    for (double spike_time : spikes) {
        if (spike_time >= current_time - window_ms) {
            spike_count++;
        }
    }
    
    // Convert to frequency (Hz)
    return (spike_count * 1000.0) / window_ms;
}

} // namespace consciousness
