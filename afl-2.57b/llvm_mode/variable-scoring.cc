#include "variable-scoring.h"
#include <llvm/Analysis/CallGraph.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Dominators.h>
#include <llvm/Support/raw_ostream.h>
#include <fstream>
#include <algorithm>
#include <cmath>

using namespace llvm;
using namespace gfuzz;

// ============================================================================
// Helper functions
// ============================================================================

static bool isPointerType(Type* ty) {
  return ty->isPointerTy();
}

static bool isStringType(Type* ty) {
  if (auto* ptrTy = dyn_cast<PointerType>(ty)) {
#if LLVM_VERSION_MAJOR >= 11
    // LLVM 11+ uses opaque pointers differently
    return false; // Simplified for now
#else
    Type* elemTy = ptrTy->getElementType();
    return elemTy->isIntegerTy(8); // char*
#endif
  }
  return false;
}

static bool isArrayType(Type* ty) {
  return ty->isArrayTy();
}

static int getCFGDistance(BasicBlock* from, BasicBlock* to) {
  // Use BFS to calculate control flow graph distance
  if (from == to) return 0;

  std::map<BasicBlock*, int> distances;
  std::queue<BasicBlock*> queue;
  queue.push(from);
  distances[from] = 0;

  while (!queue.empty()) {
    BasicBlock* current = queue.front();
    queue.pop();
    
    if (current == to) {
      return distances[to];
    }
    
    for (BasicBlock* succ : successors(current)) {
      if (distances.find(succ) == distances.end()) {
        distances[succ] = distances[current] + 1;
        queue.push(succ);
      }
    }
  }

  return 1000; // Unreachable
}

// ============================================================================
// VariableScorer implementation
// ============================================================================

void VariableScorer::loadConfig(const std::string& config_file) {
  // Parse JSON config file
  // Simplified: use default configuration
  // Full implementation would use a JSON library (e.g., nlohmann/json)
  config_ = ScoringConfig();
}

double VariableScorer::computeDistanceScore(Value* var) {
  // Calculate distance score: 1 / (1 + distance)

  if (!isa<Instruction>(var)) {
    return 0.5; // Default score for global variables or parameters
  }

  Instruction* inst = cast<Instruction>(var);
  Function* func = inst->getFunction();
  BasicBlock* bb = inst->getParent();

  // Assume target location is known (needs to be passed externally)
  // Using simplified distance calculation here
  int cg_distance = 5;  // Call graph distance
  int cfg_distance = 10; // CFG distance

  double normalized_distance = (cg_distance + cfg_distance) / 100.0;
  return 1.0 / (1.0 + normalized_distance);
}

double VariableScorer::computeTypeComplexityScore(Type* type) {
  // Type complexity scoring
  if (isPointerType(type)) {
    if (isStringType(type)) {
      return 0.8; // String pointer
    }
    return 0.9; // Regular pointer
  } else if (isArrayType(type)) {
    return 0.7; // Array
  } else if (type->isIntegerTy()) {
    int bitWidth = type->getIntegerBitWidth();
    if (bitWidth >= 32) {
      return 0.5; // int/long
    }
    return 0.3; // char/short
  } else if (type->isFloatingPointTy()) {
    return 0.4; // float/double
  }

  return 0.2; // Other types
}

double VariableScorer::computeMemoryContextScore(Value* var) {
  // Memory context scoring
  double score = 0.0;

  if (!isa<Instruction>(var)) {
    return 0.0;
  }

  Instruction* inst = cast<Instruction>(var);

  // Check if used in memory operations
  for (User* user : var->users()) {
    if (auto* call = dyn_cast<CallInst>(user)) {
      Function* callee = call->getCalledFunction();
      if (callee) {
        StringRef name = callee->getName();
        if (name.contains("malloc") || name.contains("calloc")) {
          score += 0.4;
        } else if (name.contains("free")) {
          score += 0.3;
        } else if (name.contains("memcpy") || name.contains("memset")) {
          score += 0.2;
        } else if (name.contains("strcpy") || name.contains("strcat")) {
          score += 0.25;
        }
      }
    } else if (isa<LoadInst>(user) || isa<StoreInst>(user)) {
      score += 0.1;
    }
  }

  return std::min(1.0, score);
}

double VariableScorer::computeControlFlowScore(Value* var) {
  // Control flow sensitivity scoring
  double score = 0.0;

  if (!isa<Instruction>(var)) {
    return 0.0;
  }

  // Check if used in conditional statements
  for (User* user : var->users()) {
    if (auto* icmp = dyn_cast<ICmpInst>(user)) {
      // Used in comparison instruction
      for (User* icmp_user : icmp->users()) {
        if (isa<BranchInst>(icmp_user)) {
          score += 0.4; // Used in conditional branch
        }
      }
    } else if (auto* br = dyn_cast<BranchInst>(user)) {
      if (br->isConditional()) {
        score += 0.3; // Directly used in branch
      }
    } else if (isa<SwitchInst>(user)) {
      score += 0.5; // Used in switch statement
    }
  }

  return std::min(1.0, score);
}

double VariableScorer::computeDataFlowScore(Value* var) {
  // Data flow impact scoring
  double score = 0.0;

  if (!isa<Instruction>(var)) {
    return 0.0;
  }

  // Calculate def-use chain length
  int use_count = 0;
  std::set<Value*> visited;
  std::queue<Value*> queue;
  queue.push(var);
  visited.insert(var);

  while (!queue.empty() && use_count < 50) {
    Value* current = queue.front();
    queue.pop();
    
    for (User* user : current->users()) {
      use_count++;
      if (auto* inst = dyn_cast<Instruction>(user)) {
        if (visited.find(inst) == visited.end()) {
          visited.insert(inst);
          queue.push(inst);
        }
      }
    }
  }

  // Normalize
  score = std::min(1.0, use_count / 50.0);

  return score;
}

double VariableScorer::computeFrequencyScore(Value* var) {
  // Usage frequency scoring
  if (!isa<Instruction>(var)) {
    return 0.5;
  }

  Instruction* inst = cast<Instruction>(var);
  Function* func = inst->getFunction();

  // Calculate how many basic blocks use this variable
  std::set<BasicBlock*> used_blocks;
  for (User* user : var->users()) {
    if (auto* user_inst = dyn_cast<Instruction>(user)) {
      used_blocks.insert(user_inst->getParent());
    }
  }

  int total_blocks = 0;
  for (auto& bb : *func) {
    total_blocks++;
  }

  double frequency = (double)used_blocks.size() / std::max(1, total_blocks);
  return frequency;
}

void VariableScorer::scoreVariable(Value* var) {
  VariableScore score;
  score.variable = var;

  // Extract variable information
  if (var->hasName()) {
    score.variable_name = var->getName().str();
  } else {
    score.variable_name = "<unnamed>";
  }
  score.variable_type = var->getType();

  if (auto* inst = dyn_cast<Instruction>(var)) {
    score.parent_function = inst->getFunction();
  }

  // Calculate dimension scores
  score.distance_score = computeDistanceScore(var);
  score.type_complexity_score = computeTypeComplexityScore(var->getType());
  score.memory_context_score = computeMemoryContextScore(var);
  score.control_flow_score = computeControlFlowScore(var);
  score.data_flow_score = computeDataFlowScore(var);
  score.frequency_score = computeFrequencyScore(var);

  // Calculate total score
  score.total_score = 
      config_.weight_distance * score.distance_score +
      config_.weight_type_complexity * score.type_complexity_score +
      config_.weight_memory_context * score.memory_context_score +
      config_.weight_control_flow * score.control_flow_score +
      config_.weight_data_flow * score.data_flow_score +
      config_.weight_frequency * score.frequency_score;

  scores_[var] = score;
}

void VariableScorer::scoreAllVariables(const std::vector<Value*>& variables) {
  for (Value* var : variables) {
    scoreVariable(var);
  }
}

std::vector<Value*> VariableScorer::selectTopVariables() {
  std::vector<std::pair<Value*, double>> scored_vars;

  // Collect all scores
  for (const auto& entry : scores_) {
    if (entry.second.total_score >= config_.score_threshold) {
      scored_vars.push_back({entry.first, entry.second.total_score});
    }
  }

  // Sort by score
  std::sort(scored_vars.begin(), scored_vars.end(),
            [](const auto& a, const auto& b) {
              return a.second > b.second;
            });

  // Select Top-K
  std::vector<Value*> result;
  size_t count = std::min((size_t)config_.top_k, scored_vars.size());

  for (size_t i = 0; i < count; i++) {
    result.push_back(scored_vars[i].first);
  }

  // Apply type quota constraints (simplified version)
  // Full implementation needs more complex logic for type diversity

  return result;
}

const VariableScore& VariableScorer::getScore(Value* var) const {
  static VariableScore empty_score;
  auto it = scores_.find(var);
  if (it != scores_.end()) {
    return it->second;
  }
  return empty_score;
}

void VariableScorer::printScores() const {
  errs() << "=== Variable Scores ===\n";

  std::vector<std::pair<Value*, VariableScore>> sorted_scores;
  for (const auto& entry : scores_) {
    sorted_scores.push_back({entry.first, entry.second});
  }

  std::sort(sorted_scores.begin(), sorted_scores.end(),
            [](const auto& a, const auto& b) {
              return a.second.total_score > b.second.total_score;
            });

  for (const auto& entry : sorted_scores) {
    const VariableScore& score = entry.second;
    errs() << "Variable: " << score.variable_name << "\n";
    errs() << "  Total Score: " << score.total_score << "\n";
    errs() << "  Distance: " << score.distance_score << "\n";
    errs() << "  Type: " << score.type_complexity_score << "\n";
    errs() << "  Memory: " << score.memory_context_score << "\n";
    errs() << "  ControlFlow: " << score.control_flow_score << "\n";
    errs() << "  DataFlow: " << score.data_flow_score << "\n";
    errs() << "  Frequency: " << score.frequency_score << "\n";
    errs() << "\n";
  }
}

void VariableScorer::exportScores(const std::string& filename) const {
  std::ofstream out(filename);
  if (!out.is_open()) {
    errs() << "Error: Cannot open file " << filename << "\n";
    return;
  }

  out << "variable_name,total_score,distance,type,memory,control_flow,data_flow,frequency\n";

  for (const auto& entry : scores_) {
    const VariableScore& score = entry.second;
    out << score.variable_name << ","
        << score.total_score << ","
        << score.distance_score << ","
        << score.type_complexity_score << ","
        << score.memory_context_score << ","
        << score.control_flow_score << ","
        << score.data_flow_score << ","
        << score.frequency_score << "\n";
  }

  out.close();
  errs() << "Scores exported to " << filename << "\n";
}
