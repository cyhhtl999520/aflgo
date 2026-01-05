#!/bin/bash
# migrate_to_vsdfuzz.sh
# Script to migrate GFuzz code to a new VSDFuzz repository

set -e

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║                                                               ║"
echo "║           VSDFuzz Migration Script                           ║"
echo "║     Copy GFuzz to New VSDFuzz Repository                     ║"
echo "║                                                               ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Configuration
VSDFUZZ_DIR="../VSDFuzz"
CURRENT_DIR=$(pwd)

# Check if running from aflgo repository
if [ ! -f "GFUZZ_README.md" ]; then
    echo "Error: This script must be run from the aflgo repository root"
    echo "Current directory: $CURRENT_DIR"
    exit 1
fi

# Check if VSDFuzz directory exists
if [ ! -d "$VSDFUZZ_DIR" ]; then
    echo "Error: VSDFuzz directory not found at: $VSDFUZZ_DIR"
    echo ""
    echo "Please create the VSDFuzz repository first:"
    echo "  1. Go to GitHub and create new repository: VSDFuzz"
    echo "  2. Clone it: git clone https://github.com/[your-username]/VSDFuzz.git"
    echo "  3. Make sure it's in the same parent directory as aflgo"
    echo ""
    exit 1
fi

echo "Target directory: $VSDFUZZ_DIR"
echo ""

# Step 1: Copy documentation files
echo "Step 1: Copying documentation files..."
cp -v GFUZZ_README.md "$VSDFUZZ_DIR/README.md"
cp -v GFUZZ_IMPLEMENTATION.md "$VSDFUZZ_DIR/IMPLEMENTATION.md"
cp -v GFUZZ_INTEGRATION_GUIDE.md "$VSDFUZZ_DIR/INTEGRATION_GUIDE.md"
cp -v GFUZZ_COMPLETE_SUMMARY.md "$VSDFUZZ_DIR/DOCUMENTATION.md"

if [ -f "LICENSE" ]; then
    cp -v LICENSE "$VSDFUZZ_DIR/LICENSE"
fi

echo "✓ Documentation files copied"
echo ""

# Step 2: Copy paper directory
echo "Step 2: Copying paper directory..."
if [ -d "paper" ]; then
    cp -rv paper "$VSDFUZZ_DIR/"
    echo "✓ Paper directory copied"
else
    echo "⚠ Warning: paper directory not found"
fi
echo ""

# Step 3: Copy configuration files
echo "Step 3: Copying configuration files..."
if [ -f ".gitignore" ]; then
    cp -v .gitignore "$VSDFUZZ_DIR/"
fi
echo "✓ Configuration files copied"
echo ""

# Step 4: Text replacements
echo "Step 4: Performing text replacements..."
cd "$VSDFUZZ_DIR"

# Count files to be modified
echo "Scanning files for replacements..."
MD_FILES=$(find . -type f -name "*.md" | wc -l)
TEX_FILES=$(find . -type f -name "*.tex" | wc -l)
PY_FILES=$(find . -type f -name "*.py" | wc -l)

echo "  - Found $MD_FILES Markdown files"
echo "  - Found $TEX_FILES LaTeX files"
echo "  - Found $PY_FILES Python files"
echo ""

# Replace GFuzz with VSDFuzz
echo "Replacing 'GFuzz' with 'VSDFuzz'..."
find . -type f -name "*.md" -exec sed -i 's/GFuzz/VSDFuzz/g' {} + 2>/dev/null || true
find . -type f -name "*.tex" -exec sed -i 's/GFuzz/VSDFuzz/g' {} + 2>/dev/null || true
find . -type f -name "*.py" -exec sed -i 's/GFuzz/VSDFuzz/g' {} + 2>/dev/null || true
find . -type f -name "*.sh" -exec sed -i 's/GFuzz/VSDFuzz/g' {} + 2>/dev/null || true

# Replace gfuzz with vsdfuzz (lowercase)
echo "Replacing 'gfuzz' with 'vsdfuzz' (lowercase)..."
find . -type f -name "*.md" -exec sed -i 's/gfuzz/vsdfuzz/g' {} + 2>/dev/null || true
find . -type f -name "*.py" -exec sed -i 's/gfuzz/vsdfuzz/g' {} + 2>/dev/null || true

# Specific replacements for key phrases
echo "Removing AFLGo extension references..."

# In README.md
if [ -f "README.md" ]; then
    sed -i 's/is an extension of AFLGo/is a directed greybox fuzzer/g' README.md
    sed -i 's/extends AFLGo/introduces novel techniques/g' README.md
    sed -i 's/built on top of AFLGo/implements directed fuzzing/g' README.md
fi

# In paper
if [ -f "paper/manuscript.tex" ]; then
    sed -i 's/We implemented VSDFuzz as an extension of AFLGo/We implemented VSDFuzz as a directed greybox fuzzer/g' paper/manuscript.tex
    sed -i 's/VSDFuzz extends AFLGo/VSDFuzz implements directed fuzzing/g' paper/manuscript.tex
fi

echo "✓ Text replacements completed"
echo ""

# Step 5: Update figure titles
echo "Step 5: Updating figure generation scripts..."
if [ -d "paper/figures" ]; then
    cd paper/figures
    
    # Update titles in Python scripts
    if [ -f "generate_figure1.py" ]; then
        sed -i "s/GFuzz's state-diversity-guided approach/VSDFuzz's state-diversity-guided approach/g" generate_figure1.py
        sed -i "s/'GFuzz/'VSDFuzz/g" generate_figure1.py
    fi
    
    if [ -f "generate_figure2.py" ]; then
        sed -i "s/GFuzz architecture/VSDFuzz architecture/g" generate_figure2.py
        sed -i "s/'GFuzz/'VSDFuzz/g" generate_figure2.py
    fi
    
    if [ -f "generate_figure3.py" ]; then
        sed -i "s/GFuzz/VSDFuzz/g" generate_figure3.py
    fi
    
    cd ../..
    echo "✓ Figure scripts updated"
else
    echo "⚠ Warning: paper/figures directory not found"
fi
echo ""

# Step 6: Create initial commit message
echo "Step 6: Preparing for Git commit..."
cat > COMMIT_MESSAGE.txt << 'EOF'
Initial commit: VSDFuzz - Variable State Diversity-Guided Directed Fuzzing

VSDFuzz is a directed greybox fuzzer that incorporates variable state 
diversity to enhance vulnerability detection. By monitoring runtime states 
of key variables and using state diversity as additional feedback, VSDFuzz 
significantly improves upon traditional control-flow-guided approaches.

Key Features:
- Multi-strategy variable identification
- Fine-grained state monitoring with type-specific handling
- Diversity-based seed scheduling
- Adaptive weight balancing

Performance Improvements (vs AFLGo baseline):
- +17.7% more unique paths discovered
- +12.0% better code coverage
- +26.0% more unique crashes detected
- -28.1% faster time to first crash

This repository includes:
- Complete implementation documentation
- Academic paper with full methodology and evaluation
- Figure generation code (Python scripts)
- Usage examples and integration guide
EOF

echo "✓ Commit message prepared (COMMIT_MESSAGE.txt)"
echo ""

# Step 7: Summary and next steps
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║                    Migration Complete!                       ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""
echo "Files copied to: $VSDFUZZ_DIR"
echo ""
echo "Summary of changes:"
echo "  ✓ Documentation files copied and updated"
echo "  ✓ Paper directory copied with figures"
echo "  ✓ Text replacements: GFuzz → VSDFuzz"
echo "  ✓ Removed AFLGo extension references"
echo "  ✓ Figure generation scripts updated"
echo ""
echo "Next steps:"
echo ""
echo "1. Review the changes:"
echo "   cd $VSDFUZZ_DIR"
echo "   git status"
echo "   git diff README.md"
echo ""
echo "2. Test paper compilation:"
echo "   cd paper"
echo "   ./compile.sh"
echo ""
echo "3. Generate figures:"
echo "   cd paper/figures"
echo "   pip3 install matplotlib numpy"
echo "   ./generate_all_figures.sh"
echo ""
echo "4. Manual review needed for:"
echo "   - README.md: Check installation instructions"
echo "   - IMPLEMENTATION.md: Verify technical details"
echo "   - paper/manuscript.tex: Review all AFLGo references"
echo "   - Add examples/ directory if needed"
echo ""
echo "5. Commit and push:"
echo "   git add ."
echo "   git commit -F COMMIT_MESSAGE.txt"
echo "   git push origin main"
echo ""
echo "6. Additional setup:"
echo "   - Add repository topics/tags on GitHub"
echo "   - Create releases"
echo "   - Add badges to README"
echo "   - Set up GitHub Actions (optional)"
echo ""
echo "For detailed instructions, see: VSDFUZZ_MIGRATION_GUIDE.md"
echo ""
