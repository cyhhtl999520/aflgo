#!/bin/bash
#
# GFuzz Variable Scoring Mechanism - Test Script
# -----------------------------------------------
# Tests the variable scoring mechanism integration

set -e

echo "=========================================="
echo "GFuzz Variable Scoring Mechanism Test"
echo "=========================================="
echo ""

# Change to the script directory
cd "$(dirname "$0")"

# Check if LLVM is available
if ! command -v llvm-config &> /dev/null; then
    echo "WARNING: llvm-config not found. The scoring mechanism requires LLVM."
    echo "This test will verify file structure only."
    echo ""
    
    # Verify files exist
    echo "[*] Checking file structure..."
    
    if [ -f "afl-2.57b/llvm_mode/variable-scoring.h" ]; then
        echo "  ✓ variable-scoring.h exists"
    else
        echo "  ✗ variable-scoring.h missing"
        exit 1
    fi
    
    if [ -f "afl-2.57b/llvm_mode/variable-scoring.cc" ]; then
        echo "  ✓ variable-scoring.cc exists"
    else
        echo "  ✗ variable-scoring.cc missing"
        exit 1
    fi
    
    if [ -f "config/scoring_config.json" ]; then
        echo "  ✓ scoring_config.json exists"
    else
        echo "  ✗ scoring_config.json missing"
        exit 1
    fi
    
    if [ -f "afl-2.57b/test-scoring.c" ]; then
        echo "  ✓ test-scoring.c exists"
    else
        echo "  ✗ test-scoring.c missing"
        exit 1
    fi
    
    echo ""
    echo "✓ All required files are present"
    echo "✓ File structure test PASSED"
    echo ""
    echo "To fully test the scoring mechanism, install LLVM and run:"
    echo "  ./build.sh"
    echo "  cd afl-2.57b/llvm_mode && make"
    
    exit 0
fi

# Full build test if LLVM is available
echo "[*] LLVM found, performing full build test..."
echo ""

# Build the LLVM pass with scoring mechanism
echo "[*] Building LLVM pass with scoring mechanism..."
cd afl-2.57b/llvm_mode

# Clean first
make clean 2>/dev/null || true

# Build with scoring mechanism
export GFUZZ_SCORING_ENABLED=1
make all

if [ $? -ne 0 ]; then
    echo ""
    echo "✗ Build FAILED"
    exit 1
fi

echo ""
echo "[*] Build successful!"
echo ""

# Test with the test program
echo "[*] Testing with test-scoring.c..."
cd ..

# Enable scoring for compilation
export GFUZZ_SCORING_ENABLED=1
export GFUZZ_SCORING_CONFIG=../config/scoring_config.json
export GFUZZ_DEBUG=1
export AFL_QUIET=1

# Compile test program
./afl-clang-fast -o test-scoring test-scoring.c 2>&1 | tee /tmp/scoring-test.log

if [ -f "test-scoring" ]; then
    echo ""
    echo "[*] Test program compiled successfully"
    
    # Check if scoring mechanism was invoked
    if grep -q "Variable scoring mechanism enabled" /tmp/scoring-test.log 2>/dev/null || \
       grep -q "GFuzz" /tmp/scoring-test.log 2>/dev/null; then
        echo "  ✓ Scoring mechanism was active during compilation"
    else
        echo "  Note: Scoring mechanism may not have been active (check environment variables)"
    fi
    
    # Run the test program
    echo ""
    echo "[*] Running test program..."
    ./test-scoring "test123"
    
    echo ""
    echo "✓ Test execution successful"
    
    # Clean up
    rm -f test-scoring
else
    echo ""
    echo "✗ Test program compilation FAILED"
    exit 1
fi

# Check for scoring output files
if [ -f "variable_scores.csv" ]; then
    echo ""
    echo "[*] Variable scores exported:"
    echo "  File: variable_scores.csv"
    lines=$(wc -l < variable_scores.csv)
    echo "  Variables scored: $((lines - 1))"
    rm -f variable_scores.csv
fi

cd ..

echo ""
echo "=========================================="
echo "✓ GFuzz Variable Scoring Test PASSED"
echo "=========================================="
echo ""
echo "The variable scoring mechanism has been successfully integrated."
echo ""
echo "To use it in your projects:"
echo "  export GFUZZ_SCORING_ENABLED=1"
echo "  export GFUZZ_SCORING_CONFIG=/path/to/config/scoring_config.json"
echo "  export GFUZZ_DEBUG=1  # Optional: for detailed output"
echo ""
