add_llvm_loadable_module( LLVMPass
  CountStaticInst.cc
  CountDynamicInst.cc
  ProfileBranchBias.cc
  ReachingDefinitionAnalysis.cc
  LivenessAnalysis.cc

  PLUGIN_TOOL
  opt
  )
