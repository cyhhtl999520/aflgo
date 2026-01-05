# VSDFuzz: Variable State Diversity-Guided Directed Fuzzing

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![Paper](https://img.shields.io/badge/Paper-Journal%20of%20Systems%20%26%20Software-green.svg)](paper/manuscript.pdf)

VSDFuzz is a directed greybox fuzzer that incorporates variable state diversity to enhance vulnerability detection. By monitoring runtime states of key variables and using state diversity as additional feedback for seed selection and energy allocation, VSDFuzz significantly improves upon traditional control-flow-guided approaches.

## 🎯 Key Features

- **Multi-Strategy Variable Identification**: Automatically identifies variables critical to reaching target code locations using distance-based analysis, memory-safety-related filtering, and semantic type analysis
- **Fine-Grained State Monitoring**: Tracks runtime states of key variables with type-specific handling (numeric, character, string, pointer)
- **Diversity-Based Scheduling**: Uses state diversity as additional feedback to guide seed selection alongside traditional coverage information
- **Adaptive Weight Balancing**: Dynamically adjusts the relative importance of coverage-based and state-based guidance throughout the fuzzing campaign

## 📊 Performance Improvements

Compared to traditional directed fuzzing (AFLGo baseline), VSDFuzz achieves:

| Metric | Improvement |
|--------|-------------|
| **Unique Paths** | +17.7% |
| **Code Coverage** | +12.0% |
| **Crash Detection** | +26.0% |
| **Time to First Crash** | -28.1% (faster) |
| **Runtime Overhead** | 17.9% |

*Results averaged across 4 real-world programs (mJS, binutils, libming, libxml2) with 10 trials each.*

## 🏗️ Architecture

VSDFuzz consists of four tightly integrated components:

```
┌─────────────────────────────────────────────────────────────┐
│                  PREPROCESSING PHASE                         │
├─────────────────────────────────────────────────────────────┤
│  1. Key Variable Identification                             │
│     - Distance-based filtering                              │
│     - Memory-safety-related filtering                       │
│     - Semantic type filtering                               │
│                                                              │
│  2. Distance Computation                                     │
│     - Call graph analysis                                    │
│     - CFG distance calculation                               │
└─────────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                   FUZZING PHASE (Runtime)                    │
├─────────────────────────────────────────────────────────────┤
│  3. Variable State Monitoring                               │
│     - Type-specific encoding                                │
│     - Shared memory communication                           │
│                                                              │
│  4. State Diversity Evaluation                              │
│     - Similarity computation                                │
│     - Diversity scoring                                     │
│                                                              │
│  5. Adaptive Seed Scheduling                                │
│     - Combined coverage + diversity scoring                 │
│     - Dynamic weight adjustment                             │
│     - Energy allocation                                     │
└─────────────────────────────────────────────────────────────┘
```

## 🚀 Installation

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build git \
    binutils-gold binutils-dev python3 python3-dev python3-pip \
    clang-11 llvm-11 llvm-11-dev

# Python dependencies
python3 -m pip install networkx pydot pydotplus
```

### Building VSDFuzz

```bash
# Clone the repository
git clone https://github.com/[your-username]/VSDFuzz.git
cd VSDFuzz

# Set environment variable
export VSDFUZZ=$PWD

# Build (requires sudo for installation)
sudo ./build.sh
```

## 📖 Quick Start

### Example: Fuzzing libxml2

```bash
# 1. Download and prepare target program
wget http://xmlsoft.org/download/libxml2-2.9.4.tar.gz
tar xf libxml2-2.9.4.tar.gz && cd libxml2-2.9.4

# 2. Specify target locations (BBtargets.txt)
echo "parser.c:1234" > $VSDFUZZ/targets.txt
echo "tree.c:5678" >> $VSDFUZZ/targets.txt

# 3. Preprocessing: Identify key variables and compute distances
$VSDFUZZ/scripts/preprocess.sh \
    --targets $VSDFUZZ/targets.txt \
    --out $VSDFUZZ/out/libxml2

# 4. Instrumentation: Build with VSDFuzz
CC=$VSDFUZZ/vsdfuzz-clang CXX=$VSDFUZZ/vsdfuzz-clang++ \
    ./configure --disable-shared
make clean && make

# 5. Run fuzzing
$VSDFUZZ/vsdfuzz-fuzz \
    -i seeds/ \
    -o out/ \
    -z exp \
    -c 45m \
    -- ./xmllint @@
```

## 📚 Documentation

- **[Implementation Guide](IMPLEMENTATION.md)** - Technical details of each component
- **[Integration Guide](INTEGRATION_GUIDE.md)** - Using VSDFuzz in your projects
- **[Complete Documentation](DOCUMENTATION.md)** - Comprehensive reference
- **[Academic Paper](paper/manuscript.tex)** - Full methodology and evaluation

## 🔬 Research Paper

The methodology and evaluation are described in our paper:

**"Variable State Diversity-Guided Directed Fuzzing"**  
*Journal of Systems and Software, 2025*

See the [`paper/`](paper/) directory for:
- Complete LaTeX manuscript
- Figure generation code (Python scripts)
- Experimental data
- Compilation instructions

## 🎨 Examples

The [`examples/`](examples/) directory contains ready-to-run examples:
- `libxml2.sh` - XML parsing library
- `binutils.sh` - Binary utilities
- `libming.sh` - Flash file processing

## 🤝 Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## 📄 License

VSDFuzz is released under the Apache License 2.0. See [LICENSE](LICENSE) for details.

## 📝 Citation

If you use VSDFuzz in your research, please cite:

```bibtex
@article{vsdfuzz2025,
  title={Variable State Diversity-Guided Directed Fuzzing},
  author={[Authors]},
  journal={Journal of Systems and Software},
  year={2025},
  doi={[DOI]}
}
```

## 🙏 Acknowledgments

VSDFuzz builds upon ideas from the fuzzing research community, including:
- **AFL** by Michal Zalewski - Coverage-guided greybox fuzzing
- **AFLGo** by Böhme et al. - Directed greybox fuzzing
- **Angora**, **QSYM**, **GREYONE** - Data-aware fuzzing techniques

We thank the authors of these tools for their foundational contributions.

## 📞 Contact

- **Issues**: [GitHub Issues](https://github.com/[your-username]/VSDFuzz/issues)
- **Email**: [your-email]
- **Paper**: See [`paper/`](paper/) directory

---

**Status**: ✅ Active Development | 📄 Academic Paper Submitted | 🔬 Experimental Tool
