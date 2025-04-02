#include "casm/code_generator.h"
#include <fstream>
#include <sstream>
#include <coil/instruction_set.h>

namespace casm {

CodeGenerator::CodeGenerator(std::unique_ptr<ProgramNode> ast, ErrorHandler& errorHandler, const std::string& filename)
  : ast_(std::move(ast)), errorHandler_(errorHandler), filename_(filename), currentSectionIndex_(0) {
  // Initialize COIL object
  createDefaultSections();
}

coil::CoilObject CodeGenerator::generate() {
  // Add symbols from parser
  Parser* parser = dynamic_cast<Parser*>(ast_.get());
  if (parser) {
      addSymbolsFromParser(*parser);
  }
  
  // Process each node in the AST
  for (const auto& node : ast_->getNodes()) {
      processNode(node.get());
  }
  
  return coilObj_;
}

bool CodeGenerator::writeToFile(const coil::CoilObject& coilObj, const std::string& filename) {
  try {
      // Encode COIL object to binary
      std::vector<uint8_t> binary = coilObj.encode();
      
      // Write binary to file
      std::ofstream outFile(filename, std::ios::binary);
      if (!outFile) {
          return false;
      }
      
      outFile.write(reinterpret_cast<const char*>(binary.data()), binary.size());
      outFile.close();
      
      return true;
  } catch (const std::exception& e) {
      error(std::string("Error writing COIL binary: ") + e.what());
      return false;
  }
}

void CodeGenerator::processNode(const ASTNode* node) {
  if (!node) {
      return;
  }
  
  // Process node based on type
  switch (node->getType()) {
      case NodeType::INSTRUCTION:
          processInstruction(static_cast<const InstructionNode*>(node));
          break;
      case NodeType::DIRECTIVE:
          processDirective(static_cast<const DirectiveNode*>(node));
          break;
      case NodeType::LABEL:
          processLabel(static_cast<const LabelNode*>(node));
          break;
      default:
          // Ignore other node types
          break;
  }
}

void CodeGenerator::processInstruction(const InstructionNode* node) {
  if (!node) {
      return;
  }
  
  // Get instruction name
  const std::string& instruction = node->getInstruction().lexeme;
  
  // Resolve opcode
  uint8_t opcode = resolveOpcode(instruction);
  if (opcode == 0xFF) {
      error(node->getInstruction(), "Unknown instruction: " + instruction);
      return;
  }
  
  // Generate operands
  std::vector<coil::Operand> operands;
  for (const auto& operandNode : node->getOperands()) {
      auto operand = generateOperand(operandNode.get());
      if (operand) {
          operands.push_back(*operand);
      }
  }
  
  // Create instruction
  coil::Instruction instr(opcode, operands);
  
  // Add instruction to current section
  try {
      coilObj_.addInstruction(currentSectionIndex_, instr);
  } catch (const std::exception& e) {
      error(node->getInstruction(), std::string("Error adding instruction: ") + e.what());
  }
}

void CodeGenerator::processDirective(const DirectiveNode* node) {
  if (!node) {
      return;
  }
  
  // Get directive name
  const std::string& directive = node->getDirective().lexeme;
  
  // Process directive based on type
  if (directive == "SECTION") {
      // Section directive - change current section
      if (node->getOperands().empty()) {
          error(node->getDirective(), "SECTION directive requires a name operand");
          return;
      }
      
      // Get section name
      const std::string& sectionName = node->getOperands()[0]->getToken().lexeme;
      
      // Find section index
      auto it = sectionIndices_.find(sectionName);
      if (it != sectionIndices_.end()) {
          currentSectionIndex_ = it->second;
      } else {
          error(node->getOperands()[0]->getToken(), "Unknown section: " + sectionName);
      }
  }
  // Other directives processed similarly...
}

void CodeGenerator::processLabel(const LabelNode* node) {
  if (!node) {
      return;
  }
  
  // Label processing is done in symbol resolution
}

std::optional<coil::Operand> CodeGenerator::generateOperand(const OperandNode* node) {
  if (!node) {
      return std::nullopt;
  }
  
  // Generate operand based on type
  switch (node->getOperandType()) {
      case OperandNode::OperandType::IMMEDIATE: {
          const Token& token = node->getToken();
          uint16_t type = node->getCoilType();
          
          if (token.type == TokenType::INTEGER_LITERAL) {
              int64_t value = token.getValue<int64_t>();
              
              // Create immediate operand based on type
              if (type == (coil::Type::INT8 | coil::Type::IMM)) {
                  return coil::Operand::createImmediate<int8_t>(static_cast<int8_t>(value));
              } else if (type == (coil::Type::INT16 | coil::Type::IMM)) {
                  return coil::Operand::createImmediate<int16_t>(static_cast<int16_t>(value));
              } else if (type == (coil::Type::INT32 | coil::Type::IMM)) {
                  return coil::Operand::createImmediate<int32_t>(static_cast<int32_t>(value));
              } else if (type == (coil::Type::INT64 | coil::Type::IMM)) {
                  return coil::Operand::createImmediate<int64_t>(value);
              } else {
                  // Default to 32-bit
                  return coil::Operand::createImmediate<int32_t>(static_cast<int32_t>(value));
              }
          } else if (token.type == TokenType::FLOAT_LITERAL) {
              double value = token.getValue<double>();
              
              // Create immediate operand based on type
              if (type == (coil::Type::FP32 | coil::Type::IMM)) {
                  return coil::Operand::createImmediate<float>(static_cast<float>(value));
              } else if (type == (coil::Type::FP64 | coil::Type::IMM)) {
                  return coil::Operand::createImmediate<double>(value);
              } else {
                  // Default to double
                  return coil::Operand::createImmediate<double>(value);
              }
          }
          
          error(token, "Unsupported immediate operand type");
          return std::nullopt;
      }
      
      case OperandNode::OperandType::VARIABLE: {
          const Token& token = node->getToken();
          
          // Extract variable ID from token
          if (token.type != TokenType::VAR_ID) {
              error(token, "Expected variable ID");
              return std::nullopt;
          }
          
          uint16_t varId = static_cast<uint16_t>(token.getValue<int64_t>());
          return coil::Operand::createVariable(varId);
      }
      
      case OperandNode::OperandType::REGISTER: {
          // const Token& token = node->getToken();
          
          // For simplicity, we'll use general purpose registers for all registers
          uint16_t registerId = 0; // Would map register name to ID in a real implementation
          return coil::Operand::createRegister(registerId, coil::Type::RGP);
      }
      
      case OperandNode::OperandType::LABEL: {
          const Token& token = node->getToken();
          
          // Look up symbol in symbol table to get its index
          uint16_t symbolIndex = coilObj_.findSymbol(token.lexeme);
          if (symbolIndex == UINT16_MAX) {
              error(token, "Unknown symbol: " + token.lexeme);
              return std::nullopt;
          }
          
          return coil::Operand::createSymbol(symbolIndex);
      }
      
      case OperandNode::OperandType::TYPE: {
          // const Token& token = node->getToken();
          uint16_t type = node->getCoilType();
          
          // Type operands are used in directives and special instructions
          // For simplicity, we'll create an immediate operand with the type value
          return coil::Operand::createImmediate<uint16_t>(type);
      }
      
      case OperandNode::OperandType::MEMORY: {
          // const Token& token = node->getToken();
          
          // For simplicity, we'll create a basic memory operand
          // In a real implementation, this would parse the address expression
          return coil::Operand::createMemory(0);
      }
      
      default:
          error(node->getToken(), "Unsupported operand type");
          return std::nullopt;
  }
}

void CodeGenerator::createDefaultSections() {
  // Create default sections
  
  // .text section
  coil::Symbol textSectionSymbol;
  textSectionSymbol.name = ".text";
  textSectionSymbol.name_length = static_cast<uint16_t>(textSectionSymbol.name.length());
  textSectionSymbol.attributes = coil::SymbolFlags::LOCAL;
  textSectionSymbol.value = 0;
  textSectionSymbol.section_index = 0; // Will be updated
  textSectionSymbol.processor_type = 0;
  
  coil::Section textSection;
  textSection.name_index = coilObj_.addSymbol(textSectionSymbol);
  textSection.attributes = coil::SectionFlags::EXECUTABLE | coil::SectionFlags::READABLE;
  textSection.offset = 0;
  textSection.size = 0;
  textSection.address = 0;
  textSection.alignment = 16;
  textSection.processor_type = 0;
  
  uint16_t textSectionIndex = coilObj_.addSection(textSection);
  coilObj_.setSymbolSectionIndex(textSection.name_index, textSectionIndex);
  sectionIndices_[".text"] = textSectionIndex;
  
  // .data section
  coil::Symbol dataSectionSymbol;
  dataSectionSymbol.name = ".data";
  dataSectionSymbol.name_length = static_cast<uint16_t>(dataSectionSymbol.name.length());
  dataSectionSymbol.attributes = coil::SymbolFlags::LOCAL;
  dataSectionSymbol.value = 0;
  dataSectionSymbol.section_index = 0; // Will be updated
  dataSectionSymbol.processor_type = 0;
  
  coil::Section dataSection;
  dataSection.name_index = coilObj_.addSymbol(dataSectionSymbol);
  dataSection.attributes = coil::SectionFlags::READABLE | coil::SectionFlags::WRITABLE | coil::SectionFlags::INITIALIZED;
  dataSection.offset = 0;
  dataSection.size = 0;
  dataSection.address = 0;
  dataSection.alignment = 8;
  dataSection.processor_type = 0;
  
  uint16_t dataSectionIndex = coilObj_.addSection(dataSection);
  coilObj_.setSymbolSectionIndex(dataSection.name_index, dataSectionIndex);
  sectionIndices_[".data"] = dataSectionIndex;
  
  // .bss section
  coil::Symbol bssSectionSymbol;
  bssSectionSymbol.name = ".bss";
  bssSectionSymbol.name_length = static_cast<uint16_t>(bssSectionSymbol.name.length());
  bssSectionSymbol.attributes = coil::SymbolFlags::LOCAL;
  bssSectionSymbol.value = 0;
  bssSectionSymbol.section_index = 0; // Will be updated
  bssSectionSymbol.processor_type = 0;
  
  coil::Section bssSection;
  bssSection.name_index = coilObj_.addSymbol(bssSectionSymbol);
  bssSection.attributes = coil::SectionFlags::READABLE | coil::SectionFlags::WRITABLE | coil::SectionFlags::UNINITIALIZED;
  bssSection.offset = 0;
  bssSection.size = 0;
  bssSection.address = 0;
  bssSection.alignment = 8;
  bssSection.processor_type = 0;
  
  uint16_t bssSectionIndex = coilObj_.addSection(bssSection);
  coilObj_.setSymbolSectionIndex(bssSection.name_index, bssSectionIndex);
  sectionIndices_[".bss"] = bssSectionIndex;
  
  // Set current section to .text
  currentSectionIndex_ = textSectionIndex;
}

void CodeGenerator::addSymbolsFromParser(Parser& parser) {
  const SymbolTable& symbolTable = parser.getSymbolTable();
  
  // Add each symbol to COIL object
  for (const auto& symbolInfo : symbolTable.getAllSymbols()) {
      // Skip section symbols (already added)
      if (symbolInfo.type == SymbolType::SECTION && sectionIndices_.find(symbolInfo.name) != sectionIndices_.end()) {
          continue;
      }
      
      // Create COIL symbol
      coil::Symbol symbol;
      symbol.name = symbolInfo.name;
      symbol.name_length = static_cast<uint16_t>(symbolInfo.name.length());
      symbol.attributes = symbolInfo.attributes;
      symbol.value = symbolInfo.address;
      symbol.section_index = symbolInfo.sectionIndex;
      symbol.processor_type = 0; // Default processor type
      
      // Add symbol to COIL object
      coilObj_.addSymbol(symbol);
  }
}

uint8_t CodeGenerator::resolveOpcode(const std::string& instruction) {
  // Use coil::InstructionSet to get opcode from instruction name
  auto opcode = coil::InstructionSet::getOpcodeFromName(instruction);
  if (opcode) {
      return *opcode;
  }
  
  // Unknown instruction
  return 0xFF;
}

uint16_t CodeGenerator::resolveTypeSpecifier(const std::string& typeSpec) {
  // Map CASM type specifier to COIL type
  if (typeSpec == "TYPE_INT8") return coil::Type::INT8;
  if (typeSpec == "TYPE_INT16") return coil::Type::INT16;
  if (typeSpec == "TYPE_INT32") return coil::Type::INT32;
  if (typeSpec == "TYPE_INT64") return coil::Type::INT64;
  if (typeSpec == "TYPE_UNT8") return coil::Type::UNT8;
  if (typeSpec == "TYPE_UNT16") return coil::Type::UNT16;
  if (typeSpec == "TYPE_UNT32") return coil::Type::UNT32;
  if (typeSpec == "TYPE_UNT64") return coil::Type::UNT64;
  if (typeSpec == "TYPE_FP16") return coil::Type::FP16;
  if (typeSpec == "TYPE_FP32") return coil::Type::FP32;
  if (typeSpec == "TYPE_FP64") return coil::Type::FP64;
  if (typeSpec == "TYPE_FP128") return coil::Type::FP128;
  if (typeSpec == "TYPE_V128") return coil::Type::V128;
  if (typeSpec == "TYPE_V256") return coil::Type::V256;
  if (typeSpec == "TYPE_V512") return coil::Type::V512;
  if (typeSpec == "TYPE_BIT") return coil::Type::BIT;
  if (typeSpec == "TYPE_VOID") return coil::Type::VOID;
  if (typeSpec == "TYPE_INT") return coil::Type::INT;
  if (typeSpec == "TYPE_UNT") return coil::Type::UNT;
  if (typeSpec == "TYPE_FP") return coil::Type::FP;
  if (typeSpec == "TYPE_PTR") return coil::Type::PTR;
  if (typeSpec == "TYPE_VAR") return coil::Type::VAR;
  if (typeSpec == "TYPE_SYM") return coil::Type::SYM;
  if (typeSpec == "TYPE_RGP") return coil::Type::RGP;
  if (typeSpec == "TYPE_RFP") return coil::Type::RFP;
  if (typeSpec == "TYPE_RV") return coil::Type::RV;
  if (typeSpec == "TYPE_STRUCT") return coil::Type::STRUCT;
  if (typeSpec == "TYPE_PACK") return coil::Type::PACK;
  if (typeSpec == "TYPE_UNION") return coil::Type::UNION;
  if (typeSpec == "TYPE_ARRAY") return coil::Type::ARRAY;
  
  // Unknown type
  return 0;
}

void CodeGenerator::error(const std::string& message) {
  errorHandler_.addError(
      0x01000004, // Generic code generation error code
      message,
      filename_, 0, 0,
      ErrorSeverity::ERROR
  );
}

void CodeGenerator::error(const Token& token, const std::string& message) {
  errorHandler_.addError(
      0x01000004, // Generic code generation error code
      message,
      token,
      filename_,
      ErrorSeverity::ERROR
  );
}

} // namespace casm