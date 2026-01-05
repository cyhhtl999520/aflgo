#!/bin/bash
# Create placeholder figure files for LaTeX compilation

# Check if convert (ImageMagick) is available
if command -v convert &> /dev/null; then
    echo "Creating placeholder figures with ImageMagick..."
    convert -size 800x400 xc:white -pointsize 20 -gravity center \
        -draw "text 0,0 'Figure 1: Approach Comparison\n(Placeholder - Create actual figure)'" \
        approach_comparison.pdf
    
    convert -size 1000x600 xc:white -pointsize 20 -gravity center \
        -draw "text 0,0 'Figure 2: GFuzz Architecture\n(Placeholder - Create actual figure)'" \
        gfuzz_architecture.pdf
    
    convert -size 600x400 xc:white -pointsize 20 -gravity center \
        -draw "text 0,0 'Figure 3: Coverage Over Time\n(Placeholder - Create actual figure)'" \
        coverage_over_time.pdf
    
    echo "Placeholder figures created successfully!"
else
    echo "ImageMagick not found. Creating empty placeholder files..."
    # Create minimal PDF placeholders
    for fig in approach_comparison gfuzz_architecture coverage_over_time; do
        cat > ${fig}.pdf << 'PDFEOF'
%PDF-1.4
1 0 obj
<<
/Type /Catalog
/Pages 2 0 R
>>
endobj
2 0 obj
<<
/Type /Pages
/Kids [3 0 R]
/Count 1
>>
endobj
3 0 obj
<<
/Type /Page
/Parent 2 0 R
/MediaBox [0 0 400 300]
/Contents 4 0 R
/Resources <<
/Font <<
/F1 <<
/Type /Font
/Subtype /Type1
/BaseFont /Helvetica
>>
>>
>>
>>
endobj
4 0 obj
<<
/Length 60
>>
stream
BT
/F1 12 Tf
100 150 Td
(Placeholder Figure) Tj
ET
endstream
endobj
xref
0 5
0000000000 65535 f
0000000009 00000 n
0000000058 00000 n
0000000115 00000 n
0000000324 00000 n
trailer
<<
/Size 5
/Root 1 0 R
>>
startxref
433
%%EOF
PDFEOF
    done
    echo "Basic placeholder PDFs created."
fi
