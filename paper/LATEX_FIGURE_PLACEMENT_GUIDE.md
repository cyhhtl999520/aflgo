# LaTeX Figure and Table Placement Guide
# LaTeX图表位置控制指南

## Problem / 问题

Figures and tables appearing at the end of the document instead of near their reference points.

图片和表格出现在文档末尾而不是在引用它们的位置附近。

## Solution Applied / 已应用的解决方案

### 1. Added float package / 添加float包

```latex
\usepackage{float}  % For better figure placement control
```

This package provides enhanced control over float placement, including the `H` specifier.

该包提供了对浮动体位置的增强控制，包括`H`选项。

### 2. Changed placement specifiers / 更改位置指定符

**Before / 之前:**
```latex
\begin{figure}[t]   % Only try top of page
\begin{table}[t]    % Only try top of page
```

**After / 之后:**
```latex
\begin{figure}[!htbp]   % Try here, top, bottom, separate page (override restrictions)
\begin{table}[!htbp]    % Try here, top, bottom, separate page (override restrictions)
```

### Explanation of Specifiers / 位置指定符说明

| Specifier | Meaning | 含义 |
|-----------|---------|------|
| `h` | here - Try to place at the current position | 此处 - 尝试放在当前位置 |
| `t` | top - Try to place at top of page | 顶部 - 尝试放在页面顶部 |
| `b` | bottom - Try to place at bottom of page | 底部 - 尝试放在页面底部 |
| `p` | page - Place on a separate float page | 单独页 - 放在单独的浮动页上 |
| `!` | override - Override LaTeX's internal restrictions | 覆盖 - 覆盖LaTeX的内部限制 |
| `H` | HERE - Force placement exactly here (requires float package) | 强制 - 强制放在确切位置（需要float包） |

## Additional Tips / 额外建议

### Option 1: More Aggressive Placement (Current) / 方案1：更激进的位置控制（当前）

Use `[!htbp]` to give LaTeX multiple options and override internal restrictions:

使用`[!htbp]`给LaTeX多个选项并覆盖内部限制：

```latex
\begin{figure}[!htbp]
\centering
\includegraphics[width=0.8\columnwidth]{figures/your_figure.pdf}
\caption{Your caption here}
\label{fig:your_label}
\end{figure}
```

**Advantages / 优点:**
- Figures usually appear close to where they're referenced
- LaTeX still has some flexibility to optimize page breaks
- Works well with elsarticle document class

**Disadvantages / 缺点:**
- Figures might not be in exact position
- May cause some layout adjustments

### Option 2: Force Exact Position / 方案2：强制精确位置

If you need figures in exact positions, use `[H]`:

如果需要图片在精确位置，使用`[H]`：

```latex
\begin{figure}[H]  % Capital H, not lowercase h
\centering
\includegraphics[width=0.8\columnwidth]{figures/your_figure.pdf}
\caption{Your caption here}
\label{fig:your_label}
\end{figure}
```

**Advantages / 优点:**
- Figure appears exactly where you place it in the source
- Complete control over positioning

**Disadvantages / 缺点:**
- Can cause awkward page breaks
- May leave large white spaces
- Less flexible for journal formatting requirements
- **Not recommended for journal submissions** (editors prefer float placement)

### Option 3: Reduce Figure Size / 方案3：减小图片尺寸

If figures are too large, LaTeX can't find good placement. Try reducing size:

如果图片太大，LaTeX找不到合适的位置。尝试减小尺寸：

```latex
% Before / 之前
\includegraphics[width=\columnwidth]{figures/your_figure.pdf}

% After / 之后
\includegraphics[width=0.8\columnwidth]{figures/your_figure.pdf}
\includegraphics[width=0.7\columnwidth]{figures/your_figure.pdf}
```

### Option 4: Use placeins package / 方案4：使用placeins包

Prevent floats from moving past section boundaries:

防止浮动体跨越章节边界：

```latex
\usepackage{placeins}

% In document
\section{Your Section}
% Your content and figures
\FloatBarrier  % Prevents figures from floating past this point
```

## LaTeX Float Parameters / LaTeX浮动体参数调整

You can also adjust LaTeX's float placement parameters in the preamble:

你也可以在导言区调整LaTeX的浮动体位置参数：

```latex
% Allow more floats per page
\setcounter{topnumber}{3}        % Max floats at top (default: 2)
\setcounter{bottomnumber}{2}     % Max floats at bottom (default: 1)
\setcounter{totalnumber}{4}      % Max floats per page (default: 3)

% Adjust float page thresholds
\renewcommand{\topfraction}{0.9}      % Max fraction of page for top floats (default: 0.7)
\renewcommand{\bottomfraction}{0.8}   % Max fraction of page for bottom floats (default: 0.3)
\renewcommand{\textfraction}{0.1}     % Min fraction of page for text (default: 0.2)
\renewcommand{\floatpagefraction}{0.8} % Min fraction of page for float page (default: 0.5)
```

## Best Practices for Journal Submissions / 期刊投稿最佳实践

### For JSS (Journal of Systems & Software) / 对于JSS期刊

1. **Use `[!htbp]` placement** ✓ (Current implementation)
   - Provides flexibility while keeping figures near text
   - Allows editors to adjust if needed

2. **Avoid `[H]` for journal submissions**
   - Editors prefer float placement flexibility
   - Better for final typesetting

3. **Keep figures reasonably sized**
   - 0.7-0.8 columnwidth for single-column figures
   - Full columnwidth for important architecture diagrams

4. **Place figure code near first reference**
   - Place `\begin{figure}...\end{figure}` right after the paragraph that first references it
   - This helps LaTeX find good placement

5. **Don't worry too much about exact placement in review version**
   - Reviewers focus on content, not exact figure position
   - Final typesetting is done by journal production team

## Verification / 验证

After making changes, compile the paper:

更改后编译论文以验证：

```bash
cd paper
./compile.sh
```

Check the generated PDF:
- Figures should appear near their first reference
- No large blank spaces in the text
- Figures should not all appear at the end

检查生成的PDF：
- 图片应该出现在首次引用附近
- 文本中没有大片空白
- 图片不应该全部出现在末尾

## Current Implementation / 当前实现

The manuscript has been updated with:
- ✓ `\usepackage{float}` added to preamble
- ✓ All figures use `[!htbp]` placement
- ✓ All tables use `[!htbp]` placement

当前论文已更新：
- ✓ 在导言区添加了`\usepackage{float}`
- ✓ 所有图片使用`[!htbp]`位置指定
- ✓ 所有表格使用`[!htbp]`位置指定

This should resolve the issue of figures appearing at the end of the document.

这应该能解决图片出现在文档末尾的问题。

## Troubleshooting / 故障排除

### Issue: Figures still at the end / 问题：图片仍在末尾

Try these solutions in order:

按顺序尝试以下解决方案：

1. **Reduce figure size**
   ```latex
   \includegraphics[width=0.7\columnwidth]{...}  % Instead of 0.8 or 1.0
   ```

2. **Add more text after figure reference**
   - LaTeX needs text to flow around figures
   - Add at least 2-3 paragraphs after figure code

3. **Use `[H]` for specific figures** (only if necessary)
   ```latex
   \begin{figure}[H]  % Forces exact placement
   ```

4. **Split large figures into subfigures**
   ```latex
   \begin{figure}[!htbp]
   \centering
   \begin{subfigure}{0.45\columnwidth}
     \includegraphics[width=\textwidth]{fig1.pdf}
     \caption{Part 1}
   \end{subfigure}
   \hfill
   \begin{subfigure}{0.45\columnwidth}
     \includegraphics[width=\textwidth]{fig2.pdf}
     \caption{Part 2}
   \end{subfigure}
   \caption{Overall caption}
   \end{figure}
   ```

### Issue: Figures overlap text / 问题：图片与文字重叠

If using `[H]`, switch back to `[!htbp]` to let LaTeX handle placement.

如果使用`[H]`，切换回`[!htbp]`让LaTeX处理位置。

### Issue: Compilation errors / 问题：编译错误

Make sure float package is installed:

确保安装了float包：

```bash
sudo apt-get install texlive-latex-extra  # Ubuntu/Debian
```

## References / 参考资料

- LaTeX float placement: https://en.wikibooks.org/wiki/LaTeX/Floats,_Figures_and_Captions
- float package documentation: https://ctan.org/pkg/float
- elsarticle documentation: https://www.elsevier.com/authors/policies-and-guidelines/latex-instructions

---

**Last Updated:** 2025-11-11  
**Version:** 1.0  
**Status:** Applied to manuscript.tex
