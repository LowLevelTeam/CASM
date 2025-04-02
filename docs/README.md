# CASM Assembler Documentation

## Overview

The CASM Assembler is a high-performance assembler for CASM (COIL Assembly) that generates COIL binary format. This documentation provides detailed information on using and extending the assembler.

## Table of Contents

1. [Installation](#installation)
2. [Usage](#usage)
3. [CASM Syntax](#casm-syntax)
4. [Directives](#directives)
5. [Instructions](#instructions)
6. [Types](#types)
7. [Variables](#variables)
8. [Registers](#registers)
9. [Memory Operands](#memory-operands)
10. [Sections and Segments](#sections-and-segments)
11. [Symbols and Labels](#symbols-and-labels)
12. [Error Reporting](#error-reporting)
13. [Advanced Features](#advanced-features)
14. [Examples](#examples)
15. [API Documentation](#api-documentation)
16. [Architecture](#architecture)

## Installation

### Prerequisites

- C++17 compliant compiler
- Meson build system
- Ninja build system (recommended)
- libcoil-dev installed

### Building from Source

```bash
git clone https://github.com/yourusername/casm.git
cd casm
meson setup builddir
cd builddir
ninja
sudo ninja install  # Optional, requires sudo
```

## Usage

### Basic Usage

```bash
casm [options] input.casm -o output.coil
```

### Options

- `-I <path>`: Include directory for header files
- `-o <file>`: Specify output file
- `-v`: Enable verbose output
- `-h`: Display help message
- `-d`: Enable debug information

### Examples

```bash
# Simple assembly
casm source.casm -o output.coil

# With include paths
casm -I./include source.casm -o output.coil

# With debug information
casm -d source.casm -o output.coil

# Verbose output
casm -v source.casm -o output.coil
```

## CASM Syntax

CASM uses a syntax similar to traditional assembly languages:

```
[label:] [instruction|directive] [operands...] [; comment]
```

### Comments

Comments start with a semicolon (`;`) and continue to the end of the line:

```
; This is a full-line comment
MOV #1, 42  ; This is an inline comment
```

### Labels

Labels are identifiers followed by a colon (`:`):

```
loop_start:        ; Define a label
    ; Code
    BR loop_start  ; Reference the label
```

## Directives

Directives control the assembly process:

### Section Definition

```
SECTION .text, 0x01 | 0x04      ; Executable and readable
```

### Processor, Architecture, and Mode

```
PROC 0x01                       ; CPU processor
ARCH 0x01, 0x03                 ; x86-64 architecture
```

### Data Definition

```
DATA TYPE_INT32, 42            ; 32-bit integer value
DATA TYPE_ARRAY=TYPE_UNT8, "Hello, COIL!", 10, 0  ; String
```

### Symbol Definition

```
SYM main, TYPE_PARAM0=GLOB     ; Global symbol
```

### Symbol Export/Import

```
GLOBAL symbol_name             ; Export symbol
EXTERN external_symbol         ; Import symbol
```

### Alignment

```
ALIGN 16                       ; Align to 16-byte boundary
```

## Instructions

CASM supports all COIL instructions:

### Data Movement

```
MOV #1, 42                     ; Store 42 in variable #1
MOV #2, #1                     ; Copy variable #1 to #2
MOV #3, [address]              ; Load from memory
MOV [address], #4              ; Store to memory
```

### Arithmetic

```
ADD #3, #1, #2                 ; Add: #3 = #1 + #2
SUB #3, #1, #2                 ; Subtract: #3 = #1 - #2
MUL #3, #1, #2                 ; Multiply: #3 = #1 * #2
DIV #3, #1, #2                 ; Divide: #3 = #1 / #2
INC #1                         ; Increment: #1 = #1 + 1
DEC #1                         ; Decrement: #1 = #1 - 1
```

### Control Flow

```
CMP #1, #2                     ; Compare #1 and #2
BR label                       ; Unconditional branch
BR_EQ label                    ; Branch if equal
BR_NE label                    ; Branch if not equal
BR_LT label                    ; Branch if less than
BR_GT label                    ; Branch if greater than
BR_LE label                    ; Branch if less than or equal
BR_GE label                    ; Branch if greater than or equal
```

### Function Calls

```
CALL function                  ; Call function
CALL function, param1, param2  ; Call with parameters
RET                           ; Return from function
RET value                     ; Return with value
```

## Types

CASM supports all COIL types:

### Integer Types

- `TYPE_INT8`: 8-bit signed integer
- `TYPE_INT16`: 16-bit signed integer
- `TYPE_INT32`: 32-bit signed integer
- `TYPE_INT64`: 64-bit signed integer
- `TYPE_UNT8`: 8-bit unsigned integer
- `TYPE_UNT16`: 16-bit unsigned integer
- `TYPE_UNT32`: 32-bit unsigned integer
- `TYPE_UNT64`: 64-bit unsigned integer

### Floating-Point Types

- `TYPE_FP16`: 16-bit floating-point
- `TYPE_FP32`: 32-bit floating-point
- `TYPE_FP64`: 64-bit floating-point
- `TYPE_FP128`: 128-bit floating-point

### Special Types

- `TYPE_BIT`: Bit (boolean)
- `TYPE_VOID`: Void (no value)
- `TYPE_PTR`: Pointer
- `TYPE_VAR`: Variable reference
- `TYPE_SYM`: Symbol reference

### Vector Types

- `TYPE_V128`: 128-bit vector
- `TYPE_V256`: 256-bit vector
- `TYPE_V512`: 512-bit vector

### Composite Types

- `TYPE_STRUCT`: Structure
- `TYPE_ARRAY`: Array
- `TYPE_UNION`: Union
- `TYPE_PACK`: Packed structure

## Variables

Variables are declared using the `VAR` instruction:

```
VAR #id, type [, initial_value]
```

Examples:

```
VAR #1, TYPE_INT32, 0              ; Integer with initialization
VAR #2, TYPE_PTR                   ; Pointer without initialization
VAR #3, TYPE_FP32, 3.14159         ; Floating-point with initialization
```

## Registers

CASM supports architecture-specific registers:

x86-64 registers:
- `RAX`, `RBX`, `RCX`, `RDX`, `RSI`, `RDI`, `RBP`, `RSP`
- `R8` to `R15`
- `XMM0` to `XMM15`

## Memory Operands

Memory operands are enclosed in square brackets (`[]`):

```
[address]                      ; Direct addressing
[register]                     ; Indirect addressing
[base + offset]                ; Base + offset
[base + index * scale]         ; Base + scaled index
```

## Sections and Segments

Common sections:

- `.text`: Code section (executable)
- `.data`: Initialized data section
- `.bss`: Uninitialized data section
- `.rodata`: Read-only data section

## Symbols and Labels

Symbols and labels are used to reference memory locations:

```
SYM function_name              ; Define a symbol
function_name:                 ; Define a label
    ; Function code
```

Global symbols:

```
GLOBAL function_name           ; Export symbol
```

External symbols:

```
EXTERN external_function       ; Import symbol
```

## Error Reporting

The assembler reports errors in a standardized format:

```
error:category:filename:line:column: [error_code] message
```

## Advanced Features

### Scopes

Scopes are used to manage variable lifetimes:

```
SCOPEE                         ; Enter scope
    VAR #1, TYPE_INT32, 0      ; Local variable
    ; Code
SCOPEL                         ; Exit scope (variable #1 destroyed)
```

### Conditional Assembly

Conditional assembly directives:

```
IF condition
    ; Code to include if condition is true
ELIF alternative_condition
    ; Code to include if alternative condition is true
ELSE
    ; Code to include if no conditions are true
ENDIF
```

## Examples

### Hello World

```
PROC 0x01                       ; CPU processor
ARCH 0x01, 0x03                 ; x86-64 architecture

SECTION .text, 0x01 | 0x04
SYM _start, TYPE_PARAM0=GLOB    ; Global entry point
    SCOPEE
    ; Code to print "Hello, World!"
    SCOPEL
    RET

SECTION .data, 0x02 | 0x04 | 0x08
SYM message
DATA TYPE_ARRAY=TYPE_UNT8, "Hello, World!", 10, 0
```

### Function Example

```
PROC 0x01                       ; CPU processor

SECTION .text, 0x01 | 0x04
SYM add_numbers
    SCOPEE
    VAR #1, TYPE_INT32          ; First parameter
    VAR #2, TYPE_INT32          ; Second parameter
    MOV #1, TYPE_ABICTL=ABICTL_PARAM=platform_default, 0
    MOV #2, TYPE_ABICTL=ABICTL_PARAM=platform_default, 1
    
    VAR #3, TYPE_INT32          ; Result
    ADD #3, #1, #2              ; Add numbers
    
    RET TYPE_ABICTL=ABICTL_RET=platform_default, #3
    SCOPEL
```

## API Documentation

See the header files in `include/casm/` for API documentation.

## Architecture

The CASM Assembler consists of the following components:

1. **Lexer**: Converts source code to tokens
2. **Parser**: Parses tokens into an AST
3. **Symbol Table**: Manages symbols and their attributes
4. **Directive Handler**: Processes assembly directives
5. **Code Generator**: Generates COIL binary code
6. **Error Handler**: Manages error reporting and handling