#pragma once
#include "types.h"
#include <string>
#include <string_view>
#include <vector>
#include <unordered_set>

class Lexer {
public:
    Lexer(std::string_view source);
    Token nextToken();
    Token peekToken();

private:
    std::string_view source;
    size_t position;
    size_t line;
    size_t column;
    
    Token createToken(TokenType type, const std::string& value);
    void skipWhitespace();
    Token handleComment();
    Token handleIdentifier();
    Token handleNumber();
    Token handleString();
    
    bool isDirective(const std::string& value);
    bool isInstruction(const std::string& value);
    bool isRegister(const std::string& value);
    bool isType(const std::string& value);
};