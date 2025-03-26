#include "parser.h"
#include <iostream>
#include <sstream>

Parser::Parser(Lexer& lexer, SymbolTable& symbols, CodeGenerator& codeGen)
    : lexer(lexer), symbols(symbols), codeGen(codeGen) {
    advance();
}

void Parser::advance() {
    currentToken = lexer.nextToken();
    // Skip comments
    while (currentToken.type == TokenType::COMMENT) {
        currentToken = lexer.nextToken();
    }
}

bool Parser::expect(TokenType type) {
    if (currentToken.type == type) {
        advance();
        return true;
    }
    
    std::cerr << "Error: Expected " << static_cast<int>(type) 
              << " but got " << static_cast<int>(currentToken.type)
              << " at line " << currentToken.line << ", column " << currentToken.column
              << std::endl;
    return false;
}

bool Parser::parseFile() {
    while (currentToken.type != TokenType::END_OF_FILE) {
        if (!parseStatement()) {
            return false;
        }
        
        // Skip newlines between statements
        while (currentToken.type == TokenType::NEWLINE) {
            advance();
        }
    }
    
    return true;
}

bool Parser::parseStatement() {
    // Skip empty lines
    if (currentToken.type == TokenType::NEWLINE) {
        advance();
        return true;
    }
    
    if (currentToken.type == TokenType::END_OF_FILE) {
        return true;
    }
    
    // Check for label
    if (currentToken.type == TokenType::IDENTIFIER && 
        lexer.peekToken().type == TokenType::COLON) {
        return parseLabel();
    }
    
    // Check for directive
    if (currentToken.type == TokenType::DIRECTIVE) {
        return parseDirective();
    }
    
    // Check for instruction
    if (currentToken.type == TokenType::INSTRUCTION) {
        return parseInstruction();
    }
    
    std::cerr << "Error: Unexpected token " << currentToken.value 
              << " at line " << currentToken.line << ", column " << currentToken.column
              << std::endl;
    advance();  // Skip the unexpected token
    return false;
}

bool Parser::parseLabel() {
    std::string name = currentToken.value;
    advance();  // Consume the identifier
    
    if (!expect(TokenType::COLON)) {
        return false;
    }
    
    // Define the label at the current position
    symbols.defineLabel(name, codeGen.getCurrentOffset());
    return true;
}

bool Parser::parseDirective() {
    std::string directive = currentToken.value;
    advance();  // Consume the directive
    
    // Implementation for directives like PROC, ARCH, SECTION, etc.
    // For simplicity, we'll skip the details of each directive
    
    // Skip the rest of the line for now
    while (currentToken.type != TokenType::NEWLINE && 
           currentToken.type != TokenType::END_OF_FILE) {
        advance();
    }
    
    return true;
}

bool Parser::parseInstruction() {
    std::string instruction = currentToken.value;
    advance();  // Consume the instruction
    
    std::vector<Operand> operands;
    
    // Parse operands
    while (currentToken.type != TokenType::NEWLINE && 
           currentToken.type != TokenType::END_OF_FILE) {
        
        Operand operand;
        if (!parseOperand(operand)) {
            return false;
        }
        
        operands.push_back(operand);
        
        if (currentToken.type != TokenType::COMMA) {
            break;
        }
        
        advance();  // Consume the comma
    }
    
    // Generate the instruction
    uint8_t opcode = getOpcode(instruction);
    Instruction inst{opcode, operands};
    
    codeGen.generateInstruction(inst);
    
    return true;
}

bool Parser::parseOperand(Operand& operand) {
    // Based on current token type, parse different kinds of operands
    if (currentToken.type == TokenType::NUMBER) {
        return parseImmediateOperand(operand);
    } else if (currentToken.type == TokenType::REGISTER) {
        return parseRegisterOperand(operand);
    } else if (currentToken.type == TokenType::IDENTIFIER) {
        return parseIdentifierOperand(operand);
    } else if (currentToken.type == TokenType::TYPE) {
        if (!parseType(operand.type)) {
            return false;
        }
        return true;
    } else if (currentToken.type == TokenType::LBRACKET) {
        return parseMemoryOperand(operand);
    }
    
    std::cerr << "Error: Unexpected operand type " << static_cast<int>(currentToken.type) 
              << " at line " << currentToken.line << ", column " << currentToken.column
              << std::endl;
    return false;
}

bool Parser::parseType(Type& type) {
    if (currentToken.type != TokenType::TYPE) {
        std::cerr << "Error: Expected type but got " << currentToken.value 
                  << " at line " << currentToken.line << ", column " << currentToken.column
                  << std::endl;
        return false;
    }
    
    std::string typeStr = currentToken.value;
    advance();  // Consume the type
    
    type.id = parseTypeID(typeStr);
    type.ext = 0;  // Default no extensions
    
    // Check for type extensions after plus sign
    if (currentToken.type == TokenType::PLUS) {
        advance();  // Consume the plus
        
        if (currentToken.type == TokenType::IDENTIFIER) {
            // Parse extension like CONST, VOLATILE, etc.
            if (currentToken.value == "CONST") {
                type.ext |= static_cast<uint8_t>(TypeExt::CONST);
            } else if (currentToken.value == "VOLATILE") {
                type.ext |= static_cast<uint8_t>(TypeExt::VOLATILE);
            } else if (currentToken.value == "IMM") {
                type.ext |= static_cast<uint8_t>(TypeExt::IMM);
            }
            // Add more extensions as needed
            
            advance();  // Consume the extension
        }
    }
    
    // Handle type data after equals sign - this can be nested
    if (currentToken.type == TokenType::EQUALS) {
        advance();  // Consume the equals
        
        // Parse type data like register ID, pointed type, etc.
        if (currentToken.type == TokenType::REGISTER) {
            // Handle register type data
            for (char c : currentToken.value) {
                type.type_data.push_back(static_cast<uint8_t>(c));
            }
            advance();  // Consume the register
        } else if (currentToken.type == TokenType::TYPE) {
            // Handle pointed type or array element type
            Type subType;
            if (!parseType(subType)) {
                return false;
            }
            
            // Store the subtype ID in type data
            type.type_data.push_back(static_cast<uint8_t>(subType.id));
            type.type_data.push_back(static_cast<uint8_t>(static_cast<uint16_t>(subType.id) >> 8));
        } else if (currentToken.type == TokenType::IDENTIFIER) {
            std::string identifier = currentToken.value;
            for (char c : identifier) {
                type.type_data.push_back(static_cast<uint8_t>(c));
            }
            advance();  // Consume the identifier
            
            // Check for another level (e.g., ABICTL_STANDARD=platform_default)
            if (currentToken.type == TokenType::EQUALS) {
                advance();  // Consume the equals
                
                if (currentToken.type == TokenType::IDENTIFIER) {
                    // Add a separator between parts
                    type.type_data.push_back(static_cast<uint8_t>('='));
                    
                    std::string secondId = currentToken.value;
                    for (char c : secondId) {
                        type.type_data.push_back(static_cast<uint8_t>(c));
                    }
                    advance();  // Consume the second identifier
                }
            }
        }
    }
    
    return true;
}

bool Parser::parseImmediateOperand(Operand& operand) {
    std::string numStr = currentToken.value;
    advance();  // Consume the number
    
    // Determine the number type (int, float, hex, etc.)
    // and convert to binary representation
    
    // For simplicity, we'll just use TYPE_INT32 for all numbers
    operand.type.id = TypeID::TYPE_INT32;
    operand.type.ext = static_cast<uint8_t>(TypeExt::IMM);
    
    // Convert the number to binary
    int32_t value = std::stoi(numStr, nullptr, 0);
    
    // Store the value in little-endian format
    operand.value.push_back(value & 0xFF);
    operand.value.push_back((value >> 8) & 0xFF);
    operand.value.push_back((value >> 16) & 0xFF);
    operand.value.push_back((value >> 24) & 0xFF);
    
    return true;
}

bool Parser::parseRegisterOperand(Operand& operand) {
    std::string regName = currentToken.value;
    advance();  // Consume the register
    
    // Set up a register type
    operand.type.id = TypeID::TYPE_RGP;
    operand.type.ext = 0;
    
    // For simplicity, we'll just use a fixed register ID
    // In a real implementation, we'd map register names to IDs
    uint8_t regID = 0;  // RAX for example
    
    operand.value.push_back(regID);
    
    return true;
}

bool Parser::parseIdentifierOperand(Operand& operand) {
    std::string name = currentToken.value;
    advance();  // Consume the identifier
    
    Symbol symbol;
    if (symbols.lookupSymbol(name, symbol)) {
        // Symbol already defined
        operand.type.id = TypeID::TYPE_SYM;
        operand.type.ext = 0;
        
        // Store symbol ID
        operand.value.push_back(symbol.value & 0xFF);
        operand.value.push_back((symbol.value >> 8) & 0xFF);
        operand.value.push_back((symbol.value >> 16) & 0xFF);
        operand.value.push_back((symbol.value >> 24) & 0xFF);
    } else {
        // Forward reference to a symbol
        operand.type.id = TypeID::TYPE_SYM;
        operand.type.ext = 0;
        
        // Store symbol name for later resolution
        for (char c : name) {
            operand.value.push_back(static_cast<uint8_t>(c));
        }
        operand.value.push_back(0);  // Null terminator
    }
    
    return true;
}

bool Parser::parseMemoryOperand(Operand& operand) {
    if (!expect(TokenType::LBRACKET)) {
        return false;
    }
    
    // Parse memory address expression
    // This is a simplified implementation
    
    // For now, we'll just expect an identifier as the base
    if (currentToken.type != TokenType::IDENTIFIER) {
        std::cerr << "Error: Expected identifier in memory reference at line " 
                  << currentToken.line << ", column " << currentToken.column
                  << std::endl;
        return false;
    }
    
    std::string baseName = currentToken.value;
    advance();  // Consume the base
    
    operand.type.id = TypeID::TYPE_PTR;
    operand.type.ext = 0;
    
    // Store the base name
    for (char c : baseName) {
        operand.value.push_back(static_cast<uint8_t>(c));
    }
    operand.value.push_back(0);  // Null terminator
    
    // Skip any additional memory addressing components
    while (currentToken.type != TokenType::RBRACKET && 
           currentToken.type != TokenType::END_OF_FILE) {
        advance();
    }
    
    if (!expect(TokenType::RBRACKET)) {
        return false;
    }
    
    return true;
}

uint8_t Parser::getOpcode(const std::string& instruction) {
    static const std::unordered_map<std::string, uint8_t> opcodes = {
        {"NOP", 0x00},
        {"SYM", 0x01},
        {"BR", 0x02},
        {"CALL", 0x03},
        {"RET", 0x04},
        {"CMP", 0x05},
        {"TEST", 0x06},
        {"JMP", 0x07},
        {"MOV", 0x10},
        {"PUSH", 0x11},
        {"POP", 0x12},
        {"LEA", 0x13},
        {"SCOPEE", 0x14},
        {"SCOPEL", 0x15},
        {"VAR", 0x16},
        {"AND", 0x50},
        {"OR", 0x51},
        {"XOR", 0x52},
        {"NOT", 0x53},
        {"ADD", 0x60},
        {"SUB", 0x61},
        {"MUL", 0x62},
        {"DIV", 0x63}
    };
    
    auto it = opcodes.find(instruction);
    if (it != opcodes.end()) {
        return it->second;
    }
    
    // Handle conditional instructions
    size_t underscorePos = instruction.find('_');
    if (underscorePos != std::string::npos) {
        std::string baseInst = instruction.substr(0, underscorePos);
        it = opcodes.find(baseInst);
        if (it != opcodes.end()) {
            return it->second;
        }
    }
    
    std::cerr << "Warning: Unknown instruction " << instruction << std::endl;
    return 0xFF;  // Unknown instruction
}

TypeID Parser::parseTypeID(const std::string& typeStr) {
    static const std::unordered_map<std::string, TypeID> types = {
        {"TYPE_INT8", TypeID::TYPE_INT8},          // 0x01
        {"TYPE_INT16", TypeID::TYPE_INT16},        // 0x02
        {"TYPE_INT32", TypeID::TYPE_INT32},        // 0x03
        {"TYPE_INT64", TypeID::TYPE_INT64},        // 0x04
        {"TYPE_INT128", TypeID::TYPE_INT128},      // 0x05
        {"TYPE_UNT8", TypeID::TYPE_UNT8},          // 0x10
        {"TYPE_UNT16", TypeID::TYPE_UNT16},        // 0x12
        {"TYPE_UNT32", TypeID::TYPE_UNT32},        // 0x13
        {"TYPE_UNT64", TypeID::TYPE_UNT64},        // 0x14
        {"TYPE_UNT128", TypeID::TYPE_UNT128},      // 0x15
        {"TYPE_FP32", TypeID::TYPE_FP32},          // 0x25
        {"TYPE_FP64", TypeID::TYPE_FP64},          // 0x26
        {"TYPE_V128", TypeID::TYPE_V128},          // 0x30
        {"TYPE_V256", TypeID::TYPE_V256},          // 0x31
        {"TYPE_V512", TypeID::TYPE_V512},          // 0x32
        {"TYPE_BIT", TypeID::TYPE_BIT},            // 0x40
        {"TYPE_VAR", TypeID::TYPE_VAR},            // 0x90
        {"TYPE_SYM", TypeID::TYPE_SYM},            // 0x91
        {"TYPE_RGP", TypeID::TYPE_RGP},            // 0x92
        {"TYPE_INT", TypeID::TYPE_INT},            // 0xA0
        {"TYPE_UNT", TypeID::TYPE_UNT},            // 0xA1
        {"TYPE_FP", TypeID::TYPE_FP},              // 0xA2
        {"TYPE_PTR", TypeID::TYPE_PTR},            // 0xA6
        {"TYPE_STRUCT", TypeID::TYPE_STRUCT},      // 0xD0
        {"TYPE_ARRAY", TypeID::TYPE_ARRAY},        // 0xD3
        {"TYPE_PARAM5", TypeID::TYPE_PARAM5},      // 0xF0
        {"TYPE_PARAM0", TypeID::TYPE_PARAM0},      // 0xFE
        {"TYPE_ABICTL", TypeID::TYPE_ABICTL},      // 0xF8
        {"TYPE_VOID", TypeID::TYPE_VOID}           // 0xFF
    };
    
    auto it = types.find(typeStr);
    if (it != types.end()) {
        return it->second;
    }
    
    std::cerr << "Warning: Unknown type " << typeStr << std::endl;
    return TypeID::TYPE_VOID;  // Default to void for unknown types
}

void Parser::debugPrintType(const Type& type) {
    std::cout << "Type: id=0x" << std::hex << static_cast<int>(static_cast<uint16_t>(type.id)) 
              << ", ext=0x" << static_cast<int>(type.ext) << std::dec << std::endl;
    
    std::cout << "Type data (" << type.type_data.size() << " bytes):";
    for (const auto& byte : type.type_data) {
        std::cout << " " << std::hex << static_cast<int>(byte);
    }
    std::cout << std::dec << std::endl;
}