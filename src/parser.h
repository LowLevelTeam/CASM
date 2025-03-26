#pragma once
#include "types.h"
#include "lexer.h"
#include "symbol_table.h"
#include "code_generator.h"
#include <string>
#include <vector>

class Parser {
public:
    Parser(Lexer& lexer, SymbolTable& symbols, CodeGenerator& codeGen);
    bool parseFile();

private:
    Lexer& lexer;
    SymbolTable& symbols;
    CodeGenerator& codeGen;
    Token currentToken;
    
    void advance();
    bool expect(TokenType type);
    bool parseStatement();
    bool parseLabel();
    bool parseDirective();
    bool parseInstruction();
    bool parseOperand(Operand& operand);
    bool parseType(Type& type);
    bool parseImmediateOperand(Operand& operand);
    bool parseRegisterOperand(Operand& operand);
    bool parseIdentifierOperand(Operand& operand);
    bool parseMemoryOperand(Operand& operand);
    
    uint8_t getOpcode(const std::string& instruction);
    TypeID parseTypeID(const std::string& typeStr);
    void debugPrintType(const Type& type);  // Add this line
};