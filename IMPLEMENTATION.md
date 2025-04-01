# CASM Implementation Plan

This document outlines the implementation approach, architecture, and development plans for the CASM assembler.

## Architecture Overview

CASM follows a classic compiler architecture with distinct phases:

1. **Lexical Analysis**: Convert source text to tokens
2. **Parsing**: Build abstract syntax tree (AST) from tokens
3. **Symbol Resolution**: Create symbol table and resolve references
4. **Code Generation**: Generate COIL binary code
5. **Output**: Produce COIL object file

The implementation is modular with clean separation of concerns, allowing for easy maintenance and extension.

## Directory Structure

```
casm/
├── CMakeLists.txt              # Main build system
├── LICENSE                     # Unlicense
├── README.md                   # Project overview
├── CONTRIBUTING.md             # Contribution guidelines
├── IMPLEMENTATION.md           # This file
│
├── include/                    # Public header files
│   └── casm/                   # Main include directory
│       ├── lexer.h             # Lexical analyzer
│       ├── parser.h            # Parser
│       ├── ast.h               # Abstract syntax tree
│       ├── symbol_table.h      # Symbol table
│       ├── code_generator.h    # Code generator
│       ├── assembler.h         # Main assembler interface
│       ├── directives.h        # Directive processing
│       ├── optimizer.h         # Code optimization
│       └── error_reporter.h    # Error reporting
│
├── src/                        # Implementation source files
│   ├── lexer.cpp               # Lexical analyzer implementation
│   ├── parser.cpp              # Parser implementation
│   ├── ast.cpp                 # AST implementation
│   ├── symbol_table.cpp        # Symbol table implementation
│   ├── code_generator.cpp      # Code generator implementation
│   ├── assembler.cpp           # Main assembler implementation
│   ├── directives.cpp          # Directive processing implementation
│   ├── optimizer.cpp           # Optimization implementation
│   ├── error_reporter.cpp      # Error reporting implementation
│   ├── main.cpp                # Entry point for command-line tool
│   └── utils/                  # Utility implementations
│       ├── logger.cpp          # Logging utility
│       ├── file_utils.cpp      # File handling utilities
│       └── options.cpp         # Command-line option processing
│
├── tests/                      # Test suite
│   ├── unit/                   # Unit tests
│   │   ├── lexer_tests.cpp     # Lexer tests
│   │   ├── parser_tests.cpp    # Parser tests
│   │   ├── ast_tests.cpp       # AST tests
│   │   ├── symbol_table_tests.cpp # Symbol table tests
│   │   ├── code_generator_tests.cpp # Code generator tests
│   │   ├── directive_tests.cpp  # Directive tests
│   │   └── optimizer_tests.cpp  # Optimizer tests
│   ├── integration/            # Integration tests
│   │   ├── assembly_tests.cpp   # End-to-end assembly tests
│   │   └── error_handling_tests.cpp # Error reporting tests
│   └── examples/               # Example CASM programs for testing
│       ├── hello_world.casm     # Hello world example
│       ├── arithmetic.casm      # Arithmetic operations example
│       └── functions.casm       # Function call example
│
├── examples/                   # Example CASM programs
│   ├── hello_world/            # Hello world example
│   ├── fibonacci/              # Fibonacci sequence
│   ├── sorting/                # Sorting algorithms
│   └── data_structures/        # Data structure implementations
│
└── docs/                       # Documentation
    ├── language/               # Language documentation
    ├── api/                    # API documentation
    ├── tutorials/              # Tutorials
    └── examples/               # Annotated examples
```

## Implementation Plan

### Phase 1: Core Functionality

1. **Lexical Analysis**
   - Implement token definitions
   - Create lexer using finite state machine
   - Implement error reporting for lexical errors
   - Optimize for performance

2. **Parsing**
   - Define abstract syntax tree (AST) nodes
   - Implement recursive descent parser
   - Create validation for syntax correctness
   - Handle error recovery and reporting

3. **Symbol Table**
   - Implement symbol table with scope support
   - Create symbol resolution mechanism
   - Add support for forward references
   - Implement error checking for symbols

**Estimated Time**: 4-6 weeks

### Phase 2: Code Generation

4. **Code Generation Basics**
   - Implement basic instruction encoding
   - Create section handling
   - Add directive processing
   - Implement relocation table generation

5. **Advanced Code Generation**
   - Add support for complex expressions
   - Implement addressing modes
   - Create ABI support
   - Add type system integration

6. **Binary Output**
   - Implement COIL object file format
   - Create debug information generation
   - Add symbol table output
   - Implement validation of output

**Estimated Time**: 6-8 weeks

### Phase 3: Optimization and Features

7. **Optimization**
   - Implement constant folding
   - Add peephole optimization
   - Create instruction combining
   - Add dead code elimination

8. **Advanced Features**
   - Implement macro system
   - Add conditional assembly
   - Create include handling
   - Implement platform-specific features

**Estimated Time**: 4-6 weeks

### Phase 4: Documentation and Polishing

9. **Documentation**
   - Create comprehensive language reference
   - Write API documentation
   - Develop tutorials and examples
   - Create error message guide

10. **Testing and Quality**
    - Expand test coverage
    - Perform performance optimization
    - Add fuzz testing
    - Create validation suite

**Estimated Time**: 3-4 weeks

## Technical Approach

### Lexical Analysis

The lexer will use a state machine-based approach for efficient tokenization:

```cpp
Token Lexer::nextToken() {
    skipWhitespaceAndComments();
    
    if (isAtEnd()) {
        return Token(TokenType::END_OF_FILE, "", line_, column_);
    }
    
    char c = current_;
    advance();
    
    switch (c) {
        // Single-character tokens
        case ':': return Token(TokenType::COLON, ":", line_, column_ - 1);
        case ',': return Token(TokenType::COMMA, ",", line_, column_ - 1);
        case ';': return Token(TokenType::SEMICOLON, ";", line_, column_ - 1);
        case '#': return Token(TokenType::HASH, "#", line_, column_ - 1);
        
        // Number literals
        case '0': {
            // Check for hex or binary notation
            if (peek() == 'x' || peek() == 'X') {
                advance(); // Consume 'x'
                return hexNumber();
            }
            else if (peek() == 'b' || peek() == 'B') {
                advance(); // Consume 'b'
                return binaryNumber();
            }
            // Fall back to regular number handling
            position_--;
            column_--;
            current_ = c;
            return number();
        }
        
        // Identifiers and keywords
        default:
            if (std::isalpha(c) || c == '_') {
                position_--;
                column_--;
                current_ = c;
                return identifier();
            }
            else if (std::isdigit(c)) {
                position_--;
                column_--;
                current_ = c;
                return number();
            }
            else {
                // Invalid character
                std::ostringstream oss;
                oss << "Unexpected character '" << c << "'";
                setError(0x01000001, oss.str()); // Syntax error code
                return Token(TokenType::INVALID, std::string(1, c), line_, column_ - 1);
            }
    }
}
```

### Parsing

The parser will use a recursive descent approach with predictable error recovery:

```cpp
std::unique_ptr<Statement> Parser::parseStatement() {
    // Check for a label
    if (current().type == TokenType::IDENTIFIER && peek().type == TokenType::COLON) {
        auto identifier = advance(); // Consume identifier
        advance(); // Consume colon
        return parseLabel(identifier);
    }
    
    // Check for directives (keywords)
    if (current().type >= TokenType::KW_PROC && current().type <= TokenType::KW_TARGET) {
        auto directive = advance();
        return parseDirective(directive);
    }
    
    // Check for instructions
    if (current().type >= TokenType::INSTR_MOV && current().type <= TokenType::INSTR_SYSCALL) {
        auto instruction = advance();
        return parseInstruction(instruction);
    }
    
    // Invalid statement
    std::ostringstream oss;
    oss << "Expected statement, got '" << current().lexeme << "'";
    setError(0x01020002, oss.str()); // Invalid statement error
    
    // Error recovery: skip to next statement
    while (!isAtEnd() && current().type != TokenType::SEMICOLON) {
        advance();
    }
    if (!isAtEnd()) {
        advance(); // Consume semicolon
    }
    
    return nullptr;
}
```

### Code Generation

The code generator will use a visitor pattern to traverse the AST:

```cpp
class CodeGenerator : public ASTVisitor {
public:
    CodeGenerator(const SymbolTable& symbols);
    
    std::unique_ptr<CoilObject> generate(const SourceFile& sourceFile);
    
    // Visitor methods for different AST nodes
    void visit(const LabelStatement& label) override;
    void visit(const DirectiveStatement& directive) override;
    void visit(const InstructionStatement& instruction) override;
    void visit(const IntLiteralExpr& intLiteral) override;
    void visit(const FloatLiteralExpr& floatLiteral) override;
    void visit(const StringLiteralExpr& stringLiteral) override;
    void visit(const IdentifierExpr& identifier) override;
    void visit(const VariableExpr& variable) override;
    void visit(const RegisterExpr& reg) override;
    void visit(const TypeExpr& type) override;
    void visit(const MemoryRefExpr& memoryRef) override;
    void visit(const BinaryOpExpr& binaryOp) override;
    void visit(const TypeCastExpr& typeCast) override;
    
private:
    const SymbolTable& symbols_;
    std::unique_ptr<CoilObject> currentObject_;
    Section* currentSection_;
    std::vector<uint8_t> currentSectionData_;
    uint32_t currentAddress_;
    std::unordered_map<std::string, uint16_t> symbolIndices_;
    std::vector<Relocation> relocations_;
    
    void generateInstruction(const InstructionStatement& instruction);
    void handleDirective(const DirectiveStatement& directive);
    std::vector<uint8_t> encodeOperand(const Expression& expr);
    void addSymbol(const std::string& name, uint32_t value, uint32_t attributes);
    void addRelocation(uint32_t offset, const std::string& symbolName, uint8_t type, uint8_t size);
};
```

### Optimization

The optimizer will use a pipeline approach with multiple passes:

```cpp
class Optimizer {
public:
    Optimizer(int level);
    
    std::unique_ptr<SourceFile> optimize(const SourceFile& sourceFile);
    
private:
    int level_;
    std::vector<std::unique_ptr<OptimizationPass>> passes_;
    
    void setupPasses();
    
    // Optimization passes
    class ConstantFoldingPass;
    class DeadCodeEliminationPass;
    class PeepholeOptimizationPass;
    class InstructionCombiningPass;
    class RegisterAllocationPass;
};
```

## Error Handling

CASM will use a comprehensive error handling system:

1. **Error Classification**: Structured error codes
2. **Context-Aware Reporting**: Error messages with line and column information
3. **Error Recovery**: Continue parsing after errors when possible
4. **Diagnostics**: Detailed error messages with suggestions

Example error reporting:

```
error: syntax error at line 42, column 10: expected ')', found ';'
   42 | CALL function(arg1, arg2;
                          ^
note: missing closing parenthesis
suggestion: CALL function(arg1, arg2)
```

## Testing Strategy

The testing approach for CASM includes:

1. **Unit Tests**: Test each component in isolation
   - Lexer tokenization
   - Parser syntax handling
   - AST construction
   - Code generation
   - Optimization passes

2. **Integration Tests**: Test end-to-end assembly
   - Complete programs
   - Error handling
   - Optimization effects

3. **Reference Tests**: Compare against specification
   - Instruction encoding
   - Directive behavior
   - Output format

4. **Performance Tests**: Measure and optimize
   - Assembly speed
   - Memory usage
   - Output size

The testing framework will use a modern C++ testing framework (e.g., Catch2 or Google Test) and will include both automated and manual test cases.

## Deliverables

The final deliverables for CASM include:

1. **Command-Line Tool**:
   - `casm` executable with full functionality
   - Support for all command-line options

2. **Library Components**:
   - Static and shared libraries for embedding
   - C and C++ APIs for integration

3. **Documentation**:
   - Language reference
   - API documentation
   - Usage examples
   - Error message guide

4. **Tests and Examples**:
   - Comprehensive test suite
   - Example programs
   - Benchmarks

## Timeline

| Phase | Duration | Key Milestones |
|-------|----------|---------------|
| Phase 1 | 4-6 weeks | - Basic lexer and parser<br>- Symbol table<br>- Simple instruction encoding |
| Phase 2 | 6-8 weeks | - Complete code generation<br>- COIL object file output<br>- Complex expression support |
| Phase 3 | 4-6 weeks | - Optimization passes<br>- Macro system<br>- Advanced features |
| Phase 4 | 3-4 weeks | - Documentation<br>- Performance optimization<br>- Test suite completion |

Total estimated duration: 17-24 weeks for a complete, production-ready implementation.

## Dependencies

CASM has minimal external dependencies:

1. **libcoil-dev**: Core library for COIL format and utilities
2. **C++ Standard Library**: For containers, algorithms, and I/O
3. **CMake**: Build system

No other third-party libraries are required for the core functionality.

## Development Process

The development process will follow these practices:

1. **Test-Driven Development**: Write tests before implementation
2. **Continuous Integration**: Automated testing on each commit
3. **Code Reviews**: Peer review for all code changes
4. **Documentation**: Update documentation with code changes
5. **Benchmarking**: Regular performance testing