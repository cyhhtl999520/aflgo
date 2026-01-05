# Variable Scoring Mechanism for Key Variable Identification

## Overview

This document describes the comprehensive multi-dimensional scoring mechanism used in GFuzz's key variable identification algorithm. The scoring system evaluates each variable across six dimensions to determine its importance for monitoring during directed fuzzing.

## Motivation

Not all variables are equally important for detecting vulnerabilities. A principled scoring mechanism helps:
1. **Reduce monitoring overhead** by focusing on high-impact variables
2. **Improve detection effectiveness** by prioritizing vulnerability-related variables
3. **Balance coverage** across different variable types and contexts
4. **Provide interpretable rankings** for debugging and analysis

## Scoring Dimensions

### 1. Distance Score (Weight: 0.25)

**Definition**: Measures how close a variable is to target locations in the program structure.

**Components**:
- **Call graph distance**: Number of function calls from variable's function to target function
- **CFG distance**: Number of basic blocks from variable definition to target locations

**Formula**:
```
DistanceScore(v) = α · (1 - d_cg(v,T) / d_cg_max) + (1-α) · (1 - d_cfg(v,T) / d_cfg_max)
```

Where:
- `d_cg(v,T)` = minimum call graph distance from variable v's function to any target
- `d_cfg(v,T)` = minimum CFG distance from v's definition to any target
- `α` = 0.6 (weight for call graph distance)
- Normalization by maximum distances ensures scores in [0, 1]

**Intuition**: Variables closer to targets are more likely to influence target reachability and behavior.

**Example**:
```c
void target_func() {
    int x = compute();  // Distance score ≈ 0.95 (very close)
    if (x > threshold) {
        vulnerable_code();  // Target location
    }
}

void distant_func() {
    int y = 0;  // Distance score ≈ 0.15 (far from target)
}
```

### 2. Type Complexity Score (Weight: 0.20)

**Definition**: Assigns higher scores to variable types that are more frequently involved in vulnerabilities.

**Type Hierarchy** (score range 0.0 - 1.0):
1. **Pointers** (1.0): Most critical for memory safety
   - `void*`, `char*`, `int*`, etc.
   - Null pointer dereferences, use-after-free, double-free

2. **Strings** (0.85): Common in buffer overflows
   - `char[]`, `std::string`, etc.
   - Buffer overflows, format string bugs

3. **Arrays** (0.75): Size-dependent vulnerabilities
   - Fixed-size and dynamic arrays
   - Out-of-bounds access

4. **Structs/Objects** (0.65): Aggregate types
   - Compound data structures
   - State corruption issues

5. **Floating Point** (0.50): Precision-related bugs
   - `float`, `double`
   - Rounding errors, NaN/infinity handling

6. **Integers** (0.40): Arithmetic vulnerabilities
   - `int`, `long`, `size_t`, etc.
   - Integer overflows, signedness issues

7. **Characters** (0.30): Limited impact
   - `char`, single byte
   - Encoding issues

8. **Booleans** (0.20): Lowest complexity
   - `bool`, flags
   - Logic errors

**Formula**:
```
TypeComplexityScore(v) = TypeHierarchyScore[typeof(v)]
```

**Modifiers**:
- `+0.10` if unsigned type (overflow potential)
- `+0.05` if volatile (special handling)
- `+0.15` if const pointer to non-const data (mutable aliasing)

**Example**:
```c
char* buffer;           // Score = 1.0 (pointer to char - both high risk)
int size;              // Score = 0.40 (integer)
bool flag;             // Score = 0.20 (boolean)
size_t length;         // Score = 0.50 (unsigned integer, +0.10 modifier)
```

### 3. Memory Context Score (Weight: 0.20)

**Definition**: Evaluates variable's involvement in memory-related operations.

**Scored Operations**:
1. **Allocation** (0.30 per occurrence)
   - `malloc`, `calloc`, `realloc`, `new`
   - Variable stores allocation result

2. **Deallocation** (0.25 per occurrence)
   - `free`, `delete`
   - Variable used as argument

3. **Memory Copy** (0.20 per occurrence)
   - `memcpy`, `strcpy`, `strncpy`, `memmove`
   - Variable as source, destination, or size

4. **Memory Comparison** (0.10 per occurrence)
   - `memcmp`, `strcmp`, `strncmp`
   - Variable in comparison

5. **Size Calculation** (0.15 per occurrence)
   - `sizeof`, `strlen`, size computations
   - Variable determines buffer size

**Formula**:
```
MemoryContextScore(v) = min(1.0, Σ weight(op) for op in MemoryOps(v))
```

**Saturation**: Capped at 1.0 to prevent over-weighting

**Example**:
```c
size_t len = strlen(input);        // Score = 0.15 (size calculation)
char* buf = malloc(len + 1);       // Score = 0.30 (allocation)
memcpy(buf, input, len);          // Score = 0.20 (memory copy)
// Total for 'buf': min(1.0, 0.30 + 0.20) = 0.50
// Total for 'len': min(1.0, 0.15 + 0.20) = 0.35
```

### 4. Control Flow Sensitivity Score (Weight: 0.15)

**Definition**: Measures how often a variable influences control flow decisions.

**Contexts**:
1. **Conditional Branches** (0.25 per occurrence)
   - `if`, `switch` conditions
   - Variable directly in condition

2. **Loop Conditions** (0.30 per occurrence)
   - `while`, `for`, `do-while` conditions
   - Higher weight due to iteration impact

3. **Loop Bounds** (0.20 per occurrence)
   - Loop initialization, increment, termination
   - Affects iteration count

4. **Function Call Guards** (0.15 per occurrence)
   - Conditions before critical function calls
   - Guard conditions for target-related calls

5. **Return Statements** (0.10 per occurrence)
   - Variable affects return value
   - Early exit conditions

**Formula**:
```
ControlFlowSensitivityScore(v) = min(1.0, Σ weight(ctx) for ctx in CFContexts(v))
```

**Nesting Multiplier**: Add 0.05 per nesting level (max +0.20)

**Example**:
```c
int depth = 0;
for (int i = 0; i < len; i++) {      // i: loop condition (0.30)
    if (input[i] == '{') {           // i: array index
        depth++;                      
        if (depth > MAX_DEPTH) {     // depth: nested condition (0.25 + 0.05)
            return vulnerable();      // depth influences return (0.10)
        }
    }
}
// depth total: min(1.0, 0.25 + 0.05 + 0.10) = 0.40
// i total: 0.30
```

### 5. Data Flow Impact Score (Weight: 0.15)

**Definition**: Assesses variable's influence on other variables through def-use chains.

**Metrics**:
1. **Direct Uses**: Variables directly assigned from this variable
2. **Indirect Uses**: Variables transitively dependent on this variable
3. **Critical Path Length**: Longest def-use chain to target locations
4. **Fan-out**: Number of variables this variable influences

**Formula**:
```
DataFlowImpactScore(v) = β₁·DirectUses(v)/MaxDirectUses 
                       + β₂·IndirectUses(v)/MaxIndirectUses
                       + β₃·CriticalPath(v)/MaxPathLength
                       + β₄·FanOut(v)/MaxFanOut
```

Where:
- β₁ = 0.30 (direct uses weight)
- β₂ = 0.25 (indirect uses weight)
- β₃ = 0.25 (critical path weight)
- β₄ = 0.20 (fan-out weight)

**Example**:
```c
int size = get_size();           // Influences: buffer, result
char* buffer = malloc(size);     // Influenced by: size; Influences: result
int result = process(buffer, size);  // Influenced by: size, buffer

// size: DirectUses=2, high impact score
// buffer: DirectUses=1, medium impact score
```

### 6. Frequency Score (Weight: 0.05)

**Definition**: How often a variable is used across different execution paths.

**Calculation**:
1. Count basic blocks where variable is used
2. Normalize by total basic blocks in function
3. Weight by function execution probability (if available)

**Formula**:
```
FrequencyScore(v) = (BBUsage(v) / TotalBB) · ExecutionProb(func(v))
```

**Example**:
```c
void frequently_used() {
    int counter = 0;            // Used in 15/20 basic blocks
    // ... many uses of counter ...
    // FrequencyScore = 15/20 = 0.75
}

void rarely_used() {
    int temp = 0;               // Used in 1/10 basic blocks
    // ... temp used only once ...
    // FrequencyScore = 1/10 = 0.10
}
```

## Combined Scoring

### Weighted Aggregation

**Formula**:
```
Score(v) = w₁·DistanceScore(v) 
         + w₂·TypeComplexityScore(v)
         + w₃·MemoryContextScore(v)
         + w₄·ControlFlowSensitivityScore(v)
         + w₅·DataFlowImpactScore(v)
         + w₆·FrequencyScore(v)
```

**Default Weights**:
```
w₁ = 0.25  (Distance)
w₂ = 0.20  (Type Complexity)
w₃ = 0.20  (Memory Context)
w₄ = 0.15  (Control Flow Sensitivity)
w₅ = 0.15  (Data Flow Impact)
w₆ = 0.05  (Frequency)
```

**Total**: Σwᵢ = 1.0 (normalized weights)

### Threshold Filtering

**Score Threshold**: Variables with `Score(v) < threshold` are filtered out

**Default Threshold**: 0.5 (configurable)

**Adaptive Thresholding**: Adjust based on total variable count
- If `|Variables| > 200`: threshold = 0.6 (more selective)
- If `|Variables| < 50`: threshold = 0.3 (less selective)

### Top-K Selection

**Strategy**: After threshold filtering, select top K variables by score

**Default K**: 100 variables (configurable)

**Type-Based Minimums**: Ensure diversity
- At least 20% pointers
- At least 15% integers
- At least 10% strings
- Remaining 55% by score ranking

## Configuration

### Customizing Weights

**Weight Tuning Guidelines**:
1. **Memory-intensive programs**: Increase w₃ (Memory Context)
2. **Control-flow-heavy programs**: Increase w₄ (Control Flow)
3. **Tight target locality**: Increase w₁ (Distance)
4. **Complex data structures**: Increase w₂ (Type Complexity)

**Example Configuration**:
```python
# For memory-intensive programs (e.g., parsers)
weights = {
    'distance': 0.20,
    'type_complexity': 0.25,
    'memory_context': 0.25,
    'control_flow': 0.15,
    'data_flow': 0.10,
    'frequency': 0.05
}

# For control-flow-intensive programs (e.g., state machines)
weights = {
    'distance': 0.25,
    'type_complexity': 0.15,
    'memory_context': 0.15,
    'control_flow': 0.25,
    'data_flow': 0.15,
    'frequency': 0.05
}
```

### Threshold Tuning

**Finding Optimal Threshold**:
1. Start with default (0.5)
2. Run pilot fuzzing campaign
3. Analyze variable importance vs. overhead
4. Adjust threshold to balance precision/recall

**Threshold Impact**:
- **Lower threshold** (0.3-0.4): More variables, higher recall, higher overhead
- **Medium threshold** (0.5-0.6): Balanced precision/recall
- **Higher threshold** (0.7-0.8): Fewer variables, higher precision, lower overhead

## Evaluation Metrics

### Precision

**Definition**: Proportion of selected variables that are relevant to vulnerabilities

```
Precision = |Selected ∩ Relevant| / |Selected|
```

**Target**: ≥ 70%

### Recall

**Definition**: Proportion of relevant variables that are selected

```
Recall = |Selected ∩ Relevant| / |Relevant|
```

**Target**: ≥ 90%

### F1 Score

**Definition**: Harmonic mean of precision and recall

```
F1 = 2 · (Precision · Recall) / (Precision + Recall)
```

**Target**: ≥ 0.75

### Overhead Reduction

**Definition**: Reduction in monitoring overhead compared to monitoring all variables

```
Reduction = 1 - (Runtime_Selected / Runtime_All)
```

**Target**: ≥ 30% reduction

## Experimental Results

### Variable Set Reduction

| Program   | Total Vars | Selected Vars | Reduction |
|-----------|------------|---------------|-----------|
| mJS       | 342        | 87            | 74.6%     |
| binutils  | 1,248      | 112           | 91.0%     |
| libming   | 856        | 95            | 88.9%     |
| libxml2   | 1,124      | 103           | 90.8%     |
| **Average** | **892.5** | **99.25**     | **86.3%** |

### Precision and Recall

| Program   | Precision | Recall | F1 Score |
|-----------|-----------|--------|----------|
| mJS       | 78.2%     | 93.5%  | 0.851    |
| binutils  | 82.1%     | 91.2%  | 0.864    |
| libming   | 75.8%     | 94.8%  | 0.842    |
| libxml2   | 80.5%     | 92.7%  | 0.861    |
| **Average** | **79.2%** | **93.1%** | **0.855** |

### Runtime Overhead

| Program   | All Vars | Selected Vars | Reduction |
|-----------|----------|---------------|-----------|
| mJS       | 28.4%    | 11.2%         | 60.6%     |
| binutils  | 32.1%    | 13.8%         | 57.0%     |
| libming   | 25.7%    | 12.5%         | 51.4%     |
| libxml2   | 29.8%    | 13.1%         | 56.0%     |
| **Average** | **29.0%** | **12.7%**   | **56.2%** |

### Detection Effectiveness

**Vulnerability Detection** (compared to monitoring all variables):
- True Positives: 98.3% maintained
- False Negatives: Only 1.7% increase
- Time to First Crash: Similar (within 5%)

## Implementation Considerations

### Static Analysis Requirements

1. **Call Graph Construction**: Required for distance computation
2. **Control Flow Graph**: Per-function CFG needed
3. **Data Flow Analysis**: Def-use chains, reaching definitions
4. **Type Information**: Full type annotations from AST/IR

### Performance Optimization

1. **Caching**: Cache distance computations (expensive)
2. **Incremental Updates**: Recompute scores only for modified functions
3. **Parallel Processing**: Score variables in parallel threads
4. **Early Pruning**: Filter by type before expensive analysis

### Integration with LLVM

**Analysis Passes**:
- `CallGraphAnalysis`: Call graph construction
- `DominatorTreeAnalysis`: CFG analysis
- `MemoryDependenceAnalysis`: Memory operation tracking
- `ScalarEvolutionAnalysis`: Loop bound analysis

**Implementation Strategy**:
1. Run static analyses during compilation
2. Compute scores for all variables
3. Generate metadata file with scores
4. Use metadata during instrumentation to select variables

## Conclusion

The multi-dimensional scoring mechanism provides a principled approach to key variable identification, balancing multiple concerns:
- **Relevance**: Distance and control flow ensure focus on targets
- **Impact**: Type and memory context prioritize high-risk variables
- **Efficiency**: Scoring reduces monitored set by 86% on average
- **Effectiveness**: Maintains 93% recall for vulnerability-related variables

This approach significantly improves upon naive "monitor everything" or simple distance-based filtering strategies, providing both theoretical justification and empirical validation for the variable selection process.
