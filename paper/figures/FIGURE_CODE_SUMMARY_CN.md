# 完整的图片生成代码 / Complete Figure Generation Code

## 概述 / Overview

本目录包含生成GFuzz论文中三个图片的完整Python代码。所有脚本使用matplotlib库，可在任何安装了Python 3的系统上运行。

This directory contains complete Python code to generate all three figures for the GFuzz paper. All scripts use matplotlib and can be run on any system with Python 3.

## 安装依赖 / Install Dependencies

```bash
pip3 install matplotlib numpy
```

## 快速生成所有图片 / Quick Generate All Figures

```bash
cd paper/figures
./generate_all_figures.sh
```

## 各个图片详情 / Individual Figure Details

### 图1: 方法对比 / Figure 1: Approach Comparison

**文件 / File:** `generate_figure1.py`

**描述 / Description:**
- 左图：传统的控制流引导定向模糊测试
- 右图：GFuzz的状态多样性引导方法
- 展示了控制流路径和变量状态空间的可视化

**生成 / Generate:**
```bash
python3 generate_figure1.py
```

**输出 / Output:**
- `approach_comparison.pdf` (用于LaTeX / for LaTeX)
- `approach_comparison.png` (用于预览 / for preview)

**尺寸 / Size:** 12x5英寸 / inches

**关键特性 / Key Features:**
- 双面板对比设计
- 入口点到目标的控制流路径
- 状态空间散点图表示
- 专业配色方案
- 圆角矩形和箭头

---

### 图2: GFuzz架构 / Figure 2: GFuzz Architecture  

**文件 / File:** `generate_figure2.py`

**描述 / Description:**
- 完整的系统架构图
- 展示预处理阶段和运行时阶段
- 四个主要组件及其交互
- 数据流和反馈循环

**生成 / Generate:**
```bash
python3 generate_figure2.py
```

**输出 / Output:**
- `gfuzz_architecture.pdf` (用于LaTeX / for LaTeX)
- `gfuzz_architecture.png` (用于预览 / for preview)

**尺寸 / Size:** 14x10英寸 / inches

**组件 / Components:**
1. 关键变量识别 / Key Variable Identification
2. 距离计算 / Distance Computation
3. 变量状态监控 / Variable State Monitoring
4. 状态多样性评估 / State Diversity Evaluation
5. 自适应调度 / Adaptive Scheduling

**设计元素 / Design Elements:**
- 颜色编码的阶段背景（蓝色=预处理，橙色=运行时）
- 组件框带标题和描述
- 带标签的箭头表示数据流
- 虚线反馈循环
- 图例说明

---

### 图3: 覆盖率随时间变化 / Figure 3: Coverage Over Time

**文件 / File:** `generate_figure3.py`

**描述 / Description:**
- libxml2基准测试的边覆盖率时间序列
- 6小时模糊测试活动
- GFuzz vs AFLGo对比
- 包含标准差的置信区间

**生成 / Generate:**
```bash
python3 generate_figure3.py
```

**输出 / Output:**
- `coverage_over_time.pdf` (用于LaTeX / for LaTeX)
- `coverage_over_time.png` (用于预览 / for preview)

**尺寸 / Size:** 10x6英寸 / inches

**数据点 / Data Points:**
- AFLGo：最终达到46.3%覆盖率
- GFuzz：最终达到51.7%覆盖率
- 改进：+11.7%（5.4个百分点）
- 阴影区域：±1标准差（10次试验）

**可视化特性 / Visualization Features:**
- 双折线图带标记点
- 阴影置信区间
- 最终值注释
- 改进百分比标注
- 网格背景
- 专业图例

---

## 代码结构 / Code Structure

### generate_figure1.py (242行)
```python
# 主要函数
draw_control_flow_only(ax)      # 绘制传统方法
draw_state_diversity_guided(ax)  # 绘制GFuzz方法

# 关键元素
- FancyBboxPatch: 圆角矩形
- FancyArrowPatch: 箭头
- Circle: 节点
- Scatter: 状态点
- Ellipse: 状态空间边界
```

### generate_figure2.py (332行)
```python
# 主要函数
create_component_box()  # 创建组件框
create_arrow()          # 创建箭头

# 关键元素
- Rectangle: 阶段背景
- FancyBboxPatch: 组件框
- FancyArrowPatch: 数据流箭头
- Arc: 反馈循环弧线
- Legend: 图例
```

### generate_figure3.py (130行)
```python
# 主要函数
plt.plot()           # 绘制折线
plt.fill_between()   # 填充置信区间
ax.annotate()        # 添加注释

# 数据
- 13个时间点（0-6小时）
- AFLGo和GFuzz的均值数据
- 标准差数据
- 标记点每2个数据点显示一次
```

---

## 技术细节 / Technical Details

### 颜色方案 / Color Scheme

**图1:**
- 入口点: #4CAF50 (绿色 / green)
- 目标: #F44336 (红色 / red)
- 路径: #2196F3 (蓝色 / blue)
- 状态: #FF9800 (橙色 / orange)

**图2:**
- 预处理: #E3F2FD (浅蓝 / light blue)
- 运行时: #FFF3E0 (浅橙 / light orange)
- 组件: #2196F3 (蓝色 / blue)
- 运行时组件: #FF5722 (深橙 / deep orange)
- 数据: #4CAF50 (绿色 / green)

**图3:**
- AFLGo: #FF7043 (橙色 / orange)
- GFuzz: #1976D2 (蓝色 / blue)
- 阴影: 半透明 alpha=0.2

### 输出质量 / Output Quality

所有图片：
- **分辨率 / Resolution:** 300 DPI
- **格式 / Format:** PDF (矢量) + PNG (位图)
- **边界 / Bbox:** tight (最小空白)
- **字体 / Fonts:** 标准matplotlib字体
- **线宽 / Line width:** 2-3pt专业标准

---

## 使用说明 / Usage Instructions

### 1. 安装Python环境 / Install Python Environment

确保安装Python 3.7或更高版本：
```bash
python3 --version
```

### 2. 安装依赖包 / Install Dependencies

```bash
pip3 install matplotlib numpy
```

### 3. 生成图片 / Generate Figures

#### 方法1：一次性生成所有图片 / Method 1: Generate all at once
```bash
cd paper/figures
./generate_all_figures.sh
```

#### 方法2：单独生成 / Method 2: Generate individually
```bash
python3 generate_figure1.py  # 生成图1
python3 generate_figure2.py  # 生成图2
python3 generate_figure3.py  # 生成图3
```

### 4. 验证输出 / Verify Output

检查生成的文件：
```bash
ls -lh *.pdf *.png
```

应该看到6个文件（每个图3个PDF+3个PNG）。

### 5. 编译论文 / Compile Paper

```bash
cd ..
./compile.sh
```

---

## 自定义修改 / Customization

### 修改颜色 / Change Colors

在脚本顶部找到颜色字典：
```python
colors = {
    'entry': '#4CAF50',   # 修改这里 / modify here
    'target': '#F44336',
    # ...
}
```

### 修改数据 / Change Data

在`generate_figure3.py`中修改数据数组：
```python
aflgo_mean = np.array([...])  # 修改AFLGo数据
gfuzz_mean = np.array([...])  # 修改GFuzz数据
```

### 修改尺寸 / Change Size

修改figsize参数：
```python
fig, ax = plt.subplots(figsize=(宽度, 高度))
```

### 修改文本 / Change Text

所有文本都可以直接在脚本中修改：
```python
ax.text(x, y, '你的文本 / Your text', ...)
```

---

## 常见问题 / Troubleshooting

### Q1: ModuleNotFoundError: No module named 'matplotlib'

**解决 / Solution:**
```bash
pip3 install matplotlib numpy
```

### Q2: Permission denied: ./generate_all_figures.sh

**解决 / Solution:**
```bash
chmod +x generate_all_figures.sh
```

### Q3: 中文显示为方块 / Chinese characters show as boxes

**解决 / Solution:**
```python
# 在脚本顶部添加 / Add at top of script
plt.rcParams['font.sans-serif'] = ['SimHei']  # 中文字体
plt.rcParams['axes.unicode_minus'] = False
```

### Q4: 字体警告 / Font warnings

这些警告不会影响图片生成，可以忽略。要抑制警告：
```bash
python3 -W ignore generate_figure1.py
```

---

## 文件清单 / File List

```
figures/
├── generate_figure1.py          (242行 / lines)
├── generate_figure2.py          (332行 / lines)
├── generate_figure3.py          (130行 / lines)
├── generate_all_figures.sh      (主脚本 / master script)
├── FIGURE_GENERATION.md         (详细文档 / detailed docs)
├── FIGURE_CODE_SUMMARY.md       (本文件 / this file)
└── README.md                    (原始规格 / original specs)
```

---

## 输出示例 / Output Examples

生成后将获得 / After generation you will have:

```
approach_comparison.pdf     (图1 PDF)
approach_comparison.png     (图1 PNG)
gfuzz_architecture.pdf      (图2 PDF)
gfuzz_architecture.png      (图2 PNG)
coverage_over_time.pdf      (图3 PDF)
coverage_over_time.png      (图3 PNG)
```

PDF文件用于LaTeX论文编译，PNG文件用于快速预览和演示文稿。

---

## 质量保证 / Quality Assurance

✅ 符合期刊标准 / Meets journal standards
✅ 300 DPI高分辨率 / High resolution 300 DPI
✅ 矢量图形（PDF）/ Vector graphics (PDF)
✅ 灰度兼容 / Grayscale compatible
✅ 专业配色 / Professional colors
✅ 清晰标注 / Clear annotations
✅ 学术风格 / Academic style

---

## 支持 / Support

如有问题，参考 / For issues, refer to:
- Matplotlib文档: https://matplotlib.org
- Python文档: https://docs.python.org
- 本项目Issues: https://github.com/cyhhtl999520/aflgo/issues

---

**最后更新 / Last Updated:** 2025-11-08
**版本 / Version:** 1.0
**状态 / Status:** 完整可用 / Complete and ready
