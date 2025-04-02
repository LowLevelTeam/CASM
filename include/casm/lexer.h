#ifndef CASM_LEXER_H
#define CASM_LEXER_H

#include "token.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace casm {

/**
* Lexer class for tokenizing CASM source code
*/
class Lexer {
public:
  /**
    * Constructor
    * @param source The source code to tokenize
    * @param filename Name of the source file for error reporting
    */
  Lexer(std::string source, std::string filename);
  
  /**
    * Get the next token from the source
    * @return The next token
    */
  Token getNextToken();
  
  /**
    * Get all tokens from the source
    * @return Vector of all tokens
    */
  std::vector<Token> tokenize();
  
  /**
    * Get the filename
    * @return Source filename
    */
  const std::string& getFilename() const;
  
  /**
    * Get current line number
    * @return Current line number
    */
  unsigned int getLine() const;
  
  /**
    * Get current column number
    * @return Current column number
    */
  unsigned int getColumn() const;
  
private:
  std::string source_;
  std::string filename_;
  size_t position_;
  unsigned int line_;
  unsigned int column_;
  
  static std::unordered_map<std::string, TokenType> keywords_;
  static std::unordered_map<std::string, TokenType> directives_;
  static std::unordered_map<std::string, TokenType> instructions_;
  static std::unordered_map<std::string, TokenType> registers_;
  
  char peek() const;
  char peekNext() const;
  char advance();
  bool match(char expected);
  
  void skipWhitespace();
  void skipComment();
  
  Token makeToken(TokenType type);
  Token makeToken(TokenType type, const std::string& lexeme);
  template<typename T> Token makeToken(TokenType type, T value, const std::string& lexeme);
  
  Token scanToken();
  Token scanIdentifier();
  Token scanNumber();
  Token scanString();
  Token scanVarID();
  
  bool isAtEnd() const;
  bool isDigit(char c) const;
  bool isAlpha(char c) const;
  bool isAlphaNumeric(char c) const;
  
  static bool initializeKeywordMaps();
  static bool keywordMapsInitialized_;
};

} // namespace casm

#endif // CASM_LEXER_H