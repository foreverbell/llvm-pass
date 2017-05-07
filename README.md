# llvm-pass

Exploring LLVM by writing some trivial LLVM passes.

This code is derived from UCSD CSE231 (Advanced Compilers).

Three simple LLVM passes have implemented.

* CountStaticInst: Counting the number of each IR instructions statically.

* CountDynamicInst: Counting the number of each IR instructions to execute dynamically by
  hijacking (injecting) some code before each BasicBlock. See `lib/lib_cdi.cc` for injected code.

* ProfileBranchBias: Profiling bias for each branch, i.e. how many conditionals are evaluated to true?
  See `lib/lib_bb.cc` for injected code.

## Testing

```bash
$ cmake .
$ make
$ ./run.sh
```

Tested with LLVM 3.9 and 4.0.

## IR Before / After

Demonstrate how `CountDynamicInst` pass works on `tests/test1.cc`.

### Before  (`test1.ll`)

```ll
; ModuleID = 'tests/test1.cc'
source_filename = "tests/test1.cc"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define i32 @_Z3fooj(i32) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store volatile i32 %0, i32* %2, align 4
  store volatile i32 10, i32* %3, align 4
  %4 = load volatile i32, i32* %2, align 4
  %5 = icmp ugt i32 %4, 5
  br i1 %5, label %6, label %9

; <label>:6:                                      ; preds = %1
  %7 = load volatile i32, i32* %3, align 4
  %8 = add i32 %7, 5
  store volatile i32 %8, i32* %3, align 4
  br label %12

; <label>:9:                                      ; preds = %1
  %10 = load volatile i32, i32* %3, align 4
  %11 = add i32 %10, 50
  store volatile i32 %11, i32* %3, align 4
  br label %12

; <label>:12:                                     ; preds = %9, %6
  %13 = load volatile i32, i32* %3, align 4
  %14 = load volatile i32, i32* %2, align 4
  %15 = add i32 %13, %14
  ret i32 %15
}

attributes #0 = { noinline nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = !{!"clang version 4.0.0-1ubuntu1 (tags/RELEASE_400/rc1)"}
```

### After (`test1-cdi.ll`)

```ll
; ModuleID = 'build/test1-cdi.bc'
source_filename = "tests/test1.cc"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@0 = internal constant [5 x i32] [i32 2, i32 29, i32 30, i32 31, i32 51]
@1 = internal constant [5 x i32] [i32 1, i32 2, i32 1, i32 2, i32 1]
@2 = internal constant [4 x i32] [i32 2, i32 11, i32 30, i32 31]
@3 = internal constant [4 x i32] [i32 1, i32 1, i32 1, i32 1]
@4 = internal constant [4 x i32] [i32 2, i32 11, i32 30, i32 31]
@5 = internal constant [4 x i32] [i32 1, i32 1, i32 1, i32 1]
@6 = internal constant [3 x i32] [i32 1, i32 11, i32 30]
@7 = internal constant [3 x i32] [i32 1, i32 1, i32 2]

; Function Attrs: noinline nounwind uwtable
define i32 @_Z3fooj(i32) #0 {
  call void @updateInstrInfo(i32 5, i32* getelementptr inbounds ([5 x i32], [5 x i32]* @0, i32 0, i32 0), i32* getelementptr inbounds ([5 x i32], [5 x i32]* @1, i32 0, i32 0))
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store volatile i32 %0, i32* %2, align 4
  store volatile i32 10, i32* %3, align 4
  %4 = load volatile i32, i32* %2, align 4
  %5 = icmp ugt i32 %4, 5
  br i1 %5, label %6, label %9

; <label>:6:                                      ; preds = %1
  call void @updateInstrInfo(i32 4, i32* getelementptr inbounds ([4 x i32], [4 x i32]* @2, i32 0, i32 0), i32* getelementptr inbounds ([4 x i32], [4 x i32]* @3, i32 0, i32 0))
  %7 = load volatile i32, i32* %3, align 4
  %8 = add i32 %7, 5
  store volatile i32 %8, i32* %3, align 4
  br label %12

; <label>:9:                                      ; preds = %1
  call void @updateInstrInfo(i32 4, i32* getelementptr inbounds ([4 x i32], [4 x i32]* @4, i32 0, i32 0), i32* getelementptr inbounds ([4 x i32], [4 x i32]* @5, i32 0, i32 0))
  %10 = load volatile i32, i32* %3, align 4
  %11 = add i32 %10, 50
  store volatile i32 %11, i32* %3, align 4
  br label %12

; <label>:12:                                     ; preds = %9, %6
  call void @updateInstrInfo(i32 3, i32* getelementptr inbounds ([3 x i32], [3 x i32]* @6, i32 0, i32 0), i32* getelementptr inbounds ([3 x i32], [3 x i32]* @7, i32 0, i32 0))
  call void @printOutInstrInfo()
  %13 = load volatile i32, i32* %3, align 4
  %14 = load volatile i32, i32* %2, align 4
  %15 = add i32 %13, %14
  ret i32 %15
}

declare void @updateInstrInfo(i32, i32*, i32*)

declare void @printOutInstrInfo()

attributes #0 = { noinline nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = !{!"clang version 4.0.0-1ubuntu1 (tags/RELEASE_400/rc1)"}
```

### Diff

```diff
@@ -1,10 +1,20 @@
 target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
 target triple = "x86_64-pc-linux-gnu"
 
+@0 = internal constant [5 x i32] [i32 2, i32 29, i32 30, i32 31, i32 51]
+@1 = internal constant [5 x i32] [i32 1, i32 2, i32 1, i32 2, i32 1]
+@2 = internal constant [4 x i32] [i32 2, i32 11, i32 30, i32 31]
+@3 = internal constant [4 x i32] [i32 1, i32 1, i32 1, i32 1]
+@4 = internal constant [4 x i32] [i32 2, i32 11, i32 30, i32 31]
+@5 = internal constant [4 x i32] [i32 1, i32 1, i32 1, i32 1]
+@6 = internal constant [3 x i32] [i32 1, i32 11, i32 30]
+@7 = internal constant [3 x i32] [i32 1, i32 1, i32 2]
+
 ; Function Attrs: noinline nounwind uwtable
 define i32 @_Z3fooj(i32) #0 {
+  call void @updateInstrInfo(i32 5, i32* getelementptr inbounds ([5 x i32], [5 x i32]* @0, i32 0, i32 0), i32* getelementptr inbounds ([5 x i32], [5 x i32]* @1, i32 0, i32 0))
   %2 = alloca i32, align 4
   %3 = alloca i32, align 4
   store volatile i32 %0, i32* %2, align 4
@@ -14,24 +24,32 @@
   br i1 %5, label %6, label %9
 
 ; <label>:6:                                      ; preds = %1
+  call void @updateInstrInfo(i32 4, i32* getelementptr inbounds ([4 x i32], [4 x i32]* @2, i32 0, i32 0), i32* getelementptr inbounds ([4 x i32], [4 x i32]* @3, i32 0, i32 0))
   %7 = load volatile i32, i32* %3, align 4
   %8 = add i32 %7, 5
   store volatile i32 %8, i32* %3, align 4
   br label %12
 
 ; <label>:9:                                      ; preds = %1
+  call void @updateInstrInfo(i32 4, i32* getelementptr inbounds ([4 x i32], [4 x i32]* @4, i32 0, i32 0), i32* getelementptr inbounds ([4 x i32], [4 x i32]* @5, i32 0, i32 0))
   %10 = load volatile i32, i32* %3, align 4
   %11 = add i32 %10, 50
   store volatile i32 %11, i32* %3, align 4
   br label %12
 
 ; <label>:12:                                     ; preds = %9, %6
+  call void @updateInstrInfo(i32 3, i32* getelementptr inbounds ([3 x i32], [3 x i32]* @6, i32 0, i32 0), i32* getelementptr inbounds ([3 x i32], [3 x i32]* @7, i32 0, i32 0))
+  call void @printOutInstrInfo()
   %13 = load volatile i32, i32* %3, align 4
   %14 = load volatile i32, i32* %2, align 4
   %15 = add i32 %13, %14
   ret i32 %15
 }
 
+declare void @updateInstrInfo(i32, i32*, i32*)
+
+declare void @printOutInstrInfo()
+
 attributes #0 = { noinline nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
 
 !llvm.ident = !{!0}
```
