#!/usr/bin/env python3
"""
Generate Figure 2: GFuzz architecture showing the four main components 
and their interactions during preprocessing and fuzzing phases.
"""

import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch, Rectangle
import matplotlib.lines as mlines

# Set up the figure with more space
fig, ax = plt.subplots(figsize=(14, 10))
ax.set_xlim(0, 14)
ax.set_ylim(0, 10)
ax.axis('off')

# Color scheme
colors = {
    'preprocessing': '#E3F2FD',  # Light blue
    'runtime': '#FFF3E0',         # Light orange
    'component': '#2196F3',       # Blue
    'data': '#4CAF50',            # Green
    'arrow': '#424242',           # Dark gray
    'phase_label': '#1976D2'      # Dark blue
}

# Phase backgrounds
preprocessing_bg = Rectangle((0.3, 6), 6.4, 3.5, 
                            facecolor=colors['preprocessing'],
                            edgecolor=colors['phase_label'],
                            linewidth=2, alpha=0.3)
ax.add_patch(preprocessing_bg)

runtime_bg = Rectangle((7.3, 1), 6.4, 8.5,
                       facecolor=colors['runtime'],
                       edgecolor='#E65100',
                       linewidth=2, alpha=0.3)
ax.add_patch(runtime_bg)

# Phase labels
ax.text(3.5, 9.7, 'PREPROCESSING PHASE', fontsize=13, fontweight='bold',
        ha='center', color=colors['phase_label'],
        bbox=dict(boxstyle='round', facecolor='white', 
                 edgecolor=colors['phase_label'], linewidth=2))

ax.text(10.5, 9.7, 'FUZZING PHASE (Runtime)', fontsize=13, fontweight='bold',
        ha='center', color='#E65100',
        bbox=dict(boxstyle='round', facecolor='white',
                 edgecolor='#E65100', linewidth=2))

def create_component_box(ax, x, y, width, height, title, description, color):
    """Create a component box with title and description"""
    box = FancyBboxPatch((x, y), width, height,
                         boxstyle="round,pad=0.05",
                         edgecolor=color, facecolor=color,
                         linewidth=2.5, alpha=0.2)
    ax.add_patch(box)
    
    # Border
    border = FancyBboxPatch((x, y), width, height,
                           boxstyle="round,pad=0.05",
                           edgecolor=color, facecolor='none',
                           linewidth=2.5)
    ax.add_patch(border)
    
    # Title
    ax.text(x + width/2, y + height - 0.25, title,
           ha='center', va='top', fontsize=10, fontweight='bold',
           color=color)
    
    # Description
    ax.text(x + width/2, y + height/2 - 0.1, description,
           ha='center', va='center', fontsize=8,
           color='black', wrap=True, multialignment='center')

def create_arrow(ax, start, end, label='', style='->', color='#424242', width=2):
    """Create an arrow with optional label"""
    arrow = FancyArrowPatch(start, end,
                           arrowstyle=style, mutation_scale=25,
                           color=color, linewidth=width,
                           zorder=1)
    ax.add_patch(arrow)
    
    if label:
        mid_x = (start[0] + end[0]) / 2
        mid_y = (start[1] + end[1]) / 2
        ax.text(mid_x, mid_y + 0.15, label, fontsize=8,
               ha='center', style='italic',
               bbox=dict(boxstyle='round', facecolor='white',
                        edgecolor=color, alpha=0.9, pad=0.3))

# Input at top
input_box = FancyBboxPatch((1, 9.2), 1.5, 0.5,
                          boxstyle="round,pad=0.05",
                          edgecolor=colors['data'], facecolor=colors['data'],
                          linewidth=2, alpha=0.7)
ax.add_patch(input_box)
ax.text(1.75, 9.45, 'Target\nSpec', ha='center', va='center',
       fontsize=9, fontweight='bold', color='white')

# Component 1: Key Variable Identification
create_component_box(ax, 0.5, 7, 2.5, 1.5,
                    'Component 1:\nKey Variable\nIdentification',
                    'Distance-based\nMemory-safety\nSemantic filtering',
                    colors['component'])

# Component 2: Distance Computation
create_component_box(ax, 3.5, 7, 2.5, 1.5,
                    'Component 2:\nDistance\nComputation',
                    'Call graph\nCFG analysis\nTarget distances',
                    colors['component'])

# Instrumentation
create_component_box(ax, 2, 6.5, 2, 0.4,
                    'Instrumentation',
                    '',
                    '#9C27B0')

# Component 3: Variable State Monitoring
create_component_box(ax, 7.8, 7, 2.5, 1.5,
                    'Component 3:\nState\nMonitoring',
                    'Type-specific\nencoding\nShared memory',
                    '#FF5722')

# Component 4: State Diversity Evaluation
create_component_box(ax, 10.8, 7, 2.5, 1.5,
                    'Component 4:\nDiversity\nEvaluation',
                    'Similarity metrics\nState history\nDiversity score',
                    '#FF5722')

# Seed Selection
create_component_box(ax, 7.8, 5, 2.5, 1.2,
                    'Seed\nSelection',
                    'Combined score\n(coverage + diversity)',
                    '#FFC107')

# Mutation & Execution
create_component_box(ax, 10.8, 5, 2.5, 1.2,
                    'Mutation &\nExecution',
                    'Energy allocation\nTest generation',
                    '#FFC107')

# Adaptive Scheduling
create_component_box(ax, 9.3, 3.2, 2.5, 1.2,
                    'Adaptive\nScheduling',
                    'Weight adjustment\nλ-based learning',
                    '#8BC34A')

# Seed Corpus (bottom)
corpus_box = FancyBboxPatch((7.8, 1.5), 5.5, 0.8,
                           boxstyle="round,pad=0.05",
                           edgecolor=colors['data'], facecolor=colors['data'],
                           linewidth=2, alpha=0.7)
ax.add_patch(corpus_box)
ax.text(10.55, 1.9, 'Seed Corpus', ha='center', va='center',
       fontsize=10, fontweight='bold', color='white')

# Draw arrows - Preprocessing phase
create_arrow(ax, (1.75, 9.2), (1.75, 8.5), 'BBtargets.txt', color=colors['data'])
create_arrow(ax, (1.75, 8.5), (3, 8.5), '', color=colors['arrow'])
create_arrow(ax, (1.75, 8.5), (2, 7.75), '', color=colors['arrow'])
create_arrow(ax, (3, 7.75), (4.75, 7.75), 'key vars', color=colors['arrow'])
create_arrow(ax, (4.75, 7.2), (3, 6.7), '', color=colors['arrow'])

# Arrow to runtime phase
create_arrow(ax, (4, 6.7), (9.05, 8.5), 'Instrumented\nBinary', 
            style='->', color='#9C27B0', width=3)

# Runtime phase arrows
create_arrow(ax, (9.05, 8.5), (9.05, 7.5), '', color=colors['arrow'])
create_arrow(ax, (12.05, 8.5), (12.05, 7.5), '', color=colors['arrow'])
create_arrow(ax, (9.05, 7), (9.05, 6.2), 'state\nvector', color=colors['arrow'])
create_arrow(ax, (12.05, 7), (12.05, 6.2), 'diversity', color=colors['arrow'])

create_arrow(ax, (10.3, 5.6), (11.8, 5.6), '', color=colors['arrow'])
create_arrow(ax, (11.8, 5), (10.55, 4.4), '', color=colors['arrow'])

# Feedback loop
create_arrow(ax, (10.55, 3.2), (10.55, 2.3), 'update\nweights', color=colors['arrow'])
create_arrow(ax, (7.8, 1.9), (7.8, 5.5), 'select\nseed', 
            style='->', color='#8BC34A', width=2)

# Add a circular arrow for the feedback loop
from matplotlib.patches import Arc
arc = Arc((10.55, 3.8), 4, 3, angle=0, theta1=180, theta2=360,
         color='#8BC34A', linewidth=2.5, linestyle='--')
ax.add_patch(arc)
ax.text(12.8, 4.5, 'Feedback\nLoop', fontsize=8, style='italic',
       color='#8BC34A', fontweight='bold')

# Add legend
legend_elements = [
    mpatches.Patch(facecolor=colors['preprocessing'], edgecolor=colors['phase_label'],
                  alpha=0.3, label='Preprocessing (Offline)'),
    mpatches.Patch(facecolor=colors['runtime'], edgecolor='#E65100',
                  alpha=0.3, label='Runtime (Online)'),
    mlines.Line2D([], [], color=colors['data'], linewidth=3, 
                 label='Data Flow', marker='>', markersize=8),
    mlines.Line2D([], [], color='#8BC34A', linewidth=3,
                 label='Feedback', marker='>', markersize=8, linestyle='--')
]
ax.legend(handles=legend_elements, loc='lower left', fontsize=9,
         framealpha=0.9, edgecolor='black')

plt.tight_layout()
plt.savefig('gfuzz_architecture.pdf', format='pdf', dpi=300, bbox_inches='tight')
plt.savefig('gfuzz_architecture.png', format='png', dpi=300, bbox_inches='tight')
print("Generated: gfuzz_architecture.pdf and gfuzz_architecture.png")
plt.close()
