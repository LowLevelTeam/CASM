#ifndef CASM_DIRECTIVE_HANDLER_H
#define CASM_DIRECTIVE_HANDLER_H

#include "parser.h"
#include "error_handler.h"
#include <coil/binary_format.h>
#include <string>
#include <unordered_map>
#include <functional>

namespace casm {

/**
* Directive handler class for processing assembly directives
*/
class DirectiveHandler {
public:
  /**
    * Constructor
    * @param errorHandler Error handler
    * @param coilObj COIL object
    */
  DirectiveHandler(ErrorHandler& errorHandler, coil::CoilObject& coilObj);
  
  /**
    * Process a directive
    * @param directive Directive node
    * @param symbolTable Symbol table
    * @param currentSection Current section index
    * @return True if processed successfully, false otherwise
    */
  bool processDirective(const DirectiveNode* directive, SymbolTable& symbolTable, uint16_t& currentSection);
  
private:
  ErrorHandler& errorHandler_;
  coil::CoilObject& coilObj_;
  
  uint8_t currentProcessor_;
  uint8_t currentArchitecture_;
  uint8_t currentMode_;
  
  using DirectiveFunction = std::function<bool(const DirectiveNode*, SymbolTable&, uint16_t&)>;
  std::unordered_map<std::string, DirectiveFunction> directiveHandlers_;
  
  void initializeHandlers();
  
  // Directive handlers
  bool handleProc(const DirectiveNode* directive, SymbolTable& symbolTable, uint16_t& currentSection);
  bool handleArch(const DirectiveNode* directive, SymbolTable& symbolTable, uint16_t& currentSection);
  bool handleMode(const DirectiveNode* directive, SymbolTable& symbolTable, uint16_t& currentSection);
  bool handleSection(const DirectiveNode* directive, SymbolTable& symbolTable, uint16_t& currentSection);
  bool handleAlign(const DirectiveNode* directive, SymbolTable& symbolTable, uint16_t& currentSection);
  bool handleData(const DirectiveNode* directive, SymbolTable& symbolTable, uint16_t& currentSection);
  bool handleGlobal(const DirectiveNode* directive, SymbolTable& symbolTable, uint16_t& currentSection);
  bool handleExtern(const DirectiveNode* directive, SymbolTable& symbolTable, uint16_t& currentSection);
  
  // Helper methods
  bool validateCurrentSection(uint16_t& currentSection);
  std::optional<int64_t> getIntegerValue(const OperandNode* operand);
  std::optional<std::string> getStringValue(const OperandNode* operand);
  
  void error(const Token& token, const std::string& message);
};

} // namespace casm

#endif // CASM_DIRECTIVE_HANDLER_H