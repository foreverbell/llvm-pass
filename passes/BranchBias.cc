#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include <map>
#include <vector>

using namespace llvm;

namespace {

struct BranchBiasPass : public FunctionPass {
  static char ID;
  BranchBiasPass() : FunctionPass(ID) { }

  bool runOnFunction(Function& F) override {
    Module* mod = F.getParent();

    if (mod == nullptr) {
      return false;
    }

    LLVMContext& ctx = mod->getContext();
    Function* updateF = nullptr;
    Function* printF = nullptr;

    // Define some functions for hijacking.
    updateF = cast<Function>(mod->getOrInsertFunction("updateBranchInfo",
          Type::getVoidTy(ctx), /* returning void */
          IntegerType::getInt1Ty(ctx), /* bool */
          nullptr));
    printF = cast<Function>(mod->getOrInsertFunction("printOutBranchInfo",
          Type::getVoidTy(ctx),
          nullptr));

    for (inst_iterator inst_it = inst_begin(F), inst_e = inst_end(F);
       inst_it != inst_e; ++inst_it) {
      BranchInst* br_inst = dyn_cast<BranchInst>(&*inst_it);
      ReturnInst* ret_inst = dyn_cast<ReturnInst>(&*inst_it);
      
      if (br_inst != nullptr) {
        // Update branch bias before conditional.
        if (br_inst->isConditional()) {
          IRBuilder<> builder(br_inst);
          builder.CreateCall(updateF, {br_inst->getCondition()});
        }
      } else if (ret_inst != nullptr) {
        // Print statstics before return.
        IRBuilder<> builder(ret_inst);
        builder.CreateCall(printF, {});
      }
    }

    return false;
  }
};

}

char BranchBiasPass::ID = 0;
static RegisterPass<BranchBiasPass> X(
    "bb", "Profile branch bias",
    false /* Only looks at CFG */,
    false /* Analysis Pass */);
