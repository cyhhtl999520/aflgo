#!/bin/bash
#
# Unibench - sort Fuzzing with AFLGo/GFuzz
# ----------------------------------------
# 
# This script fuzzes the sort utility from coreutils using AFLGo
# directed greybox fuzzing with optional GFuzz integration.

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  Unibench: sort Fuzzing${NC}"
echo -e "${CYAN}========================================${NC}"
echo

# Check if AFLGO is set
if [ -z "$AFLGO" ]; then
    echo -e "${RED}Error: AFLGO environment variable not set${NC}"
    echo "Please run: export AFLGO=/path/to/aflgo"
    exit 1
fi

# Configuration
SUBJECT="coreutils-8.30"
TARGET_BINARY="sort"
WORK_DIR="$PWD/${SUBJECT}_${TARGET_BINARY}"

echo -e "${YELLOW}[1/7] Downloading and preparing ${SUBJECT}...${NC}"

# Download and extract coreutils
if [ ! -d "$WORK_DIR" ]; then
    wget -q https://ftp.gnu.org/gnu/coreutils/${SUBJECT}.tar.xz
    tar -xf ${SUBJECT}.tar.xz
    mv ${SUBJECT} ${WORK_DIR}
    rm ${SUBJECT}.tar.xz
fi

cd ${WORK_DIR}
echo -e "${GREEN}✓ Source code ready${NC}"

echo -e "${YELLOW}[2/7] Specifying target locations...${NC}"

# Create targets for sort - focus on sorting algorithms and field parsing
mkdir -p obj-aflgo/temp
export TMP_DIR=$PWD/obj-aflgo/temp

# Target locations in sort.c - critical sorting and parsing paths
cat > $TMP_DIR/BBtargets.txt << 'EOF'
src/sort.c:1500
src/sort.c:1800
src/sort.c:2000
src/sort.c:2500
EOF

echo -e "${GREEN}✓ Targets specified (sorting and field parsing operations)${NC}"

echo -e "${YELLOW}[3/7] Preprocessing (generating CFG)...${NC}"

# Set up compilation environment
export CC=$AFLGO/instrument/aflgo-clang
export CXX=$AFLGO/instrument/aflgo-clang++
export LDFLAGS=-lpthread
export ADDITIONAL="-targets=$TMP_DIR/BBtargets.txt -outdir=$TMP_DIR -flto -fuse-ld=gold -Wl,-plugin-opt=save-temps"

# Configure and compile for preprocessing
./configure --disable-shared --disable-nls CFLAGS="$ADDITIONAL" CXXFLAGS="$ADDITIONAL"
cd obj-aflgo
make clean
make -j$(nproc) 2>&1 | head -20

cd ..

echo -e "${GREEN}✓ CFG generated${NC}"

if [ -f $TMP_DIR/BBnames.txt ] && [ -f $TMP_DIR/BBcalls.txt ]; then
    echo "  - Basic blocks: $(wc -l < $TMP_DIR/BBnames.txt)"
    echo "  - Call edges: $(wc -l < $TMP_DIR/BBcalls.txt)"
else
    echo -e "${RED}✗ Preprocessing failed${NC}"
    exit 1
fi

echo -e "${YELLOW}[4/7] Calculating distances...${NC}"

# Clean up intermediate files
cat $TMP_DIR/BBnames.txt | rev | cut -d: -f2- | rev | sort | uniq > $TMP_DIR/BBnames2.txt
mv $TMP_DIR/BBnames2.txt $TMP_DIR/BBnames.txt

cat $TMP_DIR/BBcalls.txt | sort | uniq > $TMP_DIR/BBcalls2.txt
mv $TMP_DIR/BBcalls2.txt $TMP_DIR/BBcalls.txt

# Generate distance file
$AFLGO/distance/gen_distance_fast.py $PWD $TMP_DIR ${TARGET_BINARY}

if [ -f $TMP_DIR/distance.cfg.txt ]; then
    echo -e "${GREEN}✓ Distance calculation complete${NC}"
    echo "  - Distance entries: $(wc -l < $TMP_DIR/distance.cfg.txt)"
else
    echo -e "${RED}✗ Distance calculation failed${NC}"
    exit 1
fi

echo -e "${YELLOW}[5/7] Instrumenting with AFLGo/GFuzz...${NC}"

# Recompile with distance information
export CFLAGS="-distance=$TMP_DIR/distance.cfg.txt"
export CXXFLAGS="-distance=$TMP_DIR/distance.cfg.txt"

# Enable GFuzz if requested
if [ "${GFUZZ_ENABLED:-0}" = "1" ]; then
    export GFUZZ_ENABLED=1
    echo -e "${CYAN}  GFuzz variable scoring enabled${NC}"
fi

./configure --disable-shared --disable-nls
cd obj-aflgo
make clean
make -j$(nproc) 2>&1 | head -20

cd ..

echo -e "${GREEN}✓ Instrumentation complete${NC}"

echo -e "${YELLOW}[6/7] Preparing fuzzing campaign...${NC}"

# Create seed corpus for sort - various text formats
mkdir -p in

cat > in/seed1.txt << 'EOF'
zebra
apple
monkey
banana
EOF

cat > in/seed2.txt << 'EOF'
100
50
200
75
EOF

cat > in/seed3.txt << 'EOF'
2024-01-01
2023-12-31
2024-01-15
2023-11-20
EOF

cat > in/seed4.txt << 'EOF'
name:age:score
alice:25:90
bob:30:85
charlie:22:95
EOF

echo -e "${GREEN}✓ Seed corpus created (4 varied text files)${NC}"

echo -e "${YELLOW}[7/7] Starting fuzzing campaign...${NC}"
echo
echo -e "${CYAN}Fuzzer configuration:${NC}"
echo "  - Target: ${TARGET_BINARY}"
echo "  - Schedule: Exponential (-z exp)"
echo "  - Time to exploitation: 45 minutes (-c 45m)"
echo "  - Input directory: in/"
echo "  - Output directory: out/"
echo "  - GFuzz: ${GFUZZ_ENABLED:-disabled}"
echo
echo -e "${CYAN}Command to run:${NC}"
echo "  \$AFLGO/afl-2.57b/afl-fuzz -m none -z exp -c 45m -i in -o out ./obj-aflgo/src/${TARGET_BINARY} @@"
echo
echo -e "${YELLOW}Note: Starting a short test run. For full fuzzing, run the command above manually.${NC}"
echo

# Run a short test (60 seconds)
timeout 60 $AFLGO/afl-2.57b/afl-fuzz -m none -z exp -c 45m -i in -o out ./obj-aflgo/src/${TARGET_BINARY} @@ 2>&1 || true

echo
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  Fuzzing Test Complete!${NC}"
echo -e "${GREEN}========================================${NC}"
echo
echo -e "${CYAN}Working directory: ${WORK_DIR}${NC}"
echo -e "${CYAN}To continue fuzzing:${NC}"
echo "  cd ${WORK_DIR}"
echo "  \$AFLGO/afl-2.57b/afl-fuzz -m none -z exp -c 45m -i- -o out ./obj-aflgo/src/${TARGET_BINARY} @@"
echo
