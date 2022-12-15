; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

define i32 @addNumbers(i32 %n) {
entry:
  %result = alloca i32, align 4
  %n1 = alloca i32, align 4
  store i32 %n, ptr %n1, align 4
  store i32 0, ptr %result, align 4
  %n2 = load i32, ptr %n1, align 4
  %cmptmp = icmp ne i32 %n2, 0
  %ifcond = icmp ne i1 %cmptmp, false
  br i1 %ifcond, label %ifthen, label %elsethen

ifthen:                                           ; preds = %entry
  %n3 = load i32, ptr %n1, align 4
  %n4 = load i32, ptr %n1, align 4
  %subtmp = sub i32 %n4, 1
  %calltmp = call i32 @addNumbers(i32 %subtmp)
  %addtmp = add i32 %n3, %calltmp
  store i32 %addtmp, ptr %result, align 4
  br label %cont

elsethen:                                         ; preds = %entry
  %n5 = load i32, ptr %n1, align 4
  store i32 %n5, ptr %result, align 4
  br label %cont

cont:                                             ; preds = %elsethen, %ifthen
  %result6 = load i32, ptr %result, align 4
  %calltmp7 = call i32 @print_int(i32 %result6)
  %result8 = load i32, ptr %result, align 4
  ret i32 %result8
}

define i32 @recursion_driver(i32 %num) {
entry:
  %num1 = alloca i32, align 4
  store i32 %num, ptr %num1, align 4
  %num2 = load i32, ptr %num1, align 4
  %calltmp = call i32 @addNumbers(i32 %num2)
  ret i32 %calltmp
}
