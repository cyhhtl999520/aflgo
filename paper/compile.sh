#!/bin/bash

# GFuzz Paper Compilation Script
# Compiles the LaTeX manuscript for Journal of Systems and Software

set -e

MANUSCRIPT="manuscript"

echo "=========================================="
echo "GFuzz Paper Compilation Script"
echo "=========================================="
echo ""

# Check if pdflatex is available
if ! command -v pdflatex &> /dev/null; then
    echo "ERROR: pdflatex not found!"
    echo "Please install LaTeX:"
    echo "  Ubuntu/Debian: sudo apt-get install texlive-full"
    echo "  macOS: brew install --cask mactex"
    exit 1
fi

# Check if bibtex is available
if ! command -v bibtex &> /dev/null; then
    echo "ERROR: bibtex not found!"
    echo "Please install LaTeX with bibtex support"
    exit 1
fi

echo "Step 1/5: First pdflatex pass (generating .aux files)..."
pdflatex -interaction=nonstopmode ${MANUSCRIPT}.tex > /dev/null 2>&1 || {
    echo "WARNING: First pass completed with warnings"
}

echo "Step 2/5: Running bibtex (processing bibliography)..."
bibtex ${MANUSCRIPT} > /dev/null 2>&1 || {
    echo "WARNING: bibtex completed with warnings"
}

echo "Step 3/5: Second pdflatex pass (resolving references)..."
pdflatex -interaction=nonstopmode ${MANUSCRIPT}.tex > /dev/null 2>&1 || {
    echo "WARNING: Second pass completed with warnings"
}

echo "Step 4/5: Third pdflatex pass (finalizing document)..."
pdflatex -interaction=nonstopmode ${MANUSCRIPT}.tex > /dev/null 2>&1 || {
    echo "WARNING: Third pass completed with warnings"
}

echo "Step 5/5: Cleaning up auxiliary files..."
rm -f ${MANUSCRIPT}.aux ${MANUSCRIPT}.log ${MANUSCRIPT}.bbl ${MANUSCRIPT}.blg \
      ${MANUSCRIPT}.out ${MANUSCRIPT}.toc ${MANUSCRIPT}.lof ${MANUSCRIPT}.lot \
      ${MANUSCRIPT}.synctex.gz 2>/dev/null || true

echo ""
echo "=========================================="
echo "Compilation complete!"
echo "=========================================="
echo ""

if [ -f "${MANUSCRIPT}.pdf" ]; then
    PDF_SIZE=$(du -h "${MANUSCRIPT}.pdf" | cut -f1)
    PDF_PAGES=$(pdfinfo "${MANUSCRIPT}.pdf" 2>/dev/null | grep "Pages:" | awk '{print $2}' || echo "?")
    
    echo "Output: ${MANUSCRIPT}.pdf"
    echo "Size: ${PDF_SIZE}"
    echo "Pages: ${PDF_PAGES}"
    echo ""
    echo "You can now view the paper:"
    echo "  Linux: xdg-open ${MANUSCRIPT}.pdf"
    echo "  macOS: open ${MANUSCRIPT}.pdf"
    echo "  Windows: start ${MANUSCRIPT}.pdf"
else
    echo "ERROR: PDF file was not generated!"
    echo "Check for compilation errors in the LaTeX source."
    exit 1
fi

echo ""
echo "Submission checklist:"
echo "  [ ] Review the generated PDF"
echo "  [ ] Check all equations and algorithms"
echo "  [ ] Verify all tables and figures"
echo "  [ ] Proofread for typos"
echo "  [ ] Update author information (if anonymized)"
echo "  [ ] Add acknowledgments with grant numbers"
echo "  [ ] Prepare supplementary materials"
echo ""
