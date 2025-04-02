#ifndef CASM_TOKEN_H
#define CASM_TOKEN_H

#include <string>
#include <variant>

namespace casm {

/**
* Enumeration of token types
*/
enum class TokenType {
  // Special tokens
  END_OF_FILE = 0,
  INVALID = -1,
  
  // Literals
  INTEGER_LITERAL = 1,
  FLOAT_LITERAL,
  STRING_LITERAL,
  REGISTER_NAME,
  
  // Punctuation
  COLON,          // :
  COMMA,          // ,
  SEMICOLON,      // ;
  OPEN_PAREN,     // (
  CLOSE_PAREN,    // )
  OPEN_BRACKET,   // [
  CLOSE_BRACKET,  // ]
  PLUS,           // +
  MINUS,          // -
  ASTERISK,       // *
  EQUAL,          // =
  PIPE,           // |
  
  // Type tokens
  TYPE_SPECIFIER,
  
  // Directives and instructions
  DIRECTIVE,
  INSTRUCTION,
  LABEL,
  IDENTIFIER,
  
  // Labels
  VAR_ID          // #n (variable ID)
};

/**
* Token value variants
*/
using TokenValue = std::variant<
  std::monostate,
  int64_t,
  double,
  std::string
>;

/**
* Token structure
*/
struct Token {
  TokenType type;
  TokenValue value;
  std::string lexeme;
  unsigned int line;
  unsigned int column;
  
  Token() : type(TokenType::INVALID), line(0), column(0) {}
  
  Token(TokenType type, unsigned int line, unsigned int column) 
      : type(type), line(line), column(column) {}
  
  Token(TokenType type, std::string lexeme, unsigned int line, unsigned int column)
      : type(type), lexeme(std::move(lexeme)), line(line), column(column) {}
  
  template<typename T>
  Token(TokenType type, T value, std::string lexeme, unsigned int line, unsigned int column)
      : type(type), value(std::move(value)), lexeme(std::move(lexeme)), line(line), column(column) {}
  
  const char *__print() {
    switch (this->type) {
      case TokenType::END_OF_FILE: return "END_OF_FILE";
      case TokenType::INVALID: return "INVALID";
      case TokenType::INTEGER_LITERAL: return "INTEGER_LITERAL";
      case TokenType::FLOAT_LITERAL: return "FLOAT_LITERAL";
      case TokenType::STRING_LITERAL: return "STRING_LITERAL";
      case TokenType::REGISTER_NAME: return "REGISTER_NAME";      
      case TokenType::COLON: return "COLON";
      case TokenType::COMMA: return "COMMA";
      case TokenType::SEMICOLON: return "SEMICOLON";
      case TokenType::OPEN_PAREN: return "OPEN_PAREN";
      case TokenType::CLOSE_PAREN: return "CLOSE_PAREN";
      case TokenType::OPEN_BRACKET: return "OPEN_BRACKET";
      case TokenType::CLOSE_BRACKET: return "CLOSE_BRACKET";
      case TokenType::PLUS: return "PLUS";
      case TokenType::MINUS: return "MINUS";
      case TokenType::ASTERISK: return "ASTERISK";
      case TokenType::EQUAL: return "EQUAL";
      case TokenType::PIPE: return "PIPE";
      case TokenType::TYPE_SPECIFIER: return "TYPE_SPECIFIER";
      case TokenType::DIRECTIVE: return "DIRECTIVE";
      case TokenType::INSTRUCTION: return "INSTRUCTION";
      case TokenType::LABEL: return "LABEL";
      case TokenType::IDENTIFIER: return "IDENTIFIER";
      case TokenType::VAR_ID: return "VAR_ID";
    }
    return "NOT A TYPE";
  }

  bool isValid() const {
      return type != TokenType::INVALID;
  }
  
  bool isEof() const {
      return type == TokenType::END_OF_FILE;
  }
  
  template<typename T>
  T getValue() const {
      return std::get<T>(value);
  }
};

} // namespace casm

#endif // CASM_TOKEN_H