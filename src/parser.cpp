/**
 * @file parser.cpp
 * @brief Implementation of parser for CASM assembly language
 */

#include "casm/parser.hpp"
#include <sstream>
#include <unordered_map>
#include <algorithm>

namespace casm {

// InstructionOperand static methods
InstructionOperand InstructionOperand::createRegister(uint32_t reg, coil::ValueType vtype) {
  InstructionOperand op;
  op.type = Type::Register;
  op.valueType = vtype;
  op.reg = reg;
  return op;
}

InstructionOperand InstructionOperand::createImmediate(int64_t imm, coil::ValueType vtype) {
  InstructionOperand op;
  op.type = Type::Immediate;
  op.valueType = vtype;
  op.imm = imm;
  return op;
}

InstructionOperand InstructionOperand::createFloatImmediate(double imm, coil::ValueType vtype) {
  InstructionOperand op;
  op.type = Type::FloatImmediate;
  op.valueType = vtype;
  op.fpImm = imm;
  return op;
}

InstructionOperand InstructionOperand::createMemory(uint32_t base, int32_t offset, coil::ValueType vtype) {
  InstructionOperand op;
  op.type = Type::Memory;
  op.valueType = vtype;
  op.mem.base = base;
  op.mem.offset = offset;
  return op;
}

InstructionOperand InstructionOperand::createLabel(const std::string& name) {
  InstructionOperand op;
  op.type = Type::Label;
  op.valueType = coil::ValueType::Void;
  op.labelName = name;
  return op;
}

// Parser constructor
Parser::Parser(const std::vector<Token>& tokens)
  : tokens(tokens), current(0) {
  initMaps();
}

// Initialize opcode, flag, and type maps
void Parser::initMaps() {
  // Initialize opcode map
  opcodeMap["nop"] = coil::Opcode::Nop;
  opcodeMap["br"] = coil::Opcode::Br;
  opcodeMap["jump"] = coil::Opcode::Jump;
  opcodeMap["call"] = coil::Opcode::Call;
  opcodeMap["ret"] = coil::Opcode::Ret;
  opcodeMap["load"] = coil::Opcode::Load;
  opcodeMap["store"] = coil::Opcode::Store;
  opcodeMap["push"] = coil::Opcode::Push;
  opcodeMap["pop"] = coil::Opcode::Pop;
  opcodeMap["add"] = coil::Opcode::Add;
  opcodeMap["sub"] = coil::Opcode::Sub;
  opcodeMap["mul"] = coil::Opcode::Mul;
  opcodeMap["div"] = coil::Opcode::Div;
  opcodeMap["rem"] = coil::Opcode::Rem;
  opcodeMap["inc"] = coil::Opcode::Inc;
  opcodeMap["dec"] = coil::Opcode::Dec;
  opcodeMap["and"] = coil::Opcode::And;
  opcodeMap["or"] = coil::Opcode::Or;
  opcodeMap["xor"] = coil::Opcode::Xor;
  opcodeMap["not"] = coil::Opcode::Not;
  opcodeMap["shl"] = coil::Opcode::Shl;
  opcodeMap["shr"] = coil::Opcode::Shr;
  opcodeMap["sar"] = coil::Opcode::Sar;
  opcodeMap["cmp"] = coil::Opcode::Cmp;
  opcodeMap["test"] = coil::Opcode::Test;
  
  // Initialize flag map
  flagMap["eq"] = coil::InstrFlag0::EQ;
  flagMap["neq"] = coil::InstrFlag0::NEQ;
  flagMap["gt"] = coil::InstrFlag0::GT;
  flagMap["gte"] = coil::InstrFlag0::GTE;
  flagMap["lt"] = coil::InstrFlag0::LT;
  flagMap["lte"] = coil::InstrFlag0::LTE;
  
  // Initialize type map
  typeMap["i8"] = coil::ValueType::I8;
  typeMap["i16"] = coil::ValueType::I16;
  typeMap["i32"] = coil::ValueType::I32;
  typeMap["i64"] = coil::ValueType::I64;
  typeMap["u8"] = coil::ValueType::U8;
  typeMap["u16"] = coil::ValueType::U16;
  typeMap["u32"] = coil::ValueType::U32;
  typeMap["u64"] = coil::ValueType::U64;
  typeMap["f32"] = coil::ValueType::F32;
  typeMap["f64"] = coil::ValueType::F64;
  typeMap["ptr"] = coil::ValueType::Ptr;
  typeMap["void"] = coil::ValueType::Void;
}

// Parse tokens into statements
std::vector<std::unique_ptr<Statement>> Parser::parse() {
  std::vector<std::unique_ptr<Statement>> statements;
  
  // Keep parsing until end of file
  while (!isAtEnd()) {
    try {
      auto statement = parseStatement();
      if (statement) {
        statements.push_back(std::move(statement));
      }
    } catch (const std::exception& e) {
      // Log error and synchronize
      error(peek(), e.what());
      synchronize();
    }
  }
  
  return statements;
}

// Get parsing errors
const std::vector<std::string>& Parser::getErrors() const {
  return errors;
}

// Parse a single statement
std::unique_ptr<Statement> Parser::parseStatement() {
  // Skip newlines
  while (match(TokenType::Newline)) {
    // Just consume the token
  }
  
  // Check for end of file
  if (check(TokenType::EndOfFile)) {
    advance(); // Consume EOF
    return nullptr;
  }
  
  // Check for different statement types
  if (check(TokenType::Directive)) {
    Token directiveToken = peek();
    
    // Check if this is a section directive
    if (directiveToken.value == ".section") {
      advance(); // Consume .section
      return parseSection();
    }
    
    // Other directives
    return parseDirective();
  }
  
  // Check for label definition
  if (check(TokenType::Identifier) && peekNext().type == TokenType::Colon) {
    return parseLabel();
  }
  
  // Must be an instruction
  return parseInstruction();
}

// Parse a section statement
std::unique_ptr<SectionStatement> Parser::parseSection() {
  // The section name could be either an Identifier (if it doesn't start with a period)
  // or a Directive (if it does start with a period, like .text, .data, etc.)
  Token name;
  
  if (check(TokenType::Identifier)) {
    name = consume(TokenType::Identifier, "Expected section name after .section");
  } else if (check(TokenType::Directive)) {
    name = consume(TokenType::Directive, "Expected section name after .section");
  } else {
    error(peek(), "Expected section name after .section");
    throw std::runtime_error("Expected section name after .section");
  }
  
  // Create section statement
  return std::make_unique<SectionStatement>(name.value, name.line);
}

// Parse a label statement
std::unique_ptr<LabelStatement> Parser::parseLabel() {
  // Get label name
  Token name = consume(TokenType::Identifier, "Expected label name");
  
  // Consume colon
  consume(TokenType::Colon, "Expected ':' after label name");
  
  // Create label statement
  return std::make_unique<LabelStatement>(name.value, name.line);
}

// Parse an instruction statement
std::unique_ptr<InstructionStatement> Parser::parseInstruction() {
  // Get opcode
  Token opcodeToken = consume(TokenType::Identifier, "Expected instruction opcode");
  std::string opcode = opcodeToken.value;
  
  // Default values
  coil::InstrFlag0 flag = coil::InstrFlag0::None;
  coil::ValueType valueType = coil::ValueType::I32; // Default type
  
  // Check for type or flag suffix in the opcode (e.g., br.lte)
  size_t dotPos = opcode.find('.');
  if (dotPos != std::string::npos) {
    // Extract the flag or type suffix
    std::string suffix = opcode.substr(dotPos + 1);
    opcode = opcode.substr(0, dotPos);
    
    // Check if it's a type
    auto typeIt = typeMap.find(suffix);
    if (typeIt != typeMap.end()) {
      valueType = typeIt->second;
    } else {
      // Must be a flag
      auto flagIt = flagMap.find(suffix);
      if (flagIt != flagMap.end()) {
        flag = flagIt->second;
      } else {
        std::stringstream ss;
        ss << "Unknown suffix: " << suffix;
        throw std::runtime_error(ss.str());
      }
    }
  }
  // Also check for explicit type or flag suffix with a period token
  else if (match(TokenType::Period)) {
    Token suffix = consume(TokenType::Identifier, "Expected type or flag after '.'");
    
    // Check if it's a type
    auto typeIt = typeMap.find(suffix.value);
    if (typeIt != typeMap.end()) {
      valueType = typeIt->second;
    } else {
      // Must be a flag
      auto flagIt = flagMap.find(suffix.value);
      if (flagIt != flagMap.end()) {
        flag = flagIt->second;
      } else {
        std::stringstream ss;
        ss << "Unknown suffix: " << suffix.value;
        throw std::runtime_error(ss.str());
      }
    }
  }
  
  // Parse operands
  std::vector<InstructionOperand> operands;
  
  // Skip newline
  if (!check(TokenType::Newline) && !check(TokenType::EndOfFile)) {
    // Parse first operand
    operands.push_back(parseOperand());
    
    // Parse additional operands
    while (match(TokenType::Comma)) {
      operands.push_back(parseOperand());
    }
  }
  
  // Create instruction statement
  return std::make_unique<InstructionStatement>(
    opcode,
    flag,
    operands,
    valueType,
    opcodeToken.line
  );
}

// Parse a directive statement
std::unique_ptr<DirectiveStatement> Parser::parseDirective() {
  // Get directive name
  Token directiveToken = consume(TokenType::Directive, "Expected directive");
  std::string name = directiveToken.value;
  
  // Parse arguments
  std::vector<std::string> args;
  
  // Skip newline
  if (!check(TokenType::Newline) && !check(TokenType::EndOfFile)) {
    // Parse arguments
    Token arg = advance();
    args.push_back(arg.value);
    
    // Parse additional arguments
    while (match(TokenType::Comma)) {
      arg = advance();
      args.push_back(arg.value);
    }
  }
  
  // Create directive statement
  return std::make_unique<DirectiveStatement>(
    name,
    args,
    directiveToken.line
  );
}

// Parse an instruction operand
InstructionOperand Parser::parseOperand() {
  // Register operand
  if (match(TokenType::Register)) {
    Token regToken = tokens[current - 1];
    
    // Extract register number
    std::string regStr = regToken.value.substr(1); // Skip 'r'
    uint32_t reg = std::stoi(regStr);
    
    return InstructionOperand::createRegister(reg, coil::ValueType::I32);
  }
  
  // Memory operand
  if (match(TokenType::LBracket)) {
    // Base register
    Token baseToken = consume(TokenType::Register, "Expected register in memory operand");
    
    // Extract register number
    std::string baseStr = baseToken.value.substr(1); // Skip 'r'
    uint32_t base = std::stoi(baseStr);
    
    // Check for offset
    int32_t offset = 0;
    if (match(TokenType::Plus)) {
      // Positive offset
      Token offsetToken = consume(TokenType::Integer, "Expected offset after '+'");
      offset = std::stoi(offsetToken.value);
    } else if (match(TokenType::Minus)) {
      // Negative offset
      Token offsetToken = consume(TokenType::Integer, "Expected offset after '-'");
      offset = -std::stoi(offsetToken.value);
    }
    
    // Closing bracket
    consume(TokenType::RBracket, "Expected ']' after memory operand");
    
    return InstructionOperand::createMemory(base, offset, coil::ValueType::I32);
  }
  
  // Immediate integer operand
  if (match(TokenType::Integer)) {
    Token immToken = tokens[current - 1];
    int64_t value;
    
    // Parse different integer formats
    if (immToken.value.size() >= 2 && immToken.value.substr(0, 2) == "0x") {
      // Hexadecimal
      value = std::stoll(immToken.value, nullptr, 16);
    } else if (immToken.value.size() >= 2 && immToken.value.substr(0, 2) == "0b") {
      // Binary
      value = std::stoll(immToken.value.substr(2), nullptr, 2);
    } else {
      // Decimal
      value = std::stoll(immToken.value);
    }
    
    return InstructionOperand::createImmediate(value, coil::ValueType::I32);
  }
    
  // Float immediate operand
  if (match(TokenType::Float)) {
    Token immToken = tokens[current - 1];
    double value = std::stod(immToken.value);
    
    return InstructionOperand::createFloatImmediate(value, coil::ValueType::F32);
  }
  
  // Label operand (identifier)
  if (match(TokenType::Identifier)) {
    Token labelToken = tokens[current - 1];
    
    return InstructionOperand::createLabel(labelToken.value);
  }
  
  throw std::runtime_error("Expected operand");
}

// Get current token
Token Parser::peek() const {
  if (isAtEnd()) {
    // Return a dummy EOF token
    Token eofToken;
    eofToken.type = TokenType::EndOfFile;
    eofToken.value = "EOF";
    eofToken.line = 0;
    eofToken.column = 0;
    return eofToken;
  }
  
  return tokens[current];
}

// Get next token without advancing
Token Parser::peekNext() const {
  if (current + 1 >= tokens.size()) {
    // Return a dummy EOF token
    Token eofToken;
    eofToken.type = TokenType::EndOfFile;
    eofToken.value = "EOF";
    eofToken.line = 0;
    eofToken.column = 0;
    return eofToken;
  }
  
  return tokens[current + 1];
}

// Advance to next token
Token Parser::advance() {
  if (!isAtEnd()) {
    current++;
  }
  return tokens[current - 1];
}

// Check if current token matches expected type
bool Parser::check(TokenType type) const {
  if (isAtEnd()) {
    return type == TokenType::EndOfFile;
  }
  
  return peek().type == type;
}

// Match and consume token if it matches expected type
bool Parser::match(TokenType type) {
  if (check(type)) {
    advance();
    return true;
  }
  
  return false;
}

// Consume token of expected type
Token Parser::consume(TokenType type, const std::string& message) {
  if (check(type)) {
    return advance();
  }
  
  error(peek(), message);
  throw std::runtime_error(message);
}

// Report an error
void Parser::error(const Token& token, const std::string& message) {
  std::stringstream ss;
  ss << "Line " << token.line << ", Column " << token.column << ": " << message;
  errors.push_back(ss.str());
}

// Check if at end of tokens
bool Parser::isAtEnd() const {
  return current >= tokens.size();
}

// Synchronize parser after error
void Parser::synchronize() {
  advance();
  
  while (!isAtEnd()) {
    // If we reached end of statement, we're synced
    if (tokens[current - 1].type == TokenType::Newline) {
      return;
    }
    
    // Certain tokens indicate start of next statement
    switch (peek().type) {
      case TokenType::Directive:
      case TokenType::Identifier:
      case TokenType::OpCode:
        return;
      default:
        break;
    }
    
    advance();
  }
}

} // namespace casm