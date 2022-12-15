# compiler_design

To run: 
- make
- ./mccomp filepath/filename.c
  
  
problems:
- no code generation 
- while loop breaks - segfault
- if else doesnt go to the else
- recursion doesnt work - doesnt think the function is not been declared - it is in the function prototypes but not in the module


limitations:
- grammar allows for no return type but llvm needs one 
- global variable type checking doesnt work for llvm 

todo:
- check the return type matches in the assignment node ie no c = void function etc - done for local declarations and arguments
- check the declared return type and the actual return type matches - done 

- type conversion stuff - int to float, float to int - do i do this in the function declarations only??
- lazy evaluation  - done 

- add level to the while loop - done 
- change the return types to nullptr - going to leave it in case i break something 
ie check return types match in the assignastnode and in the function call 


fix up code:
- get rid of stupid comments and print statements and redundant code - done
- fix ast printing 
- better error statements - try and get the line number - done

Design Choices
- prototype and body = function 
- prototype does not extend astnode

Error Messages
Syntax messages
- Expected extern declaration or function declaration or variable declaration
- Expected EOF
- Expected type specifier - 'int', 'float', 'bool', or 'void'
- Expected 'extern' keyword
- Expected function name
- Expected (
- Expected function or variable declaration 
- Expected ; or ( for variable and function declaration respectively
- Expected variable type - 'int', 'float', or 'bool'
- Incorrect parameter declaration - expected parameter type, 'void' or ')'
- Expected variable name, numerical expression, 'if', 'while', 'else', 'return', or }
- Expected 'while' keyword
- Expected expression statement or ;
- Expected 'else' statement or another statement
- Expected return statement
- Unknown token when expecting an expression

Semantic messages
- Unknown variable name called
- Variable already declared in the local scope
- Unknown type
- Invalid unary operator
- Invalid binary operator
- Type of the left and right side of the binary expression does not match
- Unknown function referenced
- Incorrect number of arguments passed
- WARNING: Implicit assignment of function argument from int to float
- Incorrect function argument type
- If statement condition must be a 'bool'
- Unknown function return type
- Return type does not match the function definition
- Return statement outside of a function


EXTRAS 
- global variabl type checking
- error messages with the names and other places that things are referenced
 