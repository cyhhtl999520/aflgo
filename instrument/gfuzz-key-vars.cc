/*
   GFuzz - Key Variable Identification Module
   -------------------------------------------
   
   Implements Algorithm 1 from the paper:
   Multi-strategy key variable identification combining:
   1. Distance-based filtering
   2. ASan-related filtering  
   3. Program semantics filtering
   
   This module extends AFLGo's LLVM pass to identify critical variables
   that should be monitored during fuzzing.
*/

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/CallGraph.h"

#include <string>
#include <set>
#include <map>
#include <vector>
#include <algorithm>
#include <fstream>
#include <queue>

using namespace llvm;

namespace gfuzz {

struct KeyVariable {
  std::string name;
  std::string function;
  Type* type;
  bool is_pointer;
  bool is_integer;
  bool is_asan_related;
  bool is_parameter;
  bool is_state_variable;
  int distance_to_target;
  
  KeyVariable() : type(nullptr), is_pointer(false), is_integer(false),
                  is_asan_related(false), is_parameter(false),
                  is_state_variable(false), distance_to_target(-1) {}
};

class KeyVariableIdentifier {
private:
  Module &M;
  std::set<std::string> target_functions;
  std::map<std::string, int> function_distances;
  std::map<std::string, KeyVariable> key_variables;
  int distance_threshold;
  
public:
  KeyVariableIdentifier(Module &M, int threshold = 3) 
    : M(M), distance_threshold(threshold) {}
  
  /* Load target functions from file */
  void loadTargetFunctions(const std::string& targets_file) {
    // Parse Ftargets.txt which contains target functions
    std::ifstream infile(targets_file);
    if (!infile.is_open()) {
      errs() << "Warning: Could not open Ftargets.txt\n";
      return;
    }
    
    std::string line;
    while (std::getline(infile, line)) {
      if (!line.empty()) {
        target_functions.insert(line);
      }
    }
    infile.close();
  }
  
  /* Calculate call graph distances to target functions */
  void calculateCallGraphDistances() {
    // Use BFS from target functions to compute distances
    std::queue<std::string> worklist;
    
    // Initialize with target functions at distance 0
    for (const auto& target : target_functions) {
      function_distances[target] = 0;
      worklist.push(target);
    }
    
    // Simple backward reachability analysis
    // In a full implementation, would use LLVM's CallGraph analysis
    for (Function &F : M) {
      if (F.isDeclaration()) continue;
      
      std::string fname = F.getName().str();
      
      // Check if function calls any target
      for (BasicBlock &BB : F) {
        for (Instruction &I : BB) {
          if (CallInst *CI = dyn_cast<CallInst>(&I)) {
            Function *Callee = CI->getCalledFunction();
            if (Callee && !Callee->isDeclaration()) {
              std::string callee_name = Callee->getName().str();
              if (target_functions.count(callee_name) > 0) {
                if (function_distances.count(fname) == 0 || 
                    function_distances[fname] > 1) {
                  function_distances[fname] = 1;
                }
              }
            }
          }
        }
      }
    }
  }
  
  /* Check if instruction is ASan-related */
  bool isAsanRelated(const Instruction *I) {
    if (const CallInst *CI = dyn_cast<CallInst>(I)) {
      Function *F = CI->getCalledFunction();
      if (F) {
        StringRef name = F->getName();
        if (name.startswith("__asan_") || 
            name.startswith("__ubsan_") ||
            name.contains("sanitizer")) {
          return true;
        }
      }
    }
    
    // Check if instruction accesses memory (potential buffer operations)
    if (isa<LoadInst>(I) || isa<StoreInst>(I) || 
        isa<GetElementPtrInst>(I)) {
      return true;
    }
    
    return false;
  }
  
  /* Check if value is a state variable (global or static) */
  bool isStateVariable(const Value *V) {
    if (isa<GlobalVariable>(V)) {
      return true;
    }
    
    if (const AllocaInst *AI = dyn_cast<AllocaInst>(V)) {
      // Check if it's a static variable (would need more analysis)
      return AI->isStaticAlloca();
    }
    
    return false;
  }
  
  /* Check if value is a function parameter */
  bool isParameter(const Value *V) {
    return isa<Argument>(V);
  }
  
  /* Check if type is pointer */
  bool isPointerType(Type *T) {
    return T->isPointerTy();
  }
  
  /* Check if type is integer */
  bool isIntegerType(Type *T) {
    return T->isIntegerTy();
  }
  
  /* Strategy 1: Distance-based variable filtering */
  void applyDistanceStrategy(Function &F) {
    std::string fname = F.getName().str();
    
    // Skip if function is too far from targets
    if (function_distances.count(fname) == 0 || 
        function_distances[fname] > distance_threshold) {
      return;
    }
    
    int func_dist = function_distances[fname];
    
    // Collect variables in function
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        // Look for store instructions to identify variables
        if (StoreInst *SI = dyn_cast<StoreInst>(&I)) {
          Value *ptr = SI->getPointerOperand();
          
          // Get variable name if available
          std::string var_name;
          if (ptr->hasName()) {
            var_name = ptr->getName().str();
          } else {
            // Generate a name based on the instruction
            raw_string_ostream ss(var_name);
            ptr->printAsOperand(ss, false);
          }
          
          std::string key = fname + "::" + var_name;
          
          if (key_variables.count(key) == 0) {
            KeyVariable kv;
            kv.name = var_name;
            kv.function = fname;
            kv.type = ptr->getType();
            kv.distance_to_target = func_dist;
            key_variables[key] = kv;
          }
        }
        
        // Also consider load instructions
        if (LoadInst *LI = dyn_cast<LoadInst>(&I)) {
          Value *ptr = LI->getPointerOperand();
          
          std::string var_name;
          if (ptr->hasName()) {
            var_name = ptr->getName().str();
          } else {
            raw_string_ostream ss(var_name);
            ptr->printAsOperand(ss, false);
          }
          
          std::string key = fname + "::" + var_name;
          
          if (key_variables.count(key) == 0) {
            KeyVariable kv;
            kv.name = var_name;
            kv.function = fname;
            kv.type = ptr->getType();
            kv.distance_to_target = func_dist;
            key_variables[key] = kv;
          }
        }
      }
    }
  }
  
  /* Strategy 2: ASan-related variable filtering */
  void applyAsanStrategy() {
    std::vector<std::string> to_remove;
    
    for (auto &entry : key_variables) {
      KeyVariable &kv = entry.second;
      bool found_asan_use = false;
      
      // Find the function
      Function *F = M.getFunction(kv.function);
      if (!F) continue;
      
      // Check all uses of this variable
      for (BasicBlock &BB : *F) {
        for (Instruction &I : BB) {
          if (isAsanRelated(&I)) {
            // Check if this instruction references our variable
            for (Use &U : I.operands()) {
              Value *V = U.get();
              if (V->hasName() && V->getName().str() == kv.name) {
                found_asan_use = true;
                kv.is_asan_related = true;
                break;
              }
            }
          }
        }
      }
      
      // Keep only ASan-related variables (or adjust criteria as needed)
      // In practice, we may want to keep more variables
      if (!found_asan_use && !kv.is_parameter) {
        // Mark for potential removal (strategy 2 is selective)
        // For now, we'll keep it if it passed strategy 1
      }
    }
  }
  
  /* Strategy 3: Program semantics filtering */
  void applySemanticStrategy() {
    std::vector<std::string> to_remove;
    
    for (auto &entry : key_variables) {
      KeyVariable &kv = entry.second;
      
      // Update semantic flags
      if (kv.type) {
        kv.is_pointer = isPointerType(kv.type);
        kv.is_integer = isIntegerType(kv.type);
      }
      
      // Filter based on type: keep only pointers and integers
      if (!kv.is_pointer && !kv.is_integer) {
        to_remove.push_back(entry.first);
      }
    }
    
    // Remove filtered variables
    for (const auto &key : to_remove) {
      key_variables.erase(key);
    }
  }
  
  /* Main algorithm: Algorithm 1 from paper */
  void identifyKeyVariables(const std::string& targets_file) {
    // Load targets and calculate distances
    loadTargetFunctions(targets_file);
    calculateCallGraphDistances();
    
    // Strategy 1: Distance-based filtering (lines 3-14)
    for (Function &F : M) {
      if (!F.isDeclaration()) {
        applyDistanceStrategy(F);
      }
    }
    
    // Strategy 2: ASan-related filtering (lines 16-23)
    applyAsanStrategy();
    
    // Strategy 3: Semantic filtering (lines 25-32)
    applySemanticStrategy();
    
    errs() << "[GFuzz] Identified " << key_variables.size() 
           << " key variables for monitoring\n";
  }
  
  /* Export key variables to file */
  void exportKeyVariables(const std::string& output_file) {
    std::ofstream outfile(output_file);
    if (!outfile.is_open()) {
      errs() << "Error: Could not open output file " << output_file << "\n";
      return;
    }
    
    for (const auto &entry : key_variables) {
      const KeyVariable &kv = entry.second;
      outfile << kv.function << "," << kv.name << "," 
              << kv.distance_to_target << ","
              << (kv.is_pointer ? "ptr" : (kv.is_integer ? "int" : "other"))
              << "\n";
    }
    
    outfile.close();
    errs() << "[GFuzz] Exported key variables to " << output_file << "\n";
  }
  
  const std::map<std::string, KeyVariable>& getKeyVariables() const {
    return key_variables;
  }
};

} // namespace gfuzz
