#!/usr/bin/env python3
"""
Generate Figure 1: Comparison between traditional control-flow-guided 
directed fuzzing and GFuzz's state-diversity-guided approach.
"""

import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch
import numpy as np

# Set up the figure
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

# Common settings
colors = {
    'entry': '#4CAF50',
    'target': '#F44336',
    'path': '#2196F3',
    'state': '#FF9800',
    'background': '#f5f5f5'
}

def draw_control_flow_only(ax):
    """Draw traditional control-flow-guided fuzzing (left panel)"""
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 10)
    ax.axis('off')
    ax.set_title('Traditional Control-Flow\nDirected Fuzzing', 
                 fontsize=14, fontweight='bold', pad=20)
    
    # Entry point
    entry = FancyBboxPatch((1, 8), 2, 1, boxstyle="round,pad=0.1",
                           edgecolor=colors['entry'], facecolor=colors['entry'],
                           linewidth=2, alpha=0.7)
    ax.add_patch(entry)
    ax.text(2, 8.5, 'Entry', ha='center', va='center', 
            fontsize=11, fontweight='bold', color='white')
    
    # Target
    target = FancyBboxPatch((7, 1), 2, 1, boxstyle="round,pad=0.1",
                           edgecolor=colors['target'], facecolor=colors['target'],
                           linewidth=2, alpha=0.7)
    ax.add_patch(target)
    ax.text(8, 1.5, 'Target', ha='center', va='center',
            fontsize=11, fontweight='bold', color='white')
    
    # Intermediate nodes representing program paths
    nodes = [
        (2, 6.5), (4, 7), (4, 5), (6, 6), (6, 4), (7, 3)
    ]
    
    for x, y in nodes:
        circle = plt.Circle((x, y), 0.3, color=colors['path'], 
                           alpha=0.5, zorder=2)
        ax.add_patch(circle)
    
    # Draw paths (arrows) showing control flow
    paths = [
        ((2, 8.5), (2, 6.5)),
        ((2, 6.5), (4, 7)),
        ((2, 6.5), (4, 5)),
        ((4, 7), (6, 6)),
        ((4, 5), (6, 4)),
        ((6, 6), (7, 3)),
        ((6, 4), (7, 3)),
        ((7, 3), (8, 1.5))
    ]
    
    for start, end in paths:
        arrow = FancyArrowPatch(start, end,
                               arrowstyle='->', mutation_scale=20,
                               color=colors['path'], linewidth=2,
                               alpha=0.6, zorder=1)
        ax.add_patch(arrow)
    
    # Add annotation
    ax.text(5, 0.3, 'Focus: Reaching Target via Control Flow',
            ha='center', fontsize=10, style='italic',
            bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.3))

def draw_state_diversity_guided(ax):
    """Draw GFuzz's state-diversity-guided approach (right panel)"""
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 10)
    ax.axis('off')
    ax.set_title('GFuzz: State-Diversity\nGuided Approach', 
                 fontsize=14, fontweight='bold', pad=20)
    
    # Entry point
    entry = FancyBboxPatch((1, 8), 2, 1, boxstyle="round,pad=0.1",
                           edgecolor=colors['entry'], facecolor=colors['entry'],
                           linewidth=2, alpha=0.7)
    ax.add_patch(entry)
    ax.text(2, 8.5, 'Entry', ha='center', va='center',
            fontsize=11, fontweight='bold', color='white')
    
    # Target with state space around it
    target = FancyBboxPatch((7, 1), 2, 1, boxstyle="round,pad=0.1",
                           edgecolor=colors['target'], facecolor=colors['target'],
                           linewidth=2, alpha=0.7)
    ax.add_patch(target)
    ax.text(8, 1.5, 'Target', ha='center', va='center',
            fontsize=11, fontweight='bold', color='white')
    
    # Intermediate nodes
    nodes = [
        (2, 6.5), (4, 7), (4, 5), (6, 6), (6, 4), (7, 3)
    ]
    
    for x, y in nodes:
        circle = plt.Circle((x, y), 0.3, color=colors['path'],
                           alpha=0.5, zorder=2)
        ax.add_patch(circle)
    
    # Draw control flow paths
    paths = [
        ((2, 8.5), (2, 6.5)),
        ((2, 6.5), (4, 7)),
        ((2, 6.5), (4, 5)),
        ((4, 7), (6, 6)),
        ((4, 5), (6, 4)),
        ((6, 6), (7, 3)),
        ((6, 4), (7, 3)),
        ((7, 3), (8, 1.5))
    ]
    
    for start, end in paths:
        arrow = FancyArrowPatch(start, end,
                               arrowstyle='->', mutation_scale=20,
                               color=colors['path'], linewidth=2,
                               alpha=0.6, zorder=1)
        ax.add_patch(arrow)
    
    # Add state space visualization around target
    # Generate random points representing different variable states
    np.random.seed(42)
    n_states = 40
    # Cluster states around target area
    state_x = 8 + np.random.randn(n_states) * 1.2
    state_y = 1.5 + np.random.randn(n_states) * 1.2
    
    # Filter to reasonable bounds
    state_x = np.clip(state_x, 5.5, 9.5)
    state_y = np.clip(state_y, 0, 4)
    
    scatter = ax.scatter(state_x, state_y, c=colors['state'], 
                        s=30, alpha=0.6, marker='o', 
                        edgecolors='darkgoldenrod', linewidth=0.5,
                        zorder=3, label='Variable States')
    
    # Draw ellipse to indicate state space region
    from matplotlib.patches import Ellipse
    ellipse = Ellipse((8, 1.5), 3.5, 3, angle=0,
                     edgecolor=colors['state'], facecolor='none',
                     linewidth=2, linestyle='--', alpha=0.7)
    ax.add_patch(ellipse)
    
    # Add state space label
    ax.text(8, 4.2, 'Variable\nState Space', ha='center', 
            fontsize=9, color=colors['state'], fontweight='bold',
            bbox=dict(boxstyle='round', facecolor='white', 
                     edgecolor=colors['state'], alpha=0.8))
    
    # Add annotation
    ax.text(5, 0.3, 'Focus: Control Flow + State Diversity',
            ha='center', fontsize=10, style='italic',
            bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.3))

# Draw both panels
draw_control_flow_only(ax1)
draw_state_diversity_guided(ax2)

plt.tight_layout()
plt.savefig('approach_comparison.pdf', format='pdf', dpi=300, bbox_inches='tight')
plt.savefig('approach_comparison.png', format='png', dpi=300, bbox_inches='tight')
print("Generated: approach_comparison.pdf and approach_comparison.png")
plt.close()
