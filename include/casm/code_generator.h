#ifndef CASM_CODE_GENERATOR_H
#define CASM_CODE_GENERATOR_H

#include "parser.h"
#include "error_handler.h"
#include <coil/binary_format.h>
#include <coil/instruction_set.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace casm {

/**
* Code generator class for generating COIL binary code
*/
class CodeGenerator {
public:
  /**
    * Constructor
    * @param ast Abstract syntax tree
    * @param errorHandler Error handler
    * @param filename Source filename
    */
  CodeGenerator(std::unique_ptr<ProgramNode> ast, ErrorHandler& errorHandler, const std::string& filename);
  
  /**
    * Generate COIL binary code
    * @return COIL object
    */
  coil::CoilObject generate();
  
  /**
    * Write COIL object to file
    * @param coilObj COIL object
    * @param filename Output filename
    * @return True if successful, false otherwise
    */
  bool writeToFile(const coil::CoilObject& coilObj, const std::string& filename);
  
private:
  std::unique_ptr<ProgramNode> ast_;
  ErrorHandler& errorHandler_;
  std::string filename_;
  coil::CoilObject coilObj_;
  std::unordered_map<std::string, uint16_t> sectionIndices_;
  uint16_t currentSectionIndex_;
  
  void processNode(const ASTNode* node);
  void processInstruction(const InstructionNode* node);
  void processDirective(const DirectiveNode* node);
  void processLabel(const LabelNode* node);
  
  std::optional<coil::Operand> generateOperand(const OperandNode* node);
  
  void createDefaultSections();
  void addSymbolsFromParser(Parser& parser);
  
  uint8_t resolveOpcode(const std::string& instruction);
  uint16_t resolveTypeSpecifier(const std::string& typeSpec);
  
  void error(const std::string& message);
  void error(const Token& token, const std::string& message);
};

} // namespace casm

#endif // CASM_CODE_GENERATOR_H