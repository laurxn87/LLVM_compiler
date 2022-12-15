; ModuleID = 'mini-c'
source_filename = "mini-c"

@test = common global i32 0, align 4
@f = common global float 0.000000e+00, align 4
@b = common global i1 false, align 4

declare i32 @print_int(i32)

define i32 @While(i32 %n) {
entry:
  %result = alloca i32, align 4
  %n1 = alloca i32, align 4
  store i32 %n, ptr %n1, align 4
  store i32 12, ptr @test, align 4
  store i32 0, ptr %result, align 4
  %test = load i32, ptr @test, align 4
  %calltmp = call i32 @print_int(i32 %test)
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
  br label %cond

afterloop:                                        ; preds = %cond
  %result4 = load i32, ptr %result, align 4
  ret i32 %result4
}
