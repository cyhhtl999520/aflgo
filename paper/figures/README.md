# Figure Descriptions for GFuzz Paper

This directory contains placeholder descriptions for the figures referenced in the manuscript. Actual figure files should be created as PDF or PNG images.

## Required Figures

### Figure 1: approach_comparison.pdf
**Caption**: Comparison between traditional control-flow-guided directed fuzzing (left) and GFuzz's state-diversity-guided approach (right). Traditional approaches focus on reaching targets through control flow, while GFuzz additionally explores diverse variable states.

**Description**: 
- Left panel: Shows a program with entry point, multiple paths, and a target location. Arrows indicate control flow paths, with traditional approach highlighting the path to target.
- Right panel: Similar structure but with additional dimension showing variable state space exploration around the target location as a scatter plot or heat map.

**Suggested dimensions**: 800x400 pixels or similar landscape format

---

### Figure 2: gfuzz_architecture.pdf
**Caption**: GFuzz architecture showing the four main components and their interactions during preprocessing and fuzzing phases.

**Description**:
- Shows workflow diagram with two main phases:
  - Preprocessing Phase: Target specification → Key Variable Identification → Distance Computation → Instrumentation
  - Fuzzing Phase: Seed Selection → Mutation → Execution with State Monitoring → Diversity Evaluation → Adaptive Scheduling → Feedback Loop
- Boxes for each component with arrows showing data flow
- Distinguish between offline (preprocessing) and online (runtime) components

**Suggested dimensions**: 1000x600 pixels, flowchart style

---

### Figure 3: coverage_over_time.pdf
**Caption**: Edge coverage over time for libxml2, showing GFuzz (blue) consistently outperforming AFLGo (orange) throughout the six-hour fuzzing campaign.

**Description**:
- Line graph with time (0-6 hours) on X-axis
- Edge coverage percentage (0-60%) on Y-axis  
- Two lines: GFuzz (blue, higher) and AFLGo (orange, lower)
- Shaded regions indicating standard deviation across 10 trials
- GFuzz line should show steady growth and plateau around 51-52%
- AFLGo line should show similar pattern but plateau around 46-47%

**Suggested dimensions**: 600x400 pixels

---

## Creating the Figures

To create these figures, you can use:
- **Drawing tools**: Inkscape, Adobe Illustrator, or PowerPoint for architecture diagrams
- **Plotting libraries**: matplotlib, gnuplot, or R ggplot2 for data plots
- **Export format**: PDF (preferred for LaTeX) or high-resolution PNG (300 dpi minimum)

## Placeholder Files

If you want to compile the manuscript before creating actual figures, you can:

1. Create placeholder PDF files with simple text:
```bash
# Using ImageMagick (if available)
convert -size 800x400 xc:white -pointsize 24 -draw "text 200,200 'Figure Placeholder'" approach_comparison.pdf
convert -size 1000x600 xc:white -pointsize 24 -draw "text 300,300 'Architecture Diagram'" gfuzz_architecture.pdf
convert -size 600x400 xc:white -pointsize 24 -draw "text 150,200 'Coverage Plot'" coverage_over_time.pdf
```

2. Or use LaTeX's draft mode which shows figure placeholders automatically.

## Notes

- All figures should be referenced in the text before they appear
- Figures should be placed at the top or bottom of pages (using [t] or [b] placement)
- Each figure must have a descriptive caption
- Color figures are acceptable for JSS online version
- Ensure figures are readable when printed in grayscale
