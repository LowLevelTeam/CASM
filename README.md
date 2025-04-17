# CASM - COIL Assembly Language

CASM (COIL Assembly) is a lightweight assembler for the COIL (Compiler Optimization & Instruction Library) intermediate representation format. It provides a simple, human-readable assembly language that compiles directly to the COIL binary format.

## Features

- Simple, line-oriented syntax that maps directly to COIL instructions
- Support for all COIL opcodes and operand types
- Directives for section definitions, data declarations, and symbols
- Minimal memory usage with efficient single-pass compilation
- Complete integration with the COIL library ecosystem

## Design Principles

CASM follows the same design principles as the COIL library:

- **Minimal Allocations**: Avoid heap allocations where possible
- **Stack First**: Prefer stack allocations and references
- **Clean API**: Simple, intuitive interfaces
- **Performance**: Zero-cost abstractions that compile to efficient machine code
- **Portability**: Works across platforms with consistent behavior

## Building

```bash
meson setup build
cd build
ninja
```

## Installation

```bash
ninja install
```

## Usage

Basic usage:

```bash
casm input.casm -o output.coil
```

Options:

```
-o, --output FILE    Specify output file (default is input file with .coil extension)
-v, --verbose        Enable verbose output
-h, --help           Show help message
```

## Example

```
; Simple CASM program
.section .text
.global main

main:
  push r1
  load r1, [r2+8]
  add r1, r1, 42
  store [r2], r1
  pop r1
  ret
```

## License

This project is in the public domain. See the LICENSE file for details.

## Documentation

See GRAMMAR.md for detailed language specification.