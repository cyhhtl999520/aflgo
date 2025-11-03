# GFuzz Academic Paper

## 变量状态多样性引导的定向模糊测试 (Variable State Diversity-Guided Directed Fuzzing)

This directory contains the academic paper manuscript for GFuzz, formatted according to The Journal of Systems & Software (JSS) requirements.

## Files

- `manuscript.tex` - Main LaTeX manuscript file
- `README.md` - This file
- `compile.sh` - Script to compile the paper

## Compilation Instructions

### Prerequisites

You need a LaTeX distribution installed with the following packages:
- elsarticle class (provided by Elsevier)
- Standard packages: amsmath, amssymb, algorithm, algorithmic, graphicx, booktabs, etc.

On Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install texlive-full texlive-publishers
```

Or for a minimal installation:
```bash
sudo apt-get install texlive-latex-base texlive-latex-extra texlive-science
```

### Compiling the Paper

#### Method 1: Using pdflatex (recommended)

```bash
cd paper
pdflatex manuscript.tex
bibtex manuscript
pdflatex manuscript.tex
pdflatex manuscript.tex
```

#### Method 2: Using the provided script

```bash
cd paper
chmod +x compile.sh
./compile.sh
```

The compiled PDF will be generated as `manuscript.pdf`.

## Paper Structure

The paper follows The Journal of Systems & Software format and includes:

### 1. Front Matter
- Title: "Variable State Diversity-Guided Directed Fuzzing"
- Abstract (250 words)
- Keywords: Directed fuzzing, Greybox fuzzing, Variable state diversity, Vulnerability detection, Program testing

### 2. Main Sections
1. **Introduction** (~2.5 pages)
   - Motivation and problem statement
   - Key contributions
   - Paper organization

2. **Background and Related Work** (~2 pages)
   - Greybox fuzzing overview
   - Directed greybox fuzzing
   - Data-aware fuzzing approaches
   - State-aware testing

3. **Approach** (~5 pages)
   - Overview of GFuzz architecture
   - Algorithm 1: Key Variable Identification
   - Algorithm 2: Variable State Monitoring
   - Algorithm 3: State Diversity Evaluation
   - Algorithm 4: Adaptive Seed Scheduling

4. **Implementation** (~1.5 pages)
   - Key variable identification module
   - State monitoring instrumentation
   - Diversity engine
   - Fuzzer integration
   - Configuration and parameters

5. **Evaluation** (~3 pages)
   - Research questions
   - Experimental setup (4 real-world benchmarks)
   - Results and analysis
   - Ablation study

6. **Discussion** (~1.5 pages)
   - When GFuzz excels
   - Limitations
   - Threats to validity
   - Future work

7. **Conclusion** (~0.5 pages)
   - Summary of contributions
   - Impact and future directions

### 3. References
- 16 key references covering:
  - Fuzzing tools (AFL, AFLGo, Hawkeye, etc.)
  - Data-aware approaches (VUzzer, Angora, GREYONE)
  - Hybrid techniques (QSYM, Driller, KLEE)

## Key Results Summary

The paper presents comprehensive evaluation results:

| Metric | Improvement vs AFLGo |
|--------|---------------------|
| Path Discovery | +17.7% average |
| Code Coverage | +12.0% average |
| Crash Detection | +26.0% average |
| Time to First Crash | -28.1% (faster) |
| Runtime Overhead | 17.9% |

## Submission Guidelines (JSS)

The Journal of Systems & Software submission requirements:

### Format Requirements
- ✓ Uses elsarticle document class
- ✓ Review mode with line numbers enabled
- ✓ Standard font sizes and margins
- ✓ Numbered sections and subsections
- ✓ Algorithms formatted with algorithm/algorithmic packages
- ✓ Tables and figures with captions
- ✓ Bibliography in elsarticle-num style

### Content Requirements
- ✓ Abstract: ~250 words
- ✓ Keywords: 5 relevant keywords
- ✓ Total length: ~15 pages (within JSS guidelines)
- ✓ Figures: Camera-ready quality (when added)
- ✓ References: Complete and properly formatted
- ✓ Reproducibility: Data availability statement included

### Submission Checklist
- [ ] Final proofreading completed
- [ ] All author information updated (currently anonymized for review)
- [ ] Acknowledgments updated with correct grant numbers
- [ ] All figures added and properly referenced
- [ ] Supplementary materials prepared
- [ ] Copyright transfer agreement signed
- [ ] Cover letter prepared

## Figures and Tables

The paper includes the following tables:

1. **Table 1**: Path discovery and code coverage comparison
2. **Table 2**: Crash detection comparison
3. **Table 3**: Runtime overhead comparison
4. **Table 4**: Ablation study results

Recommended figures to add (not yet included):

1. **Figure 1**: GFuzz architecture overview
   - Four-component diagram showing the workflow
   
2. **Figure 2**: Motivating example
   - Code snippet demonstrating data-dependent vulnerability
   
3. **Figure 3**: Path discovery over time
   - Line graph comparing AFLGo vs GFuzz
   
4. **Figure 4**: State diversity evolution
   - Graph showing how diversity metric changes during fuzzing

## Additional Materials

### Supplementary Materials (to be prepared)
1. Complete experimental data
2. Statistical analysis (mean, median, standard deviation)
3. Full benchmark descriptions
4. Extended ablation study results
5. Parameter sensitivity analysis

### Artifact Evaluation Package
For artifact evaluation (if applicable), prepare:
1. Docker image with GFuzz pre-installed
2. Step-by-step reproduction instructions
3. Expected results and tolerances
4. Troubleshooting guide

## Citation

If you use this work, please cite:

```bibtex
@article{gfuzz2025,
  title={Variable State Diversity-Guided Directed Fuzzing},
  author={[To be updated]},
  journal={Journal of Systems and Software},
  year={2025},
  note={Under review}
}
```

## Contact

For questions about the paper or GFuzz implementation:
- GitHub: https://github.com/cyhhtl999520/aflgo
- Issues: https://github.com/cyhhtl999520/aflgo/issues

## License

The paper content is subject to standard academic copyright.
The GFuzz implementation is licensed under Apache 2.0 (see main repository).

## Version History

- v1.0 (2025-11-03): Initial manuscript submission
