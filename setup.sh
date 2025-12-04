#!/bin/bash

echo "Setting up Breaking the Sorting Barrier SSSP project"
echo "======================================================"

# Check for Homebrew on macOS
if [[ "$OSTYPE" == "darwin"* ]]; then
    if ! command -v brew &> /dev/null; then
        echo "Homebrew not found. Please install Homebrew first:"
        echo '/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"'
        exit 1
    fi

    echo "Installing dependencies via Homebrew..."
    brew install cmake

    # LEMON library (optional - CMake will download if not found)
    # brew install lemon  # Uncomment if available in your brew
fi

# Check for cmake
if ! command -v cmake &> /dev/null; then
    echo "CMake is required but not installed."
    echo "Please install CMake:"
    echo "  - macOS: brew install cmake"
    echo "  - Ubuntu: sudo apt install cmake"
    echo "  - Fedora: sudo dnf install cmake"
    exit 1
fi

echo ""
echo "Dependencies installed. Run ./build.sh to build the project."
