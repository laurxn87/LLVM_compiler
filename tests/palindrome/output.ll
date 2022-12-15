; ModuleID = 'mini-c'
source_filename = "mini-c"

define i1 @palindrome(i32 %number) {
entry:
  %result = alloca i1, align 1
  %rmndr = alloca i32, align 4
  %rev = alloca i32, align 4
  %t = alloca i32, align 4
  %number1 = alloca i32, align 4
  store i32 %number, ptr %number1, align 4
  store i32 0, ptr %rev, align 4
  store i1 false, ptr %result, align 1
  %number2 = load i32, ptr %number1, align 4
  store i32 %number2, ptr %t, align 4
  br label %cond

cond:                                             ; preds = %loop, %entry
  %number3 = load i32, ptr %number1, align 4
  %cmptmp = icmp sgt i32 %number3, 0
  %ifcond = icmp ne i1 %cmptmp, false
  br i1 %ifcond, label %loop, label %afterloop

loop:                                             ; preds = %cond
  %number4 = load i32, ptr %number1, align 4
  %remtmp = srem i32 %number4, 10
  store i32 %remtmp, ptr %rmndr, align 4
  %rev5 = load i32, ptr %rev, align 4
  %multmp = mul i32 %rev5, 10
  %rmndr6 = load i32, ptr %rmndr, align 4
  %addtmp = add i32 %multmp, %rmndr6
  store i32 %addtmp, ptr %rev, align 4
  %number7 = load i32, ptr %number1, align 4
  %divtmp = sdiv i32 %number7, 10
  store i32 %divtmp, ptr %number1, align 4
  br label %cond

afterloop:                                        ; preds = %cond
  %t8 = load i32, ptr %t, align 4
  %rev9 = load i32, ptr %rev, align 4
  %cmptmp10 = icmp eq i32 %t8, %rev9
  %ifcond11 = icmp ne i1 %cmptmp10, false
  br i1 %ifcond11, label %ifthen, label %elsethen

ifthen:                                           ; preds = %afterloop
  store i1 true, ptr %result, align 1
  br label %cont

elsethen:                                         ; preds = %afterloop
  store i1 false, ptr %result, align 1
  br label %cont

cont:                                             ; preds = %elsethen, %ifthen
  %result12 = load i1, ptr %result, align 1
  ret i1 %result12
}
