# CASM Grammar Specification

CASM (COIL Assembly) is a simple assembly language designed to compile directly to the COIL binary format. This document defines the formal grammar for CASM.

## Lexical Elements

### Comments
Comments start with a semicolon (`;`) and continue to the end of the line.
```
; This is a comment
```

### Labels
Labels define locations in code and are prefixed with a hash (`#`).
```
#loop_start
#main
```

### Instructions
Instructions correspond to COIL opcodes and are written in lowercase.
```
add %r1, %r2, %r3
mov %r1, $id42
```

### Directives
Directives control the assembly process and are prefixed with a period (`.`).
```
.section .text
.i32 1, 2, 3
```

### Registers
Registers are prefixed with a percent sign (`%`) followed by 'r' and a number.
```
%r0, %r1, %r2
```

### Immediates
Immediate values (constants) use a dollar sign (`$`) prefix and a format specifier:
```
$id42      ; Decimal integer (id = integer decimal)
$ix2A      ; Hexadecimal integer (ix = integer hex)
$ib101     ; Binary integer (ib = integer binary)
$fd3.14    ; Decimal float (fd = float decimal)
$'A'       ; Character literal
$"Hello"   ; String literal
```

### Memory References
Memory references use square brackets and can include a base register and offset.
```
[%r1]      ; Memory at address in r1
[%r1+8]    ; Memory at address (r1+8)
[%r1-4]    ; Memory at address (r1-4)
```

### Label References
References to labels use an at sign (`@`).
```
@loop      ; Reference to label 'loop'
```

### Parameters
Some instructions take parameters prefixed with a caret (`^`).
```
br ^eq @label  ; Branch if equal
```

## Syntax

### General Line Format
```
[#label] [instruction|.directive] [[^parameter|@label|%register|$immediate]...] [; comment]
```

### Sections
Sections are defined using the `.section` directive.
```
.section .text           ; Code section
.section .data           ; Data section
.section .bss            ; Uninitialized data section

; Advanced section syntax with flags
.section .text ^ProgBits ^Code ^Alloc    ; A code section
.section .data ^ProgBits ^Write ^Alloc   ; A data section
```

### Symbol Directives
```
.global @main    ; Make symbol globally visible
.local @helper   ; Make symbol local to file
```

### Data Definitions
```
.i8 1, 2, 3             ; Define 8-bit signed integers
.i16 1000, 2000         ; Define 16-bit signed integers
.i32 100000, 200000     ; Define 32-bit signed integers
.i64 1000000000000      ; Define 64-bit signed integers

.u8 255, 128            ; Define 8-bit unsigned integers
.u16 65000              ; Define 16-bit unsigned integers
.u32 4000000000         ; Define 32-bit unsigned integers
.u64 18000000000000000  ; Define 64-bit unsigned integers

.f32 3.14, 2.71         ; Define 32-bit floating point numbers
.f64 3.141592653589     ; Define 64-bit floating point numbers

.ascii "Hello, World!"  ; Define an ASCII string (no null terminator)
.asciiz "Hello"         ; Define a null-terminated ASCII string

.zero 10                ; Reserve 10 bytes of zeros
```

### Control Flow Instructions
```
nop                     ; No operation
jmp @label              ; Unconditional jump to label
jmp %r1                 ; Unconditional jump to address in register
jmp $ix7C00             ; Unconditional Jump to Immediate

br ^eq @label           ; Branch to label if equal (after comparison)
br ^neq @label          ; Branch if not equal
br ^gt @label           ; Branch if greater than
br ^gte @label          ; Branch if greater than or equal
br ^lt @label          ; Branch if less than
br ^lte @label         ; Branch if less than or equal

call @function          ; Call function
ret                     ; Return from function
```

### Memory Operations
```
load %r1, [%r2]         ; Load from memory at address in r2 into r1
load %r1, [%r2+8]       ; Load from memory at address (r2+8) into r1
store [%r1], %r2        ; Store r2 into memory at address in r1
store [%r1+8], %r2      ; Store r2 into memory at address (r1+8)
push %r1                ; Push r1 onto stack
pop %r1                 ; Pop from stack into r1
mov %r1, %r2            ; Copy r2 to r1
```

### Arithmetic Instructions
```
add %r1, %r2, %r3       ; r1 = r2 + r3
add %r1, %r2, $id42     ; r1 = r2 + 42
sub %r1, %r2, %r3       ; r1 = r2 - r3
mul %r1, %r2, %r3       ; r1 = r2 * r3
div %r1, %r2, %r3       ; r1 = r2 / r3
rem %r1, %r2, %r3       ; r1 = r2 % r3
inc %r1                 ; Increment r1
dec %r1                 ; Decrement r1
neg %r1, %r2            ; r1 = -r2
```

### Bitwise Instructions
```
and %r1, %r2, %r3       ; r1 = r2 & r3
or  %r1, %r2, %r3       ; r1 = r2 | r3
xor %r1, %r2, %r3       ; r1 = r2 ^ r3
not %r1, %r2            ; r1 = ~r2
shl %r1, %r2, %r3       ; r1 = r2 << r3
shr %r1, %r2, %r3       ; r1 = r2 >> r3 (logical)
sar %r1, %r2, %r3       ; r1 = r2 >> r3 (arithmetic)
```

### Comparison Instructions
```
cmp %r1, %r2            ; Compare r1 and r2
test %r1, $id42         ; Test r1 & 42
```

## Example Programs

### Factorial Example
```
; Calculate factorial of n
.section .text
.global @factorial

#factorial
  ; r1 = input value
  ; returns factorial in r1
  cmp %r1, $id0
  br ^eq @base_case
  
  ; Factorial(n) = n * Factorial(n-1)
  push %r1              ; Save n
  dec %r1               ; n-1
  call @factorial       ; Compute Factorial(n-1)
  mov %r2, %r1          ; r2 = Factorial(n-1)
  pop %r1               ; Restore n
  mul %r1, %r1, %r2     ; r1 = n * Factorial(n-1)
  ret
  
#base_case
  mov %r1, $id1         ; Factorial(0) = 1
  ret
```

### Hello World Example
```
; Hello world program
.section .text
.global @main

#main
  ; Load address of hello string
  mov %r1, @hello
  
  ; Call print function
  call @print
  
  ; Exit
  ret

#print
  ; Print function implementation
  ; ... implementation specific to target platform ...
  ret

.section .data
#hello
  .asciiz "Hello, world!"  ; Null-terminated string
```