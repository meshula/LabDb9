#!/bin/bash

# Build script for HH C++ Single Neuron Simulator
# Supports macOS, Linux, and Windows (with appropriate toolchains)

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

# Detect platform
case "$(uname -s)" in
    Darwin*)    PLATFORM=macos ;;
    Linux*)     PLATFORM=linux ;;
    MINGW*|MSYS*|CYGWIN*) PLATFORM=windows ;;
    *)          echo "Unknown platform"; exit 1 ;;
esac

echo "Building HH C++ Single Neuron Simulator for $PLATFORM..."

# Clean and create build directory
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning existing build directory..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo "Configuring with CMake..."
if [ "$PLATFORM" = "windows" ]; then
    # Windows with MinGW
    cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
elif [ "$PLATFORM" = "macos" ]; then
    # macOS with CMake
    cmake -G "Xcode" -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
elif [ "$PLATFORM" = "linux" ]; then
    # Linux with CMake
    cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..
fi

# Build
echo "Building..."
cmake --build . --config Release

echo ""
echo "Build completed successfully!"
echo "Executable location:"

case "$PLATFORM" in
    macos|linux)
        echo "  $BUILD_DIR/hh_neuron"
        ;;
    windows)
        echo "  $BUILD_DIR/hh_neuron.exe"
        ;;
esac

echo ""
echo "To run:"
case "$PLATFORM" in
    macos|linux)
        echo "  cd $BUILD_DIR && ./hh_neuron"
        ;;
    windows)
        echo "  cd $BUILD_DIR && hh_neuron.exe"
        ;;
esac

echo ""
echo "Triadic Architecture Summary:"
echo "  Application Layer (C++): HH neuron simulation, consciousness framework"
echo "  Interface Layer (C++):   Dear ImGui UI, parameter management"  
echo "  Rendering Layer (C):     Sokol graphics, real-time visualization"
