# CASM Assembler

A high-performance assembler for CASM (COIL Assembly) that generates COIL binary format.

## Overview

The CASM Assembler is a core component of the COIL toolchain, responsible for translating CASM assembly language into COIL binary format. It serves as the primary interface for developers writing low-level COIL code.

## Features

- Full support for CASM syntax
- Precise error reporting with line and column information
- Comprehensive symbol resolution
- Support for all COIL instruction types
- Integration with libcoil-dev

## Building

### Prerequisites

- C++17 compliant compiler
- CMake 3.12 or higher
- libcoil-dev installed

### Build Instructions

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

```bash
casm [options] input.casm -o output.coil
```

### Options

- `-I <path>`: Include directory for header files
- `-o <file>`: Specify output file
- `-v`: Enable verbose output
- `-h`: Display help message
- `-d`: Enable debug information

## Examples

Simple assembly:
```bash
casm source.casm -o output.coil
```

With include paths:
```bash
casm -I./include source.casm -o output.coil
```

## License

This project is licensed under the same terms as libcoil-dev.