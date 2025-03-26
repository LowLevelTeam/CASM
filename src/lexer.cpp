#include "lexer.h"
#include <cctype>

Lexer::Lexer(std::string_view source) : source(source), position(0), line(1), column(1) {}

Token Lexer::nextToken() {
    skipWhitespace();
    
    if (position >= source.size()) {
        return createToken(TokenType::END_OF_FILE, "");
    }
    
    char c = source[position];
    
    // Handle comments
    if (c == ';') {
        return handleComment();
    }
    
    // Handle identifiers and keywords
    if (isalpha(c) || c == '_') {
        return handleIdentifier();
    }
    
    // Handle numbers
    if (isdigit(c) || (c == '0' && position + 1 < source.size() && 
                        (source[position + 1] == 'x' || source[position + 1] == 'b'))) {
        return handleNumber();
    }
    
    // Handle string literals
    if (c == '"') {
        return handleString();
    }
    
    // Handle single-character tokens
    TokenType type = TokenType::UNKNOWN;
    std::string value(1, c);
    
    switch (c) {
        case ':': type = TokenType::COLON; break;
        case ',': type = TokenType::COMMA; break;
        case '[': type = TokenType::LBRACKET; break;
        case ']': type = TokenType::RBRACKET; break;
        case '=': type = TokenType::EQUALS; break;
        case '+': type = TokenType::PLUS; break;
        case '-': type = TokenType::MINUS; break;
        case '*': type = TokenType::ASTERISK; break;
        case '(': type = TokenType::LPAREN; break;
        case ')': type = TokenType::RPAREN; break;
        case '\n': 
            type = TokenType::NEWLINE; 
            line++;
            column = 0;
            break;
        default: type = TokenType::UNKNOWN; break;
    }
    
    position++;
    column++;
    return createToken(type, value);
}

Token Lexer::peekToken() {
    size_t oldPos = position;
    size_t oldLine = line;
    size_t oldColumn = column;
    
    Token token = nextToken();
    
    position = oldPos;
    line = oldLine;
    column = oldColumn;
    
    return token;
}

Token Lexer::createToken(TokenType type, const std::string& value) {
    return Token{type, value, line, column - value.length()};
}

void Lexer::skipWhitespace() {
    while (position < source.size() && (source[position] == ' ' || source[position] == '\t')) {
        position++;
        column++;
    }
}

Token Lexer::handleComment() {
    size_t start = position;
    while (position < source.size() && source[position] != '\n') {
        position++;
        column++;
    }
    std::string value = std::string(source.substr(start, position - start));
    return createToken(TokenType::COMMENT, value);
}

Token Lexer::handleIdentifier() {
    size_t start = position;
    while (position < source.size() && 
           (isalnum(source[position]) || source[position] == '_' || source[position] == '.')) {
        position++;
        column++;
    }
    
    std::string value = std::string(source.substr(start, position - start));
    TokenType type = TokenType::IDENTIFIER;
    
    // Special handling for specific identifiers
    if (isDirective(value)) {
        type = TokenType::DIRECTIVE;
    } else if (isInstruction(value)) {
        type = TokenType::INSTRUCTION;
    } else if (isRegister(value)) {
        type = TokenType::REGISTER;
    } else if (isType(value)) {
        type = TokenType::TYPE;
    } else if (value.find("ABICTL_") == 0 || 
               value.find("BRANCH_") == 0 || 
               value.find("MEMORY_") == 0 ||
               value == "platform_default" || 
               value == "CONST" || 
               value == "VOLATILE" || 
               value == "IMM") {
        // Handle special identifiers that might be part of type expressions
        type = TokenType::IDENTIFIER;
    }
    
    return createToken(type, value);
}

Token Lexer::handleNumber() {
    size_t start = position;
    bool isHex = false;
    bool isBinary = false;
    
    // Check for hex or binary prefix
    if (position + 1 < source.size() && source[position] == '0') {
        if (source[position + 1] == 'x' || source[position + 1] == 'X') {
            isHex = true;
            position += 2;
            column += 2;
        } else if (source[position + 1] == 'b' || source[position + 1] == 'B') {
            isBinary = true;
            position += 2;
            column += 2;
        }
    }
    
    // Parse the digits
    while (position < source.size()) {
        char c = source[position];
        if (isHex && isxdigit(c)) {
            position++;
            column++;
        } else if (isBinary && (c == '0' || c == '1')) {
            position++;
            column++;
        } else if (!isHex && !isBinary && (isdigit(c) || c == '.')) {
            position++;
            column++;
        } else {
            break;
        }
    }
    
    std::string value = std::string(source.substr(start, position - start));
    return createToken(TokenType::NUMBER, value);
}

Token Lexer::handleString() {
    size_t start = position;
    position++;  // Skip opening quote
    column++;
    
    while (position < source.size() && source[position] != '"') {
        // Handle escape sequences
        if (source[position] == '\\' && position + 1 < source.size()) {
            position += 2;
            column += 2;
        } else {
            position++;
            column++;
        }
    }
    
    if (position < source.size()) {
        position++;  // Skip closing quote
        column++;
    }
    
    std::string value = std::string(source.substr(start, position - start));
    return createToken(TokenType::STRING, value);
}

bool Lexer::isDirective(const std::string& value) {
    static const std::unordered_set<std::string> directives = {
        "PROC", "ARCH", "MODE", "ALIGN", "SECTION", "DATA", "IF", "ELIF",
        "ELSE", "ENDIF", "ABI", "EXIT", "EXTERN", "GLOBAL", "INCLUDE", "VERSION",
        "PARAMS", "RETS", "CALLER", "CALLEE", "SALLIGN", "RZONE"
    };
    return directives.find(value) != directives.end();
}

bool Lexer::isInstruction(const std::string& value) {
    static const std::unordered_set<std::string> instructions = {
        "MOV", "ADD", "SUB", "MUL", "DIV", "CMP", "JMP", "BR", "CALL", "RET",
        "PUSH", "POP", "AND", "OR", "XOR", "NOT", "SHL", "SHR", "LEA", "NOP",
        "SCOPEE", "SCOPEL", "VAR", "SYM", "MEMCPY", "MEMSET", "TEST"
    };
    
    std::string baseInst = value;
    size_t underscorePos = value.find('_');
    if (underscorePos != std::string::npos) {
        baseInst = value.substr(0, underscorePos);
    }
    
    return instructions.find(baseInst) != instructions.end();
}

bool Lexer::isRegister(const std::string& value) {
    static const std::unordered_set<std::string> registers = {
        "RAX", "RBX", "RCX", "RDX", "RSI", "RDI", "RBP", "RSP",
        "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15"
    };
    return registers.find(value) != registers.end();
}

bool Lexer::isType(const std::string& value) {
    return value.compare(0, 5, "TYPE_") == 0;
}