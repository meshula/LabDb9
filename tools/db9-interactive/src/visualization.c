/*
 * Visualization Implementation - Rendering Layer (C with Sokol)
 * 
 * Simplified implementation that provides the C interface for the visualization layer
 * while delegating actual rendering to ImGui in the neuron_ui.cpp
 */

#include "visualization.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Sokol includes
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_glue.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Internal state structure */
struct viz_state_s {
    sg_pass_action pass_action;
    neuron_colors_t colors;
    int width, height;
    float dpi_scale;
};

/* Default color scheme */
neuron_colors_t viz_get_default_colors(void) {
    neuron_colors_t colors = {
        .voltage_trace = {0.29f, 0.62f, 1.0f, 1.0f},     // Blue
        .m_gate_trace = {1.0f, 0.42f, 0.42f, 1.0f},      // Red
        .h_gate_trace = {0.31f, 0.80f, 0.78f, 1.0f},     // Cyan
        .n_gate_trace = {0.27f, 0.72f, 0.82f, 1.0f},     // Light blue
        .background = {0.06f, 0.06f, 0.11f, 1.0f},       // Dark blue
        .grid = {1.0f, 1.0f, 1.0f, 0.1f},                // Faint white
        .text = {0.8f, 0.8f, 0.8f, 1.0f},                // Light gray
        .stimulus_indicator = {1.0f, 0.8f, 0.2f, 1.0f},  // Yellow
        .spike_indicator = {1.0f, 1.0f, 1.0f, 0.8f}      // White
    };
    return colors;
}

/* Color utilities */
color_t viz_voltage_to_color(float voltage) {
    // Map voltage to color: blue (hyperpolarized) to red (depolarized)
    float normalized = (voltage + 100.0f) / 200.0f; // -100mV to +100mV -> 0 to 1
    normalized = fmaxf(0.0f, fminf(1.0f, normalized));
    
    color_t color;
    if (normalized < 0.5f) {
        // Blue to cyan
        float t = normalized * 2.0f;
        color.r = 0.1f + t * 0.2f;
        color.g = 0.3f + t * 0.5f;
        color.b = 1.0f;
        color.a = 1.0f;
    } else {
        // Cyan to red
        float t = (normalized - 0.5f) * 2.0f;
        color.r = 0.3f + t * 0.7f;
        color.g = 0.8f - t * 0.8f;
        color.b = 1.0f - t * 1.0f;
        color.a = 1.0f;
    }
    return color;
}

color_t viz_lerp_color(color_t a, color_t b, float t) {
    t = fmaxf(0.0f, fminf(1.0f, t));
    color_t result;
    result.r = a.r + t * (b.r - a.r);
    result.g = a.g + t * (b.g - a.g);
    result.b = a.b + t * (b.b - a.b);
    result.a = a.a + t * (b.a - a.a);
    return result;
}

/* Initialization */
viz_state_t* viz_init(const viz_config_t* config) {
    viz_state_t* viz = malloc(sizeof(viz_state_t));
    if (!viz) {
        return NULL;
    }
    
    viz->width = config->window_width;
    viz->height = config->window_height;
    viz->dpi_scale = config->dpi_scale;
    viz->colors = viz_get_default_colors();
    
    // Set up pass action
    viz->pass_action = (sg_pass_action) {
        .colors[0] = {
            .load_action = SG_LOADACTION_CLEAR,
            .clear_value = { 
                viz->colors.background.r, 
                viz->colors.background.g, 
                viz->colors.background.b, 
                viz->colors.background.a
            }
        }
    };
    
    printf("Visualization layer initialized: %dx%d (DPI: %.1f)\n", 
           viz->width, viz->height, viz->dpi_scale);
    
    return viz;
}

void viz_shutdown(viz_state_t* viz) {
    if (viz) {
        free(viz);
    }
}

/* Frame management */
void viz_begin_frame(viz_state_t* viz) {
    if (!viz) return;

    sg_pass pass = {
        .action    = viz->pass_action,
        .swapchain = sglue_swapchain(),
    };
    sg_begin_pass(&pass);
}

void viz_end_frame(viz_state_t* viz) {
    if (!viz) return;
    
    sg_end_pass();
    // Note: sg_commit() is called in main.cpp after ImGui rendering
}

/* Configuration */
void viz_set_colors(viz_state_t* viz, const neuron_colors_t* colors) {
    if (viz && colors) {
        viz->colors = *colors;
        
        // Update pass action background color
        viz->pass_action.colors[0].clear_value = (sg_color) { 
                colors->background.r, 
                colors->background.g, 
                colors->background.b, 
                colors->background.a
            };
    }
}

void viz_resize(viz_state_t* viz, int width, int height) {
    if (viz) {
        viz->width = width;
        viz->height = height;
    }
}

/* 
 * Rendering functions - Interface delegation
 * 
 * The actual visualization is handled by ImGui in neuron_ui.cpp
 * These functions provide the C interface for completeness
 */

void viz_render_neuron_traces(viz_state_t* viz, const neuron_viz_data_t* data) {
    if (!viz || !data) return;
    
    // Debug output occasionally to confirm the function is being called
    static int frame_count = 0;
    if (++frame_count % 300 == 0) {  // Every 5 seconds at 60fps
        printf("Membrane dynamics active: V=%.2f mV, t=%.2f ms, %d history points\n", 
               data->voltage, data->time, data->history_count);
    }
}

void viz_render_membrane_indicator(viz_state_t* viz, const neuron_viz_data_t* data) {
    if (!viz || !data) return;
    // Handled by ImGui in neuron_ui.cpp
}

void viz_render_phase_indicator(viz_state_t* viz, const neuron_viz_data_t* data) {
    if (!viz || !data) return;
    // Handled by ImGui in neuron_ui.cpp
}

void viz_render_stimulus_indicator(viz_state_t* viz, const neuron_viz_data_t* data) {
    if (!viz || !data) return;
    // Handled by ImGui in neuron_ui.cpp
}

void viz_render_grid(viz_state_t* viz, float x, float y, float w, float h) {
    if (!viz) return;
    // Handled by ImGui in neuron_ui.cpp
}

void viz_render_axes(viz_state_t* viz, float x, float y, float w, float h, 
                     float min_x, float max_x, float min_y, float max_y) {
    if (!viz) return;
    // Handled by ImGui in neuron_ui.cpp
}

void viz_render_text(viz_state_t* viz, const char* text, float x, float y, color_t color) {
    if (!viz || !text) return;
    // Handled by ImGui in neuron_ui.cpp
}
