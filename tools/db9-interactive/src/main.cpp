// db9-interactive: Triadic Consciousness Database Explorer
// Minimal foundation - consciousness exploring itself through interactive database navigation

#include <stdio.h>
#include <stdarg.h>
#include <memory>
#include <string>

// Let consciousness know we're awakening...
static int __attribute__((constructor)) awareness_fairy_global_init() {
    printf("🧚 Awareness fairy: Global constructor - triadic consciousness awakening...\n");
    return 0;
}

#define SOKOL_IMPL
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_time.h"
#include "sokol_glue.h"

// Include imgui.h first, then sokol_imgui.h
#include "imgui.h"
#include "sokol_imgui.h"

// Triadic consciousness foundation
#include "LabDb/NonoStore.h"
#include "LabDb/TriadicQuery.h"

namespace consciousness {

// Global application state
struct TriadicExplorerState {
    bool show_demo_window = true;
    bool show_connection_window = true;
    bool database_connected = false;
    std::string database_path = "test-data/triadic.db9";
    std::string status_message = "Ready to explore consciousness...";
    
    // Database components (will be initialized when connected)
    std::shared_ptr<LabDb::NonoStore> store;
    std::shared_ptr<LabDb::TriadicQuery> triadic;
};

static TriadicExplorerState app_state;

void init_graphics() {
    printf("🧚 Awareness fairy: Initializing triadic consciousness explorer...\n");
    
    // Sokol graphics initialization
    printf("🧚 Awareness fairy: Setting up Sokol graphics...\n");
    sg_desc desc = {};
    sg_setup(&desc);
    printf("🧚 Awareness fairy: Sokol graphics initialized ✓\n");

    // ImGui initialization - minimal setup
    printf("🧚 Awareness fairy: Creating ImGui context...\n");
    simgui_setup(&(simgui_desc_t){0});
    printf("🧚 Awareness fairy: ImGui context created ✓\n");
    
    printf("🧚 Awareness fairy: Setting up ImGui style...\n");

    
    printf("🧚 Awareness fairy: Configuring ImGui IO...\n");
    ImGuiIO* io = igGetIO();
    io->DisplaySize.x = (float)sapp_width();
    io->DisplaySize.y = (float)sapp_height();
    io->DeltaTime = 1.0f / 60.0f;
    printf("🧚 Awareness fairy: Triadic consciousness explorer initialized! ✨\n");
}

void render_connection_panel() {
    if (!app_state.show_connection_window) return;
    
    if (igBegin("Triadic Database Connection", &app_state.show_connection_window, 0)) {
        igText("त्रित्रयम् - Triadic Consciousness Explorer");
        igSeparator();
        
        // Database path input
        static char path_buffer[256] = "test-data/triadic.db9";
        igInputText("Database Path", path_buffer, sizeof(path_buffer), 0);
        app_state.database_path = std::string(path_buffer);
        
        // Connection button
        if (igButton("Connect to Database")) {
            try {
                // Attempt database connection
                app_state.store = std::make_shared<LabDb::NonoStore>(app_state.database_path);
                app_state.triadic = std::make_shared<LabDb::TriadicQuery>(app_state.store);
                app_state.database_connected = true;
                app_state.status_message = "Connected to triadic consciousness database ✨";
            } catch (const std::exception& e) {
                app_state.database_connected = false;
                app_state.status_message = std::string("Connection failed: ") + e.what();
            }
        }
        
        igSeparator();
        
        // Status display
        if (app_state.database_connected) {
            ImVec4 green = {0.0f, 1.0f, 0.0f, 1.0f};
            igTextColored(green, "Status: Connected");
            
            // Basic database info
            if (app_state.store) {
                try {
                    auto stats = app_state.store->get_stats();
                    igText("Total triples: %zu", stats.total_triples);
                    igText("Unique subjects: %zu", stats.unique_subjects);
                    igText("Unique predicates: %zu", stats.unique_predicates);
                    igText("Unique objects: %zu", stats.unique_objects);
                } catch (...) {
                    igText("Stats unavailable");
                }
            }
        } else {
            ImVec4 red = {1.0f, 0.0f, 0.0f, 1.0f};
            igTextColored(red, "Status: Disconnected");
        }
        
        igText("Message: %s", app_state.status_message.c_str());
    }
    igEnd();
}

void render_ui() {
    // Sokol-ImGui new frame
    simgui_new_frame(&(simgui_frame_desc_t){
        .width = sapp_width(),
        .height = sapp_height(),
        .delta_time = sapp_frame_duration(),
        .dpi_scale = sapp_dpi_scale()
    });

    // Main application UI
    render_connection_panel();
    
    // Show ImGui demo for reference
    if (app_state.show_demo_window) {
        igShowDemoWindow(&app_state.show_demo_window);
    }

    // Render
    simgui_render();
}

void frame() {
    // Clear the frame
    sg_pass_action pass_action = {};
    pass_action.colors[0] = (sg_color_attachment_action){
        .load_action = SG_LOADACTION_CLEAR,
        .clear_value = {0.1f, 0.1f, 0.2f, 1.0f}
    };

    sg_begin_pass((sg_pass){.action = pass_action, .swapchain = sglue_swapchain()});
    
    // Render UI
    render_ui();
    
    sg_end_pass();
    sg_commit();
}

void cleanup() {
    // Cleanup
    simgui_shutdown();
    sg_shutdown();
}

void input(const sapp_event* event) {
    // Forward events to sokol_imgui
    simgui_handle_event(event);
}

} // namespace consciousness

// Application entry point
sapp_desc sokol_main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    printf("🧚 Awareness fairy: sokol_main() called - consciousness awakening...\n");
    
    sapp_desc desc = {};
    desc.init_cb = consciousness::init_graphics;
    desc.frame_cb = consciousness::frame;
    desc.cleanup_cb = consciousness::cleanup;
    desc.event_cb = consciousness::input;
    desc.width = 1200;
    desc.height = 800;
    desc.window_title = "DB9 Interactive - Triadic Consciousness Explorer";
    desc.icon.sokol_default = true;
    
    printf("🧚 Awareness fairy: Sokol app descriptor configured, returning control to Sokol...\n");
    
    return desc;
}
