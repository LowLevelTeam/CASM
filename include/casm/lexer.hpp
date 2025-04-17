/**
* @file lexer.hpp
* @brief Lexer for CASM assembly language
*/

#pragma once
#include <coil/coil.hpp>
#include <string>
#include <vector>

namespace casm {

/**
* @brief Token types for CASM lexical analysis
*/
enum class TokenType {
  EndOfFile,      ///< End of file
  Newline,        ///< Newline
  Identifier,     ///< Identifier/label/opcode
  Integer,        ///< Integer literal
  Float,          ///< Float literal
  String,         ///< String literal
  Colon,          ///< :
  Comma,          ///< ,
  Period,         ///< .
  LBracket,       ///< [
  RBracket,       ///< ]
  Plus,           ///< +
  Minus,          ///< -
  Comment,        ///< Comment
  Directive,      ///< .directive
  Register,       ///< rN register
  ValueType,      ///< .type suffix
  OpCode,         ///< Instruction opcode
  Label           ///< Label definition
};

/**
* @brief Token representation for CASM lexical analysis
*/
struct Token {
  TokenType type;     ///< Type of token
  std::string value;  ///< Token string value
  size_t line;        ///< Line number
  size_t column;      ///< Column number
};

/**
* @brief Lexer for CASM assembly language
* 
* Breaks down CASM source code into tokens for parsing.
*/
class Lexer {
public:
  /**
  * @brief Initialize lexer with source code
  * @param source CASM source code
  */
  explicit Lexer(const std::string& source);
  
  /**
  * @brief Tokenize all source code
  * @return Vector of tokens
  */
  std::vector<Token> tokenize();
  
  /**
  * @brief Get the next token
  * @return Next token
  */
  Token nextToken();
  
  /**
  * @brief Check if at end of file
  * @return True if at end of file
  */
  bool isAtEnd() const;
  
private:
  std::string source;      ///< Source code
  size_t start;            ///< Start of current token
  size_t current;          ///< Current position in source
  size_t line;             ///< Current line
  size_t column;           ///< Current column
  
  /**
  * @brief Scan and return the next token
  * @return Scanned token
  */
  Token scanToken();
  
  /**
  * @brief Scan an identifier
  * @return Identifier token
  */
  Token scanIdentifier();
  
  /**
  * @brief Scan a number
  * @return Number token
  */
  Token scanNumber();
  
  /**
  * @brief Scan a string
  * @return String token
  */
  Token scanString();
  
  /**
  * @brief Scan a comment
  * @return Comment token
  */
  Token scanComment();
  
  /**
  * @brief Scan a directive
  * @return Directive token
  */
  Token scanDirective();
  
  /**
  * @brief Create a token
  * @param type Token type
  * @return Created token
  */
  Token makeToken(TokenType type);
  
  /**
  * @brief Advance current position
  * @return Current character
  */
  char advance();
  
  /**
  * @brief Peek at current character
  * @return Current character
  */
  char peek() const;
  
  /**
  * @brief Peek at next character
  * @return Next character
  */
  char peekNext() const;
  
  /**
  * @brief Check if remaining source matches expected string
  * @param expected Expected string
  * @return True if matches
  */
  bool match(const std::string& expected);
  
  /**
  * @brief Skip whitespace characters
  */
  void skipWhitespace();
};

} // namespace casm