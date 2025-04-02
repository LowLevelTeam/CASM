# Contributing to CASM Assembler

Thank you for your interest in contributing to the CASM Assembler! This document provides guidelines and instructions for contributing to the project.

## Getting Started

1. Fork the repository
2. Clone your fork: `git clone https://github.com/yourusername/casm.git`
3. Create a new branch: `git checkout -b feature/your-feature-name`
4. Make your changes
5. Run the tests to ensure everything works correctly
6. Commit your changes: `git commit -m "Add your message here"`
7. Push to your fork: `git push origin feature/your-feature-name`
8. Create a pull request

## Development Environment

### Prerequisites

- C++17 compliant compiler
- CMake 3.12 or higher
- libcoil-dev installed

### Building

```bash
mkdir build
cd build
cmake ..
make
```

### Testing

You can test the assembler on the provided test files:

```bash
./casm ../tests/test.casm -o test.coil
```

## Code Style Guidelines

- Use 4 spaces for indentation (no tabs)
- Follow the existing code style
- Use descriptive variable and function names
- Add comments for complex logic
- Document public APIs using doxygen-style comments

## Project Structure

- `include/casm/`: Header files for the assembler
- `src/`: Implementation files
- `tests/`: Test files and test infrastructure
- `build/`: Build output (generated)

## Components Overview

### Lexer

The lexer (`include/casm/lexer.h` and `src/lexer.cpp`) converts CASM source code into tokens.

### Parser

The parser (`include/casm/parser.h` and `src/parser.cpp`) parses tokens into an AST (Abstract Syntax Tree).

### Symbol Table

The symbol table (`include/casm/symbol_table.h` and `src/symbol_table.cpp`) manages symbols, labels, and their attributes.

### Code Generator

The code generator (`include/casm/code_generator.h` and `src/code_generator.cpp`) turns the AST into COIL binary format.

### Directive Handler

The directive handler (`include/casm/directive_handler.h` and `src/directive_handler.cpp`) processes assembly directives.

### Error Handler

The error handler (`include/casm/error_handler.h` and `src/error_handler.cpp`) manages error reporting and handling.

## Common Tasks

### Adding a New Instruction

1. Update `Lexer::initializeKeywordMaps()` in `src/lexer.cpp`
2. Implement instruction handling in `CodeGenerator::processInstruction()` in `src/code_generator.cpp`

### Adding a New Directive

1. Update `Lexer::initializeKeywordMaps()` in `src/lexer.cpp`
2. Add a handler in `DirectiveHandler::initializeHandlers()` in `src/directive_handler.cpp`
3. Implement the handler function in `DirectiveHandler` class

### Adding New Tests

Add new test cases in the `tests/` directory.

## Pull Request Process

1. Ensure your code follows the style guidelines
2. Update the documentation if necessary
3. Add tests for new functionality
4. Make sure all tests pass
5. Request a review from a maintainer
6. Address any feedback from code review

## License

By contributing to this project, you agree that your contributions will be licensed under the same license as the project.