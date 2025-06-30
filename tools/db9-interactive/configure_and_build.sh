#!/bin/bash

cd /Users/nporcino/dev/Lab/LabDb/tools/db9-interactive

echo "Configuring db9 interactive consciousness explorer..."
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug

echo "Building..."
cmake --build build

echo "Build completed!"
echo "Executable: build/db9_explorer"
