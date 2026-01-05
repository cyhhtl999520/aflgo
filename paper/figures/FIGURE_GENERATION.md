# Figure Generation Scripts for GFuzz Paper

This directory contains Python scripts to generate all three figures for the GFuzz paper.

## Prerequisites

You need Python 3 with matplotlib and numpy installed:

```bash
pip3 install matplotlib numpy
```

## Quick Start - Generate All Figures

Run the master script to generate all figures at once:

```bash
./generate_all_figures.sh
```

This will create both PDF (for LaTeX) and PNG (for preview) versions of all figures.

## Individual Figure Scripts

You can also generate figures individually:

### Figure 1: Approach Comparison
```bash
python3 generate_figure1.py
```
**Output:** `approach_comparison.pdf` and `approach_comparison.png`

**Description:** Side-by-side comparison showing traditional control-flow-guided directed fuzzing (left) versus GFuzz's state-diversity-guided approach (right). The left panel shows control flow paths from entry to target. The right panel adds a visualization of variable state space exploration around the target location.

**Dimensions:** 12x5 inches (landscape, two panels)

### Figure 2: GFuzz Architecture
```bash
python3 generate_figure2.py
```
**Output:** `gfuzz_architecture.pdf` and `gfuzz_architecture.png`

**Description:** Comprehensive architecture diagram showing the four main components and their interactions during preprocessing (offline) and fuzzing (runtime) phases. Shows the complete workflow from target specification through instrumentation to the fuzzing loop with feedback.

**Dimensions:** 14x10 inches (large flowchart)

**Components shown:**
1. Key Variable Identification (preprocessing)
2. Distance Computation (preprocessing)
3. Variable State Monitoring (runtime)
4. State Diversity Evaluation (runtime)
5. Adaptive Scheduling (runtime)
6. Feedback loop

### Figure 3: Coverage Over Time
```bash
python3 generate_figure3.py
```
**Output:** `coverage_over_time.pdf` and `coverage_over_time.png`

**Description:** Line graph showing edge coverage percentage over a 6-hour fuzzing campaign for the libxml2 benchmark. Compares GFuzz (blue, reaching ~51.7%) against AFLGo (orange, reaching ~46.3%). Includes shaded regions showing ±1 standard deviation across 10 trials.

**Dimensions:** 10x6 inches (standard plot)

## Script Details

### generate_figure1.py
- Uses matplotlib with patches and arrows
- Two side-by-side subplots
- Shows control flow graph with nodes and directed edges
- Right panel adds scattered points representing variable states
- Professional color scheme with rounded boxes

### generate_figure2.py
- Complex flowchart with multiple components
- Color-coded phases (preprocessing vs runtime)
- Component boxes with titles and descriptions
- Directed arrows showing data flow
- Feedback loop visualization
- Legend explaining colors and arrow types

### generate_figure3.py
- Time-series line plot with confidence intervals
- Two lines (GFuzz and AFLGo) with markers
- Shaded regions for standard deviation
- Annotations showing final values and improvement percentage
- Grid and professional styling

## Output Files

Each script generates two versions:
- **PDF version**: High-quality vector graphics for LaTeX compilation
- **PNG version**: Raster graphics for quick preview and presentations

## Customization

All scripts use configurable parameters at the top:
- Colors (defined in color dictionaries)
- Font sizes
- Line widths
- Figure dimensions
- Data points

To customize, edit the respective Python script and adjust the parameters.

## Troubleshooting

### Import Error: No module named 'matplotlib'
Install matplotlib:
```bash
pip3 install matplotlib numpy
```

### Permission Denied
Make scripts executable:
```bash
chmod +x generate_all_figures.sh
chmod +x generate_figure*.py
```

### Font Warnings
Matplotlib may show font warnings. These are non-critical and figures will still generate correctly. To suppress:
```bash
python3 -W ignore generate_figure1.py
```

## Integration with LaTeX

The generated PDF files are already referenced in `manuscript.tex`:
- `\includegraphics{figures/approach_comparison.pdf}`
- `\includegraphics{figures/gfuzz_architecture.pdf}`
- `\includegraphics{figures/coverage_over_time.pdf}`

After generating the figures, simply compile the manuscript:
```bash
cd ..
./compile.sh
```

## Quality Settings

All figures are generated with:
- **DPI:** 300 (publication quality)
- **Format:** PDF (vector) and PNG (raster)
- **Bbox:** Tight (minimal whitespace)
- **Transparency:** Supported where needed

## Notes

- Figures are designed for the Journal of Systems & Software format
- All dimensions are optimized for the two-column layout
- Colors are chosen to be distinguishable in both color and grayscale
- Professional styling following academic publication standards

## Support

For issues or customization help, refer to the matplotlib documentation:
https://matplotlib.org/stable/index.html
