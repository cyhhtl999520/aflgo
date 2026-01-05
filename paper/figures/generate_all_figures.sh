#!/bin/bash
# Master script to generate all three figures for the GFuzz paper

set -e  # Exit on error

echo "=========================================="
echo "Generating GFuzz Paper Figures"
echo "=========================================="
echo ""

# Check if Python3 is available
if ! command -v python3 &> /dev/null; then
    echo "Error: python3 is required but not found"
    exit 1
fi

# Check if matplotlib is installed
if ! python3 -c "import matplotlib" &> /dev/null; then
    echo "Error: matplotlib is required but not installed"
    echo "Install with: pip3 install matplotlib numpy"
    exit 1
fi

# Get the directory where the script is located
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

echo "Generating Figure 1: Approach Comparison..."
python3 generate_figure1.py
echo "✓ Figure 1 complete"
echo ""

echo "Generating Figure 2: GFuzz Architecture..."
python3 generate_figure2.py
echo "✓ Figure 2 complete"
echo ""

echo "Generating Figure 3: Coverage Over Time..."
python3 generate_figure3.py
echo "✓ Figure 3 complete"
echo ""

echo "=========================================="
echo "All figures generated successfully!"
echo "=========================================="
echo ""
echo "Generated files:"
echo "  - approach_comparison.pdf / .png"
echo "  - gfuzz_architecture.pdf / .png"
echo "  - coverage_over_time.pdf / .png"
echo ""
echo "PDF files are ready for LaTeX compilation."
echo "PNG files can be used for previewing."
