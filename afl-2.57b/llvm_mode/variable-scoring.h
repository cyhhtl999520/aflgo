#ifndef VARIABLE_SCORING_H
#define VARIABLE_SCORING_H

#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <map>
#include <vector>
#include <string>
#include <queue>
#include <set>

namespace gfuzz {

// Variable scoring structure
struct VariableScore {
  llvm::Value* variable;
  double distance_score;
  double type_complexity_score;
  double memory_context_score;
  double control_flow_score;
  double data_flow_score;
  double frequency_score;
  double total_score;

  // Metadata
  std::string variable_name;
  llvm::Type* variable_type;
  llvm::Function* parent_function;

  VariableScore() : variable(nullptr), 
                    distance_score(0.0),
                    type_complexity_score(0.0),
                    memory_context_score(0.0),
                    control_flow_score(0.0),
                    data_flow_score(0.0),
                    frequency_score(0.0),
                    total_score(0.0),
                    variable_type(nullptr),
                    parent_function(nullptr) {}
};

// Configuration parameters
struct ScoringConfig {
  double weight_distance;
  double weight_type_complexity;
  double weight_memory_context;
  double weight_control_flow;
  double weight_data_flow;
  double weight_frequency;

  double score_threshold;
  unsigned int top_k;

  double pointer_min_quota;
  double integer_min_quota;
  double string_min_quota;

  ScoringConfig() : 
      weight_distance(0.25),
      weight_type_complexity(0.20),
      weight_memory_context(0.20),
      weight_control_flow(0.15),
      weight_data_flow(0.15),
      weight_frequency(0.05),
      score_threshold(0.5),
      top_k(100),
      pointer_min_quota(0.20),
      integer_min_quota(0.15),
      string_min_quota(0.10) {}
};

// Variable scorer class
class VariableScorer {
private:
  ScoringConfig config_;
  std::map<llvm::Value*, VariableScore> scores_;

  // Private helper methods
  double computeDistanceScore(llvm::Value* var);
  double computeTypeComplexityScore(llvm::Type* type);
  double computeMemoryContextScore(llvm::Value* var);
  double computeControlFlowScore(llvm::Value* var);
  double computeDataFlowScore(llvm::Value* var);
  double computeFrequencyScore(llvm::Value* var);

public:
  VariableScorer() {}
  VariableScorer(const ScoringConfig& config) : config_(config) {}

  // Main interface
  void loadConfig(const std::string& config_file);
  void scoreVariable(llvm::Value* var);
  void scoreAllVariables(const std::vector<llvm::Value*>& variables);
  std::vector<llvm::Value*> selectTopVariables();

  // Accessors
  const VariableScore& getScore(llvm::Value* var) const;
  const ScoringConfig& getConfig() const { return config_; }
  void setConfig(const ScoringConfig& config) { config_ = config; }

  // Debug and analysis
  void printScores() const;
  void exportScores(const std::string& filename) const;
};

} // namespace gfuzz

#endif // VARIABLE_SCORING_H
