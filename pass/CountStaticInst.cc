#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/raw_ostream.h"

#include <map>

using namespace llvm;

namespace {

struct CountSIPass : public FunctionPass {
  static char ID;
  CountSIPass() : FunctionPass(ID) { }

  bool runOnFunction(Function& F) override {
    std::map<const char*, int> inst_count;

    for (inst_iterator inst_it = inst_begin(F), inst_e = inst_end(F);
         inst_it != inst_e; ++inst_it) {
      Instruction& inst = *inst_it;

      inst_count[inst.getOpcodeName()] += 1;
    }

    for (std::map<const char*, int>::iterator it = inst_count.begin();
         it != inst_count.end(); ++it) {
      errs() << it->first << '\t' << it->second << '\n';
    }
    return false;
  }
};

}

char CountSIPass::ID = 0;
static RegisterPass<CountSIPass> X(
    "csi", "Count static instructions",
    false /* Only looks at CFG */,
    false /* Analysis Pass */);
