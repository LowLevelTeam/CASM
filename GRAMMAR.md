# CASM Language Specification

CASM (COIL Assembly) is a simple assembly language designed to compile directly to the COIL binary format. This document describes the syntax and semantics of the language.

## Syntax Overview

CASM is a line-oriented assembly language with a simple, consistent syntax:

```
[label:] [instruction|directive] [operands...] [; comment]
```

Each line can contain:
- An optional label (ending with a colon)
- An instruction or directive
- Zero or more operands (depending on the instruction)
- An optional comment (starting with a semicolon)

## Instructions

Instructions correspond directly to COIL opcodes, with a simple syntax for operands.

### Control Flow

```
nop                     ; No operation
jump label              ; Unconditional jump to label
jump r1                 ; Unconditional jump to address in register
br.eq label             ; Branch to label if equal (after comparison)
br.neq label            ; Branch if not equal
br.gt label             ; Branch if greater than
br.gte label            ; Branch if greater than or equal
br.lt label             ; Branch if less than
br.lte label            ; Branch if less than or equal
call function           ; Call function
ret                     ; Return from function
```

### Memory Operations

```
load r1, [r2]           ; Load from memory at address in r2 into r1
load r1, [r2+8]         ; Load from memory at address (r2+8) into r1
store [r1], r2          ; Store r2 into memory at address in r1
store [r1+8], r2        ; Store r2 into memory at address (r1+8)
push r1                 ; Push r1 onto stack
pop r1                  ; Pop from stack into r1
```

### Arithmetic

```
add r1, r2, r3          ; r1 = r2 + r3
add r1, r2, 42          ; r1 = r2 + 42
sub r1, r2, r3          ; r1 = r2 - r3
mul r1, r2, r3          ; r1 = r2 * r3
div r1, r2, r3          ; r1 = r2 / r3
rem r1, r2, r3          ; r1 = r2 % r3
inc r1                  ; Increment r1
dec r1                  ; Decrement r1
```

### Bitwise

```
and r1, r2, r3          ; r1 = r2 & r3
or r1, r2, r3           ; r1 = r2 | r3
xor r1, r2, r3          ; r1 = r2 ^ r3
not r1, r2              ; r1 = ~r2
shl r1, r2, r3          ; r1 = r2 << r3
shr r1, r2, r3          ; r1 = r2 >> r3 (logical)
sar r1, r2, r3          ; r1 = r2 >> r3 (arithmetic)
```

### Comparison

```
cmp r1, r2              ; Compare r1 and r2
test r1, 42             ; Test r1 & 42
```

## Directives

Directives control the assembly process and define data.

### Section Directives

```
.section .text          ; Start text section
.section .data          ; Start data section
.section .bss           ; Start BSS section (uninitialized data)
```

### Symbol Directives

```
.global symbol          ; Export symbol globally
.local symbol           ; Keep symbol local to file
```

### Data Directives

```
.align 4                ; Align to 4-byte boundary
.i8 1, 2, 3             ; Define 8-bit signed integers
.i16 1, 2, 3            ; Define 16-bit signed integers
.i32 1, 2, 3            ; Define 32-bit signed integers
.i64 1, 2, 3            ; Define 64-bit signed integers
.u8 1, 2, 3             ; Define 8-bit unsigned integers
.u16 1, 2, 3            ; Define 16-bit unsigned integers
.u32 1, 2, 3            ; Define 32-bit unsigned integers
.u64 1, 2, 3            ; Define 64-bit unsigned integers
.f32 1.0, 2.0, 3.0      ; Define 32-bit floating point numbers
.f64 1.0, 2.0, 3.0      ; Define 64-bit floating point numbers
.string "Hello, world!" ; Define a string (null-terminated)
.bytes 0x01, 0x02, 0x03 ; Define raw bytes
.space 10               ; Reserve 10 bytes of zeros
```

## Registers

Registers are referenced with an r prefix followed by a number, and optionally a type suffix:

```
r0                      ; Register 0 (default type from context)
r1.i32                  ; Register 1 (32-bit signed integer)
r2.i64                  ; Register 2 (64-bit signed integer)
r3.f32                  ; Register 3 (32-bit float)
r4.f64                  ; Register 4 (64-bit float)
```

Valid register types:
- `.i8`: 8-bit signed integer
- `.i16`: 16-bit signed integer
- `.i32`: 32-bit signed integer
- `.i64`: 64-bit signed integer
- `.u8`: 8-bit unsigned integer
- `.u16`: 16-bit unsigned integer
- `.u32`: 32-bit unsigned integer
- `.u64`: 64-bit unsigned integer
- `.f32`: 32-bit float
- `.f64`: 64-bit float
- `.ptr`: Pointer (platform-dependent size)

## Immediate Values

Immediate values (constants) can be specified in various formats:

```
42                      ; Decimal integer
0xff                    ; Hexadecimal integer
0b1010                  ; Binary integer
'A'                     ; Character (ASCII value)
3.14159                 ; Floating point
```

## Memory References

Memory references use square brackets and can include a base register and offset:

```
[r1]                    ; Memory at address in r1
[r1+8]                  ; Memory at address (r1+8)
[r1-4]                  ; Memory at address (r1-4)
```

## Labels

Labels define locations in the code that can be referenced by instructions:

```
loop:                   ; Define a label
  inc r1
  cmp r1, 10
  br.lt loop            ; Jump to label if r1 < 10
```

## Comments

Comments start with a semicolon and continue to the end of the line:

```
; This is a comment
add r1, r2, r3          ; This is an end-of-line comment
```

## Example Program

```
; Calculate factorial of 5

.section .text
.global main

main:
  ; Initialize
  load r1, [factorial_input]  ; Load input value
  push r1
  call factorial
  pop r2                      ; Get result
  ret

factorial:
  ; r1 = input value
  ; returns factorial in r1
  push r2
  push r3
  
  mov r2, r1                  ; r2 = n
  mov r3, 1                   ; r3 = result = 1
  
loop:
  cmp r2, 0
  br.eq done                  ; if n == 0, we're done
  
  mul r3, r3, r2              ; result *= n
  dec r2                      ; n--
  jump loop
  
done:
  mov r1, r3                  ; return result in r1
  pop r3
  pop r2
  ret

.section .data
factorial_input: .i32 5
```