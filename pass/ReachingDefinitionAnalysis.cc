#include "DataflowAnalysis.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

#include <set>

using namespace llvm;

class ReachingInfo : public AnalysisInfo {
 public:
  void insert(int var) {
    reachable_.insert(var);
  }

  size_t size() const {
    return reachable_.size();
  }

  virtual void Print() {
    for (std::set<int>::iterator it = reachable_.begin(); it != reachable_.end(); ++it) {
      errs() << *it << '|';
    }
    errs() << '\n';
  }

  static ReachingInfo Bottom() {
    return ReachingInfo();
  }

  static bool Equals(const ReachingInfo* info1, const ReachingInfo* info2) {
    return info1->reachable_ == info2->reachable_;
  }

  static std::unique_ptr<ReachingInfo> Join(const ReachingInfo* info1,
      const ReachingInfo* info2) {
    std::unique_ptr<ReachingInfo> ret(new ReachingInfo(*info1));

    for (int var : info2->reachable_) {
      ret->insert(var);
    }
    return ret;
  }

 private:
  std::set<int> reachable_;
};

class ReachingDefinitionAnalysis
   : public DataFlowAnalysis<ReachingInfo, true /* Direction */> {

 public:
  ReachingDefinitionAnalysis()
    : DataFlowAnalysis<ReachingInfo, true>(
        ReachingInfo::Bottom(), ReachingInfo::Bottom()) { }

 private:
  void FlowFunction(
      Instruction* I,
      int inst_index,
      const ReachingInfo& in,
      const std::vector<Edge>& outs,
      std::vector<ReachingInfo>& infos) const override;
};

void ReachingDefinitionAnalysis::FlowFunction(
    Instruction* I,
    int inst_index,
    const ReachingInfo& in,
    const std::vector<Edge>& outs,
    std::vector<ReachingInfo>& infos) const {

  ReachingInfo out = in;

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
      out.insert(inst_index);
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
        
        std::map<Instruction*, int>::const_iterator inst_mpit = inst_map_.find(cur_inst);
        assert(inst_mpit != inst_map_.end());

        out.insert(inst_mpit->second);
      }
    } break;
  }

  // Distribute out to all outgoing edges.
  infos.resize(outs.size());
  for (size_t i = 0; i < infos.size(); ++i) {
    infos[i] = out;
  }
}

namespace {

struct ReachingDefinitionAnalysisPass : public FunctionPass {
  static char ID;
  ReachingDefinitionAnalysisPass() : FunctionPass(ID) { }

  bool runOnFunction(Function& F) override {
    ReachingDefinitionAnalysis analyzer;

    analyzer.RunWorklistAlgorithm(&F);
    analyzer.Print();

    return false;
  }
};

}  /* namespace */

char ReachingDefinitionAnalysisPass::ID = 0;
static RegisterPass<ReachingDefinitionAnalysisPass> X(
    "reaching", "Reaching definition analysis pass",
    false /* Only looks at CFG */,
    false /* Analysis Pass */);
