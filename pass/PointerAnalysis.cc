#include "DataflowAnalysis.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

#include <map>
#include <set>
#include <string>

using namespace llvm;

class PointerInfo : public AnalysisInfo {
 public:
  std::string PrintPtrMem(int x) {
    if (x > 0) {
      return "R" + std::to_string(x);
    } else {
      return "M" + std::to_string(x - 0x80000000);
    }
  }

  virtual void Print() {
    for (std::map<int, std::set<int>>::iterator it = pointer_.begin();
         it != pointer_.end(); ++it) {
      errs() << PrintPtrMem(it->first) << "->(";
      for (int x : it->second) {
        errs() << PrintPtrMem(x) << '/';
      }
      errs() << ")|";
    }
    errs() << '\n';
  }

  void add(int R, int M) {
    pointer_[R].insert(M);
  }

  // move <b> to <a>.
  void move(int a, int b) {
    if (a == b) return;

    std::map<int, std::set<int>>::iterator it = pointer_.find(b);

    if (it != pointer_.end()) {
      std::set<int>& s = pointer_[a];

      for (int x : it->second) {
        s.insert(x);
      }
    }
  }

  // move all <x> in <b> to <a>.
  void move2(int a, int b) {
    std::map<int, std::set<int>>::iterator it = pointer_.find(b);

    if (it != pointer_.end()) {
      std::set<int> s_b = it->second;
      for (int x : s_b) {
        move(a, x);
      }
    }
  }

  // if a->x and b->y, add y->x.
  void combine(int a, int b) {
    std::map<int, std::set<int>>::iterator it_a = pointer_.find(a);
    std::map<int, std::set<int>>::iterator it_b = pointer_.find(b);

    if (it_a == pointer_.end() || it_b == pointer_.end()) {
      return;
    }
    std::set<int> s_a = it_a->second;
    std::set<int> s_b = it_b->second;

    for (int y : s_b) {
      std::set<int>& s = pointer_[y];

      for (int x : s_a) {
        s.insert(x);
      }
    }
  }

  static PointerInfo Bottom() {
    return PointerInfo();
  }

  static bool Equals(const PointerInfo* info1, const PointerInfo* info2) {
    return info1->pointer_ == info2->pointer_;
  }

  static std::unique_ptr<PointerInfo> Join(const PointerInfo* info1,
      const PointerInfo* info2) {
    std::unique_ptr<PointerInfo> ret(new PointerInfo(*info1));

    for (const auto& pts : info2->pointer_) {
      for (int y : pts.second) {
        ret->add(pts.first, y);
      }
    }
    return ret;
  }

 private:
  std::map<int, std::set<int>> pointer_;
};

class PointerAnalysis
    : public DataFlowAnalysis<PointerInfo, true /* Direction */> {

 public:
  PointerAnalysis()
    : DataFlowAnalysis<PointerInfo, true>(
        PointerInfo::Bottom(), PointerInfo::Bottom()) { }

 private:
  virtual void FlowFunction(
      Instruction* I,
      int inst_index,
      const PointerInfo& in,
      const std::vector<Edge>& outs,
      std::vector<PointerInfo>& infos) const override;
};

void PointerAnalysis::FlowFunction(
    Instruction* I,
    int inst_index,
    const PointerInfo& in,
    const std::vector<Edge>& outs,
    std::vector<PointerInfo>& infos) const {

  PointerInfo out = in;

  switch (I->getOpcode()) {
    // alloca.
    case Instruction::Alloca: {
      out.add(inst_index, 0x80000000 + inst_index);
    } break;

    // bitcast.
    case Instruction::BitCast: {
      Instruction* inst = cast<Instruction>(I->getOperand(0));
      std::map<Instruction*, int>::const_iterator inst_mpit = inst_map_.find(inst);

      if (inst_mpit != inst_map_.end()) {
        out.move(inst_index, inst_mpit->second);
      }
    } break;

    // getelementptr.
    case Instruction::GetElementPtr: {
      GetElementPtrInst* inst = cast<GetElementPtrInst>(I);
      std::map<Instruction*, int>::const_iterator inst_mpit = inst_map_.find(
          cast<Instruction>(inst->getPointerOperand()));

      if (inst_mpit != inst_map_.end()) {
        out.move(inst_index, inst_mpit->second);
      }
    } break;

    // load.
    case Instruction::Load: {
      LoadInst* inst = cast<LoadInst>(I);
      std::map<Instruction*, int>::const_iterator inst_mpit = inst_map_.find(
          cast<Instruction>(inst->getPointerOperand()));

      if (inst_mpit != inst_map_.end()) {
        out.move2(inst_index, inst_mpit->second);
      }
    } break;

    // store.
    case Instruction::Store: {
      StoreInst* inst = cast<StoreInst>(I);
      std::map<Instruction*, int>::const_iterator inst_mpit1 = inst_map_.find(
          cast<Instruction>(inst->getPointerOperand()));
      std::map<Instruction*, int>::const_iterator inst_mpit2 = inst_map_.find(
          cast<Instruction>(inst->getValueOperand()));

      if (inst_mpit1 != inst_map_.end() && inst_mpit2 != inst_map_.end()) {
        out.combine(inst_mpit2->second, inst_mpit1->second);
      }
    } break;

    // select.
    case Instruction::Select: {
      SelectInst* inst = cast<SelectInst>(I);
      std::map<Instruction*, int>::const_iterator inst_mpit;

      inst_mpit = inst_map_.find(cast<Instruction>(inst->getTrueValue()));
      if (inst_mpit != inst_map_.end()) {
        out.move(inst_index, inst_mpit->second);
      }

      inst_mpit = inst_map_.find(cast<Instruction>(inst->getFalseValue()));
      if (inst_mpit != inst_map_.end()) {
        out.move(inst_index, inst_mpit->second);
      }
    } break;

    // phi.
    case Instruction::PHI: {
      BasicBlock* block = I->getParent();

      // PHI instruction in DFA CFG must be the first instruction.
      assert(&*block->begin() == I);
      for (auto inst_it = block->begin(), inst_e = block->end();
           inst_it != inst_e; ++inst_it) {
        Instruction* cur_inst = &*inst_it;
        if (!isa<PHINode>(cur_inst) || cur_inst == block->getTerminator()) {
          break;
        }

        PHINode* phi = cast<PHINode>(cur_inst);
        std::map<Instruction*, int>::const_iterator inst_mpit = inst_map_.find(cur_inst);

        assert(inst_mpit != inst_map_.end());
        int cur = inst_mpit->second;
        int val_index = 0;

        for (auto blk_it = phi->block_begin(); blk_it != phi->block_end(); ++blk_it) {
          Instruction* inst = cast<Instruction>(phi->getIncomingValue(val_index));
          std::map<Instruction*, int>::const_iterator inst_mpit = inst_map_.find(inst);

          if (inst_mpit != inst_map_.end()) {
            out.move(cur, inst_mpit->second);
          }
          val_index += 1;
        }
      }
    } break;
  }

  // Distribute out to all outgoing edges.
  infos.resize(outs.size());
  for (size_t i = 0; i < outs.size(); ++i) {
    infos[i] = out;
  }
}

namespace {

struct PointerAnalysisPass : public FunctionPass {
  static char ID;
  PointerAnalysisPass() : FunctionPass(ID) { }

  bool runOnFunction(Function& F) override {
    PointerAnalysis analyzer;

    analyzer.RunWorklistAlgorithm(&F);
    analyzer.Print();

    return false;
  }
};

}  /* namespace */

char PointerAnalysisPass::ID = 0;
static RegisterPass<PointerAnalysisPass> X(
    "pointer", "Pointer analysis pass",
    false /* Only looks at CFG */,
    false /* Analysis Pass */);
