/**
* @file codegen.hpp
* @brief Code generator for CASM assembly language
*/

#pragma once
#include "casm/parser.hpp"
#include <coil/instr.hpp>
#include <coil/obj.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace casm {

/**
* @brief Code generator for CASM assembly language
* 
* Generates COIL binary from parsed CASM statements.
*/
class CodeGenerator {
public:
  /**
  * @brief Initialize code generator
  */
  CodeGenerator();
  
  /**
  * @brief Generate COIL object from statements
  * @param statements Parsed CASM statements
  * @return Generated COIL object or nullptr on error
  */
  std::unique_ptr<coil::Object> generate(const std::vector<std::unique_ptr<Statement>>& statements);
  
  /**
  * @brief Get code generation errors
  * @return Vector of error messages
  */
  const std::vector<std::string>& getErrors() const;
  
private:
  std::vector<std::string> errors;                        ///< Code generation errors
  std::unordered_map<std::string, uint32_t> labels;       ///< Map of label names to instruction indexes
  std::unordered_map<uint32_t, std::string> pendingLabels;///< Map of instruction indexes to label names
  std::unordered_map<std::string, coil::Opcode> opcodeMap;///< Map of opcode strings to enums
  coil::Object obj;                                      ///< COIL object being built
  coil::InstructionBlock currentBlock;                   ///< Current instruction block
  uint16_t currentSectionIndex;                          ///< Current section index
  
  /**
  * @brief Initialize opcode map
  */
  void initOpcodeMap();
  
  /**
  * @brief Process a section statement
  * @param section Section statement
  * @return Result of operation
  */
  coil::Result processSection(const SectionStatement* section);
  
  /**
  * @brief Process a label statement
  * @param label Label statement
  * @return Result of operation
  */
  coil::Result processLabel(const LabelStatement* label);
  
  /**
  * @brief Process an instruction statement
  * @param instr Instruction statement
  * @return Result of operation
  */
  coil::Result processInstruction(const InstructionStatement* instr);
  
  /**
  * @brief Process a directive statement
  * @param directive Directive statement
  * @return Result of operation
  */
  coil::Result processDirective(const DirectiveStatement* directive);
  
  /**
  * @brief Convert instruction operand to COIL operand
  * @param op Instruction operand
  * @return COIL operand
  */
  coil::Operand convertOperand(const InstructionOperand& op);
  
  /**
  * @brief Create a COIL instruction
  * @param instr Instruction statement
  * @return Created instruction
  */
  coil::Instruction createInstruction(const InstructionStatement* instr);
  
  /**
  * @brief Report an error
  * @param message Error message
  * @param line Line number
  */
  void error(const std::string& message, size_t line);
  
  /**
  * @brief Resolve pending label references
  * @return True if all labels resolved
  */
  bool resolveLabels();
  
  /**
  * @brief Add a section to the object
  * @param name Section name
  * @param type Section type
  * @param flags Section flags
  * @return Section index
  */
  uint16_t addSection(const std::string& name, coil::SectionType type, coil::SectionFlag flags);
  
  /**
  * @brief Finalize the current section
  * @return Result of operation
  */
  coil::Result finalizeCurrentSection();
};

} // namespace casm