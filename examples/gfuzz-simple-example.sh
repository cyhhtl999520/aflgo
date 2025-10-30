#!/bin/bash
#
# GFuzz Example: Fuzzing a Simple Vulnerable Program
# ---------------------------------------------------
#
# This script demonstrates the complete GFuzz workflow on a simple
# test program with a buffer overflow vulnerability.

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  GFuzz Example: Simple Test Program${NC}"
echo -e "${CYAN}========================================${NC}"
echo

# Check if AFLGO is set
if [ -z "$AFLGO" ]; then
    echo -e "${RED}Error: AFLGO environment variable not set${NC}"
    echo "Please run: export AFLGO=/path/to/aflgo"
    exit 1
fi

# Create working directory
WORK_DIR="/tmp/gfuzz-example-$$"
mkdir -p "$WORK_DIR"
cd "$WORK_DIR"

echo -e "${YELLOW}[1/7] Creating test program...${NC}"

# Create a simple vulnerable test program
cat > test_program.c << 'EOF'
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Vulnerable function with buffer overflow
int process_input(char *input) {
    char buffer[32];  // Key variable: buffer
    int length;       // Key variable: length
    int result = 0;   // Key variable: result
    
    length = strlen(input);
    
    // Vulnerable copy - target site
    if (length > 0) {
        strcpy(buffer, input);  // Line 15 - TARGET
        result = 1;
    }
    
    return result;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }
    
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        perror("fopen");
        return 1;
    }
    
    char input[256];
    if (fgets(input, sizeof(input), f)) {
        int ret = process_input(input);
        printf("Processed: %d\n", ret);
    }
    
    fclose(f);
    return 0;
}
EOF

echo -e "${GREEN}✓ Test program created${NC}"

echo -e "${YELLOW}[2/7] Specifying target locations...${NC}"

# Create targets file (the strcpy line)
mkdir -p temp
echo "test_program.c:15" > temp/BBtargets.txt

echo -e "${GREEN}✓ Target: test_program.c:15 (strcpy)${NC}"

echo -e "${YELLOW}[3/7] Preprocessing (generating CFG and identifying key variables)...${NC}"

# Set up compilation for preprocessing
export CC="$AFLGO/instrument/aflgo-clang"
export CXX="$AFLGO/instrument/aflgo-clang++"
export ADDITIONAL="-targets=$WORK_DIR/temp/BBtargets.txt -outdir=$WORK_DIR/temp -flto -fuse-ld=gold -Wl,-plugin-opt=save-temps"
export CFLAGS="$ADDITIONAL"
export CXXFLAGS="$ADDITIONAL"

# Compile for preprocessing
$CC $CFLAGS test_program.c -o test_program_temp 2>&1 | grep -i "aflgo\|gfuzz" || true

echo -e "${GREEN}✓ CFG generated${NC}"

# Check if preprocessing succeeded
if [ -f temp/BBnames.txt ] && [ -f temp/BBcalls.txt ]; then
    echo -e "${GREEN}✓ Preprocessing complete${NC}"
    echo "  - Functions: $(cat temp/Fnames.txt 2>/dev/null | wc -l)"
    echo "  - Basic blocks: $(cat temp/BBnames.txt 2>/dev/null | wc -l)"
else
    echo -e "${RED}✗ Preprocessing failed${NC}"
    exit 1
fi

echo -e "${YELLOW}[4/7] Calculating distances...${NC}"

# Clean up intermediate files
cat temp/BBnames.txt | grep -v "^$" | rev | cut -d: -f2- | rev | sort | uniq > temp/BBnames2.txt
mv temp/BBnames2.txt temp/BBnames.txt

cat temp/BBcalls.txt | grep -Ev "^[^,]*$|^([^,]*,){2,}[^,]*$" | sort | uniq > temp/BBcalls2.txt 2>/dev/null || touch temp/BBcalls2.txt
mv temp/BBcalls2.txt temp/BBcalls.txt

# Generate distance file
$AFLGO/distance/gen_distance_fast.py $WORK_DIR temp test_program_temp 2>&1 | head -5

if [ -f temp/distance.cfg.txt ]; then
    echo -e "${GREEN}✓ Distance calculation complete${NC}"
    echo "  - Distance entries: $(wc -l < temp/distance.cfg.txt)"
else
    echo -e "${RED}✗ Distance calculation failed${NC}"
    exit 1
fi

echo -e "${YELLOW}[5/7] Instrumenting with GFuzz...${NC}"

# Enable GFuzz
export GFUZZ_ENABLED=1
export CFLAGS="-distance=$WORK_DIR/temp/distance.cfg.txt"
export CXXFLAGS="-distance=$WORK_DIR/temp/distance.cfg.txt"

# Compile with instrumentation
$CC $CFLAGS test_program.c -o test_program 2>&1 | grep -i "aflgo\|gfuzz" || true

echo -e "${GREEN}✓ Instrumentation complete${NC}"

echo -e "${YELLOW}[6/7] Preparing fuzzing campaign...${NC}"

# Create seed corpus
mkdir -p in
echo "Hello" > in/seed1.txt
echo "Test" > in/seed2.txt
echo "AAAA" > in/seed3.txt

echo -e "${GREEN}✓ Seed corpus created (3 seeds)${NC}"

echo -e "${YELLOW}[7/7] Starting GFuzz fuzzing campaign...${NC}"
echo
echo -e "${CYAN}Fuzzer configuration:${NC}"
echo "  - Schedule: Exponential annealing (-z exp)"
echo "  - Time to exploitation: 5 minutes (-c 5m)"
echo "  - Input directory: in/"
echo "  - Output directory: out/"
echo "  - GFuzz enabled: YES"
echo
echo -e "${CYAN}Press Ctrl+C to stop fuzzing${NC}"
echo

# Run fuzzer
export GFUZZ_ENABLED=1
timeout 60 $AFLGO/afl-2.57b/afl-fuzz \
    -S gfuzz_test \
    -z exp \
    -c 5m \
    -i in \
    -o out \
    ./test_program @@ 2>&1 || true

echo
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  Fuzzing Campaign Complete!${NC}"
echo -e "${GREEN}========================================${NC}"
echo

# Show results
if [ -d out/gfuzz_test ]; then
    echo -e "${CYAN}Results Summary:${NC}"
    echo "  - Queue size: $(ls out/gfuzz_test/queue/ 2>/dev/null | wc -l)"
    echo "  - Crashes: $(ls out/gfuzz_test/crashes/ 2>/dev/null | grep -v README | wc -l)"
    echo "  - Hangs: $(ls out/gfuzz_test/hangs/ 2>/dev/null | grep -v README | wc -l)"
    echo
    
    if [ -f out/gfuzz_test/fuzzer_stats ]; then
        echo -e "${CYAN}Fuzzer Statistics:${NC}"
        grep "execs_done\|execs_per_sec\|paths_total\|pending_total\|unique_crashes" out/gfuzz_test/fuzzer_stats || true
    fi
fi

echo
echo -e "${CYAN}Working directory: ${WORK_DIR}${NC}"
echo -e "${CYAN}Output directory: ${WORK_DIR}/out/gfuzz_test${NC}"
echo
echo -e "${YELLOW}To continue fuzzing:${NC}"
echo "  cd $WORK_DIR"
echo "  export GFUZZ_ENABLED=1"
echo "  $AFLGO/afl-2.57b/afl-fuzz -S gfuzz_test -z exp -c 5m -i- -o out ./test_program @@"
echo
