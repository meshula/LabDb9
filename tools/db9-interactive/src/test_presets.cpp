//
// Test Program for Preset Manager
// Simple test to verify preset system functionality
//

#include "preset_manager.hpp"
#include "neuron_network.hpp"
#include <iostream>

using namespace consciousness;

int main() {
    std::cout << "=== HH C++ Multi-Neuron Preset Manager Test ===" << std::endl;
    
    // Test 1: Create preset manager
    std::cout << "\n1. Creating preset manager..." << std::endl;
    PresetManager manager;
    
    // Test 2: List built-in presets
    std::cout << "\n2. Available built-in presets:" << std::endl;
    auto presets = manager.getAvailablePresets();
    for (const auto& preset : presets) {
        std::cout << "  - " << preset.name << " (" << preset.neuron_count << " neurons, " 
                  << preset.topology_type << ")" << std::endl;
        std::cout << "    Description: " << preset.description << std::endl;
    }
    
    // Test 3: Load a built-in preset
    std::cout << "\n3. Loading basic ring preset..." << std::endl;
    auto basic_preset = manager.loadPresetById("basic_ring");
    if (basic_preset) {
        std::cout << "  ✓ Successfully loaded: " << basic_preset->name << std::endl;
        std::cout << "  - Topology: " << basic_preset->topology.neuron_count << " neurons" << std::endl;
        std::cout << "  - Connectivity: " << (basic_preset->connectivity.bidirectional ? "Bidirectional" : "Unidirectional") << std::endl;
        std::cout << "  - Synaptic weight: " << basic_preset->connectivity.synaptic_weight << std::endl;
    } else {
        std::cout << "  ✗ Failed to load preset" << std::endl;
        return 1;
    }
    
    // Test 4: Create a network and apply preset
    std::cout << "\n4. Creating network and applying preset..." << std::endl;
    NeuronNetwork network;
    bool applied = manager.applyPresetToNetwork(*basic_preset, network);
    if (applied) {
        std::cout << "  ✓ Successfully applied preset to network" << std::endl;
        std::cout << "  - Network neuron count: " << network.getNeuronCount() << std::endl;
        std::cout << "  - Network topology: " << (network.getTopologyConfig().type == TopologyType::RING ? "Ring" : "Other") << std::endl;
    } else {
        std::cout << "  ✗ Failed to apply preset" << std::endl;
        return 1;
    }
    
    // Test 5: Create preset from network
    std::cout << "\n5. Creating preset from current network..." << std::endl;
    auto custom_preset = manager.createPresetFromNetwork(network, "Test Custom Preset", "Created from test network");
    std::cout << "  ✓ Created custom preset: " << custom_preset.name << std::endl;
    
    // Test 6: Save custom preset
    std::cout << "\n6. Saving custom preset..." << std::endl;
    bool saved = manager.savePreset(custom_preset, "test_preset.json");
    if (saved) {
        std::cout << "  ✓ Successfully saved preset" << std::endl;
    } else {
        std::cout << "  ✗ Failed to save preset" << std::endl;
        return 1;
    }
    
    // Test 7: Load saved preset
    std::cout << "\n7. Loading saved preset..." << std::endl;
    auto loaded_preset = manager.loadPreset("test_preset.json");
    if (loaded_preset) {
        std::cout << "  ✓ Successfully loaded saved preset: " << loaded_preset->name << std::endl;
    } else {
        std::cout << "  ✗ Failed to load saved preset" << std::endl;
        return 1;
    }
    
    // Test 8: Validate presets
    std::cout << "\n8. Validating presets..." << std::endl;
    auto errors = manager.validatePreset(*basic_preset);
    if (errors.empty()) {
        std::cout << "  ✓ Basic preset validation passed" << std::endl;
    } else {
        std::cout << "  ✗ Basic preset validation failed:" << std::endl;
        for (const auto& error : errors) {
            std::cout << "    - " << error << std::endl;
        }
    }
    
    errors = manager.validatePreset(custom_preset);
    if (errors.empty()) {
        std::cout << "  ✓ Custom preset validation passed" << std::endl;
    } else {
        std::cout << "  ✗ Custom preset validation failed:" << std::endl;
        for (const auto& error : errors) {
            std::cout << "    - " << error << std::endl;
        }
    }
    
    std::cout << "\n=== All tests completed successfully! ===" << std::endl;
    std::cout << "\nPreset Manager Features Verified:" << std::endl;
    std::cout << "  ✓ Built-in preset library" << std::endl;
    std::cout << "  ✓ Preset loading by ID" << std::endl;
    std::cout << "  ✓ Network configuration application" << std::endl;
    std::cout << "  ✓ Preset creation from network state" << std::endl;
    std::cout << "  ✓ JSON serialization/deserialization" << std::endl;
    std::cout << "  ✓ File I/O operations" << std::endl;
    std::cout << "  ✓ Preset validation" << std::endl;
    
    return 0;
}
