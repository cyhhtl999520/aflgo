/*
   GFuzz - Variable State Monitoring Instrumentation
   --------------------------------------------------
   
   Implements Algorithm 2 from the paper:
   Variable state monitoring and instrumentation at LLVM IR level.
   
   This extends AFLGo's instrumentation pass to:
   1. Identify key variables during compilation
   2. Insert monitoring code to track variable states
   3. Normalize values to 32-bit representation for efficiency
*/

#ifndef GFUZZ_INSTRUMENTATION_H
#define GFUZZ_INSTRUMENTATION_H

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"

#include <map>
#include <string>
#include <fstream>

#include "../gfuzz-config.h"

using namespace llvm;

namespace gfuzz {

class VariableStateInstrumenter {
private:
  Module &M;
  LLVMContext &C;
  
  /* Runtime functions for state tracking */
  FunctionCallee gfuzz_record_numeric;
  FunctionCallee gfuzz_record_char;
  FunctionCallee gfuzz_record_string;
  FunctionCallee gfuzz_record_pointer;
  
  /* Map from variable key to unique ID */
  std::map<std::string, unsigned> var_to_id;
  unsigned next_var_id;
  
  /* Key variables loaded from preprocessing */
  std::map<std::string, int> key_variables;
  
public:
  VariableStateInstrumenter(Module &M) 
    : M(M), C(M.getContext()), next_var_id(0) {
    
    /* Declare runtime functions for state recording */
    
    // void gfuzz_record_numeric(uint32_t var_id, uint32_t value)
    {
      Type *ArgTypes[] = {
        Type::getInt32Ty(C),  // var_id
        Type::getInt32Ty(C)   // value
      };
      FunctionType *FT = FunctionType::get(Type::getVoidTy(C), ArgTypes, false);
      gfuzz_record_numeric = M.getOrInsertFunction("__gfuzz_record_numeric", FT);
    }
    
    // void gfuzz_record_char(uint32_t var_id, uint8_t value)
    {
      Type *ArgTypes[] = {
        Type::getInt32Ty(C),  // var_id
        Type::getInt8Ty(C)    // value
      };
      FunctionType *FT = FunctionType::get(Type::getVoidTy(C), ArgTypes, false);
      gfuzz_record_char = M.getOrInsertFunction("__gfuzz_record_char", FT);
    }
    
    // void gfuzz_record_string(uint32_t var_id, char* value)
    {
      Type *ArgTypes[] = {
        Type::getInt32Ty(C),           // var_id
        Type::getInt8PtrTy(C)          // value
      };
      FunctionType *FT = FunctionType::get(Type::getVoidTy(C), ArgTypes, false);
      gfuzz_record_string = M.getOrInsertFunction("__gfuzz_record_string", FT);
    }
    
    // void gfuzz_record_pointer(uint32_t var_id, void* value)
    {
      Type *ArgTypes[] = {
        Type::getInt32Ty(C),           // var_id
        Type::getInt8PtrTy(C)          // value (as void*)
      };
      FunctionType *FT = FunctionType::get(Type::getVoidTy(C), ArgTypes, false);
      gfuzz_record_pointer = M.getOrInsertFunction("__gfuzz_record_pointer", FT);
    }
  }
  
  /* Load key variables from preprocessing phase */
  void loadKeyVariables(const std::string& key_vars_file) {
    std::ifstream infile(key_vars_file);
    if (!infile.is_open()) {
      errs() << "[GFuzz] Warning: Could not open key variables file: " 
             << key_vars_file << "\n";
      return;
    }
    
    std::string line;
    while (std::getline(infile, line)) {
      // Format: function,variable,distance,type
      size_t pos1 = line.find(',');
      size_t pos2 = line.find(',', pos1 + 1);
      
      if (pos1 != std::string::npos && pos2 != std::string::npos) {
        std::string func = line.substr(0, pos1);
        std::string var = line.substr(pos1 + 1, pos2 - pos1 - 1);
        std::string key = func + "::" + var;
        
        key_variables[key] = next_var_id++;
        
        if (next_var_id >= GFUZZ_MAX_KEY_VARS) {
          errs() << "[GFuzz] Warning: Reached maximum key variables limit\n";
          break;
        }
      }
    }
    
    infile.close();
    errs() << "[GFuzz] Loaded " << key_variables.size() 
           << " key variables for instrumentation\n";
  }
  
  /* Get unique ID for a variable */
  unsigned getVariableID(const std::string& func_name, const std::string& var_name) {
    std::string key = func_name + "::" + var_name;
    
    if (key_variables.count(key) > 0) {
      return key_variables[key];
    }
    
    return UINT32_MAX; // Invalid ID
  }
  
  /* Extract and normalize value to 32-bit representation */
  Value* extractAndNormalizeValue(IRBuilder<> &Builder, Value *V) {
    Type *VTy = V->getType();
    
    // Handle integers
    if (VTy->isIntegerTy()) {
      unsigned BitWidth = VTy->getIntegerBitWidth();
      
      if (BitWidth == 32) {
        return V;
      } else if (BitWidth < 32) {
        // Zero-extend to 32 bits
        return Builder.CreateZExt(V, Type::getInt32Ty(C));
      } else {
        // Truncate to 32 bits (lose precision but maintain performance)
        return Builder.CreateTrunc(V, Type::getInt32Ty(C));
      }
    }
    
    // Handle pointers - convert to integer
    if (VTy->isPointerTy()) {
      Value *PtrInt = Builder.CreatePtrToInt(V, Type::getInt64Ty(C));
      return Builder.CreateTrunc(PtrInt, Type::getInt32Ty(C));
    }
    
    // Handle floats - convert to bits
    if (VTy->isFloatTy()) {
      return Builder.CreateBitCast(V, Type::getInt32Ty(C));
    }
    
    // Handle doubles - extract lower 32 bits
    if (VTy->isDoubleTy()) {
      Value *Bits = Builder.CreateBitCast(V, Type::getInt64Ty(C));
      return Builder.CreateTrunc(Bits, Type::getInt32Ty(C));
    }
    
    // Default: return 0
    return ConstantInt::get(Type::getInt32Ty(C), 0);
  }
  
  /* Determine variable type category */
  int getVariableTypeCategory(Type *T) {
    if (T->isPointerTy()) {
      Type *ElementTy = T->getPointerElementType();
      if (ElementTy->isIntegerTy(8)) {
        // Could be string (char*)
        return GFUZZ_VAR_TYPE_STRING;
      }
      return GFUZZ_VAR_TYPE_POINTER;
    }
    
    if (T->isIntegerTy()) {
      unsigned BitWidth = T->getIntegerBitWidth();
      if (BitWidth == 8) {
        return GFUZZ_VAR_TYPE_CHAR;
      }
      return GFUZZ_VAR_TYPE_NUMERIC;
    }
    
    return GFUZZ_VAR_TYPE_NUMERIC; // Default
  }
  
  /* Insert instrumentation for a variable update */
  void instrumentVariableAccess(Instruction *I, Value *Var, Value *Val,
                                const std::string &FuncName) {
    if (!Var || !Val) return;
    
    std::string var_name;
    if (Var->hasName()) {
      var_name = Var->getName().str();
    } else {
      return; // Skip unnamed variables for now
    }
    
    unsigned var_id = getVariableID(FuncName, var_name);
    if (var_id == UINT32_MAX) {
      return; // Not a key variable
    }
    
    // Insert monitoring code after the instruction
    IRBuilder<> Builder(I);
    Builder.SetInsertPoint(I->getNextNode());
    
    // Determine type and insert appropriate recording call
    Type *ValTy = Val->getType();
    int type_cat = getVariableTypeCategory(ValTy);
    
    Value *VarID = ConstantInt::get(Type::getInt32Ty(C), var_id);
    
    switch (type_cat) {
      case GFUZZ_VAR_TYPE_NUMERIC: {
        Value *NormVal = extractAndNormalizeValue(Builder, Val);
        Builder.CreateCall(gfuzz_record_numeric, {VarID, NormVal});
        break;
      }
      
      case GFUZZ_VAR_TYPE_CHAR: {
        if (ValTy->isIntegerTy(8)) {
          Builder.CreateCall(gfuzz_record_char, {VarID, Val});
        } else {
          // Truncate to 8 bits
          Value *CharVal = Builder.CreateTrunc(Val, Type::getInt8Ty(C));
          Builder.CreateCall(gfuzz_record_char, {VarID, CharVal});
        }
        break;
      }
      
      case GFUZZ_VAR_TYPE_STRING: {
        // Val should be a pointer to char
        if (ValTy->isPointerTy()) {
          Builder.CreateCall(gfuzz_record_string, {VarID, Val});
        }
        break;
      }
      
      case GFUZZ_VAR_TYPE_POINTER: {
        if (ValTy->isPointerTy()) {
          // Cast to void* (i8*)
          Value *VoidPtr = Builder.CreateBitCast(Val, Type::getInt8PtrTy(C));
          Builder.CreateCall(gfuzz_record_pointer, {VarID, VoidPtr});
        }
        break;
      }
    }
  }
  
  /* Instrument all key variables in a function (Algorithm 2) */
  void instrumentFunction(Function &F) {
    std::string FuncName = F.getName().str();
    
    // Collect instructions to instrument (avoid iterator invalidation)
    std::vector<Instruction*> to_instrument;
    
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        // Instrument store instructions
        if (StoreInst *SI = dyn_cast<StoreInst>(&I)) {
          Value *Ptr = SI->getPointerOperand();
          Value *Val = SI->getValueOperand();
          
          if (Ptr->hasName()) {
            std::string var_name = Ptr->getName().str();
            unsigned var_id = getVariableID(FuncName, var_name);
            
            if (var_id != UINT32_MAX) {
              to_instrument.push_back(SI);
            }
          }
        }
        
        // Could also instrument load instructions if needed
        // For now, focusing on stores (variable updates)
      }
    }
    
    // Apply instrumentation
    for (Instruction *I : to_instrument) {
      if (StoreInst *SI = dyn_cast<StoreInst>(I)) {
        instrumentVariableAccess(SI, SI->getPointerOperand(),
                                SI->getValueOperand(), FuncName);
      }
    }
  }
  
  /* Export variable ID mapping */
  void exportVariableMapping(const std::string& output_file) {
    std::ofstream outfile(output_file);
    if (!outfile.is_open()) {
      errs() << "[GFuzz] Error: Could not open output file " << output_file << "\n";
      return;
    }
    
    for (const auto &entry : key_variables) {
      outfile << entry.second << "," << entry.first << "\n";
    }
    
    outfile.close();
    errs() << "[GFuzz] Exported variable mapping to " << output_file << "\n";
  }
};

} // namespace gfuzz

#endif // GFUZZ_INSTRUMENTATION_H
