; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

define void @Void() {
entry:
  %result = alloca i32, align 4
  store i32 0, ptr %result, align 4
  %result1 = load i32, ptr %result, align 4
  %calltmp = call i32 @print_int(i32 %result1)
  br label %cond

cond:                                             ; preds = %loop, %entry
  %result2 = load i32, ptr %result, align 4
  %cmptmp = icmp slt i32 %result2, 10
  %ifcond = icmp ne i1 %cmptmp, false
  br i1 %ifcond, label %loop, label %afterloop

loop:                                             ; preds = %cond
  %result3 = load i32, ptr %result, align 4
  %addtmp = add i32 %result3, 1
  store i32 %addtmp, ptr %result, align 4
  %result4 = load i32, ptr %result, align 4
  %calltmp5 = call i32 @print_int(i32 %result4)
  br label %cond

afterloop:                                        ; preds = %cond
  ret void
}
