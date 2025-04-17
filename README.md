# CASM Assembler

CASM (COIL Assembly) is an assembler for the COIL (Computer Oriented Intermediate Language) binary format. It compiles human-readable assembly language into COIL binary objects.

## Overview

CASM provides a simple way to create COIL binaries by writing assembly code. The language syntax is designed for ease of parsing rather than complex features, primarily intended for:

- Creating test binaries for the COIL toolchain
- Serving as an explicit disassembly format for COIL binaries
- Providing a reference implementation for COIL code generation

## Features

- Simple, consistent syntax with straightforward token rules
- Direct mapping to COIL opcodes and object format
- Support for labels, sections, and symbols
- Data directives for defining initialized and uninitialized data
- Full control over COIL binary format features

## Building

CASM depends on the COIL library (libcoil-dev). Make sure it's installed before building.

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

```bash
casm [options] input_file output_file
```

Options:
- `-h, --help` - Show help message
- `-v, --verbose` - Enable verbose output

## Example

```assembly
; Simple program that calculates factorial
.section .text
.global @factorial

#factorial
  ; r1 = input value, returns factorial in r1
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

## Grammar

For detailed documentation of the CASM language syntax, see the [GRAMMAR.md](GRAMMAR.md) file.

## Architecture

CASM consists of several components:

1. **Lexer**: Tokenizes the CASM source code into a stream of tokens
2. **Parser**: Converts the token stream into a structured representation
3. **Assembler**: Generates COIL binary objects from the parsed representation

The assembler performs a two-pass compilation process:
- First pass: Collect labels and calculate sizes
- Second pass: Resolve references and generate code

## Integration with COIL

CASM integrates with the COIL library to produce binary objects that conform to the COIL format specification, including:
- Section-based layout
- Symbol tables
- Proper encoding of instructions and operands

## License

This project is in the public domain.