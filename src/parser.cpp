#include <casm/parser.hpp>
#include <sstream>

namespace casm {

// Operand implementation
std::unique_ptr<Operand> Operand::createRegister(const std::string& name) {
  return std::make_unique<RegisterOperand>(name);
}

std::unique_ptr<Operand> Operand::createImmediate(const ImmediateValue& value) {
  return std::make_unique<ImmediateOperand>(value);
}

std::unique_ptr<Operand> Operand::createMemory(const MemoryReference& memRef) {
  return std::make_unique<MemoryOperand>(memRef);
}

std::unique_ptr<Operand> Operand::createLabel(const std::string& label) {
  return std::make_unique<LabelOperand>(label);
}

// RegisterOperand implementation
RegisterOperand::RegisterOperand(std::string name)
  : m_name(std::move(name)) {
}

std::string RegisterOperand::toString() const {
  return "%r" + m_name;
}

// ImmediateOperand implementation
ImmediateOperand::ImmediateOperand(ImmediateValue value)
  : m_value(std::move(value)) {
}

std::string ImmediateOperand::toString() const {
  std::ostringstream ss;
  ss << "$" << m_value.toString();
  return ss.str();
}

// MemoryOperand implementation
MemoryOperand::MemoryOperand(MemoryReference memRef)
  : m_memRef(std::move(memRef)) {
}

std::string MemoryOperand::toString() const {
  std::ostringstream ss;
  ss << "[%r" << m_memRef.reg;
  if (m_memRef.offset > 0) {
    ss << "+" << m_memRef.offset;
  } else if (m_memRef.offset < 0) {
    ss << m_memRef.offset; // Negative sign is included
  }
  ss << "]";
  return ss.str();
}

// LabelOperand implementation
LabelOperand::LabelOperand(std::string label)
  : m_label(std::move(label)) {
}

std::string LabelOperand::toString() const {
  return "@" + m_label;
}

// Instruction implementation
Instruction::Instruction(std::string name, std::vector<std::string> parameters)
  : m_name(std::move(name)), m_parameters(std::move(parameters)) {
}

void Instruction::addOperand(std::unique_ptr<Operand> operand) {
  m_operands.push_back(std::move(operand));
}

std::string Instruction::toString() const {
  std::ostringstream ss;
  ss << m_name;
  
  // Add parameters if any
  for (const auto& param : m_parameters) {
    ss << " ^" << param;
  }
  
  // Add operands if any
  for (const auto& op : m_operands) {
    ss << " " << op->toString();
  }
  
  return ss.str();
}

// Directive implementation
Directive::Directive(std::string name, std::vector<std::unique_ptr<Operand>> operands)
  : m_name(std::move(name)) {
  for (auto& op : operands) {
    m_operands.push_back(std::move(op));
  }
}

void Directive::addOperand(std::unique_ptr<Operand> operand) {
  m_operands.push_back(std::move(operand));
}

std::string Directive::toString() const {
  std::ostringstream ss;
  ss << "." << m_name;
  
  // Add operands if any
  for (const auto& op : m_operands) {
    ss << " " << op->toString();
  }
  
  return ss.str();
}

// Statement implementation
Statement::Statement()
  : m_type(Type::Empty) {
}

Statement::Statement(std::string label)
  : m_type(Type::Label), m_label(std::move(label)) {
}

Statement::Statement(std::unique_ptr<Instruction> instruction, std::string label)
  : m_type(Type::Instruction), m_label(std::move(label)), m_instruction(std::move(instruction)) {
}

Statement::Statement(std::unique_ptr<Directive> directive, std::string label)
  : m_type(Type::Directive), m_label(std::move(label)), m_directive(std::move(directive)) {
}

std::string Statement::toString() const {
  std::ostringstream ss;
  
  // Add label if present
  if (!m_label.empty()) {
    ss << "#" << m_label << " ";
  }
  
  // Add instruction or directive
  switch (m_type) {
    case Type::Instruction:
      ss << m_instruction->toString();
      break;
    case Type::Directive:
      ss << m_directive->toString();
      break;
    case Type::Label:
      // Label-only statement, already output above
      break;
    case Type::Empty:
      // Empty statement, nothing to output
      break;
  }
  
  return ss.str();
}

// Parser implementation
Parser::Parser(Lexer& lexer)
  : m_lexer(lexer) {
}

std::vector<Statement> Parser::parse() {
  std::vector<Statement> statements;
  
  while (peek().type != TokenType::EndOfFile) {
    try {
      statements.push_back(parseStatement());
    } catch (const ParserException& e) {
      m_errors.push_back(e.what());
      
      // Skip tokens until end of line or end of file
      while (peek().type != TokenType::EndOfLine && peek().type != TokenType::EndOfFile) {
        advance();
      }
      
      // Consume the EOL if present
      if (peek().type == TokenType::EndOfLine) {
        advance();
      }
    }
  }
  
  return statements;
}

Statement Parser::parseStatement() {
  // Skip any leading comments and empty lines
  while (peek().type == TokenType::Comment || peek().type == TokenType::EndOfLine) {
    advance();
  }
  
  // Check for end of file
  if (peek().type == TokenType::EndOfFile) {
    return Statement();
  }
  
  // Parse optional label
  std::string label;
  if (peek().type == TokenType::Label) {
    label = parseLabel();
    
    // If EOL after label, it's a label-only statement
    if (peek().type == TokenType::EndOfLine || peek().type == TokenType::EndOfFile) {
      advance(); // Consume EOL
      return Statement(label);
    }
  }
  
  // Parse instruction or directive
  if (peek().type == TokenType::Instruction) {
    auto instruction = parseInstruction();
    
    // Consume EOL
    consume(TokenType::EndOfLine, "Expected end of line after instruction");
    
    return Statement(std::move(instruction), label);
  } else if (peek().type == TokenType::Directive) {
    auto directive = parseDirective();
    
    // Consume EOL
    consume(TokenType::EndOfLine, "Expected end of line after directive");
    
    return Statement(std::move(directive), label);
  } else if (!label.empty()) {
    // We had a label but no instruction or directive
    advance(); // Consume the current token
    return Statement(label);
  } else {
    std::ostringstream ss;
    ss << "Expected instruction or directive, got " << peek().toString();
    throw ParserException(ss.str());
  }
}

Token Parser::consume(TokenType type, const std::string& expected) {
  if (peek().type == type) {
    return advance();
  }
  
  std::ostringstream ss;
  ss << expected << ", got " << peek().toString();
  throw ParserException(ss.str());
}

bool Parser::match(TokenType type) {
  if (peek().type == type) {
    advance();
    return true;
  }
  return false;
}

Token Parser::peek() {
  return m_lexer.peekToken();
}

Token Parser::advance() {
  return m_lexer.nextToken();
}

std::string Parser::parseLabel() {
  Token token = consume(TokenType::Label, "Expected label");
  return token.value;
}

std::unique_ptr<Instruction> Parser::parseInstruction() {
  Token token = consume(TokenType::Instruction, "Expected instruction");
  
  // Parse optional parameters
  std::vector<std::string> parameters = parseParameters();
  
  // Create instruction
  auto instruction = std::make_unique<Instruction>(token.value, parameters);
  
  // Parse operands
  while (peek().type != TokenType::EndOfLine && peek().type != TokenType::EndOfFile) {
    // Parse operand
    auto operand = parseOperand();
    instruction->addOperand(std::move(operand));
    
    // Check for comma
    if (peek().type == TokenType::Comma) {
      advance(); // Consume comma
    } else if (peek().type != TokenType::EndOfLine && peek().type != TokenType::EndOfFile) {
      // If not EOL or EOF, expect a comma
      std::ostringstream ss;
      ss << "Expected comma or end of line, got " << peek().toString();
      throw ParserException(ss.str());
    }
  }
  
  return instruction;
}

std::unique_ptr<Directive> Parser::parseDirective() {
  Token token = consume(TokenType::Directive, "Expected directive");
  
  // Create directive
  auto directive = std::make_unique<Directive>(token.value);
  
  // Parse operands
  while (peek().type != TokenType::EndOfLine && peek().type != TokenType::EndOfFile) {
    // Parse operand
    auto operand = parseOperand();
    directive->addOperand(std::move(operand));
    
    // Check for comma
    if (peek().type == TokenType::Comma) {
      advance(); // Consume comma
    } else if (peek().type != TokenType::EndOfLine && peek().type != TokenType::EndOfFile) {
      // If not EOL or EOF, expect a comma
      std::ostringstream ss;
      ss << "Expected comma or end of line, got " << peek().toString();
      throw ParserException(ss.str());
    }
  }
  
  return directive;
}

std::unique_ptr<Operand> Parser::parseOperand() {
  switch (peek().type) {
    case TokenType::Register:
      return parseRegister();
    case TokenType::Immediate:
      return parseImmediate();
    case TokenType::MemoryRef:
      return parseMemoryRef();
    case TokenType::LabelRef:
      return parseLabelRef();
    case TokenType::Parameter: {
      // Parameters are handled separately for instructions
      std::ostringstream ss;
      ss << "Unexpected parameter in operand position: " << peek().toString();
      throw ParserException(ss.str());
    } default: {
      std::ostringstream ss;
      ss << "Expected operand, got " << peek().toString();
      throw ParserException(ss.str());
    }
  }
}

std::vector<std::string> Parser::parseParameters() {
  std::vector<std::string> parameters;
  
  while (peek().type == TokenType::Parameter) {
    Token token = advance();
    parameters.push_back(token.value);
  }
  
  return parameters;
}

std::unique_ptr<Operand> Parser::parseRegister() {
  Token token = consume(TokenType::Register, "Expected register");
  return Operand::createRegister(token.value);
}

std::unique_ptr<Operand> Parser::parseImmediate() {
  Token token = consume(TokenType::Immediate, "Expected immediate value");
  
  if (!token.immediateValue) {
    std::ostringstream ss;
    ss << "Invalid immediate value: " << token.value;
    throw ParserException(ss.str());
  }
  
  return Operand::createImmediate(*token.immediateValue);
}

std::unique_ptr<Operand> Parser::parseMemoryRef() {
  Token token = consume(TokenType::MemoryRef, "Expected memory reference");
  
  if (!token.memoryRef) {
    std::ostringstream ss;
    ss << "Invalid memory reference: " << token.value;
    throw ParserException(ss.str());
  }
  
  return Operand::createMemory(*token.memoryRef);
}

std::unique_ptr<Operand> Parser::parseLabelRef() {
  Token token = consume(TokenType::LabelRef, "Expected label reference");
  return Operand::createLabel(token.value);
}

} // namespace casm