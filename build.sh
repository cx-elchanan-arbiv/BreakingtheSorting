#!/bin/bash

set -e

echo "Building Breaking the Sorting Barrier SSSP Implementation"
echo "=========================================================="

# Create build directory
mkdir -p build
cd build

# Configure
echo ""
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo ""
echo "Building..."
cmake --build . -j$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

echo ""
echo "Build complete!"
echo ""
echo "Run tests with:      ./build/sssp_tests"
echo "Run benchmarks with: ./build/sssp_benchmarks"
echo "Run demo with:       ./build/sssp_demo [n] [m]"
