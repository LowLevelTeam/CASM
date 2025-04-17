#pragma once
#include "casm/token.hpp"
#include <string>
#include <vector>
#include <istream>

namespace casm {

/**
 * @brief Lexical analyzer for CASM that converts source code into tokens
 */
class Lexer {
public:
  /**
   * @brief Construct a lexer from a filename and input stream
   * @param filename Name of the source file (for error reporting)
   * @param input Input stream containing source code
   */
  Lexer(std::string filename, std::istream& input);
  
  /**
   * @brief Construct a lexer from a filename and source string
   * @param filename Name of the source file (for error reporting)
   * @param source String containing source code
   */
  Lexer(std::string filename, const std::string& source);
  
  /**
   * @brief Tokenize the entire input and return all tokens
   * @return Vector of all tokens in the input
   */
  std::vector<Token> tokenize();
  
  /**
   * @brief Get the next token from the input
   * @return The next token
   */
  Token nextToken();
  
  /**
   * @brief Peek at the next token without consuming it
   * @return The next token without advancing
   */
  Token peekToken();
  
private:
  std::string m_filename;              // Source filename
  std::string m_source;                // Source text
  size_t m_position = 0;               // Current position in source
  size_t m_line = 1;                   // Current line number
  size_t m_column = 1;                 // Current column number
  std::vector<Token> m_tokenBuffer;    // Buffer for peeked tokens
  
  /**
   * @brief Skip whitespace in the input
   */
  void skipWhitespace();
  
  /**
   * @brief Get the current source location
   * @return Current source location
   */
  SourceLocation currentLocation() const;
  
  /**
   * @brief Advance the current position and update line/column
   * @param count Number of characters to advance
   */
  void advance(size_t count = 1);
  
  /**
   * @brief Check if we've reached the end of input
   * @return True if at end of input
   */
  bool isAtEnd() const;
  
  /**
   * @brief Get the current character
   * @return Current character or '\0' if at end
   */
  char current() const;
  
  /**
   * @brief Peek ahead at a future character
   * @param offset Offset from current position
   * @return Character at offset or '\0' if beyond end
   */
  char peek(size_t offset = 0) const;
  
  /**
   * @brief Try to match and consume a string
   * @param str String to match
   * @return True if string was matched and consumed
   */
  bool match(const std::string& str);
  
  /**
   * @brief Try to match and consume a character
   * @param c Character to match
   * @return True if character was matched and consumed
   */
  bool match(char c);
  
  // Token recognizers
  Token scanToken();
  Token scanLabel();
  Token scanDirective();
  Token scanInstruction();
  Token scanRegister();
  Token scanImmediate();
  Token scanMemoryRef();
  Token scanLabelRef();
  Token scanParameter();
  Token scanComment();
};

} // namespace casm