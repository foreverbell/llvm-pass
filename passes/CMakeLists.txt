add_llvm_loadable_module( LLVMPass
  CountStaticInstructions.cc
  CountDynamicInstructions.cc
  BranchBias.cc

  PLUGIN_TOOL
  opt
  )
