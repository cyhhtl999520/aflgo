/*
  Copyright 2015 Google LLC All rights reserved.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at:

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

/*
   american fuzzy lop - LLVM-mode instrumentation pass
   ---------------------------------------------------

   Written by Laszlo Szekeres <lszekeres@google.com> and
              Michal Zalewski <lcamtuf@google.com>

   LLVM integration design comes from Laszlo Szekeres. C bits copied-and-pasted
   from afl-as.c are Michal's fault.

   This library is plugged into LLVM when invoking clang through afl-clang-fast.
   It tells the compiler to add code roughly equivalent to the bits discussed
   in ../afl-as.h.
*/

#define AFL_LLVM_PASS

#include "../config.h"
#include "../debug.h"
#include "variable-scoring.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;
using namespace gfuzz;

namespace {

  class AFLCoverage : public ModulePass {

    public:

      static char ID;
      AFLCoverage() : ModulePass(ID) { }

      bool runOnModule(Module &M) override;

      // StringRef getPassName() const override {
      //  return "American Fuzzy Lop Instrumentation";
      // }

  };

}


char AFLCoverage::ID = 0;


bool AFLCoverage::runOnModule(Module &M) {

  LLVMContext &C = M.getContext();

  IntegerType *Int8Ty  = IntegerType::getInt8Ty(C);
  IntegerType *Int32Ty = IntegerType::getInt32Ty(C);

  /* Show a banner */

  char be_quiet = 0;

  if (isatty(2) && !getenv("AFL_QUIET")) {

    SAYF(cCYA "afl-llvm-pass " cBRI VERSION cRST " by <lszekeres@google.com>\n");

  } else be_quiet = 1;

  /* Decide instrumentation ratio */

  char* inst_ratio_str = getenv("AFL_INST_RATIO");
  unsigned int inst_ratio = 100;

  if (inst_ratio_str) {

    if (sscanf(inst_ratio_str, "%u", &inst_ratio) != 1 || !inst_ratio ||
        inst_ratio > 100)
      FATAL("Bad value of AFL_INST_RATIO (must be between 1 and 100)");

  }

  /* Get globals for the SHM region and the previous location. Note that
     __afl_prev_loc is thread-local. */

  GlobalVariable *AFLMapPtr =
      new GlobalVariable(M, PointerType::get(Int8Ty, 0), false,
                         GlobalValue::ExternalLinkage, 0, "__afl_area_ptr");

  GlobalVariable *AFLPrevLoc = new GlobalVariable(
      M, Int32Ty, false, GlobalValue::ExternalLinkage, 0, "__afl_prev_loc",
      0, GlobalVariable::GeneralDynamicTLSModel, 0, false);

  /* Instrument all the things! */

  int inst_blocks = 0;

  for (auto &F : M)
    for (auto &BB : F) {

      BasicBlock::iterator IP = BB.getFirstInsertionPt();
      IRBuilder<> IRB(&(*IP));

      if (AFL_R(100) >= inst_ratio) continue;

      /* Make up cur_loc */

      unsigned int cur_loc = AFL_R(MAP_SIZE);

      ConstantInt *CurLoc = ConstantInt::get(Int32Ty, cur_loc);

      /* Load prev_loc */

      LoadInst *PrevLoc = IRB.CreateLoad(AFLPrevLoc);
      PrevLoc->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
      Value *PrevLocCasted = IRB.CreateZExt(PrevLoc, IRB.getInt32Ty());

      /* Load SHM pointer */

      LoadInst *MapPtr = IRB.CreateLoad(AFLMapPtr);
      MapPtr->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
      Value *MapPtrIdx =
          IRB.CreateGEP(MapPtr, IRB.CreateXor(PrevLocCasted, CurLoc));

      /* Update bitmap */

      LoadInst *Counter = IRB.CreateLoad(MapPtrIdx);
      Counter->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
      Value *Incr = IRB.CreateAdd(Counter, ConstantInt::get(Int8Ty, 1));
      IRB.CreateStore(Incr, MapPtrIdx)
          ->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));

      /* Set prev_loc to cur_loc >> 1 */

      StoreInst *Store =
          IRB.CreateStore(ConstantInt::get(Int32Ty, cur_loc >> 1), AFLPrevLoc);
      Store->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));

      inst_blocks++;

    }

  /* Variable scoring mechanism (optional) */
  
  char* gfuzz_scoring_enabled = getenv("GFUZZ_SCORING_ENABLED");
  if (gfuzz_scoring_enabled != NULL && 
      (strcmp(gfuzz_scoring_enabled, "1") == 0 || 
       strcasecmp(gfuzz_scoring_enabled, "true") == 0 ||
       strcasecmp(gfuzz_scoring_enabled, "on") == 0)) {
    
    if (!be_quiet) {
      SAYF(cCYA "[GFuzz] Variable scoring mechanism enabled\n" cRST);
    }
    
    // Collect candidate variables from all functions
    std::vector<Value*> all_candidates;
    
    for (auto &F : M) {
      if (F.isDeclaration()) continue;
      
      for (auto &BB : F) {
        for (auto &I : BB) {
          // Collect interesting variables: pointers, integers, etc.
          if (I.getType()->isPointerTy() || 
              I.getType()->isIntegerTy() ||
              I.getType()->isFloatingPointTy()) {
            all_candidates.push_back(&I);
          }
        }
      }
      
      // Also collect function arguments
      for (auto &Arg : F.args()) {
        if (Arg.getType()->isPointerTy() || 
            Arg.getType()->isIntegerTy() ||
            Arg.getType()->isFloatingPointTy()) {
          all_candidates.push_back(&Arg);
        }
      }
    }
    
    if (!all_candidates.empty()) {
      // Initialize variable scorer
      VariableScorer scorer;
      
      // Load configuration if available
      char* config_file = getenv("GFUZZ_SCORING_CONFIG");
      if (config_file) {
        scorer.loadConfig(config_file);
      }
      
      // Score all candidate variables
      scorer.scoreAllVariables(all_candidates);
      
      // Select top variables
      std::vector<Value*> selected = scorer.selectTopVariables();
      
      if (!be_quiet) {
        SAYF(cGRN "[GFuzz] Scored %zu variables, selected %zu key variables\n" cRST,
             all_candidates.size(), selected.size());
        
        double reduction = (1.0 - (double)selected.size() / all_candidates.size()) * 100.0;
        SAYF(cGRN "[GFuzz] Variable reduction: %.1f%%\n" cRST, reduction);
      }
      
      // Export scores if debug mode is enabled
      char* debug_mode = getenv("GFUZZ_DEBUG");
      if (debug_mode != NULL && 
          (strcmp(debug_mode, "1") == 0 || 
           strcasecmp(debug_mode, "true") == 0 ||
           strcasecmp(debug_mode, "on") == 0)) {
        scorer.printScores();
        scorer.exportScores("variable_scores.csv");
      }
    }
  }

  /* Say something nice. */

  if (!be_quiet) {

    if (!inst_blocks) WARNF("No instrumentation targets found.");
    else OKF("Instrumented %u locations (%s mode, ratio %u%%).",
             inst_blocks, getenv("AFL_HARDEN") ? "hardened" :
             ((getenv("AFL_USE_ASAN") || getenv("AFL_USE_MSAN")) ?
              "ASAN/MSAN" : "non-hardened"), inst_ratio);

  }

  return true;

}


static void registerAFLPass(const PassManagerBuilder &,
                            legacy::PassManagerBase &PM) {

  PM.add(new AFLCoverage());

}


static RegisterStandardPasses RegisterAFLPass(
    PassManagerBuilder::EP_ModuleOptimizerEarly, registerAFLPass);

static RegisterStandardPasses RegisterAFLPass0(
    PassManagerBuilder::EP_EnabledOnOptLevel0, registerAFLPass);
