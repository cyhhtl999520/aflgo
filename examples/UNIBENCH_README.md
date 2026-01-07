# Unibench Test Scripts for AFLGo/GFuzz

This directory contains automated fuzzing scripts for Unibench C/C++ programs using AFLGo directed greybox fuzzing with optional GFuzz integration.

## Available Scripts

### Unibench Programs from GNU Coreutils

1. **unibench-base64.sh** - Fuzzing base64 encoding/decoding utility
   - Targets: Decode operations (vulnerable to malformed input)
   - Focus: Base64 parsing and buffer operations

2. **unibench-md5sum.sh** - Fuzzing MD5 checksum utility
   - Targets: Hash computation and verification paths
   - Focus: Cryptographic hash operations

3. **unibench-who.sh** - Fuzzing who login information utility
   - Targets: UTMP file parsing operations
   - Focus: Login record processing

4. **unibench-uniq.sh** - Fuzzing uniq duplicate line filter
   - Targets: Line comparison and field parsing
   - Focus: Text processing and comparison logic

5. **unibench-sort.sh** - Fuzzing sort text sorting utility
   - Targets: Sorting algorithms and field parsing
   - Focus: Complex text processing and memory operations

## Usage

### Prerequisites

Set the AFLGO environment variable:
```bash
export AFLGO=/path/to/aflgo
```

### Basic Usage

Run any script directly:
```bash
./unibench-base64.sh
```

The script will:
1. Download and prepare the source code (coreutils-8.30)
2. Specify target locations for directed fuzzing
3. Generate control flow graph (CFG)
4. Calculate distances to target locations
5. Instrument the binary with AFLGo/GFuzz
6. Prepare seed corpus
7. Start a short fuzzing test run (60 seconds)

### With GFuzz Integration

Enable GFuzz variable scoring before running:
```bash
export GFUZZ_ENABLED=1
./unibench-base64.sh
```

### Full Fuzzing Campaign

For extended fuzzing, use the command displayed at the end of each script:
```bash
cd <working_directory>
$AFLGO/afl-2.57b/afl-fuzz -m none -z exp -c 45m -i- -o out ./obj-aflgo/src/<binary> @@
```

## Script Structure

Each script follows a consistent 7-step workflow:

1. **Download and prepare** - Fetch source code
2. **Specify targets** - Define vulnerable code locations
3. **Preprocessing** - Generate CFG and identify key variables
4. **Calculate distances** - Compute distances to targets
5. **Instrumentation** - Instrument with AFLGo/GFuzz
6. **Prepare corpus** - Create appropriate seed inputs
7. **Start fuzzing** - Begin fuzzing campaign

## Target Selection

Targets are carefully chosen based on:
- Historical vulnerabilities
- Complex input parsing code
- Memory operations (strcpy, malloc, etc.)
- Critical algorithm implementations

## Output

Each script creates:
- Working directory: `coreutils-8.30_<binary>/`
- Instrumented binary: `obj-aflgo/src/<binary>`
- Fuzzing output: `out/` directory
- Intermediate files: `obj-aflgo/temp/` directory

## Customization

### Modify Targets

Edit the BBtargets.txt generation section:
```bash
cat > $TMP_DIR/BBtargets.txt << 'EOF'
src/program.c:100
src/program.c:200
EOF
```

### Adjust Fuzzing Parameters

Common parameters:
- `-z exp`: Exponential annealing schedule
- `-c 45m`: Time to exploitation (45 minutes)
- `-m none`: No memory limit
- `-i in`: Input directory
- `-o out`: Output directory

### Seed Corpus

Customize seed inputs in step 6 of each script to match your testing requirements.

## Performance Tips

1. **Parallel Fuzzing**: Run multiple instances for better coverage
   ```bash
   for i in {1..4}; do
       $AFLGO/afl-2.57b/afl-fuzz -S "fuzzer$i" -i in -o out ./target @@ &
   done
   ```

2. **GFuzz Optimization**: Enable variable scoring to reduce overhead
   ```bash
   export GFUZZ_SCORING_ENABLED=1
   export GFUZZ_SCORING_CONFIG=/path/to/scoring_config.json
   ```

3. **Resource Allocation**: Use sufficient CPU cores and memory

## Troubleshooting

### Build Failures

If compilation fails:
```bash
# Install required dependencies
sudo apt-get install build-essential clang llvm libgmp-dev

# Clean and rebuild
make distclean
./configure --disable-shared --disable-nls
make clean && make
```

### Distance Calculation Issues

If distance calculation fails:
```bash
# Check intermediate files
ls -lh obj-aflgo/temp/BBnames.txt
ls -lh obj-aflgo/temp/BBcalls.txt

# Verify targets exist
cat obj-aflgo/temp/BBtargets.txt
```

### Fuzzing Not Starting

Ensure:
- AFLGO environment variable is set
- Binary is properly instrumented
- Seed corpus exists and is readable
- AFL shared memory is available

## References

- Unibench: Standard benchmark suite for fuzzing evaluation
- AFLGo: https://github.com/aflgo/aflgo
- GNU Coreutils: https://www.gnu.org/software/coreutils/

## Contributing

To add more Unibench programs:
1. Follow the existing script structure
2. Select appropriate target locations
3. Create relevant seed corpus
4. Test the complete workflow
5. Document any special requirements

## License

These scripts follow the same license as AFLGo (Apache 2.0).
