#include "casm/parser.h"
#include <coil/type_system.h>
#include <sstream>
#include <iostream> // TODO: delete

namespace casm {

// OperandNode implementation
OperandNode::OperandNode(OperandType operandType, Token token, uint16_t type)
  : operandType_(operandType), token_(std::move(token)), coilType_(type) {
}

OperandNode::OperandType OperandNode::getOperandType() const {
  return operandType_;
}

const Token& OperandNode::getToken() const {
  return token_;
}

uint16_t OperandNode::getCoilType() const {
  return coilType_;
}

void OperandNode::setCoilType(uint16_t type) {
  coilType_ = type;
}

NodeType OperandNode::getType() const {
  return NodeType::OPERAND;
}

// InstructionNode implementation
InstructionNode::InstructionNode(Token instruction)
  : instruction_(std::move(instruction)) {
}

void InstructionNode::addOperand(std::unique_ptr<OperandNode> operand) {
  operands_.push_back(std::move(operand));
}

const Token& InstructionNode::getInstruction() const {
  return instruction_;
}

const std::vector<std::unique_ptr<OperandNode>>& InstructionNode::getOperands() const {
  return operands_;
}

NodeType InstructionNode::getType() const {
  return NodeType::INSTRUCTION;
}

// DirectiveNode implementation
DirectiveNode::DirectiveNode(Token directive)
  : directive_(std::move(directive)) {
}

void DirectiveNode::addOperand(std::unique_ptr<OperandNode> operand) {
  operands_.push_back(std::move(operand));
}

const Token& DirectiveNode::getDirective() const {
  return directive_;
}

const std::vector<std::unique_ptr<OperandNode>>& DirectiveNode::getOperands() const {
  return operands_;
}

NodeType DirectiveNode::getType() const {
  return NodeType::DIRECTIVE;
}

// LabelNode implementation
LabelNode::LabelNode(Token label)
  : label_(std::move(label)) {
}

const Token& LabelNode::getLabel() const {
  return label_;
}

NodeType LabelNode::getType() const {
  return NodeType::LABEL;
}

// ProgramNode implementation
ProgramNode::ProgramNode() {
}

void ProgramNode::addNode(std::unique_ptr<ASTNode> node) {
  nodes_.push_back(std::move(node));
}

const std::vector<std::unique_ptr<ASTNode>>& ProgramNode::getNodes() const {
  return nodes_;
}

NodeType ProgramNode::getType() const {
  return NodeType::PROGRAM;
}

// Parser implementation
Parser::Parser(std::vector<Token> tokens, ErrorHandler& errorHandler, const std::string& filename)
  : tokens_(std::move(tokens)), errorHandler_(errorHandler), filename_(filename), current_(0), currentSection_(0) {
}

std::unique_ptr<ProgramNode> Parser::parse() {
  auto program = std::make_unique<ProgramNode>();
  
  // Enter global scope
  symbolTable_.enterScope("global");
  
  // Parse each token in the token stream
  while (!isAtEnd()) {
      try {
          // Skip invalid tokens
          if (!peek().isValid()) {
              advance();
              continue;
          }
          
          // Process token based on type
          if (match(TokenType::LABEL)) {
            program->addNode(parseLabel());
          } else if (match(TokenType::INSTRUCTION)) {
            program->addNode(parseInstruction());
          } else if (match(TokenType::DIRECTIVE)) {
            program->addNode(parseDirective());
          } else {
            // Unexpected token
            error(peek(), "Unexpected token");
            advance();
          }
      } catch (const std::exception& e) {
          // Report error and try to recover
          error(peek(), std::string("Error during parsing: ") + e.what());
          synchronize();
      }
  }
  
  // Leave global scope
  symbolTable_.leaveScope();
  
  return program;
}

SymbolTable& Parser::getSymbolTable() {
  return symbolTable_;
}

Token Parser::peek() const {
  if (isAtEnd()) {
      return tokens_.back(); // EOF token
  }
  return tokens_[current_];
}

Token Parser::previous() const {
  if (current_ == 0) {
      return tokens_[0];
  }
  return tokens_[current_ - 1];
}

Token Parser::advance() {
  if (!isAtEnd()) {
      current_++;
  }
  return previous();
}

bool Parser::check(TokenType type) const {
  if (isAtEnd()) {
      return false;
  }
  return peek().type == type;
}

bool Parser::match(TokenType type) {
  if (check(type)) {
      advance();
      return true;
  }
  return false;
}

bool Parser::isAtEnd() const {
  return current_ >= tokens_.size();
}

bool Parser::consume(TokenType type, const std::string& message) {
  if (check(type)) {
      advance();
      return true;
  }
  
  error(peek(), message);
  return false;
}

void Parser::synchronize() {
  advance();
  
  while (!isAtEnd()) {
      // Stop at statement boundaries
      if (previous().type == TokenType::SEMICOLON) {
          return;
      }
      
      // Stop at certain token types that start new statements
      switch (peek().type) {
          case TokenType::INSTRUCTION:
          case TokenType::DIRECTIVE:
          case TokenType::LABEL:
              return;
          default:
              break;
      }
      
      advance();
  }
}

std::unique_ptr<LabelNode> Parser::parseLabel() {
  Token label = previous();
  
  // Define the label in the symbol table
  defineLabel(label);
  
  return std::make_unique<LabelNode>(label);
}

std::unique_ptr<InstructionNode> Parser::parseInstruction() {
  Token instruction = previous();
  auto node = std::make_unique<InstructionNode>(instruction);
  
  // Parse operands until end of line or EOF
  while (!isAtEnd() && peek().type != TokenType::SEMICOLON) {
      // Parse operand
      auto operand = parseOperand();
      
      // Add operand to instruction
      node->addOperand(std::move(operand));
      
      // Check for comma between operands
      if (check(TokenType::COMMA)) {
        advance(); // Consume comma
      } else {
        break; // No more operands
      }
  }
  
  // Consume optional semicolon
  if (check(TokenType::SEMICOLON)) {
      advance();
  }
  
  return node;
}

std::unique_ptr<DirectiveNode> Parser::parseDirective() {
  Token directive = previous();
  auto node = std::make_unique<DirectiveNode>(directive);
  
  // Parse operands until end of line or EOF
  while (!isAtEnd() && peek().type != TokenType::SEMICOLON) {
      // Parse operand
      auto operand = parseOperand();
      
      // Add operand to directive
      node->addOperand(std::move(operand));
      
      // Check for comma between operands
      if (check(TokenType::COMMA)) {
          advance(); // Consume comma
      } else {
          break; // No more operands
      }
  }
  
  // Consume optional semicolon
  if (check(TokenType::SEMICOLON)) {
      advance();
  }
  
  // Process special directives
  handleDirective(*node);
  
  return node;
}

std::unique_ptr<OperandNode> Parser::parseOperand() {
  Token token_last;
  Token token = peek();

  OperandNode::OperandType operandType;
  uint16_t coilType = 0;
  int repeating = 0;
  
repeat:
  // Determine operand type based on token
  if (token.type == TokenType::INTEGER_LITERAL) {
      operandType = OperandNode::OperandType::IMMEDIATE;
      // Determine integer type based on value size

      int64_t value;
      if (repeating) {
        value = token_last.getValue<int64_t>() | token.getValue<int64_t>();
      } else {
        value = token.getValue<int64_t>();
      }
      if (value >= INT8_MIN && value <= INT8_MAX) {
          coilType = coil::Type::INT8 | coil::Type::IMM;
      } else if (value >= INT16_MIN && value <= INT16_MAX) {
          coilType = coil::Type::INT16 | coil::Type::IMM;
      } else if (value >= INT32_MIN && value <= INT32_MAX) {
          coilType = coil::Type::INT32 | coil::Type::IMM;
      } else {
          coilType = coil::Type::INT64 | coil::Type::IMM;
      }
      advance();
  } else if (token.type == TokenType::FLOAT_LITERAL) {
      operandType = OperandNode::OperandType::IMMEDIATE;
      // Determine float type (default to FP64)
      coilType = coil::Type::FP64 | coil::Type::IMM;
      advance();
  } else if (token.type == TokenType::STRING_LITERAL) {
      operandType = OperandNode::OperandType::IMMEDIATE;
      // String literals have no direct COIL type
      advance();
  } else if (token.type == TokenType::REGISTER_NAME) {
      operandType = OperandNode::OperandType::REGISTER;
      // Register type will be determined during code generation
      advance();
  } else if (token.type == TokenType::VAR_ID) {
      operandType = OperandNode::OperandType::VARIABLE;
      // Variable types are resolved during code generation
      coilType = coil::Type::VAR | coil::Type::VAR_ID;
      advance();
  } else if (token.type == TokenType::IDENTIFIER) {
      operandType = OperandNode::OperandType::LABEL;
      advance();
  } else if (token.type == TokenType::TYPE_SPECIFIER) {
      operandType = OperandNode::OperandType::TYPE;
      auto type = parseType(token);
      if (type) {
        coilType = *type;
      }
      advance();

    type_repeat:
      Token tok1 = peek();
      if (tok1.type == TokenType::EQUAL) {
        advance();
        tok1 = peek();
        
        // parameters are not even close to being implemented
        std::cout << "next token after equals: " << tok1.__print() << std::endl;
        
        advance();
        goto type_repeat;
      } else if (tok1.type == TokenType::PLUS) { // type extension (imm/var/symbol, const, volatile, etc...)
        advance();
        tok1 = peek();
        advance();
        goto type_repeat;
      }
  } else if (token.type == TokenType::OPEN_BRACKET) {
      // Memory operand: [expression]
      operandType = OperandNode::OperandType::MEMORY;
      advance(); // Consume '['
      
      // Parse memory expression (simplified for now)
      while (!isAtEnd() && peek().type != TokenType::CLOSE_BRACKET) {
          advance();
      }
      
      // Check for closing bracket
      if (!consume(TokenType::CLOSE_BRACKET, "Expected ']' after memory operand")) {
          // Error already reported
      }
  } else {
      error(token, "Unexpected token in operand");
      operandType = OperandNode::OperandType::EXPRESSION;
      advance();
  }

  token_last = token;
  token = peek();
  if (token.type == TokenType::PIPE) {
    advance(); // advance pipe
    token = peek();
    repeating = 1;
    goto repeat;
  }  
  
  return std::make_unique<OperandNode>(operandType, token, coilType);
}

void Parser::handleDirective(const DirectiveNode& directive) {
  // Process directive based on type
  const std::string& directiveName = directive.getDirective().lexeme;
  
  if (directiveName == "SECTION") {
      defineSection(directive);
  }
  // Other directives handled similarly...
}

void Parser::defineLabel(const Token& label) {
  // Create symbol info
  SymbolInfo symbolInfo;
  symbolInfo.name = label.lexeme;
  symbolInfo.type = SymbolType::LABEL;
  symbolInfo.address = 0; // Address will be resolved during code generation
  symbolInfo.sectionIndex = currentSection_;
  symbolInfo.attributes = 0;
  symbolInfo.defined = true;
  
  // Add to symbol table
  if (!symbolTable_.addSymbol(symbolInfo)) {
      error(label, "Label already defined: " + label.lexeme);
  }
}

void Parser::defineSection(const DirectiveNode& directive) {
  // Check for required operands
  const auto& operands = directive.getOperands();
  if (operands.size() < 1) {
      error(directive.getDirective(), "SECTION directive requires at least a name operand");
      return;
  }
  
  // Get section name
  const Token& nameToken = operands[0]->getToken();
  std::string sectionName = nameToken.lexeme;
  
  // Create symbol info for section
  SymbolInfo symbolInfo;
  symbolInfo.name = sectionName;
  symbolInfo.type = SymbolType::SECTION;
  symbolInfo.address = 0;
  symbolInfo.sectionIndex = 0; // Will be updated during code generation
  symbolInfo.attributes = 0;
  symbolInfo.defined = true;
  
  // Add to symbol table
  if (!symbolTable_.addSymbol(symbolInfo)) {
      error(nameToken, "Section already defined: " + sectionName);
  }
  
  // Current section will be updated during code generation
}

std::optional<uint16_t> Parser::parseType(const Token& token) {
  const std::string& typeName = token.lexeme;
  
  // Map CASM type name to COIL type
  if (typeName == "TYPE_INT8") return coil::Type::INT8;
  if (typeName == "TYPE_INT16") return coil::Type::INT16;
  if (typeName == "TYPE_INT32") return coil::Type::INT32;
  if (typeName == "TYPE_INT64") return coil::Type::INT64;
  if (typeName == "TYPE_UNT8") return coil::Type::UNT8;
  if (typeName == "TYPE_UNT16") return coil::Type::UNT16;
  if (typeName == "TYPE_UNT32") return coil::Type::UNT32;
  if (typeName == "TYPE_UNT64") return coil::Type::UNT64;
  if (typeName == "TYPE_FP16") return coil::Type::FP16;
  if (typeName == "TYPE_FP32") return coil::Type::FP32;
  if (typeName == "TYPE_FP64") return coil::Type::FP64;
  if (typeName == "TYPE_FP128") return coil::Type::FP128;
  if (typeName == "TYPE_V128") return coil::Type::V128;
  if (typeName == "TYPE_V256") return coil::Type::V256;
  if (typeName == "TYPE_V512") return coil::Type::V512;
  if (typeName == "TYPE_BIT") return coil::Type::BIT;
  if (typeName == "TYPE_VOID") return coil::Type::VOID;
  if (typeName == "TYPE_INT") return coil::Type::INT;
  if (typeName == "TYPE_UNT") return coil::Type::UNT;
  if (typeName == "TYPE_FP") return coil::Type::FP;
  if (typeName == "TYPE_PTR") return coil::Type::PTR;
  if (typeName == "TYPE_VAR") return coil::Type::VAR;
  if (typeName == "TYPE_SYM") return coil::Type::SYM;
  if (typeName == "TYPE_RGP") return coil::Type::RGP;
  if (typeName == "TYPE_RFP") return coil::Type::RFP;
  if (typeName == "TYPE_RV") return coil::Type::RV;
  if (typeName == "TYPE_STRUCT") return coil::Type::STRUCT;
  if (typeName == "TYPE_PACK") return coil::Type::PACK;
  if (typeName == "TYPE_UNION") return coil::Type::UNION;
  if (typeName == "TYPE_ARRAY") return coil::Type::ARRAY;
  if (typeName == "TYPE_PARAM4") return coil::Type::PARAM4;
  if (typeName == "TYPE_PARAM3") return coil::Type::PARAM3;
  if (typeName == "TYPE_PARAM2") return coil::Type::PARAM2;
  if (typeName == "TYPE_PARAM1") return coil::Type::PARAM1;
  if (typeName == "TYPE_PARAM0") return coil::Type::PARAM0;
  
  // Unknown type
  error(token, "Unknown type specifier: " + typeName);
  return std::nullopt;
}

void Parser::error(const Token& token, const std::string& message) {
  errorHandler_.addError(
      0x01000003, // Generic parsing error code
      message,
      token,
      filename_,
      ErrorSeverity::ERROR
  );
}

} // namespace casm