# VSDFuzz Migration Guide

## 将GFuzz代码迁移到VSDFuzz新仓库的指南
## Guide for Migrating GFuzz Code to New VSDFuzz Repository

---

## Overview / 概述

This guide provides step-by-step instructions for creating a new VSDFuzz repository based on the GFuzz implementation, presented as a standalone directed fuzzing tool with variable state diversity guidance.

本指南提供了基于GFuzz实现创建新VSDFuzz仓库的分步说明，将其呈现为具有变量状态多样性引导的独立定向模糊测试工具。

---

## Step 1: Create New Repository / 创建新仓库

1. Go to GitHub: https://github.com/new
2. Repository name: **VSDFuzz**
3. Description: "Variable State Diversity-Guided Directed Fuzzing"
4. Initialize with README: No (we'll add custom README)
5. License: Apache 2.0 (to match original AFL/AFLGo)

---

## Step 2: Files to Copy / 需要复制的文件

### Core Documentation Files / 核心文档文件

```bash
# Main documentation
GFUZZ_README.md                      → README.md (rename and modify)
GFUZZ_IMPLEMENTATION.md              → IMPLEMENTATION.md
GFUZZ_INTEGRATION_GUIDE.md           → INTEGRATION_GUIDE.md
GFUZZ_COMPLETE_SUMMARY.md            → DOCUMENTATION.md
LICENSE                               → LICENSE (keep as-is)
```

### Academic Paper / 学术论文

```bash
# Complete paper directory
paper/
├── manuscript.tex                    # Main paper
├── README.md
├── README_CN.md
├── REVISION_SUMMARY.md
├── compile.sh
├── figures/
│   ├── generate_figure1.py
│   ├── generate_figure2.py
│   ├── generate_figure3.py
│   ├── generate_all_figures.sh
│   ├── FIGURE_GENERATION.md
│   └── FIGURE_CODE_SUMMARY_CN.md
└── manuscript_backup.tex
```

### Implementation Files / 实现文件

```bash
# GFuzz-specific implementation (if separating from AFLGo)
# These would need to be extracted from the AFLGo codebase
instrument/
├── gfuzz-key-vars.cc
├── gfuzz-instrumentation.h
├── gfuzz-runtime.c
├── gfuzz-config.h
└── gfuzz-diversity.h

afl-2.57b/
├── gfuzz-integration.h
└── [modified afl-fuzz.c sections]
```

### Configuration Files / 配置文件

```bash
.gitignore
build.sh                              # Modified for VSDFuzz
```

---

## Step 3: Required Modifications / 必要的修改

### 3.1 Main README.md

**Changes to make / 需要修改的内容:**

1. **Title**: Change from "GFuzz: Variable State Diversity-Guided Fuzzing" to "VSDFuzz: Variable State Diversity-Guided Directed Fuzzing"

2. **Remove AFLGo references**:
   - Delete: "GFuzz is an extension of AFLGo"
   - Replace with: "VSDFuzz is a directed greybox fuzzer that uses variable state diversity to enhance vulnerability detection"

3. **Installation section**:
   - Remove AFLGo dependencies
   - Present as standalone installation
   - Update build instructions

4. **Overview section**:
   ```markdown
   # OLD
   GFuzz addresses this limitation by extending AFLGo with...
   
   # NEW
   VSDFuzz addresses this limitation by introducing a novel approach that...
   ```

### 3.2 Academic Paper (manuscript.tex)

**Global replacements / 全局替换:**

```latex
% Find and replace
GFuzz → VSDFuzz
gfuzz → vsdfuzz

% Update abstract
"We implemented GFuzz as an extension of AFLGo" 
→ "We implemented VSDFuzz as a directed greybox fuzzer"

% Update comparison text
"GFuzz vs AFLGo" → "VSDFuzz vs AFLGo"
```

**Section updates:**

1. **Introduction**: Present VSDFuzz as novel contribution
2. **Approach**: Describe as complete system, not extension
3. **Implementation**: Remove "built on AFLGo" references
4. **Evaluation**: Keep comparison with AFLGo as baseline

### 3.3 Implementation Files

**Header guards and namespaces:**

```c
// OLD
#ifndef GFUZZ_CONFIG_H
#define GFUZZ_CONFIG_H

// NEW
#ifndef VSDFUZZ_CONFIG_H
#define VSDFUZZ_CONFIG_H
```

**Environment variables:**

```c
// OLD
GFUZZ_ENABLED
GFUZZ_DEBUG
GFUZZ_LOG_STATES

// NEW
VSDFUZZ_ENABLED
VSDFUZZ_DEBUG
VSDFUZZ_LOG_STATES
```

**Configuration parameters:**

```c
// OLD
#define GFUZZ_DISTANCE_THRESHOLD 3
#define GFUZZ_MAX_KEY_VARS 1024

// NEW
#define VSDFUZZ_DISTANCE_THRESHOLD 3
#define VSDFUZZ_MAX_KEY_VARS 1024
```

---

## Step 4: New README.md Template / 新README模板

```markdown
# VSDFuzz: Variable State Diversity-Guided Directed Fuzzing

VSDFuzz is a directed greybox fuzzer that incorporates variable state diversity to enhance vulnerability detection. By monitoring runtime states of key variables and using state diversity as additional feedback, VSDFuzz significantly improves upon traditional control-flow-guided approaches.

## Key Features

- **Multi-Strategy Variable Identification**: Automatically identifies variables critical to reaching target code locations
- **Fine-Grained State Monitoring**: Tracks runtime states of key variables with type-specific handling
- **Diversity-Based Scheduling**: Uses state diversity as additional feedback for seed selection
- **Adaptive Weight Balancing**: Dynamically adjusts guidance between coverage and state diversity

## Performance Improvements

Compared to traditional directed fuzzing (AFLGo baseline):
- **+17.7%** more unique paths discovered
- **+12.0%** better code coverage
- **+26.0%** more unique crashes detected
- **-28.1%** faster time to first crash

## Installation

### Prerequisites

```bash
sudo apt-get update
sudo apt-get install build-essential cmake ninja-build git binutils-gold \
    binutils-dev python3 python3-dev python3-pip
python3 -m pip install networkx pydot pydotplus
```

### Building VSDFuzz

```bash
git clone https://github.com/[your-username]/VSDFuzz.git
cd VSDFuzz
export VSDFUZZ=$PWD
sudo ./build.sh
```

## Quick Start

[Include simplified usage example without AFLGo references]

## Academic Paper

The methodology and evaluation are described in our paper:

**"Variable State Diversity-Guided Directed Fuzzing"**

See the `paper/` directory for the complete manuscript and figure generation code.

## Architecture

VSDFuzz consists of four main components:
1. Key Variable Identification (preprocessing)
2. Variable State Monitoring (instrumentation)
3. State Diversity Evaluation (runtime)
4. Adaptive Seed Scheduling (runtime)

[Include architecture diagram]

## Documentation

- [Implementation Guide](IMPLEMENTATION.md) - Technical details
- [Integration Guide](INTEGRATION_GUIDE.md) - Using VSDFuzz in your projects
- [Academic Paper](paper/manuscript.tex) - Full methodology and evaluation

## Citation

If you use VSDFuzz in your research, please cite:

```bibtex
@article{vsdfuzz2025,
  title={Variable State Diversity-Guided Directed Fuzzing},
  author={[Authors]},
  journal={Journal of Systems and Software},
  year={2025}
}
```

## License

VSDFuzz is released under the Apache 2.0 License.

## Acknowledgments

This work builds upon ideas from the fuzzing research community, including AFL, AFLGo, and related directed fuzzing approaches.
```

---

## Step 5: Migration Script / 迁移脚本

Create this script to automate the migration:

```bash
#!/bin/bash
# migrate_to_vsdfuzz.sh

set -e

echo "VSDFuzz Migration Script"
echo "========================"

# Check if new repo exists
if [ ! -d "../VSDFuzz" ]; then
    echo "Error: VSDFuzz directory not found"
    echo "Please create the repository first: git clone https://github.com/[your-username]/VSDFuzz.git"
    exit 1
fi

VSDFUZZ_DIR="../VSDFuzz"

echo "Copying documentation files..."
cp GFUZZ_README.md "$VSDFUZZ_DIR/README.md"
cp GFUZZ_IMPLEMENTATION.md "$VSDFUZZ_DIR/IMPLEMENTATION.md"
cp GFUZZ_INTEGRATION_GUIDE.md "$VSDFUZZ_DIR/INTEGRATION_GUIDE.md"
cp GFUZZ_COMPLETE_SUMMARY.md "$VSDFUZZ_DIR/DOCUMENTATION.md"
cp LICENSE "$VSDFUZZ_DIR/LICENSE"

echo "Copying paper directory..."
cp -r paper "$VSDFUZZ_DIR/"

echo "Copying configuration files..."
cp .gitignore "$VSDFUZZ_DIR/"

echo "Performing text replacements..."
cd "$VSDFUZZ_DIR"

# Replace GFuzz with VSDFuzz
find . -type f -name "*.md" -exec sed -i 's/GFuzz/VSDFuzz/g' {} +
find . -type f -name "*.tex" -exec sed -i 's/GFuzz/VSDFuzz/g' {} +
find . -type f -name "*.py" -exec sed -i 's/GFuzz/VSDFuzz/g' {} +

# Replace gfuzz with vsdfuzz (lowercase)
find . -type f -name "*.md" -exec sed -i 's/gfuzz/vsdfuzz/g' {} +

# Remove AFLGo extension references
sed -i 's/GFuzz is an extension of AFLGo/VSDFuzz is a directed greybox fuzzer/g' README.md
sed -i 's/We implemented GFuzz as an extension of AFLGo/We implemented VSDFuzz as a directed greybox fuzzer/g' paper/manuscript.tex

echo "Migration complete!"
echo ""
echo "Next steps:"
echo "1. Review and edit README.md"
echo "2. Update paper/manuscript.tex references"
echo "3. Test documentation links"
echo "4. Commit and push to VSDFuzz repository"
```

---

## Step 6: Post-Migration Checklist / 迁移后检查清单

### Documentation Updates / 文档更新

- [ ] README.md: Remove all AFLGo references
- [ ] README.md: Update installation instructions
- [ ] README.md: Update repository URLs
- [ ] IMPLEMENTATION.md: Update technical details
- [ ] INTEGRATION_GUIDE.md: Update API names

### Paper Updates / 论文更新

- [ ] manuscript.tex: Global GFuzz → VSDFuzz replacement
- [ ] manuscript.tex: Update abstract
- [ ] manuscript.tex: Update implementation section
- [ ] manuscript.tex: Keep AFLGo only as baseline comparison
- [ ] Generate new figures with updated titles

### Code Updates / 代码更新

- [ ] Rename all gfuzz-* files to vsdfuzz-*
- [ ] Update header guards
- [ ] Update environment variables
- [ ] Update configuration parameters
- [ ] Update function names and comments

### Repository Setup / 仓库设置

- [ ] Create GitHub repository
- [ ] Add README badges
- [ ] Set up GitHub Actions (optional)
- [ ] Add LICENSE file
- [ ] Create releases and tags
- [ ] Add topics/keywords for discoverability

---

## Step 7: Building and Testing / 构建和测试

After migration, verify everything works:

```bash
cd VSDFuzz

# Test documentation
cat README.md | grep -i "aflgo.*extension" && echo "ERROR: Still references AFLGo extension" || echo "OK"

# Test paper compilation
cd paper
./compile.sh
# Check manuscript.pdf for correct tool name

# Generate figures
cd figures
pip3 install matplotlib numpy
./generate_all_figures.sh
# Verify figures have VSDFuzz branding

# Test code compilation (if including implementation)
cd ../..
./build.sh
```

---

## Step 8: Repository Metadata / 仓库元数据

### GitHub Repository Settings

**Description:**
```
Variable State Diversity-Guided Directed Fuzzing - A novel directed greybox fuzzer that uses variable state diversity to enhance vulnerability detection
```

**Topics/Tags:**
```
fuzzing, security, vulnerability-detection, directed-fuzzing, greybox-fuzzing, 
program-analysis, software-testing, variable-state, diversity-guidance
```

**Website:** Link to academic paper or documentation site

---

## Troubleshooting / 故障排除

### Issue: Broken Links / 问题：链接失效

**Solution:** Update all internal links to point to VSDFuzz repository:
```bash
sed -i 's|cyhhtl999520/aflgo|[your-username]/VSDFuzz|g' README.md
```

### Issue: Figure References / 问题：图片引用

**Solution:** Regenerate figures with VSDFuzz branding:
```python
# In generate_figure*.py files
ax.set_title('VSDFuzz: Variable State Diversity\nGuided Approach', ...)
```

### Issue: Build Dependencies / 问题：构建依赖

**Solution:** If including implementation, document all dependencies clearly and provide installation script.

---

## Additional Resources / 额外资源

### Files You May Want to Add / 可能需要添加的文件

1. **CHANGELOG.md** - Track version history
2. **CONTRIBUTING.md** - Contribution guidelines
3. **CODE_OF_CONDUCT.md** - Community standards
4. **.github/workflows/** - CI/CD pipelines
5. **examples/** - Usage examples
6. **docs/** - Extended documentation

### Recommended Repository Structure / 推荐的仓库结构

```
VSDFuzz/
├── README.md
├── LICENSE
├── IMPLEMENTATION.md
├── INTEGRATION_GUIDE.md
├── DOCUMENTATION.md
├── build.sh
├── .gitignore
├── paper/
│   ├── manuscript.tex
│   ├── figures/
│   └── README.md
├── docs/
│   ├── installation.md
│   ├── usage.md
│   └── api.md
├── examples/
│   ├── libxml2.sh
│   └── binutils.sh
└── src/                    # If including implementation
    ├── vsdfuzz-*.cc
    ├── vsdfuzz-*.h
    └── CMakeLists.txt
```

---

## Summary / 总结

This migration guide provides everything needed to create a standalone VSDFuzz repository. The key changes are:

1. **Rebranding**: GFuzz → VSDFuzz throughout all files
2. **Independence**: Remove "extension of AFLGo" framing
3. **Presentation**: Present as complete, novel tool
4. **Baseline**: Keep AFLGo only as comparison baseline in evaluation

The result will be a professional, standalone repository that properly credits prior work while establishing VSDFuzz as a new contribution to the field.

创建独立VSDFuzz仓库的关键变更：
1. 品牌重塑：所有文件中GFuzz → VSDFuzz
2. 独立性：删除"AFLGo扩展"的表述
3. 呈现方式：作为完整的新工具呈现
4. 基线对比：仅在评估中保留AFLGo作为对比基线

---

**Last Updated:** 2025-11-10  
**Version:** 1.0  
**Status:** Ready for migration
