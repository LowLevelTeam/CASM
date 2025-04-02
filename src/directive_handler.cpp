#include "casm/directive_handler.h"
#include <coil/binary_format.h>
#include <sstream>

namespace casm {

DirectiveHandler::DirectiveHandler(ErrorHandler& errorHandler, coil::CoilObject& coilObj)
  : errorHandler_(errorHandler), coilObj_(coilObj), 
    currentProcessor_(0x01), currentArchitecture_(0x01), currentMode_(0x03) {
  // Initialize directive handlers
  initializeHandlers();
}

bool DirectiveHandler::processDirective(const DirectiveNode* directive, SymbolTable& symbolTable, uint16_t& currentSection) {
  if (!directive) {
      return false;
  }
  
  // Get directive name
  const std::string& directiveName = directive->getDirective().lexeme;
  
  // Find handler for directive
  auto it = directiveHandlers_.find(directiveName);
  if (it != directiveHandlers_.end()) {
      // Execute handler
      return it->second(directive, symbolTable, currentSection);
  }
  
  // Unknown directive
  error(directive->getDirective(), "Unknown directive: " + directiveName);
  return false;
}

void DirectiveHandler::initializeHandlers() {
  // Register directive handlers
  directiveHandlers_["PROC"] = [this](const DirectiveNode* d, SymbolTable& s, uint16_t& c) { 
      return handleProc(d, s, c); 
  };
  
  directiveHandlers_["ARCH"] = [this](const DirectiveNode* d, SymbolTable& s, uint16_t& c) { 
      return handleArch(d, s, c); 
  };
  
  directiveHandlers_["MODE"] = [this](const DirectiveNode* d, SymbolTable& s, uint16_t& c) { 
      return handleMode(d, s, c); 
  };
  
  directiveHandlers_["SECTION"] = [this](const DirectiveNode* d, SymbolTable& s, uint16_t& c) { 
      return handleSection(d, s, c); 
  };
  
  directiveHandlers_["ALIGN"] = [this](const DirectiveNode* d, SymbolTable& s, uint16_t& c) { 
      return handleAlign(d, s, c); 
  };
  
  directiveHandlers_["DATA"] = [this](const DirectiveNode* d, SymbolTable& s, uint16_t& c) { 
      return handleData(d, s, c); 
  };
  
  directiveHandlers_["GLOBAL"] = [this](const DirectiveNode* d, SymbolTable& s, uint16_t& c) { 
      return handleGlobal(d, s, c); 
  };
  
  directiveHandlers_["EXTERN"] = [this](const DirectiveNode* d, SymbolTable& s, uint16_t& c) { 
      return handleExtern(d, s, c); 
  };
  
  // Add more directive handlers as needed...
}

bool DirectiveHandler::handleProc(const DirectiveNode* directive, SymbolTable& symbolTable, uint16_t& currentSection) {
  // Check for required operands
  const auto& operands = directive->getOperands();
  if (operands.empty()) {
      error(directive->getDirective(), "PROC directive requires a processor operand");
      return false;
  }
  
  // Get processor value
  auto procValue = getIntegerValue(operands[0].get());
  if (!procValue) {
      error(operands[0]->getToken(), "Expected integer value for processor");
      return false;
  }
  
  // Update current processor
  currentProcessor_ = static_cast<uint8_t>(*procValue);
  
  return true;
}

bool DirectiveHandler::handleArch(const DirectiveNode* directive, SymbolTable& symbolTable, uint16_t& currentSection) {
  // Check for required operands
  const auto& operands = directive->getOperands();
  if (operands.empty()) {
      error(directive->getDirective(), "ARCH directive requires an architecture operand");
      return false;
  }
  
  // Get architecture value
  auto archValue = getIntegerValue(operands[0].get());
  if (!archValue) {
      error(operands[0]->getToken(), "Expected integer value for architecture");
      return false;
  }
  
  // Update current architecture
  currentArchitecture_ = static_cast<uint8_t>(*archValue);
  
  return true;
}

bool DirectiveHandler::handleMode(const DirectiveNode* directive, SymbolTable& symbolTable, uint16_t& currentSection) {
  // Check for required operands
  const auto& operands = directive->getOperands();
  if (operands.empty()) {
      error(directive->getDirective(), "MODE directive requires a mode operand");
      return false;
  }
  
  // Get mode value
  auto modeValue = getIntegerValue(operands[0].get());
  if (!modeValue) {
      error(operands[0]->getToken(), "Expected integer value for mode");
      return false;
  }
  
  // Update current mode
  currentMode_ = static_cast<uint8_t>(*modeValue);
  
  return true;
}

bool DirectiveHandler::handleSection(const DirectiveNode* directive, SymbolTable& symbolTable, uint16_t& currentSection) {
  // Check for required operands
  const auto& operands = directive->getOperands();
  if (operands.empty()) {
      error(directive->getDirective(), "SECTION directive requires a name operand");
      return false;
  }
  
  // Get section name
  const Token& nameToken = operands[0]->getToken();
  std::string sectionName = nameToken.lexeme;
  
  // Get section attributes (optional)
  uint32_t attributes = coil::SectionFlags::READABLE; // Default
  if (operands.size() > 1) {
      auto attrValue = getIntegerValue(operands[1].get());
      if (attrValue) {
          attributes = static_cast<uint32_t>(*attrValue);
      }
  }
  
  // Check if section already exists
  uint16_t sectionIndex = UINT16_MAX;
  for (uint16_t i = 0; i < coilObj_.getSectionCount(); i++) {
      const coil::Section& section = coilObj_.getSection(i);
      const coil::Symbol& sectionSymbol = coilObj_.getSymbol(section.name_index);
      
      if (sectionSymbol.name == sectionName) {
          sectionIndex = i;
          break;
      }
  }
  
  if (sectionIndex != UINT16_MAX) {
      // Section already exists, update current section
      currentSection = sectionIndex;
      return true;
  }
  
  // Create new section
  coil::Symbol sectionSymbol;
  sectionSymbol.name = sectionName;
  sectionSymbol.name_length = static_cast<uint16_t>(sectionName.length());
  sectionSymbol.attributes = coil::SymbolFlags::LOCAL;
  sectionSymbol.value = 0;
  sectionSymbol.section_index = 0; // Will be updated
  sectionSymbol.processor_type = currentProcessor_;
  
  coil::Section section;
  section.name_index = coilObj_.addSymbol(sectionSymbol);
  section.attributes = attributes;
  section.offset = 0;
  section.size = 0;
  section.address = 0;
  section.alignment = 8; // Default alignment
  section.processor_type = currentProcessor_;
  
  sectionIndex = coilObj_.addSection(section);
  coilObj_.setSymbolSectionIndex(section.name_index, sectionIndex);
  
  // Add to symbol table
  SymbolInfo symbolInfo;
  symbolInfo.name = sectionName;
  symbolInfo.type = SymbolType::SECTION;
  symbolInfo.address = 0;
  symbolInfo.sectionIndex = sectionIndex;
  symbolInfo.attributes = coil::SymbolFlags::LOCAL;
  symbolInfo.defined = true;
  
  symbolTable.addSymbol(symbolInfo);
  
  // Update current section
  currentSection = sectionIndex;
  
  return true;
}

bool DirectiveHandler::handleAlign(const DirectiveNode* directive, SymbolTable& symbolTable, uint16_t& currentSection) {
  // Check for required operands
  const auto& operands = directive->getOperands();
  if (operands.empty()) {
      error(directive->getDirective(), "ALIGN directive requires an alignment operand");
      return false;
  }
  
  // Get alignment value
  auto alignValue = getIntegerValue(operands[0].get());
  if (!alignValue) {
      error(operands[0]->getToken(), "Expected integer value for alignment");
      return false;
  }
  
  // Validate current section
  if (!validateCurrentSection(currentSection)) {
      return false;
  }
  
  // Get section
  coil::Section section = coilObj_.getSection(currentSection);
  
  // Update section alignment
  section.alignment = static_cast<uint32_t>(*alignValue);
  coilObj_.updateSection(currentSection, section);
  
  return true;
}

bool DirectiveHandler::handleData(const DirectiveNode* directive, SymbolTable& symbolTable, uint16_t& currentSection) {
  // Check for required operands
  const auto& operands = directive->getOperands();
  if (operands.size() < 2) {
      error(directive->getDirective(), "DATA directive requires a type and value operand");
      return false;
  }
  
  // Validate current section
  if (!validateCurrentSection(currentSection)) {
      return false;
  }
  
  // Get type
  const Token& typeToken = operands[0]->getToken();
  
  // Get value
  const Token& valueToken = operands[1]->getToken();
  
  // Process based on type and value
  if (typeToken.lexeme == "TYPE_ARRAY=TYPE_UNT8" || typeToken.lexeme == "TYPE_ARRAY=TYPE_INT8") {
      // String or byte array
      if (valueToken.type == TokenType::STRING_LITERAL) {
          // String literal
          std::string value = valueToken.getValue<std::string>();
          
          // Add string data to section
          std::vector<uint8_t> data(value.begin(), value.end());
          
          // Append data to section
          coil::Section section = coilObj_.getSection(currentSection);
          section.data.insert(section.data.end(), data.begin(), data.end());
          section.size = static_cast<uint32_t>(section.data.size());
          coilObj_.updateSection(currentSection, section);
          
          return true;
      }
  }
  
  // Integer data
  auto intValue = getIntegerValue(operands[1].get());
  if (intValue) {
      // Determine data size based on type
      if (typeToken.lexeme == "TYPE_INT8" || typeToken.lexeme == "TYPE_UNT8") {
          // 8-bit integer
          uint8_t value = static_cast<uint8_t>(*intValue);
          
          // Add to section data
          coil::Section section = coilObj_.getSection(currentSection);
          section.data.push_back(value);
          section.size = static_cast<uint32_t>(section.data.size());
          coilObj_.updateSection(currentSection, section);
          
          return true;
      } else if (typeToken.lexeme == "TYPE_INT16" || typeToken.lexeme == "TYPE_UNT16") {
          // 16-bit integer
          uint16_t value = static_cast<uint16_t>(*intValue);
          
          // Add to section data (little-endian)
          coil::Section section = coilObj_.getSection(currentSection);
          section.data.push_back(value & 0xFF);
          section.data.push_back((value >> 8) & 0xFF);
          section.size = static_cast<uint32_t>(section.data.size());
          coilObj_.updateSection(currentSection, section);
          
          return true;
      } else if (typeToken.lexeme == "TYPE_INT32" || typeToken.lexeme == "TYPE_UNT32") {
          // 32-bit integer
          uint32_t value = static_cast<uint32_t>(*intValue);
          
          // Add to section data (little-endian)
          coil::Section section = coilObj_.getSection(currentSection);
          section.data.push_back(value & 0xFF);
          section.data.push_back((value >> 8) & 0xFF);
          section.data.push_back((value >> 16) & 0xFF);
          section.data.push_back((value >> 24) & 0xFF);
          section.size = static_cast<uint32_t>(section.data.size());
          coilObj_.updateSection(currentSection, section);
          
          return true;
      } else if (typeToken.lexeme == "TYPE_INT64" || typeToken.lexeme == "TYPE_UNT64") {
          // 64-bit integer
          uint64_t value = static_cast<uint64_t>(*intValue);
          
          // Add to section data (little-endian)
          coil::Section section = coilObj_.getSection(currentSection);
          section.data.push_back(value & 0xFF);
          section.data.push_back((value >> 8) & 0xFF);
          section.data.push_back((value >> 16) & 0xFF);
          section.data.push_back((value >> 24) & 0xFF);
          section.data.push_back((value >> 32) & 0xFF);
          section.data.push_back((value >> 40) & 0xFF);
          section.data.push_back((value >> 48) & 0xFF);
          section.data.push_back((value >> 56) & 0xFF);
          section.size = static_cast<uint32_t>(section.data.size());
          coilObj_.updateSection(currentSection, section);
          
          return true;
      }
  }
  
  // Unsupported data type or value
  error(directive->getDirective(), "Unsupported data type or value");
  return false;
}

bool DirectiveHandler::handleGlobal(const DirectiveNode* directive, SymbolTable& symbolTable, uint16_t& currentSection) {
  // Check for required operands
  const auto& operands = directive->getOperands();
  if (operands.empty()) {
      error(directive->getDirective(), "GLOBAL directive requires a symbol operand");
      return false;
  }
  
  // Get symbol name
  const Token& symbolToken = operands[0]->getToken();
  std::string symbolName = symbolToken.lexeme;
  
  // Look up symbol in symbol table
  SymbolInfo* symbol = symbolTable.getSymbol(symbolName);
  if (!symbol) {
      // Symbol not defined yet, create a forward reference
      SymbolInfo symbolInfo;
      symbolInfo.name = symbolName;
      symbolInfo.type = SymbolType::UNKNOWN;
      symbolInfo.address = 0;
      symbolInfo.sectionIndex = 0;
      symbolInfo.attributes = coil::SymbolFlags::GLOBAL;
      symbolInfo.defined = false;
      
      symbolTable.addSymbol(symbolInfo);
  } else {
      // Update symbol attributes
      symbol->attributes |= coil::SymbolFlags::GLOBAL;
      
      // Update COIL symbol
      uint16_t symbolIndex = coilObj_.findSymbol(symbolName);
      if (symbolIndex != UINT16_MAX) {
          coil::Symbol coilSymbol = coilObj_.getSymbol(symbolIndex);
          coilSymbol.attributes |= coil::SymbolFlags::GLOBAL;
          coilObj_.updateSymbol(symbolIndex, coilSymbol);
      }
  }
  
  return true;
}

bool DirectiveHandler::handleExtern(const DirectiveNode* directive, SymbolTable& symbolTable, uint16_t& currentSection) {
  // Check for required operands
  const auto& operands = directive->getOperands();
  if (operands.empty()) {
      error(directive->getDirective(), "EXTERN directive requires a symbol operand");
      return false;
  }
  
  // Get symbol name
  const Token& symbolToken = operands[0]->getToken();
  std::string symbolName = symbolToken.lexeme;
  
  // Check if symbol already defined
  if (symbolTable.hasSymbol(symbolName)) {
      error(symbolToken, "Symbol already defined: " + symbolName);
      return false;
  }
  
  // Create external symbol
  SymbolInfo symbolInfo;
  symbolInfo.name = symbolName;
  symbolInfo.type = SymbolType::UNKNOWN;
  symbolInfo.address = 0;
  symbolInfo.sectionIndex = UINT16_MAX; // External symbol
  symbolInfo.attributes = coil::SymbolFlags::GLOBAL | coil::SymbolFlags::EXPORTED;
  symbolInfo.defined = false;
  
  symbolTable.addSymbol(symbolInfo);
  
  // Add to COIL object
  coil::Symbol symbol;
  symbol.name = symbolName;
  symbol.name_length = static_cast<uint16_t>(symbolName.length());
  symbol.attributes = coil::SymbolFlags::GLOBAL | coil::SymbolFlags::EXPORTED;
  symbol.value = 0;
  symbol.section_index = UINT16_MAX; // External symbol
  symbol.processor_type = currentProcessor_;
  
  coilObj_.addSymbol(symbol);
  
  return true;
}

bool DirectiveHandler::validateCurrentSection(uint16_t& currentSection) {
  if (currentSection >= coilObj_.getSectionCount()) {
      errorHandler_.addError(
          0x01000007, // Generic section error code
          "No active section",
          "", 0, 0,
          ErrorSeverity::ERROR
      );
      return false;
  }
  
  return true;
}

std::optional<int64_t> DirectiveHandler::getIntegerValue(const OperandNode* operand) {
  if (!operand) {
      return std::nullopt;
  }
  
  const Token& token = operand->getToken();
  
  if (token.type == TokenType::INTEGER_LITERAL) {
      return token.getValue<int64_t>();
  }
  
  // Try to parse from lexeme
  try {
      return std::stoll(token.lexeme);
  } catch (...) {
      return std::nullopt;
  }
}

std::optional<std::string> DirectiveHandler::getStringValue(const OperandNode* operand) {
  if (!operand) {
      return std::nullopt;
  }
  
  const Token& token = operand->getToken();
  
  if (token.type == TokenType::STRING_LITERAL) {
      return token.getValue<std::string>();
  }
  
  return token.lexeme;
}

void DirectiveHandler::error(const Token& token, const std::string& message) {
  errorHandler_.addError(
      0x01000005, // Generic directive error code
      message,
      token,
      "",
      ErrorSeverity::ERROR
  );
}

} // namespace casm