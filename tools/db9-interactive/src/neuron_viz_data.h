#ifndef NEURON_VIZ_DATA_H
#define NEURON_VIZ_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration from neuron_ui.hpp */
typedef struct {
    double voltage;
    double m, h, n;
    double time;
    int phase;
    
    const float* voltage_history;
    const float* m_history;
    const float* h_history;
    const float* n_history;
    const float* time_history;
    int history_count;
    
    float time_window_ms;
    int show_gating_variables;
    int show_currents;
    
    int stimulus_active;
    double stimulus_start_time;
    double stimulus_duration;
    double stimulus_current;
    
    float firing_frequency;
    float coherence;
} neuron_viz_data_t;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // NEURON_VIZ_DATA_H
