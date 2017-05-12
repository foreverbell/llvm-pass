#include "DataflowAnalysis.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

#include <map>
#include <set>

using namespace llvm;

class LivenessInfo : public AnalysisInfo {
 public:
  void add(int var) {
    live_.insert(var);
  }

  void erase(int var) {
    live_.erase(var);
  }

  size_t size() const {
    return live_.size();
  }

  virtual void Print() {
    for (std::set<int>::iterator it = live_.begin(); it != live_.end(); ++it) {
      errs() << *it << '|';
    }
    errs() << '\n';
  }

  static LivenessInfo Bottom() {
    return LivenessInfo();
  }

  static bool Equals(const LivenessInfo* info1, const LivenessInfo* info2) {
    return info1->live_ == info2->live_;
  }

  static std::unique_ptr<LivenessInfo> Join(LivenessInfo* info1,
      LivenessInfo* info2) {
    std::unique_ptr<LivenessInfo> ret(new LivenessInfo(*info1));

    for (int var : info2->live_) {
      ret->add(var);
    }
    return ret;
  }

 private:
  std::set<int> live_;
};

class LivenessAnalysis
    : public DataFlowAnalysis<LivenessInfo, false /* Direction */> {

 public:
  LivenessAnalysis()
    : DataFlowAnalysis<LivenessInfo, false>(
        LivenessInfo::Bottom(), LivenessInfo::Bottom()) { }

 private:
  virtual void FlowFunction(
      Instruction* I,
      int inst_index,
      const LivenessInfo& in,
      const std::vector<Edge>& outs,
      std::vector<LivenessInfo>& infos) const override;
};

void LivenessAnalysis::FlowFunction(
      Instruction* I,
      int inst_index,
      const LivenessInfo& in,
      const std::vector<Edge>& outs,
      std::vector<LivenessInfo>& infos) const {

  // PHI instruction needs to be handled specially.
  if (I->getOpcode() == Instruction::PHI) {
    infos.resize(outs.size());
    for (size_t i = 0; i < outs.size(); ++i) {
      infos[i] = in;
    }

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
      int index = inst_mpit->second;

      for (size_t i = 0; i < outs.size(); ++i) {
        infos[i].erase(index);

        BasicBlock* outgoing_blk = insts_[outs[i].first]->getParent();
        int val_index = 0;

        for (auto blk_it = phi->block_begin(); blk_it != phi->block_end(); ++blk_it) {
          if (*blk_it == outgoing_blk) {
            Instruction* inst = cast<Instruction>(phi->getIncomingValue(val_index));
            std::map<Instruction*, int>::const_iterator inst_mpit = inst_map_.find(inst);

            if (inst_mpit != inst_map_.end()) {
              infos[i].add(inst_mpit->second);
            }
            break;
          }
          val_index += 1;
        }
      }
    }
  } else {
    LivenessInfo out = in;
    bool defined_nvar = false;

    switch (I->getOpcode()) {
      // Binary operations.
      case Instruction::Add:
      case Instruction::FAdd:
      case Instruction::Sub:
      case Instruction::FSub:
      case Instruction::Mul:
      case Instruction::FMul:
      case Instruction::UDiv:
      case Instruction::SDiv:
      case Instruction::FDiv:
      case Instruction::URem:
      case Instruction::SRem:
      case Instruction::FRem:
      // Binary bitwise operations.
      case Instruction::Shl:
      case Instruction::LShr:
      case Instruction::AShr:
      case Instruction::And:
      case Instruction::Or:
      case Instruction::Xor:
      // alloca.
      case Instruction::Alloca:
      // load.
      case Instruction::Load:
      // getelementptr.
      case Instruction::GetElementPtr:
      // i(f)cmp.
      case Instruction::ICmp:
      case Instruction::FCmp:
      // select.
      case Instruction::Select: {
        defined_nvar = true;
      } break;
    }

    if (defined_nvar) {
      out.erase(inst_index);
    }

    for (User::op_iterator op_it = I->op_begin(), op_e = I->op_end();
         op_it != op_e; ++op_it) {
      Value* operand = op_it->get();

      // evil hack, may not fall into type Instruction.
      Instruction* inst = cast<Instruction>(operand);
      std::map<Instruction*, int>::const_iterator inst_mpit = inst_map_.find(inst);

      if (inst_mpit != inst_map_.end()) {
        out.add(inst_mpit->second);
      }
    }

    // Distribute out to all outgoing edges.
    infos.resize(outs.size());
    for (size_t i = 0; i < outs.size(); ++i) {
      infos[i] = out;
    }
  }
}

namespace {

struct LivenessAnalysisPass : public FunctionPass {
  static char ID;
  LivenessAnalysisPass() : FunctionPass(ID) { }

  bool runOnFunction(Function& F) override {
    LivenessAnalysis analyzer;

    analyzer.RunWorklistAlgorithm(&F);
    analyzer.Print();

    return false;
  }
};

}  /* namespace */

char LivenessAnalysisPass::ID = 0;
static RegisterPass<LivenessAnalysisPass> X(
    "cse231-liveness", "Liveness analysis pass",
    false /* Only looks at CFG */,
    false /* Analysis Pass */);
