; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

define i32 @fibonacci(i32 %n) {
entry:
  %total = alloca i32, align 4
  %c = alloca i32, align 4
  %next = alloca i32, align 4
  %second = alloca i32, align 4
  %first = alloca i32, align 4
  %n1 = alloca i32, align 4
  store i32 %n, ptr %n1, align 4
  %n2 = load i32, ptr %n1, align 4
  %calltmp = call i32 @print_int(i32 %n2)
  store i32 0, ptr %first, align 4
  store i32 1, ptr %second, align 4
  store i32 1, ptr %c, align 4
  store i32 0, ptr %total, align 4
  br label %cond

cond:                                             ; preds = %cont, %entry
  %c3 = load i32, ptr %c, align 4
  %n4 = load i32, ptr %n1, align 4
  %cmptmp = icmp slt i32 %c3, %n4
  %ifcond = icmp ne i1 %cmptmp, false
  br i1 %ifcond, label %loop, label %afterloop

loop:                                             ; preds = %cond
  %c5 = load i32, ptr %c, align 4
  %cmptmp6 = icmp sle i32 %c5, 1
  %ifcond7 = icmp ne i1 %cmptmp6, false
  br i1 %ifcond7, label %ifthen, label %elsethen

ifthen:                                           ; preds = %loop
  %c8 = load i32, ptr %c, align 4
  store i32 %c8, ptr %next, align 4
  br label %cont

elsethen:                                         ; preds = %loop
  %first9 = load i32, ptr %first, align 4
  %second10 = load i32, ptr %second, align 4
  %addtmp = add i32 %first9, %second10
  store i32 %addtmp, ptr %next, align 4
  %second11 = load i32, ptr %second, align 4
  store i32 %second11, ptr %first, align 4
  %next12 = load i32, ptr %next, align 4
  store i32 %next12, ptr %second, align 4
  br label %cont

cont:                                             ; preds = %elsethen, %ifthen
  %next13 = load i32, ptr %next, align 4
  %calltmp14 = call i32 @print_int(i32 %next13)
  %c15 = load i32, ptr %c, align 4
  %addtmp16 = add i32 %c15, 1
  store i32 %addtmp16, ptr %c, align 4
  %total17 = load i32, ptr %total, align 4
  %next18 = load i32, ptr %next, align 4
  %addtmp19 = add i32 %total17, %next18
  store i32 %addtmp19, ptr %total, align 4
  br label %cond

afterloop:                                        ; preds = %cond
  %total20 = load i32, ptr %total, align 4
  %calltmp21 = call i32 @print_int(i32 %total20)
  %total22 = load i32, ptr %total, align 4
  ret i32 %total22
}
