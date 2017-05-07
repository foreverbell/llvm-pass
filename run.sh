#!/bin/bash

# Compile tests to LLVM IR.
clang++ -c -O0 test/test1.cc -emit-llvm -S -o build/test1.ll
clang++ -c test/test1-main.cc -emit-llvm -S -o build/test1-main.ll

# Compile libs to LLVM IR.
clang++ -c lib/lib_cdi.cc -emit-llvm -S -o build/lib_cdi.ll
clang++ -c lib/lib_bb.cc -emit-llvm -S -o build/lib_bb.ll

# Run LLVM IR through our LLVM passes.
opt -load pass/LLVMPass.so -csi < build/test1.ll > /dev/null 2> build/csi.result
opt -load pass/LLVMPass.so -cdi < build/test1.ll -o build/test1-cdi.bc
opt -load pass/LLVMPass.so -bb < build/test1.ll -o build/test1-bb.bc

# Disassmble bitcode to human readable IR.
llvm-dis build/test1-cdi.bc
llvm-dis build/test1-bb.bc

# Link hijacked programs.
clang++ build/test1-cdi.ll build/lib_cdi.ll build/test1-main.ll -o build/cdi_test1
clang++ build/test1-bb.ll build/lib_bb.ll build/test1-main.ll -o build/bb_test1

# Execute hijacked programs.
build/cdi_test1 2> build/cdi.result
build/bb_test1 2> build/bb.result
