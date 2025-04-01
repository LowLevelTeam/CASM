# CASM: COIL Assembler

[![License: Unlicense](https://img.shields.io/badge/license-Unlicense-blue.svg)](https://unlicense.org)
[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)]()

## Overview

CASM is the official assembler for the COIL (Computer Oriented Intermediate Language) Toolchain. It translates human-readable CASM source code into COIL binary format for further processing by other toolchain components.

CASM serves as the front-end of the COIL compilation process, handling the initial translation from text to binary representation.

## Features

- **Complete CASM Syntax Support**: Full implementation of the COIL assembly language
- **Multi-pass Assembly**: Resolves forward references and optimizes code
- **Directive Processing**: Supports all CASM directives for controlling assembly
- **Macro System**: Powerful macro capabilities for code reuse
- **Debug Information**: Generates comprehensive debug information
- **Error Reporting**: Detailed error messages with source context

## COIL Toolchain Integration

CASM is typically the first step in the COIL compilation workflow:

```
Source code (.casm)
       |
       v
  CASM Assembler
       |
       v
 COIL object (.coil)
       |
       v
  COIL Processor (coilp)
       |
       v
COIL output object (.coilo)
       |
       v
OS-specific Linker
       |
       v
  Executable (.exe/.out)
```

## Installation

### Prerequisites

- C++17 compliant compiler
- CMake 3.15+
- libcoil-dev 1.0.0+

### Building from Source

```bash
git clone https://github.com/LLT/casm.git
cd casm
mkdir build && cd build
cmake ..
cmake --build .
cmake --install .
```

## Usage

### Basic Usage

```bash
casm [options] input.casm -o output.coil
```

### Command Line Options

| Option | Description |
|--------|-------------|
| `-o, --output <file>` | Specify output file |
| `-I <path>` | Add include directory |
| `-D <symbol>[=value]` | Define symbol |
| `-O <level>` | Set optimization level (0-3) |
| `-g` | Include debug information |
| `-W <warning>` | Enable specific warning |
| `-v, --verbose` | Enable verbose output |
| `-h, --help` | Display help message |

### Example

```bash
# Assemble with optimization level 2 and debug info
casm -O2 -g -I./include program.casm -o program.coil
```

## CASM Language Syntax

CASM uses a clean assembly-like syntax with special support for COIL's type system and variable model.

### Basic Syntax

Each line in CASM follows this pattern:
```
[label:] [instruction|directive] [operands...] [; comment]
```

### Examples

```asm
; Comments start with semicolon
loop_start:        ; Define a label
    VAR #1, TYPE_INT32, 0    ; Declare a variable
    MOV #1, 42               ; Initialize it
    CMP #1, 100              ; Compare with 100
    BR_LT loop_start         ; Branch if less than

SYM function_name            ; Define a symbol
    SCOPEE                   ; Enter a new scope
        ; Function body
    SCOPEL                   ; Leave the scope
    RET                      ; Return from function
```

## Hello World Example

```asm
; Hello World in CASM
PROC 0x01                       ; CPU processor

; Data section for message
SECTION .data, 0x02 | 0x04 | 0x08
SYM hello_msg
DATA TYPE_ARRAY=TYPE_UNT8, "Hello, World!", 10, 0

; Code section
SECTION .text, 0x01 | 0x04
SYM _start, TYPE_PARAM0=GLOB
    SCOPEE
    ; Write to stdout
    VAR #1, TYPE_UNT64, 1       ; syscall number (write)
    VAR #2, TYPE_UNT64, 1       ; file descriptor (stdout)
    VAR #3, TYPE_PTR, hello_msg ; message pointer
    VAR #4, TYPE_UNT64, 14      ; message length
    
    ; Write syscall
    SYSCALL #1, #2, #3, #4
    
    ; Exit program
    VAR #5, TYPE_UNT64, 60      ; syscall number (exit)
    VAR #6, TYPE_UNT64, 0       ; exit code
    SYSCALL #5, #6
    SCOPEL
```

## Documentation

Comprehensive documentation is available in the `docs/` directory:

- [CASM Language Reference](docs/language-reference.md)
- [Command Line Reference](docs/command-line.md)
- [Optimization Guide](docs/optimization.md)
- [Examples](docs/examples/)

## Workflow Integration

CASM works seamlessly with other COIL ecosystem tools:

```bash
# Assemble
casm program.casm -o program.coil

# Process
coilp program.coil -o program.coilo

# Link
ld program.coilo -o program

# Run
./program
```

## License

This project is released under the Unlicense. See [LICENSE](LICENSE) for details.