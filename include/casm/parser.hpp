/**
 * @file parser.hpp
 * @brief CASM parser interface
 */

#pragma once
#include "casm/types.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace casm {

/**
 * @brief CASM parser class
 * 
 * Parses CASM assembly code line by line into instructions and directives.
 */
class Parser {
public:
  /**
   * @brief Constructor
   */
  Parser();
  
  /**
   * @brief Destructor
   */
  ~Parser();
  
  /**
   * @brief Parse a single line of assembly code
   * @param line Line to parse
   * @param line_number Line number in source
   * @return Parsed line
   */
  Result parseLine(const std::string& line, size_t line_number, Line& parsed_line);
  
  /**
   * @brief Set error callback function
   * @param callback Error callback function
   * @param user_data User data to pass to callback
   */
  void setErrorCallback(void (*callback)(const char* message, size_t line, size_t column, void* user_data), void* user_data);
  
  /**
   * @brief Get the last error message
   * @return Last error message
   */
  const std::string& getLastError() const;

private:
  /**
   * @brief Tokenize a line of assembly code
   * @param line Line to tokenize
   * @param line_number Line number in source
   * @param tokens Output vector of tokens
   * @return Result of tokenization
   */
  Result tokenize(const std::string& line, size_t line_number, std::vector<Token>& tokens);
  
  /**
   * @brief Parse a label from tokens
   * @param tokens Tokens to parse
   * @param index Current token index (in/out)
   * @param label Output label
   * @return Result of parsing
   */
  Result parseLabel(const std::vector<Token>& tokens, size_t& index, std::string& label);
  
  /**
   * @brief Parse an instruction from tokens
   * @param tokens Tokens to parse
   * @param index Current token index (in/out)
   * @param instruction Output instruction
   * @return Result of parsing
   */
  Result parseInstruction(const std::vector<Token>& tokens, size_t& index, Instruction& instruction);
  
  /**
   * @brief Parse a directive from tokens
   * @param tokens Tokens to parse
   * @param index Current token index (in/out)
   * @param directive Output directive
   * @return Result of parsing
   */
  Result parseDirective(const std::vector<Token>& tokens, size_t& index, Directive& directive);
  
  /**
   * @brief Parse an operand from tokens
   * @param tokens Tokens to parse
   * @param index Current token index (in/out)
   * @param operand Output operand
   * @return Result of parsing
   */
  Result parseOperand(const std::vector<Token>& tokens, size_t& index, Operand& operand);
  
  /**
   * @brief Parse a register reference from tokens
   * @param tokens Tokens to parse
   * @param index Current token index (in/out)
   * @param reg Output register reference
   * @return Result of parsing
   */
  Result parseRegister(const std::vector<Token>& tokens, size_t& index, RegisterRef& reg);
  
  /**
   * @brief Parse a memory reference from tokens
   * @param tokens Tokens to parse
   * @param index Current token index (in/out)
   * @param mem Output memory reference
   * @return Result of parsing
   */
  Result parseMemory(const std::vector<Token>& tokens, size_t& index, MemoryRef& mem);
  
  /**
   * @brief Parse an immediate value from tokens
   * @param tokens Tokens to parse
   * @param index Current token index (in/out)
   * @param imm Output immediate value
   * @return Result of parsing
   */
  Result parseImmediate(const std::vector<Token>& tokens, size_t& index, Operand& imm);
  
  /**
   * @brief Report an error
   * @param message Error message
   * @param line Line number
   * @param column Column number
   */
  void reportError(const char* message, size_t line, size_t column);
  
  /**
   * @brief Format an error message
   * @param format Format string
   * @param ... Arguments
   */
  void formatError(const char* format, ...);
  
  // Maps opcode name to COIL opcode
  std::unordered_map<std::string, coil::Opcode> opcode_map;
  
  // Maps condition name to COIL condition flag
  std::unordered_map<std::string, coil::InstrFlag0> condition_map;
  
  // Maps type name to COIL ValueType
  std::unordered_map<std::string, coil::ValueType> type_map;
  
  // Error callback function
  void (*error_callback)(const char* message, size_t line, size_t column, void* user_data);
  
  // User data for error callback
  void* error_user_data;
  
  // Last error message
  std::string last_error;
  
  // Error buffer for formatting
  char error_buffer[1024];
};

} // namespace casm