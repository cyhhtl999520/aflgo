# GFuzz: Variable State Diversity-Guided Fuzzing

GFuzz is an extension of AFLGo that implements **Variable State Diversity-Guided Fuzzing**, a novel approach that combines traditional control-flow coverage with data-state awareness to improve vulnerability detection.

## Overview

Traditional directed greybox fuzzing (DGF) tools like AFLGo primarily rely on control-flow information (code coverage and path distance) to guide testing. However, many vulnerabilities require specific data conditions to trigger, which pure control-flow guidance may miss.

GFuzz addresses this limitation by:

1. **Identifying Key Variables**: Automatically identifying variables critical to reaching target code locations
2. **Monitoring Variable States**: Tracking runtime states of key variables during execution
3. **Diversity-Guided Scheduling**: Using state diversity as additional feedback to guide seed selection and energy allocation
4. **Adaptive Weight Balancing**: Dynamically adjusting the balance between coverage-based and state-based guidance

## Key Features

- **Multi-Strategy Variable Identification** (Algorithm 1):
  - Distance-based filtering: Variables in functions close to targets
  - ASan-related filtering: Variables involved in memory safety operations
  - Semantic filtering: Focus on pointers and integers

- **Fine-Grained State Monitoring** (Algorithm 2):
  - Support for numeric, character, and string types
  - Efficient 32-bit normalization for performance
  - LLVM-level instrumentation for precise tracking

- **State Diversity Evaluation** (Algorithm 3):
  - Type-specific similarity calculations
  - Coverage ratio tracking
  - Historical state comparison

- **Adaptive Seed Scheduling** (Algorithm 4):
  - Dynamic weight adjustment based on gains
  - Energy allocation boosted by state diversity
  - Probabilistic seed selection

## Installation

### Prerequisites

GFuzz inherits AFLGo's requirements:

```bash
# System packages
sudo apt-get update
sudo apt-get install build-essential cmake ninja-build git binutils-gold \
    binutils-dev python3 python3-dev python3-pip pkg-config autoconf \
    automake libtool-bin gawk libboost-all-dev

# Python packages
python3 -m pip install networkx pydot pydotplus
```

### Building GFuzz

GFuzz is built alongside AFLGo:

```bash
git clone https://github.com/cyhhtl999520/aflgo.git
cd aflgo
export AFLGO=$PWD

# Build everything (LLVM, AFLGo, GFuzz)
sudo ./build.sh
```

## Usage

GFuzz follows a similar workflow to AFLGo with additional preprocessing steps for key variable identification.

### Step 1: Prepare Your Target

Clone and set up your target program:

```bash
# Example: libxml2
git clone https://gitlab.gnome.org/GNOME/libxml2
export SUBJECT=$PWD/libxml2
```

### Step 2: Specify Targets

Create `BBtargets.txt` with target code locations:

```bash
mkdir temp
export TMP_DIR=$PWD/temp

# Example: Extract targets from a commit diff
cd $SUBJECT
git diff -U0 HEAD^ HEAD > $TMP_DIR/commit.diff
cd ..

# Process diff to extract line numbers
cat $TMP_DIR/commit.diff | \
  awk -f showlinenum.awk show_header=0 path=1 | \
  grep -e "\.[ch]:[0-9]*:+" | \
  cut -d+ -f1 | rev | cut -c2- | rev > $TMP_DIR/BBtargets.txt
```

### Step 3: Generate Distance and Key Variables (Preprocessing)

```bash
export CC=$AFLGO/instrument/aflgo-clang
export CXX=$AFLGO/instrument/aflgo-clang++

# Configure with GFuzz-enabled flags
export ADDITIONAL="-targets=$TMP_DIR/BBtargets.txt -outdir=$TMP_DIR \
    -flto -fuse-ld=gold -Wl,-plugin-opt=save-temps"
export CFLAGS="$CFLAGS $ADDITIONAL"
export CXXFLAGS="$CXXFLAGS $ADDITIONAL"

# Build to generate CFG and identify key variables
cd $SUBJECT
./autogen.sh
./configure --disable-shared
make clean
make xmllint
cd ..

# Clean up and generate distance file
cat $TMP_DIR/BBnames.txt | grep -v "^$" | rev | cut -d: -f2- | rev | \
    sort | uniq > $TMP_DIR/BBnames2.txt
mv $TMP_DIR/BBnames2.txt $TMP_DIR/BBnames.txt

cat $TMP_DIR/BBcalls.txt | grep -Ev "^[^,]*$|^([^,]*,){2,}[^,]*$" | \
    sort | uniq > $TMP_DIR/BBcalls2.txt
mv $TMP_DIR/BBcalls2.txt $TMP_DIR/BBcalls.txt

$AFLGO/distance/gen_distance_fast.py $SUBJECT $TMP_DIR xmllint
```

### Step 4: Instrument with Distance and State Tracking

```bash
# Enable GFuzz
export GFUZZ_ENABLED=1

# Configure with distance information
export CFLAGS="-distance=$TMP_DIR/distance.cfg.txt"
export CXXFLAGS="-distance=$TMP_DIR/distance.cfg.txt"

# Rebuild with full instrumentation
cd $SUBJECT
make clean
./configure --disable-shared
make xmllint
cd ..
```

### Step 5: Fuzz with GFuzz

```bash
# Prepare seed corpus
mkdir in
cp -r $SUBJECT/test/dtd* in

# Run GFuzz (with exponential annealing schedule)
export GFUZZ_ENABLED=1
$AFLGO/afl-2.57b/afl-fuzz \
    -S gfuzz \
    -z exp \
    -c 45m \
    -i in \
    -o out \
    $SUBJECT/xmllint --valid --recover @@
```

## Configuration Parameters

GFuzz parameters are defined in `gfuzz-config.h`:

| Parameter | Default | Description |
|-----------|---------|-------------|
| `GFUZZ_DISTANCE_THRESHOLD` | 3 | Max call distance to targets for variable selection |
| `GFUZZ_MAX_KEY_VARS` | 1024 | Maximum number of key variables to track |
| `GFUZZ_WEIGHT_NUMERIC` | 1.0 | Weight for numeric variable similarity |
| `GFUZZ_WEIGHT_CHAR` | 0.8 | Weight for character variable similarity |
| `GFUZZ_WEIGHT_STRING` | 1.2 | Weight for string variable similarity |
| `GFUZZ_LEARNING_RATE` | 0.1 | Learning rate (λ) for adaptive weights |
| `GFUZZ_ENERGY_COEFF` | 0.5 | Energy coefficient (γ) for state-based boost |
| `GFUZZ_STATE_HISTORY_SIZE` | 100 | Number of historical states to maintain |

## Environment Variables

- `GFUZZ_ENABLED=1`: Enable GFuzz state tracking (default: disabled)
- `GFUZZ_DEBUG=1`: Enable verbose debug output
- `GFUZZ_LOG_STATES=1`: Log all state changes to stderr
- Standard AFLGo variables also apply (`AFLGO_INST_RATIO`, etc.)

## Performance Considerations

GFuzz introduces modest overhead for state tracking:

- **Compilation**: Additional preprocessing to identify key variables (< 10% slower)
- **Runtime**: Variable state recording adds ~9% execution overhead
- **Memory**: Additional shared memory for state map (~256KB with default config)

The performance impact is acceptable given the significant improvements in:
- Path discovery: +17.4% average
- Code coverage: +11.8% average  
- Crash detection: +25.9% average

## Architecture

```
GFuzz Components:
│
├── Static Analysis (Preprocessing)
│   ├── gfuzz-key-vars.cc      # Algorithm 1: Key variable identification
│   └── gfuzz-instrumentation.h # Algorithm 2: State monitoring insertion
│
├── Runtime (Linked with target)
│   └── gfuzz-runtime.c        # State tracking and recording
│
├── Fuzzer Integration (afl-fuzz extensions)
│   ├── gfuzz-diversity.h      # Algorithm 3: State diversity evaluation
│   └── [afl-fuzz.c patches]   # Algorithm 4: Adaptive scheduling
│
└── Configuration
    └── gfuzz-config.h         # Tunable parameters
```

## Comparison with AFLGo

| Feature | AFLGo | GFuzz |
|---------|-------|-------|
| Directed Fuzzing | ✓ | ✓ |
| Distance-based Guidance | ✓ | ✓ |
| Variable State Tracking | ✗ | ✓ |
| State Diversity Feedback | ✗ | ✓ |
| Adaptive Scheduling | Basic | Advanced |
| Data Condition Awareness | ✗ | ✓ |

## Research Paper

This implementation is based on the research paper:

**"Variable State Diversity-Guided Fuzzing Method"**

Key contributions:
1. Multi-strategy key variable identification combining distance, ASan, and semantic analysis
2. Fine-grained variable state monitoring with type-specific handling
3. State diversity-based seed scheduling with adaptive weight adjustment
4. Comprehensive evaluation showing 17.4% more paths, 11.8% better coverage, and 25.9% more crashes

## Examples

See the `examples/` directory for complete fuzzing scripts:
- `libxml2-ef709ce2.sh` - XML parser vulnerability
- `binutils-2c49145.sh` - CVE-2017-15939
- More coming soon...

## Troubleshooting

### Key Variables Not Identified

- Ensure `BBtargets.txt` has valid target locations
- Check that targets are reachable from your program entry point
- Verify `Ftargets.txt` was generated during preprocessing

### High Runtime Overhead

- Reduce `GFUZZ_DISTANCE_THRESHOLD` to track fewer variables
- Lower `GFUZZ_MAX_KEY_VARS` limit
- Adjust `GFUZZ_INST_RATIO` to instrument selectively

### No State Diversity Observed

- Confirm `GFUZZ_ENABLED=1` is set when running fuzzer
- Check that key variables are actually being executed
- Enable `GFUZZ_DEBUG=1` to see state updates

## Contributing

Contributions are welcome! Areas for improvement:
- More sophisticated variable identification strategies
- Better string similarity metrics
- Machine learning integration for weight adaptation
- Parallel/distributed fuzzing support

## License

GFuzz inherits AFLGo's Apache 2.0 license.

## Citation

If you use GFuzz in your research, please cite:

```bibtex
@inproceedings{gfuzz2025,
  title={Variable State Diversity-Guided Fuzzing Method},
  author={[Authors]},
  booktitle={Proceedings of IEEE/ACM ICSE 2025},
  year={2025}
}
```

## Acknowledgments

GFuzz is built on top of [AFLGo](https://github.com/aflgo/aflgo) by Marcel Böhme et al.
Original AFL by Michal Zalewski.
