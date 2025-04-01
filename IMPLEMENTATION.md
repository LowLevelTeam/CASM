# CASM Implementation Plan

This document outlines the implementation approach for the CASM assembler, a core component of the LLT COIL Toolchain.

## Architecture Overview

CASM follows a classic assembler architecture with distinct phases:

1. **Lexical Analysis**: Convert source text to tokens
2. **Parsing**: Build abstract syntax tree (AST) from tokens
3. **Symbol Resolution**: Create symbol table and resolve references
4. **Code Generation**: Generate COIL binary code
5. **Output**: Produce COIL object file

The implementation is modular with clean separation of concerns, allowing for easy maintenance and extension.

## Directory Structure

```
casm/
├── CMakeLists.txt
├── LICENSE                     # Unlicense
├── README.md                   # Project overview
├── include/                    # Public header files
│   └── casm/
│       ├── lexer.h             # Lexical analyzer
│       ├── parser.h            # Parser
│       ├── ast.h               # Abstract syntax tree
│       ├── symbol_table.h      # Symbol table
│       ├── code_generator.h    # Code generator
│       ├── assembler.h         # Main assembler interface
│       ├── directives.h        # Directive processing
│       └── optimizer.h         # Code optimization
├── src/                        # Implementation source files
├── tests/                      # Test suite
└── examples/                   # Example programs
```

## Implementation Plan

### Phase 1: Core Functionality (4-6 weeks)

1. **Lexical Analysis**
   - Implement token definitions
   - Create lexer using finite state machine
   - Add support for all CASM syntax elements

2. **Parsing**
   - Define abstract syntax tree (AST) nodes
   - Implement recursive descent parser
   - Add error recovery mechanisms

3. **Symbol Table**
   - Implement symbol table with scope support
   - Create symbol resolution mechanism
   - Add support for forward references

### Phase 2: Code Generation (6-8 weeks)

4. **Directive Processing**
   - Implement section handling
   - Add processor and architecture directives
   - Create data directives

5. **Code Generation**
   - Implement instruction encoding
   - Create variable handling
   - Add scope management
   - Implement relocation generation

6. **Binary Output**
   - Implement COIL object file format
   - Create symbol table output
   - Add debug information generation

### Phase 3: Optimization and Advanced Features (4-6 weeks)

7. **Optimization**
   - Implement constant folding
   - Add peephole optimization
   - Create instruction combining

8. **Advanced Features**
   - Implement macro system
   - Add conditional assembly
   - Create include handling

## Technical Approach

### Lexical Analysis

The lexer will use a state machine-based approach for efficient tokenization:

```cpp
class Lexer {
public:
    Lexer(const std::string& source);
    
    // Get the next token from source
    Token nextToken();
    
    // Get current position information
    size_t getLine() const;
    size_t getColumn() const;
    
private:
    std::string source_;
    size_t position_;
    size_t line_;
    size_t column_;
    
    // Helper methods
    char currentChar() const;
    char peekNext() const;
    void advance();
    
    // Token parsing methods
    Token parseIdentifier();
    Token parseNumber();
    Token parseString();
    
    // Skip whitespace and comments
    void skipWhitespaceAndComments();
};
```

### Parsing

The parser will use a recursive descent approach with predictable error recovery:

```cpp
class Parser {
public:
    Parser(Lexer& lexer, ErrorReporter& errorReporter);
    
    // Parse a complete source file
    std::unique_ptr<SourceFile> parse();
    
private:
    Lexer& lexer_;
    ErrorReporter& errorReporter_;
    Token currentToken_;
    
    // Helper methods
    Token consume();
    Token expect(TokenType type, const std::string& errorMessage);
    bool match(TokenType type);
    
    // Parsing methods
    std::unique_ptr<Statement> parseStatement();
    std::unique_ptr<Instruction> parseInstruction();
    std::unique_ptr<Directive> parseDirective();
    std::unique_ptr<Expression> parseExpression();
    std::unique_ptr<Label> parseLabel();
    
    // Error recovery
    void synchronize();
};
```

### Code Generation

The code generator will use a visitor pattern to traverse the AST:

```cpp
class CodeGenerator : public ASTVisitor {
public:
    CodeGenerator(SymbolTable& symbols, ErrorReporter& errorReporter);
    
    // Generate code for a source file
    std::unique_ptr<CoilObject> generate(const SourceFile& sourceFile);
    
    // Visit methods for AST nodes
    void visit(const InstructionNode& instruction) override;
    void visit(const DirectiveNode& directive) override;
    void visit(const LabelNode& label) override;
    void visit(const ExpressionNode& expression) override;
    
private:
    SymbolTable& symbols_;
    ErrorReporter& errorReporter_;
    std::unique_ptr<CoilObject> coilObject_;
    Section* currentSection_;
    
    // Helper methods
    void processInstruction(const InstructionNode& instruction);
    void processDirective(const DirectiveNode& directive);
    void processLabel(const LabelNode& label);
    
    // Generate binary for operands
    std::vector<uint8_t> generateOperand(const ExpressionNode& expression);
};
```

### Optimization

The optimizer will use a pipeline approach with multiple passes:

```cpp
class Optimizer {
public:
    Optimizer(int level);
    
    // Optimize a source file
    void optimize(SourceFile& sourceFile);
    
private:
    int level_;
    
    // Optimization passes
    void constantFolding(SourceFile& sourceFile);
    void instructionCombining(SourceFile& sourceFile);
    void deadCodeElimination(SourceFile& sourceFile);
    void peepholeOptimization(SourceFile& sourceFile);
};
```

## Testing Strategy

The testing approach for CASM includes:

1. **Unit Tests**: Test each component in isolation
   - Lexer tokenization
   - Parser syntax handling
   - AST construction
   - Code generation

2. **Integration Tests**: Test end-to-end assembly
   - Complete programs
   - Error handling
   - Optimization effects

3. **Reference Tests**: Test against specification
   - Instruction encoding
   - Directive behavior
   - Output format

The testing framework will use a modern C++ testing framework (Catch2 or Google Test) and include comprehensive coverage of all components.

## Timeline

| Phase | Duration | Key Milestones |
|-------|----------|---------------|
| Phase 1 | 4-6 weeks | Basic lexer, parser, symbol table |
| Phase 2 | 6-8 weeks | Code generation, directives, output |
| Phase 3 | 4-6 weeks | Optimization, macros, includes |

Total estimated duration: 14-20 weeks for a complete implementation.