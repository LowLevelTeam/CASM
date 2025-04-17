/**
 * @file codegen.hpp
 * @brief CASM code generator interface
 */

#pragma once
#include "casm/types.hpp"
#include "coil/obj.hpp"
#include "coil/instr.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace casm {

/**
 * @brief Code generator class
 * 
 * Translates parsed CASM code into COIL binary format.
 */
class CodeGenerator {
public:
  /**
   * @brief Constructor
   */
  CodeGenerator();
  
  /**
   * @brief Destructor
   */
  ~CodeGenerator();
  
  /**
   * @brief Initialize the code generator
   * @return Result of initialization
   */
  Result initialize();
  
  /**
   * @brief Generate code for a parsed line
   * @param line Parsed line
   * @return Result of code generation
   */
  Result generateLine(const Line& line);
  
  /**
   * @brief Finalize code generation and create object file
   * @param obj Output object file
   * @return Result of finalization
   */
  Result finalize(coil::Object& obj);
  
  /**
   * @brief Set error callback function
   * @param callback Error callback function
   * @param user_data User data to pass to callback
   */
  void setErrorCallback(void (*callback)(const char* message, size_t line, void* user_data), void* user_data);
  
  /**
   * @brief Get the last error message
   * @return Last error message
   */
  const std::string& getLastError() const;

private:
  /**
   * @brief Generate code for an instruction
   * @param instr Instruction to generate code for
   * @param instr_line Line number for error reporting
   * @return Result of code generation
   */
  Result generateInstruction(const Instruction& instr, size_t instr_line);
  
  /**
   * @brief Process a directive
   * @param dir Directive to process
   * @param dir_line Line number for error reporting
   * @return Result of processing
   */
  Result processDirective(const Directive& dir, size_t dir_line);
  
  /**
   * @brief Define a label
   * @param label Label to define
   * @param label_line Line number for error reporting
   * @return Result of definition
   */
  Result defineLabel(const std::string& label, size_t label_line);
  
  /**
   * @brief Define a symbol
   * @param name Symbol name
   * @param value Symbol value
   * @param section_index Section index
   * @param is_global Whether symbol is global
   * @param sym_type Symbol type
   * @param sym_line Line number for error reporting
   * @return Result of definition
   */
  Result defineSymbol(const std::string& name, uint32_t value, uint16_t section_index, 
                     bool is_global, coil::SymbolType sym_type, size_t sym_line);
  
  /**
   * @brief Reference a symbol
   * @param name Symbol name
   * @param ref_line Line number for error reporting
   * @return Symbol index or 0 on error
   */
  uint16_t referenceSymbol(const std::string& name, size_t ref_line);
  
  /**
   * @brief Get or create a section
   * @param name Section name
   * @param type Section type
   * @param flags Section flags
   * @return Section index or 0 on error
   */
  uint16_t getOrCreateSection(const std::string& name, coil::SectionType type, coil::SectionFlag flags);
  
  /**
   * @brief Convert CASM operand to COIL operand
   * @param src CASM operand
   * @param dst COIL operand
   * @param inst_line Line number for error reporting
   * @return Result of conversion
   */
  Result convertOperand(const Operand& src, coil::Operand& dst, size_t inst_line);
  
  /**
   * @brief Align section data to specified boundary
   * @param alignment Alignment boundary (power of 2)
   * @return Result of alignment
   */
  Result alignSection(uint32_t alignment);
  
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
  
  // Active sections
  std::unordered_map<std::string, Section> sections;
  
  // Current section
  Section* current_section;
  
  // Symbol table
  std::unordered_map<std::string, Symbol> symbols;
  
  // Current instruction block (for text sections)
  coil::InstructionBlock current_block;
  
  // Instruction buffer (for temporary storage)
  std::vector<coil::Instruction> instr_buffer;
  
  // Maps opcode name to COIL opcode
  std::unordered_map<std::string, coil::Opcode> opcode_map;
  
  // Maps condition name to COIL condition flag
  std::unordered_map<std::string, coil::InstrFlag0> condition_map;
  
  // Error callback function
  void (*error_callback)(const char* message, size_t line, void* user_data);
  
  // User data for error callback
  void* error_user_data;
  
  // Last error message
  std::string last_error;
  
  // Error buffer for formatting
  char error_buffer[1024];
  
  // Flag indicating whether code generation has been initialized
  bool initialized;
};

} // namespace casm