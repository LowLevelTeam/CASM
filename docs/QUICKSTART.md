# CASM Assembler Quickstart Guide

This quickstart guide will help you get up and running with the CASM Assembler quickly.

## Installation

### Prerequisites

- C++17 compliant compiler (GCC 8+, Clang 6+, MSVC 2019+)
- CMake 3.12 or higher
- libcoil-dev installed

### Installing from Source

```bash
# Clone the repository
git clone https://github.com/yourusername/casm.git

# Enter the directory
cd casm

# Create a build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build
make

# Install (optional)
sudo make install
```

## Your First CASM Program

Let's create a simple program that adds two numbers:

Create a file named `add.casm`:

```
; Simple addition program
PROC 0x01                       ; CPU processor
ARCH 0x01, 0x03                 ; x86-64 architecture

SECTION .text, 0x01 | 0x04      ; Executable and readable

; Main entry point
SYM main, TYPE_PARAM0=GLOB
    SCOPEE                      ; Enter scope
    
    ; Create variables
    VAR #1, TYPE_INT32, 10      ; First number
    VAR #2, TYPE_INT32, 20      ; Second number
    VAR #3, TYPE_INT32          ; Result
    
    ; Add numbers
    ADD #3, #1, #2              ; #3 = #1 + #2
    
    ; Return result
    RET TYPE_ABICTL=ABICTL_RET=platform_default, #3
    
    SCOPEL                      ; Leave scope
```

## Compiling Your Program

Compile the program using the CASM assembler:

```bash
casm add.casm -o add.coil
```

This will generate a COIL binary file named `add.coil`.

## CASM Syntax Basics

### Comments

Comments start with a semicolon (`;`):

```
; This is a full-line comment
MOV #1, 42  ; This is an inline comment
```

### Labels

Labels are identifiers followed by a colon (`:`):

```
loop_start:        ; Define a label
    ; Code
    BR loop_start  ; Branch to the label
```

### Variables

Variables are declared using the `VAR` instruction:

```
VAR #1, TYPE_INT32, 0      ; Integer variable with initial value 0
VAR #2, TYPE_PTR           ; Pointer variable without initialization
VAR #3, TYPE_FP32, 3.14    ; Float variable with initial value 3.14
```

### Instructions

CASM supports a wide range of instructions:

```
MOV #1, 42                 ; Move immediate value to variable
ADD #3, #1, #2             ; Add variables
CMP #1, #2                 ; Compare variables
BR_EQ label                ; Branch if equal
```

### Sections

Code and data are organized into sections:

```
SECTION .text, 0x01 | 0x04      ; Code section (executable, readable)
SECTION .data, 0x02 | 0x04 | 0x08  ; Data section (writable, readable, initialized)
SECTION .bss, 0x02 | 0x04 | 0x10   ; BSS section (writable, readable, uninitialized)
```

## Common Patterns

### Function Definition

```
SYM function_name
    SCOPEE
    ; Function parameters
    VAR #1, TYPE_INT32
    MOV #1, TYPE_ABICTL=ABICTL_PARAM=platform_default, 0
    
    ; Function body
    
    ; Return value
    RET TYPE_ABICTL=ABICTL_RET=platform_default, return_value
    SCOPEL
```

### Conditional Execution

```
CMP #1, #2                 ; Compare #1 and #2
BR_EQ equal_case           ; Branch if equal
BR_NE not_equal_case       ; Branch if not equal
```

### Loops

```
loop_start:
    ; Loop body
    
    ; Update loop counter
    INC #1
    
    ; Check loop condition
    CMP #1, #2
    BR_LT loop_start       ; Branch if less than
```

## Next Steps

- Check the full [documentation](README.md) for detailed information
- Look at the [examples](../examples/) directory for more examples
- Read the [CONTRIBUTING.md](../CONTRIBUTING.md) if you want to contribute

## Common Issues

### Compilation Errors

If you encounter compilation errors:

1. Check syntax errors in your CASM code
2. Ensure variable declarations before use
3. Verify section declarations
4. Check for invalid instructions or directives

### Linker Errors

If you encounter linker errors:

1. Check for undefined symbols
2. Verify symbol visibility (GLOBAL, EXTERN)
3. Ensure sections are properly defined

## Getting Help

If you need help:

1. Check the [documentation](README.md)
2. Look for similar issues in the issue tracker
3. Ask a question on the project's discussions page