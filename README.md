# CASM: COIL Assembler

[![License: Unlicense](https://img.shields.io/badge/license-Unlicense-blue.svg)](https://unlicense.org)
[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)]()

## Overview

CASM is the official assembler for the COIL (Computer Oriented Intermediate Language) ecosystem. It translates human-readable CASM (COIL Assembly) source code into COIL binary format, which can then be processed by the COIL processor or interpreter.

CASM provides a clean, consistent syntax for writing low-level code that can target multiple architectures without changing the source code.

## Features

- **Complete CASM Syntax Support**: Full implementation of the COIL assembly language
- **Multi-pass Assembly**: Resolves forward references and optimizes code
- **Directive Processing**: Supports all CASM directives for controlling assembly
- **Macro System**: Powerful macro capabilities for code reuse
- **Debug Information**: Generates comprehensive debug information
- **Optimization**: Multiple optimization levels
- **Error Reporting**: Detailed error messages with source context
- **Cross-platform**: Runs on Windows, macOS, and Linux

## Installation

### Prerequisites

- C++17 compliant compiler
- CMake 3.15+
- libcoil-dev 1.0.0+

### Building from Source

```bash
# Clone the repository
git clone https://github.com/LLT/casm.git
cd casm

# Create build directory
mkdir build && cd build

# Generate build files
cmake ..

# Build
cmake --build .

# Install
cmake --install .
```

### Pre-built Binaries

Pre-built binaries are available for major platforms:

- [Windows x64](https://github.com/LLT/casm/releases/download/v1.0.0/casm-1.0.0-win-x64.zip)
- [macOS x64](https://github.com/LLT/casm/releases/download/v1.0.0/casm-1.0.0-macos-x64.tar.gz)
- [Linux x64](https://github.com/LLT/casm/releases/download/v1.0.0/casm-1.0.0-linux-x64.tar.gz)

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
| `-t <target>` | Target triple (device-arch-mode) |
| `-v, --verbose` | Enable verbose output |
| `-h, --help` | Display help message |
| `--version` | Display version information |

### Example

```bash
# Assemble with optimization level 2 and debug info
casm -O2 -g -I./include program.casm -o program.coil

# Specify target architecture
casm -t cpu-x86-64 program.casm -o program.coil
```

## CASM Language Syntax

CASM uses a clean, assembly-like syntax with special support for COIL's type system and variable model.

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

### Hello World Example

```asm
; Hello World in CASM
PROC 0x01                       ; CPU processor
ARCH 0x01, 0x03                 ; x86-64 architecture

; Data section for message
SECTION .data, 0x02 | 0x04 | 0x08
SYM hello_msg
DATA TYPE_ARRAY=TYPE_UNT8, "Hello, World!", 10, 0  ; String with newline and null terminator

; Code section
SECTION .text, 0x01 | 0x04
SYM _start, TYPE_PARAM0=GLOB    ; Global entry point
    SCOPEE
    ; System call variables
    VAR #1, TYPE_UNT64, 1       ; syscall number (write)
    VAR #2, TYPE_UNT64, 1       ; file descriptor (stdout)
    VAR #3, TYPE_PTR, hello_msg ; message pointer
    VAR #4, TYPE_UNT64, 14      ; message length
    
    ; Write syscall
    MOV TYPE_RGP=RAX, #1        ; Syscall number
    MOV TYPE_RGP=RDI, #2        ; First argument (fd)
    MOV TYPE_RGP=RSI, #3        ; Second argument (buffer)
    MOV TYPE_RGP=RDX, #4        ; Third argument (count)
    SYSCALL
    
    ; Exit syscall
    VAR #5, TYPE_UNT64, 60      ; syscall number (exit)
    VAR #6, TYPE_UNT64, 0       ; exit code
    MOV TYPE_RGP=RAX, #5
    MOV TYPE_RGP=RDI, #6
    SYSCALL
    SCOPEL
```

## Documentation

Comprehensive documentation is available in the `docs/` directory and online at [coil-lang.org/casm/docs](https://coil-lang.org/casm/docs):

- [CASM Language Reference](https://coil-lang.org/casm/docs/language-reference.html)
- [Command Line Reference](https://coil-lang.org/casm/docs/command-line.html)
- [Optimization Guide](https://coil-lang.org/casm/docs/optimization.html)
- [Tutorials](https://coil-lang.org/casm/docs/tutorials/)
- [Examples](https://coil-lang.org/casm/docs/examples/)

## Integration with Other Tools

CASM is designed to work seamlessly with other COIL ecosystem tools:

- `coilp`: Process COIL objects into architecture-specific code
- `cbcrun`: Run COIL Byte Code directly
- `coildebug`: Debug CASM programs

Typical workflow:
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

## Contributing

Please see [CONTRIBUTING.md](CONTRIBUTING.md) for details on how to contribute to CASM.

## Implementation

For details on the implementation approach, architecture, and development plans, see [IMPLEMENTATION.md](IMPLEMENTATION.md).

## License

This project is released under the Unlicense. See [LICENSE](LICENSE) for details.