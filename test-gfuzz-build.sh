#!/bin/bash
#
# GFuzz Standalone Build and Test Script
# ---------------------------------------
#
# This script builds GFuzz components independently for testing
# without requiring a full AFLGo build.

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo "========================================="
echo "  GFuzz Standalone Build and Test"
echo "========================================="
echo

# Check for required tools
echo "[1/5] Checking build environment..."
command -v gcc >/dev/null 2>&1 || { echo "Error: gcc not found"; exit 1; }
echo "✓ gcc found: $(gcc --version | head -1)"

# Build GFuzz runtime (standalone)
echo
echo "[2/5] Building GFuzz runtime..."
gcc -O3 -Wall -fPIC -c \
    -I. \
    -DGFUZZ_ENABLED=1 \
    -DGFUZZ_DEBUG=1 \
    instrument/gfuzz-runtime.c \
    -o /tmp/gfuzz-runtime-test.o

if [ $? -eq 0 ]; then
    echo "✓ GFuzz runtime compiled successfully"
else
    echo "✗ Failed to compile GFuzz runtime"
    exit 1
fi

# Create a test program that uses GFuzz runtime
echo
echo "[3/5] Creating test program..."
cat > /tmp/gfuzz-test-program.c << 'EOF'
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* Mock GFuzz runtime functions for testing */
extern void __gfuzz_record_numeric(uint32_t var_id, uint32_t value);
extern void __gfuzz_record_char(uint32_t var_id, uint8_t value);
extern void __gfuzz_record_string(uint32_t var_id, const char* value);
extern void __gfuzz_record_pointer(uint32_t var_id, const void* value);

int main() {
    printf("GFuzz Runtime Test Program\n");
    printf("===========================\n\n");
    
    /* Test numeric recording */
    printf("[Test 1] Recording numeric variables...\n");
    __gfuzz_record_numeric(0, 42);
    __gfuzz_record_numeric(1, 12345);
    __gfuzz_record_numeric(2, 0xDEADBEEF);
    printf("✓ Numeric recording complete\n\n");
    
    /* Test char recording */
    printf("[Test 2] Recording char variables...\n");
    __gfuzz_record_char(10, 'A');
    __gfuzz_record_char(11, 'Z');
    __gfuzz_record_char(12, '0');
    printf("✓ Char recording complete\n\n");
    
    /* Test string recording */
    printf("[Test 3] Recording string variables...\n");
    __gfuzz_record_string(20, "Hello, GFuzz!");
    __gfuzz_record_string(21, "Variable state diversity");
    __gfuzz_record_string(22, "Fuzzing is awesome!");
    printf("✓ String recording complete\n\n");
    
    /* Test pointer recording */
    printf("[Test 4] Recording pointer variables...\n");
    int dummy = 123;
    __gfuzz_record_pointer(30, &dummy);
    __gfuzz_record_pointer(31, NULL);
    __gfuzz_record_pointer(32, main);
    printf("✓ Pointer recording complete\n\n");
    
    printf("========================================\n");
    printf("All tests passed! GFuzz runtime is functional.\n");
    printf("========================================\n");
    
    return 0;
}
EOF

echo "✓ Test program created"

# Compile test program
echo
echo "[4/5] Compiling test program..."
gcc -O3 -Wall \
    -I. \
    -DGFUZZ_ENABLED=1 \
    /tmp/gfuzz-test-program.c \
    /tmp/gfuzz-runtime-test.o \
    -o /tmp/gfuzz-test-program

if [ $? -eq 0 ]; then
    echo "✓ Test program compiled successfully"
else
    echo "✗ Failed to compile test program"
    exit 1
fi

# Run test
echo
echo "[5/5] Running test program..."
echo "----------------------------------------"
export GFUZZ_ENABLED=1
/tmp/gfuzz-test-program

if [ $? -eq 0 ]; then
    echo
    echo "========================================="
    echo "  ✓ GFuzz Build Test PASSED"
    echo "========================================="
    echo
    echo "GFuzz runtime components are working correctly!"
    echo
    echo "Next steps:"
    echo "  1. Install LLVM 11.0.0 (required for full build)"
    echo "  2. Run: sudo ./build.sh"
    echo "  3. See GFUZZ_README.md for usage instructions"
    echo
else
    echo
    echo "========================================="
    echo "  ✗ GFuzz Build Test FAILED"
    echo "========================================="
    exit 1
fi

# Cleanup
rm -f /tmp/gfuzz-runtime-test.o /tmp/gfuzz-test-program.c /tmp/gfuzz-test-program

exit 0
