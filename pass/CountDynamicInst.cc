#include "llvm/Pass.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include <map>
#include <vector>

using namespace llvm;

namespace {

struct CountDIPass : public FunctionPass {
  static char ID;
  CountDIPass() : FunctionPass(ID) { }

  static ConstantInt* getInt32(LLVMContext& ctx, uint64_t n) {
    return ConstantInt::get(ctx, APInt(32 /* nbits */, n, false /* is_signed */));
  }

  bool runOnFunction(Function& F) override {
    Module* mod = F.getParent();

    if (mod == nullptr) {
      return false;
    }

    LLVMContext& ctx = mod->getContext();
    Function* updateF = nullptr;
    Function* printF = nullptr;

    // Define some functions for hijacking.
    updateF = cast<Function>(mod->getOrInsertFunction("updateInstrInfo",
          Type::getVoidTy(ctx), /* returning void */
          IntegerType::getInt32Ty(ctx),
          PointerType::get(IntegerType::getInt32Ty(ctx), 0),
          PointerType::get(IntegerType::getInt32Ty(ctx), 0),
          nullptr));
    printF = cast<Function>(mod->getOrInsertFunction("printOutInstrInfo",
          Type::getVoidTy(ctx),
          nullptr));

    for (Function::iterator blk_it = F.begin(), blk_e = F.end();
         blk_it != blk_e; ++blk_it) {
      std::map<uint32_t, uint32_t> inst_cnt;

      // Count instruction statistics in this basic block.
      for (BasicBlock::iterator inst_it = blk_it->begin(), inst_e = blk_it->end();
           inst_it != inst_e; ++inst_it) {
        inst_cnt[inst_it->getOpcode()] += 1;
      }

      // Prepare argments.
      std::vector<Constant*> const_keys, const_vals;
      for (std::map<uint32_t, uint32_t>::iterator it = inst_cnt.begin();
           it != inst_cnt.end(); ++it) {
        const_keys.push_back(getInt32(ctx, it->first));
        const_vals.push_back(getInt32(ctx, it->second));
      }

      Constant* const_n = getInt32(ctx, inst_cnt.size());
      ArrayType* array_ty = ArrayType::get(IntegerType::get(ctx, 32), inst_cnt.size());
      GlobalVariable* g_array_keys = new GlobalVariable(
          *mod, array_ty, true /* is_constant */, GlobalValue::InternalLinkage,
          ConstantArray::get(array_ty, const_keys));
      GlobalVariable* g_array_vals = new GlobalVariable(
          *mod, array_ty, true /* is_constant */, GlobalValue::InternalLinkage,
          ConstantArray::get(array_ty, const_vals));

      // Build function call to updateInstrInfo.
      IRBuilder<> builder(&*blk_it->begin());
      Value* keys_0 = builder.CreateInBoundsGEP(
          g_array_keys, llvm::ArrayRef<llvm::Value*>({getInt32(ctx, 0), getInt32(ctx, 0)}));
      Value* vals_0 = builder.CreateInBoundsGEP(
          g_array_vals, llvm::ArrayRef<llvm::Value*>({getInt32(ctx, 0), getInt32(ctx, 0)}));
      builder.CreateCall(updateF, {const_n, keys_0, vals_0});
    }

    for (inst_iterator inst_it = inst_begin(F), inst_e = inst_end(F);
       inst_it != inst_e; ++inst_it) {
      ReturnInst* ret_inst = dyn_cast<ReturnInst>(&*inst_it);

      if (ret_inst != nullptr) {
        IRBuilder<> builder(ret_inst);

        // Build function call to printInstrInfo before every ret.
        builder.CreateCall(printF, {});
        break;
      }
    }

    return false;
  }
};

}

char CountDIPass::ID = 0;
static RegisterPass<CountDIPass> X(
    "cdi", "Count dynamic instructions",
    false /* Only looks at CFG */,
    false /* Analysis Pass */);
