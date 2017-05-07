add_llvm_loadable_module( LLVMPass
  CountStaticInst.cc
  CountDynamicInst.cc
  ProfileBranchBias.cc

  PLUGIN_TOOL
  opt
  )
