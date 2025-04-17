/**
 * @file assembler.hpp
 * @brief CASM assembler interface
 */

#pragma once
#include "casm/types.hpp"
#include "casm/parser.hpp"
#include "casm/codegen.hpp"
#include "coil/obj.hpp"
#include <string>
#include <vector>

namespace casm {

/**
 * @brief CASM assembler configuration
 */
struct AssemblerConfig {
  bool verbose;      ///< Enable verbose output
  bool debug;        ///< Enable debug output
  
  AssemblerConfig() : verbose(false), debug(false) {}
};

/**
 * @brief CASM assembler class
 * 
 * Main assembler interface that coordinates parsing and code generation.
 */
class Assembler {
public:
  /**
   * @brief Constructor
   * @param config Assembler configuration
   */
  explicit Assembler(const AssemblerConfig& config = AssemblerConfig());
  
  /**
   * @brief Destructor
   */
  ~Assembler();
  
  /**
   * @brief Assemble source code
   * @param source Source code
   * @param source_name Source name (for error reporting)
   * @param obj Output object file
   * @return Result of assembly
   */
  Result assemble(const std::string& source, const std::string& source_name, coil::Object& obj);
  
  /**
   * @brief Assemble source file
   * @param filename Source filename
   * @param obj Output object file
   * @return Result of assembly
   */
  Result assembleFile(const std::string& filename, coil::Object& obj);
  
  /**
   * @brief Save object file
   * @param obj Object file
   * @param filename Output filename
   * @return Result of save
   */
  Result saveObjectFile(const coil::Object& obj, const std::string& filename);
  
  /**
   * @brief Get the last error message
   * @return Last error message
   */
  const std::string& getLastError() const;
  
  /**
   * @brief Set error callback
   * @param callback Error callback function
   * @param user_data User data to pass to callback
   */
  void setErrorCallback(void (*callback)(const char* message, size_t line, const char* source, void* user_data), void* user_data);

private:
  /**
   * @brief Error callback for parser
   * @param message Error message
   * @param line Line number
   * @param column Column number
   * @param user_data User data
   */
  static void parserErrorCallback(const char* message, size_t line, size_t column, void* user_data);
  
  /**
   * @brief Error callback for code generator
   * @param message Error message
   * @param line Line number
   * @param user_data User data
   */
  static void codegenErrorCallback(const char* message, size_t line, void* user_data);
  
  /**
   * @brief Report an error
   * @param message Error message
   * @param line Line number
   */
  void reportError(const char* message, size_t line);
  
  /**
   * @brief Format an error message
   * @param format Format string
   * @param ... Arguments
   */
  void formatError(const char* format, ...);
  
  // Parser instance
  Parser parser;
  
  // Code generator instance
  CodeGenerator codegen;
  
  // Assembler configuration
  AssemblerConfig config;
  
  // Current source name
  std::string current_source;
  
  // Error callback function
  void (*error_callback)(const char* message, size_t line, const char* source, void* user_data);
  
  // User data for error callback
  void* error_user_data;
  
  // Last error message
  std::string last_error;
  
  // Error buffer for formatting
  char error_buffer[1024];
};

} // namespace casm