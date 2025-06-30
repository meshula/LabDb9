/*
 * Visualization - Rendering Layer (C with Sokol)
 * 
 * This is the Rendering Layer in the triadic architecture:
 * - Pure C implementation using sokol graphics
 * - Real-time neuron visualization
 * - High-performance rendering
 */

#pragma once

#include "neuron_viz_data.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Visualization configuration */
typedef struct {
    int window_width;
    int window_height;
    float dpi_scale;
    int msaa_samples;
} viz_config_t;

/* Color definitions */
typedef struct {
    float r, g, b, a;
} color_t;

/* Neuron visualization colors */
typedef struct {
    color_t voltage_trace;
    color_t m_gate_trace;
    color_t h_gate_trace;
    color_t n_gate_trace;
    color_t background;
    color_t grid;
    color_t text;
    color_t stimulus_indicator;
    color_t spike_indicator;
} neuron_colors_t;

/* Rendering state */
typedef struct viz_state_s viz_state_t;

/* Initialization and cleanup */
viz_state_t* viz_init(const viz_config_t* config);
void viz_shutdown(viz_state_t* viz);

/* Main rendering functions */
void viz_begin_frame(viz_state_t* viz);
void viz_end_frame(viz_state_t* viz);

/* Neuron-specific rendering */
void viz_render_neuron_traces(viz_state_t* viz, const neuron_viz_data_t* data);
void viz_render_membrane_indicator(viz_state_t* viz, const neuron_viz_data_t* data);
void viz_render_phase_indicator(viz_state_t* viz, const neuron_viz_data_t* data);
void viz_render_stimulus_indicator(viz_state_t* viz, const neuron_viz_data_t* data);

/* Utility rendering functions */
void viz_render_grid(viz_state_t* viz, float x, float y, float w, float h);
void viz_render_axes(viz_state_t* viz, float x, float y, float w, float h, 
                     float min_x, float max_x, float min_y, float max_y);
void viz_render_text(viz_state_t* viz, const char* text, float x, float y, color_t color);

/* Color utilities */
neuron_colors_t viz_get_default_colors(void);
color_t viz_voltage_to_color(float voltage);
color_t viz_lerp_color(color_t a, color_t b, float t);

/* Configuration */
void viz_set_colors(viz_state_t* viz, const neuron_colors_t* colors);
void viz_resize(viz_state_t* viz, int width, int height);

#ifdef __cplusplus
}
#endif
