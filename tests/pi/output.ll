; ModuleID = 'mini-c'
source_filename = "mini-c"

define float @pi() {
entry:
  %i = alloca i32, align 4
  %PI = alloca float, align 4
  %flag = alloca i1, align 1
  store i1 true, ptr %flag, align 1
  store float 3.000000e+00, ptr %PI, align 4
  store i32 2, ptr %i, align 4
  br label %cond

cond:                                             ; preds = %cont, %entry
  %i1 = load i32, ptr %i, align 4
  %cmptmp = icmp slt i32 %i1, 100
  %ifcond = icmp ne i1 %cmptmp, false
  br i1 %ifcond, label %loop, label %afterloop

loop:                                             ; preds = %cond
  %flag2 = load i1, ptr %flag, align 1
  %ifcond3 = icmp ne i1 %flag2, false
  br i1 %ifcond3, label %ifthen, label %elsethen

ifthen:                                           ; preds = %loop
  %PI4 = load float, ptr %PI, align 4
  %i5 = load i32, ptr %i, align 4
  %i6 = load i32, ptr %i, align 4
  %addtmp = add i32 %i6, 1
  %multmp = mul i32 %i5, %addtmp
  %i7 = load i32, ptr %i, align 4
  %addtmp8 = add i32 %i7, 2
  %multmp9 = mul i32 %multmp, %addtmp8
  %casttmp = sitofp i32 %multmp9 to float
  %divtmp = fdiv float 4.000000e+00, %casttmp
  %addtmp10 = fadd float %PI4, %divtmp
  store float %addtmp10, ptr %PI, align 4
  br label %cont

elsethen:                                         ; preds = %loop
  %PI11 = load float, ptr %PI, align 4
  %i12 = load i32, ptr %i, align 4
  %i13 = load i32, ptr %i, align 4
  %addtmp14 = add i32 %i13, 1
  %multmp15 = mul i32 %i12, %addtmp14
  %i16 = load i32, ptr %i, align 4
  %addtmp17 = add i32 %i16, 2
  %multmp18 = mul i32 %multmp15, %addtmp17
  %casttmp19 = sitofp i32 %multmp18 to float
  %divtmp20 = fdiv float 4.000000e+00, %casttmp19
  %subtmp = fsub float %PI11, %divtmp20
  store float %subtmp, ptr %PI, align 4
  br label %cont

cont:                                             ; preds = %elsethen, %ifthen
  %flag21 = load i1, ptr %flag, align 1
  %nottmp = xor i1 %flag21, true
  store i1 %nottmp, ptr %flag, align 1
  %i22 = load i32, ptr %i, align 4
  %addtmp23 = add i32 %i22, 2
  store i32 %addtmp23, ptr %i, align 4
  br label %cond

afterloop:                                        ; preds = %cond
  %PI24 = load float, ptr %PI, align 4
  ret float %PI24
}
