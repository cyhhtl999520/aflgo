# Variable Scoring Mechanism - Implementation Guide

## Overview

This guide provides step-by-step instructions for implementing the multi-dimensional variable scoring mechanism in GFuzz's key variable identification algorithm.

## Architecture

### Components

1. **Static Analyzers**: Extract program structure and metadata
2. **Score Calculators**: Compute individual dimension scores
3. **Aggregator**: Combine scores with weights
4. **Selector**: Filter and rank variables

### Data Flow

```
Source Code
    ↓
[LLVM IR Generation]
    ↓
[Static Analysis Passes]
    ↓
[Score Computation]
    ↓
[Aggregation & Filtering]
    ↓
Selected Variables + Metadata
```

## Implementation Steps

### Step 1: Static Analysis Infrastructure

#### 1.1 Call Graph Construction

**Purpose**: Compute call graph distances

**LLVM Pass**: `CallGraphWrapperPass`

**Implementation**:
```cpp
#include "llvm/Analysis/CallGraph.h"

class DistanceAnalyzer {
private:
    CallGraph& CG;
    std::set<Function*> targets;
    
public:
    // Compute minimum call graph distance to any target
    unsigned getCallGraphDistance(Function* F) {
        if (targets.count(F)) return 0;
        
        unsigned minDist = UINT_MAX;
        std::queue<std::pair<Function*, unsigned>> worklist;
        std::set<Function*> visited;
        
        worklist.push({F, 0});
        
        while (!worklist.empty()) {
            auto [current, dist] = worklist.front();
            worklist.pop();
            
            if (visited.count(current)) continue;
            visited.insert(current);
            
            if (targets.count(current)) {
                minDist = std::min(minDist, dist);
                continue;
            }
            
            CallGraphNode* node = CG[current];
            for (auto& edge : *node) {
                Function* callee = edge.second->getFunction();
                if (callee && !visited.count(callee)) {
                    worklist.push({callee, dist + 1});
                }
            }
        }
        
        return minDist;
    }
};
```

#### 1.2 Control Flow Graph Analysis

**Purpose**: Compute CFG distances and control flow sensitivity

**LLVM Pass**: `DominatorTreeWrapperPass`

**Implementation**:
```cpp
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/CFG.h"

class CFGAnalyzer {
private:
    Function& F;
    DominatorTree& DT;
    std::set<BasicBlock*> targetBlocks;
    
public:
    // Compute minimum CFG distance to target blocks
    unsigned getCFGDistance(Instruction* I) {
        BasicBlock* BB = I->getParent();
        if (targetBlocks.count(BB)) return 0;
        
        unsigned minDist = UINT_MAX;
        std::queue<std::pair<BasicBlock*, unsigned>> worklist;
        std::set<BasicBlock*> visited;
        
        worklist.push({BB, 0});
        
        while (!worklist.empty()) {
            auto [current, dist] = worklist.front();
            worklist.pop();
            
            if (visited.count(current)) continue;
            visited.insert(current);
            
            if (targetBlocks.count(current)) {
                minDist = std::min(minDist, dist);
                continue;
            }
            
            for (BasicBlock* succ : successors(current)) {
                if (!visited.count(succ)) {
                    worklist.push({succ, dist + 1});
                }
            }
        }
        
        return minDist;
    }
    
    // Check if variable is used in conditional
    bool isUsedInConditional(Value* V) {
        for (User* U : V->users()) {
            if (BranchInst* BI = dyn_cast<BranchInst>(U)) {
                if (BI->isConditional() && BI->getCondition() == V) {
                    return true;
                }
            }
            else if (SwitchInst* SI = dyn_cast<SwitchInst>(U)) {
                if (SI->getCondition() == V) {
                    return true;
                }
            }
        }
        return false;
    }
};
```

#### 1.3 Data Flow Analysis

**Purpose**: Compute def-use chains and data dependencies

**LLVM Pass**: `MemoryDependenceWrapperPass`

**Implementation**:
```cpp
#include "llvm/Analysis/MemoryDependenceAnalysis.h"

class DataFlowAnalyzer {
private:
    Function& F;
    
public:
    // Count direct uses of a variable
    unsigned countDirectUses(Value* V) {
        return V->getNumUses();
    }
    
    // Count indirect uses (transitive dependencies)
    unsigned countIndirectUses(Value* V) {
        std::set<Value*> visited;
        std::queue<Value*> worklist;
        worklist.push(V);
        
        while (!worklist.empty()) {
            Value* current = worklist.front();
            worklist.pop();
            
            if (visited.count(current)) continue;
            visited.insert(current);
            
            for (User* U : current->users()) {
                if (Instruction* I = dyn_cast<Instruction>(U)) {
                    worklist.push(I);
                }
            }
        }
        
        return visited.size() - 1; // Exclude the variable itself
    }
    
    // Compute fan-out (number of influenced variables)
    unsigned computeFanOut(Value* V) {
        std::set<Value*> influenced;
        
        for (User* U : V->users()) {
            if (StoreInst* SI = dyn_cast<StoreInst>(U)) {
                influenced.insert(SI->getPointerOperand());
            }
            else if (Instruction* I = dyn_cast<Instruction>(U)) {
                influenced.insert(I);
            }
        }
        
        return influenced.size();
    }
};
```

### Step 2: Score Calculator Implementation

#### 2.1 Distance Score

```cpp
class DistanceScoreCalculator {
private:
    DistanceAnalyzer& DA;
    CFGAnalyzer& CFGA;
    unsigned maxCGDist;
    unsigned maxCFGDist;
    
public:
    double compute(Value* V, Instruction* DefInst) {
        Function* F = DefInst->getParent()->getParent();
        
        unsigned cgDist = DA.getCallGraphDistance(F);
        unsigned cfgDist = CFGA.getCFGDistance(DefInst);
        
        double cgScore = 1.0 - (double)cgDist / maxCGDist;
        double cfgScore = 1.0 - (double)cfgDist / maxCFGDist;
        
        double alpha = 0.6; // Weight for call graph distance
        return alpha * cgScore + (1.0 - alpha) * cfgScore;
    }
};
```

#### 2.2 Type Complexity Score

```cpp
class TypeComplexityScoreCalculator {
public:
    double compute(Type* T) {
        double baseScore = getBaseScore(T);
        double modifier = 0.0;
        
        if (T->isPointerTy()) {
            Type* pointeeType = T->getPointerElementType();
            if (pointeeType->isIntegerTy(8)) {
                baseScore = 1.0; // char* (string)
            }
        }
        
        // Add modifiers for unsigned, volatile, etc.
        if (isUnsignedType(T)) modifier += 0.10;
        
        return std::min(1.0, baseScore + modifier);
    }
    
private:
    double getBaseScore(Type* T) {
        if (T->isPointerTy()) return 1.0;
        if (T->isArrayTy()) {
            Type* elemType = T->getArrayElementType();
            if (elemType->isIntegerTy(8)) return 0.85; // string
            return 0.75; // other arrays
        }
        if (T->isStructTy()) return 0.65;
        if (T->isFloatingPointTy()) return 0.50;
        if (T->isIntegerTy()) return 0.40;
        if (T->isIntegerTy(1)) return 0.20; // boolean
        return 0.30; // default
    }
    
    bool isUnsignedType(Type* T) {
        // Implementation depends on type metadata
        return false; // Placeholder
    }
};
```

#### 2.3 Memory Context Score

```cpp
class MemoryContextScoreCalculator {
private:
    std::set<std::string> allocFuncs = {"malloc", "calloc", "realloc"};
    std::set<std::string> freeFuncs = {"free"};
    std::set<std::string> copyFuncs = {"memcpy", "strcpy", "strncpy", "memmove"};
    
public:
    double compute(Value* V) {
        double score = 0.0;
        
        for (User* U : V->users()) {
            if (CallInst* CI = dyn_cast<CallInst>(U)) {
                Function* calledFunc = CI->getCalledFunction();
                if (!calledFunc) continue;
                
                std::string funcName = calledFunc->getName().str();
                
                if (allocFuncs.count(funcName)) {
                    score += 0.30;
                }
                else if (freeFuncs.count(funcName)) {
                    score += 0.25;
                }
                else if (copyFuncs.count(funcName)) {
                    score += 0.20;
                }
            }
            else if (isa<LoadInst>(U) || isa<StoreInst>(U)) {
                score += 0.05; // Memory access
            }
        }
        
        return std::min(1.0, score);
    }
};
```

#### 2.4 Control Flow Sensitivity Score

```cpp
class ControlFlowSensitivityScoreCalculator {
private:
    CFGAnalyzer& CFGA;
    
public:
    double compute(Value* V) {
        double score = 0.0;
        unsigned nestingLevel = 0;
        
        for (User* U : V->users()) {
            Instruction* I = dyn_cast<Instruction>(U);
            if (!I) continue;
            
            // Check for conditional branches
            if (CFGA.isUsedInConditional(V)) {
                score += 0.25;
            }
            
            // Check for loop conditions
            if (isLoopCondition(I)) {
                score += 0.30;
            }
            
            // Check for loop bounds
            if (isLoopBound(I)) {
                score += 0.20;
            }
            
            // Check for return statements
            if (ReturnInst* RI = dyn_cast<ReturnInst>(I)) {
                if (RI->getReturnValue() == V) {
                    score += 0.10;
                }
            }
        }
        
        // Add nesting bonus
        nestingLevel = computeNestingLevel(V);
        score += std::min(0.20, nestingLevel * 0.05);
        
        return std::min(1.0, score);
    }
    
private:
    bool isLoopCondition(Instruction* I) {
        // Check if instruction is part of loop header
        Loop* L = getLoopFor(I->getParent());
        return L && L->getHeader() == I->getParent();
    }
    
    bool isLoopBound(Instruction* I) {
        // Check if instruction affects loop bounds
        return false; // Placeholder - requires loop analysis
    }
    
    unsigned computeNestingLevel(Value* V) {
        // Compute nesting level of control structures
        return 0; // Placeholder
    }
};
```

#### 2.5 Data Flow Impact Score

```cpp
class DataFlowImpactScoreCalculator {
private:
    DataFlowAnalyzer& DFA;
    unsigned maxDirectUses;
    unsigned maxIndirectUses;
    unsigned maxFanOut;
    
public:
    double compute(Value* V) {
        unsigned directUses = DFA.countDirectUses(V);
        unsigned indirectUses = DFA.countIndirectUses(V);
        unsigned fanOut = DFA.computeFanOut(V);
        
        double beta1 = 0.30, beta2 = 0.25, beta3 = 0.25, beta4 = 0.20;
        
        double directScore = (double)directUses / maxDirectUses;
        double indirectScore = (double)indirectUses / maxIndirectUses;
        double fanOutScore = (double)fanOut / maxFanOut;
        
        return beta1 * directScore + beta2 * indirectScore + 
               beta3 * 0.5 + beta4 * fanOutScore; // beta3 placeholder
    }
};
```

#### 2.6 Frequency Score

```cpp
class FrequencyScoreCalculator {
public:
    double compute(Value* V, Function* F) {
        unsigned bbUsage = countBasicBlocksUsingVariable(V);
        unsigned totalBB = F->size();
        
        double usageRatio = (double)bbUsage / totalBB;
        double execProb = 1.0; // Placeholder - requires profile data
        
        return usageRatio * execProb;
    }
    
private:
    unsigned countBasicBlocksUsingVariable(Value* V) {
        std::set<BasicBlock*> blocks;
        
        for (User* U : V->users()) {
            if (Instruction* I = dyn_cast<Instruction>(U)) {
                blocks.insert(I->getParent());
            }
        }
        
        return blocks.size();
    }
};
```

### Step 3: Score Aggregation

```cpp
class VariableScoreAggregator {
private:
    struct Weights {
        double distance = 0.25;
        double typeComplexity = 0.20;
        double memoryContext = 0.20;
        double controlFlowSensitivity = 0.15;
        double dataFlowImpact = 0.15;
        double frequency = 0.05;
    } weights;
    
    DistanceScoreCalculator distCalc;
    TypeComplexityScoreCalculator typeCalc;
    MemoryContextScoreCalculator memCalc;
    ControlFlowSensitivityScoreCalculator cfCalc;
    DataFlowImpactScoreCalculator dfCalc;
    FrequencyScoreCalculator freqCalc;
    
public:
    struct VariableScore {
        Value* variable;
        double totalScore;
        double distanceScore;
        double typeComplexityScore;
        double memoryContextScore;
        double controlFlowSensitivityScore;
        double dataFlowImpactScore;
        double frequencyScore;
    };
    
    VariableScore compute(Value* V, Instruction* DefInst, Function* F) {
        VariableScore result;
        result.variable = V;
        
        result.distanceScore = distCalc.compute(V, DefInst);
        result.typeComplexityScore = typeCalc.compute(V->getType());
        result.memoryContextScore = memCalc.compute(V);
        result.controlFlowSensitivityScore = cfCalc.compute(V);
        result.dataFlowImpactScore = dfCalc.compute(V);
        result.frequencyScore = freqCalc.compute(V, F);
        
        result.totalScore = 
            weights.distance * result.distanceScore +
            weights.typeComplexity * result.typeComplexityScore +
            weights.memoryContext * result.memoryContextScore +
            weights.controlFlowSensitivity * result.controlFlowSensitivityScore +
            weights.dataFlowImpact * result.dataFlowImpactScore +
            weights.frequency * result.frequencyScore;
        
        return result;
    }
    
    void setWeights(const Weights& w) {
        weights = w;
    }
};
```

### Step 4: Variable Selection

```cpp
class VariableSelector {
private:
    double scoreThreshold = 0.5;
    unsigned topK = 100;
    
    struct TypeQuotas {
        double pointerMin = 0.20;
        double integerMin = 0.15;
        double stringMin = 0.10;
    };
    
public:
    std::vector<VariableScoreAggregator::VariableScore> 
    select(std::vector<VariableScoreAggregator::VariableScore>& allScores) {
        // Step 1: Threshold filtering
        std::vector<VariableScoreAggregator::VariableScore> filtered;
        for (auto& score : allScores) {
            if (score.totalScore >= scoreThreshold) {
                filtered.push_back(score);
            }
        }
        
        // Step 2: Sort by score (descending)
        std::sort(filtered.begin(), filtered.end(),
                  [](const auto& a, const auto& b) {
                      return a.totalScore > b.totalScore;
                  });
        
        // Step 3: Apply top-K limit
        if (filtered.size() > topK) {
            filtered.resize(topK);
        }
        
        // Step 4: Ensure type diversity (simplified)
        ensureTypeDiversity(filtered);
        
        return filtered;
    }
    
private:
    void ensureTypeDiversity(std::vector<VariableScoreAggregator::VariableScore>& vars) {
        // Count types
        unsigned pointers = 0, integers = 0, strings = 0;
        
        for (auto& v : vars) {
            Type* T = v.variable->getType();
            if (T->isPointerTy()) pointers++;
            else if (T->isIntegerTy()) integers++;
        }
        
        // Ensure minimums (implementation depends on requirements)
    }
};
```

### Step 5: Integration with GFuzz

```cpp
class KeyVariableIdentificationPass : public ModulePass {
public:
    bool runOnModule(Module& M) override {
        // Initialize analyzers
        DistanceAnalyzer distAnalyzer(/* ... */);
        CFGAnalyzer cfgAnalyzer(/* ... */);
        DataFlowAnalyzer dfAnalyzer(/* ... */);
        
        // Initialize score calculators
        VariableScoreAggregator aggregator(/* ... */);
        VariableSelector selector;
        
        // Collect all variables
        std::vector<Value*> allVariables;
        for (Function& F : M) {
            for (BasicBlock& BB : F) {
                for (Instruction& I : BB) {
                    if (AllocaInst* AI = dyn_cast<AllocaInst>(&I)) {
                        allVariables.push_back(AI);
                    }
                }
            }
        }
        
        // Score all variables
        std::vector<VariableScoreAggregator::VariableScore> scores;
        for (Value* V : allVariables) {
            auto score = aggregator.compute(V, /* ... */);
            scores.push_back(score);
        }
        
        // Select key variables
        auto selectedVars = selector.select(scores);
        
        // Output selected variables with metadata
        outputSelectedVariables(selectedVars);
        
        return false; // Module not modified
    }
};
```

## Testing

### Unit Tests

```cpp
TEST(DistanceScoreCalculator, ComputesCorrectDistance) {
    // Create test function graph
    // Verify distance computation
}

TEST(TypeComplexityScoreCalculator, AssignsCorrectScores) {
    // Test different type categories
    EXPECT_DOUBLE_EQ(1.0, calc.compute(pointerType));
    EXPECT_DOUBLE_EQ(0.40, calc.compute(integerType));
}

TEST(VariableScoreAggregator, CombinesScoresCorrectly) {
    // Test weighted combination
    // Verify total score calculation
}

TEST(VariableSelector, FiltersAndSelectsCorrectly) {
    // Test threshold filtering
    // Test top-K selection
    // Test type diversity
}
```

### Integration Tests

```cpp
TEST(KeyVariableIdentification, SelectsRelevantVariables) {
    // Load test program with known vulnerabilities
    // Run identification pass
    // Verify relevant variables are selected
    // Verify precision and recall
}
```

## Performance Optimization

### 1. Caching

```cpp
class DistanceCache {
private:
    std::unordered_map<Function*, unsigned> cgDistanceCache;
    std::unordered_map<Instruction*, unsigned> cfgDistanceCache;
    
public:
    unsigned getCachedCGDistance(Function* F) {
        if (cgDistanceCache.count(F)) {
            return cgDistanceCache[F];
        }
        unsigned dist = computeCGDistance(F);
        cgDistanceCache[F] = dist;
        return dist;
    }
};
```

### 2. Parallel Processing

```cpp
void scoreVariablesInParallel(std::vector<Value*>& vars) {
    #pragma omp parallel for
    for (size_t i = 0; i < vars.size(); ++i) {
        scores[i] = aggregator.compute(vars[i], /* ... */);
    }
}
```

### 3. Early Pruning

```cpp
bool quickReject(Value* V) {
    // Fast checks to reject obviously irrelevant variables
    Type* T = V->getType();
    if (T->isIntegerTy(1)) return true; // Boolean, usually not critical
    if (V->getNumUses() == 0) return true; // Unused variable
    return false;
}
```

## Debugging

### Visualization

```cpp
void dumpVariableScores(const std::vector<VariableScore>& scores) {
    for (auto& score : scores) {
        errs() << "Variable: " << *score.variable << "\n";
        errs() << "  Total Score: " << score.totalScore << "\n";
        errs() << "  Distance: " << score.distanceScore << "\n";
        errs() << "  Type: " << score.typeComplexityScore << "\n";
        errs() << "  Memory: " << score.memoryContextScore << "\n";
        errs() << "  Control Flow: " << score.controlFlowSensitivityScore << "\n";
        errs() << "  Data Flow: " << score.dataFlowImpactScore << "\n";
        errs() << "  Frequency: " << score.frequencyScore << "\n";
        errs() << "\n";
    }
}
```

## Configuration File Format

```json
{
    "weights": {
        "distance": 0.25,
        "type_complexity": 0.20,
        "memory_context": 0.20,
        "control_flow_sensitivity": 0.15,
        "data_flow_impact": 0.15,
        "frequency": 0.05
    },
    "selection": {
        "score_threshold": 0.5,
        "top_k": 100,
        "type_quotas": {
            "pointer_min": 0.20,
            "integer_min": 0.15,
            "string_min": 0.10
        }
    },
    "adaptive_threshold": {
        "enabled": true,
        "high_count_threshold": 200,
        "high_count_score": 0.6,
        "low_count_threshold": 50,
        "low_count_score": 0.3
    }
}
```

## Conclusion

This implementation guide provides the complete structure for integrating the multi-dimensional variable scoring mechanism into GFuzz. The modular design allows for easy extension and customization while maintaining efficiency and correctness.
