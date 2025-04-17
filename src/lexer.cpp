/**
 * @file lexer.cpp
 * @brief Implementation of lexer for CASM assembly language
 */

#include "casm/lexer.hpp"
#include <cctype>
#include <unordered_map>

namespace casm {

// Constructor
Lexer::Lexer(const std::string& source)
  : source(source), start(0), current(0), line(1), column(1) {
}

// Check if at end of file
bool Lexer::isAtEnd() const {
  return current >= source.length();
}

// Tokenize all source code
std::vector<Token> Lexer::tokenize() {
  std::vector<Token> tokens;
  
  // Keep tokenizing until end of file
  while (!isAtEnd()) {
    start = current;
    Token token = nextToken();
    
    // Skip comment tokens
    if (token.type != TokenType::Comment) {
      tokens.push_back(token);
    }
    
    // Stop at end of file
    if (token.type == TokenType::EndOfFile) {
      break;
    }
  }
  
  return tokens;
}

// Get next token
Token Lexer::nextToken() {
  // Skip whitespace
  skipWhitespace();
  
  // Check if at end
  if (isAtEnd()) {
    return makeToken(TokenType::EndOfFile);
  }
  
  // Get current character
  char c = peek();
  start = current;
  
  // Advance to next character
  advance();
  
  // Check for newline
  if (c == '\n') {
    line++;
    column = 1;
    return makeToken(TokenType::Newline);
  }
  
  // Check for semicolon (comment)
  if (c == ';') {
    return scanComment();
  }
  
  // Check for special characters
  switch (c) {
    case ':': return makeToken(TokenType::Colon);
    case ',': return makeToken(TokenType::Comma);
    case '.': return scanDirective();
    case '[': return makeToken(TokenType::LBracket);
    case ']': return makeToken(TokenType::RBracket);
    case '+': return makeToken(TokenType::Plus);
    case '-': return makeToken(TokenType::Minus);
    case '"': return scanString();
  }
  
  // Check for register (r0-r31)
  if (c == 'r' && std::isdigit(peek())) {
    // Scan register number
    while (std::isdigit(peek())) {
      advance();
    }
    return makeToken(TokenType::Register);
  }
  
  // Check for number
  if (std::isdigit(c) || (c == '-' && std::isdigit(peek()))) {
    return scanNumber();
  }
  
  // Check for identifier
  if (std::isalpha(c) || c == '_') {
    return scanIdentifier();
  }
  
  // Unknown character
  Token token = makeToken(TokenType::EndOfFile);
  token.value = std::string(1, c);
  return token;
}

// Scan an identifier
Token Lexer::scanIdentifier() {
  // Keep scanning until end of identifier
  while (std::isalnum(peek()) || peek() == '_' || peek() == '.') {
    advance();
  }
  
  // Check for known opcodes or labels
  std::string value = source.substr(start, current - start);
  
  // Create token
  return makeToken(TokenType::Identifier);
}

// Scan a number
Token Lexer::scanNumber() {
  bool isFloat = false;
  
  // Handle different number formats
  if (peek() == 'x' && source[start] == '0') {
    // Hexadecimal
    advance();
    while (std::isxdigit(peek())) {
      advance();
    }
  } else if (peek() == 'b' && source[start] == '0') {
    // Binary
    advance();
    while (peek() == '0' || peek() == '1') {
      advance();
    }
  } else {
    // Decimal
    while (std::isdigit(peek())) {
      advance();
    }
    
    // Check for floating point
    if (peek() == '.' && std::isdigit(peekNext())) {
      isFloat = true;
      advance(); // Consume the '.'
      
      // Scan fraction part
      while (std::isdigit(peek())) {
        advance();
      }
    }
    
    // Check for exponent
    if ((peek() == 'e' || peek() == 'E') &&
        (std::isdigit(peekNext()) || 
         ((peekNext() == '+' || peekNext() == '-') && std::isdigit(source[current + 2])))) {
      
      isFloat = true;
      advance(); // Consume the 'e' or 'E'
      
      // Check for sign
      if (peek() == '+' || peek() == '-') {
        advance();
      }
      
      // Scan exponent part
      while (std::isdigit(peek())) {
        advance();
      }
    }
  }
  
  // Create token based on type
  return makeToken(isFloat ? TokenType::Float : TokenType::Integer);
}

// Scan a string
Token Lexer::scanString() {
  // Keep scanning until end of string or end of file
  while (peek() != '"' && !isAtEnd()) {
    if (peek() == '\n') {
      line++;
      column = 1;
    }
    advance();
  }
  
  // Check if string was closed
  if (isAtEnd()) {
    // Unterminated string
    Token token = makeToken(TokenType::EndOfFile);
    token.value = "Unterminated string";
    return token;
  }
  
  // Consume closing quote
  advance();
  
  // Create token
  return makeToken(TokenType::String);
}

// Scan a comment
Token Lexer::scanComment() {
  // Keep scanning until end of line or end of file
  while (peek() != '\n' && !isAtEnd()) {
    advance();
  }
  
  // Create token
  return makeToken(TokenType::Comment);
}

// Scan a directive
Token Lexer::scanDirective() {
  // Keep scanning until end of directive
  while (std::isalnum(peek()) || peek() == '_') {
    advance();
  }
  
  // Create token
  return makeToken(TokenType::Directive);
}

// Create a token
Token Lexer::makeToken(TokenType type) {
  Token token;
  token.type = type;
  token.value = source.substr(start, current - start);
  token.line = line;
  token.column = column - (current - start);
  return token;
}

// Advance current position
char Lexer::advance() {
  column++;
  return source[current++];
}

// Peek at current character
char Lexer::peek() const {
  if (isAtEnd()) {
    return '\0';
  }
  return source[current];
}

// Peek at next character
char Lexer::peekNext() const {
  if (current + 1 >= source.length()) {
    return '\0';
  }
  return source[current + 1];
}

// Check if remaining source matches expected string
bool Lexer::match(const std::string& expected) {
  if (current + expected.length() > source.length()) {
    return false;
  }
  
  for (size_t i = 0; i < expected.length(); i++) {
    if (source[current + i] != expected[i]) {
      return false;
    }
  }
  
  current += expected.length();
  column += expected.length();
  return true;
}

// Skip whitespace characters
void Lexer::skipWhitespace() {
  while (!isAtEnd()) {
    char c = peek();
    
    // Skip spaces and tabs
    if (c == ' ' || c == '\t' || c == '\r') {
      advance();
    } else {
      break;
    }
  }
}

} // namespace casm