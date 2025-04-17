#pragma once
#include "casm/lexer.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>

namespace casm {

// Forward declarations
class Instruction;
class Directive;
class Statement;

/**
 * @brief Base class for operands in instructions
 */
class Operand {
public:
  enum class Type {
    Register,   // Register operand (%r1)
    Immediate,  // Immediate value ($imm)
    Memory,     // Memory reference ([%r1+8])
    Label       // Label reference (@label)
  };
  
  virtual ~Operand() = default;
  virtual Type getType() const = 0;
  virtual std::string toString() const = 0;
  
  // Helper functions to create operands
  static std::unique_ptr<Operand> createRegister(const std::string& name);
  static std::unique_ptr<Operand> createImmediate(const ImmediateValue& value);
  static std::unique_ptr<Operand> createMemory(const MemoryReference& memRef);
  static std::unique_ptr<Operand> createLabel(const std::string& label);
};

/**
 * @brief Register operand (%r1)
 */
class RegisterOperand : public Operand {
public:
  explicit RegisterOperand(std::string name);
  
  Type getType() const override { return Type::Register; }
  std::string toString() const override;
  
  const std::string& getName() const { return m_name; }
  
private:
  std::string m_name;
};

/**
 * @brief Immediate value operand ($imm)
 */
class ImmediateOperand : public Operand {
public:
  explicit ImmediateOperand(ImmediateValue value);
  
  Type getType() const override { return Type::Immediate; }
  std::string toString() const override;
  
  const ImmediateValue& getValue() const { return m_value; }
  
private:
  ImmediateValue m_value;
};

/**
 * @brief Memory reference operand ([%r1+8])
 */
class MemoryOperand : public Operand {
public:
  explicit MemoryOperand(MemoryReference memRef);
  
  Type getType() const override { return Type::Memory; }
  std::string toString() const override;
  
  const MemoryReference& getReference() const { return m_memRef; }
  
private:
  MemoryReference m_memRef;
};

/**
 * @brief Label reference operand (@label)
 */
class LabelOperand : public Operand {
public:
  explicit LabelOperand(std::string label);
  
  Type getType() const override { return Type::Label; }
  std::string toString() const override;
  
  const std::string& getLabel() const { return m_label; }
  
private:
  std::string m_label;
};

/**
 * @brief Instruction in the CASM language
 */
class Instruction {
public:
  Instruction(std::string name, 
              std::vector<std::string> parameters = {});
  
  void addOperand(std::unique_ptr<Operand> operand);
  
  const std::string& getName() const { return m_name; }
  const std::vector<std::string>& getParameters() const { return m_parameters; }
  const std::vector<std::unique_ptr<Operand>>& getOperands() const { return m_operands; }
  
  std::string toString() const;
  
private:
  std::string m_name;
  std::vector<std::string> m_parameters;
  std::vector<std::unique_ptr<Operand>> m_operands;
};

/**
 * @brief Directive in the CASM language
 */
class Directive {
public:
  Directive(std::string name, 
            std::vector<std::unique_ptr<Operand>> operands = {});
  
  void addOperand(std::unique_ptr<Operand> operand);
  
  const std::string& getName() const { return m_name; }
  const std::vector<std::unique_ptr<Operand>>& getOperands() const { return m_operands; }
  
  std::string toString() const;
  
private:
  std::string m_name;
  std::vector<std::unique_ptr<Operand>> m_operands;
};

/**
 * @brief A statement in the CASM language (instruction, directive, or label)
 */
class Statement {
public:
  enum class Type {
    Instruction,  // Instruction statement
    Directive,    // Directive statement
    Label,        // Label definition
    Empty         // Empty statement (e.g., blank line or comment-only)
  };
  
  // Empty statement
  Statement();
  
  // Label statement
  explicit Statement(std::string label);
  
  // Instruction statement with optional label
  Statement(std::unique_ptr<Instruction> instruction, 
            std::string label = "");
  
  // Directive statement with optional label
  Statement(std::unique_ptr<Directive> directive, 
            std::string label = "");
  
  Type getType() const { return m_type; }
  const std::string& getLabel() const { return m_label; }
  
  Instruction* getInstruction() const { 
    return m_type == Type::Instruction ? m_instruction.get() : nullptr; 
  }
  
  Directive* getDirective() const { 
    return m_type == Type::Directive ? m_directive.get() : nullptr; 
  }
  
  std::string toString() const;
  
private:
  Type m_type;
  std::string m_label;
  std::unique_ptr<Instruction> m_instruction;
  std::unique_ptr<Directive> m_directive;
};

/**
 * @brief Parser for the CASM language
 */
class Parser {
public:
  /**
   * @brief Construct a parser from a lexer
   * @param lexer Lexer to read tokens from
   */
  explicit Parser(Lexer& lexer);
  
  /**
   * @brief Parse the entire input and return all statements
   * @return Vector of parsed statements
   */
  std::vector<Statement> parse();
  
  /**
   * @brief Parse a single statement
   * @return Parsed statement
   */
  Statement parseStatement();
  
  /**
   * @brief Get parser errors
   * @return Vector of error messages
   */
  const std::vector<std::string>& getErrors() const { return m_errors; }
  
private:
  Lexer& m_lexer;
  std::vector<std::string> m_errors;
  
  // Helper methods for parsing
  Token consume(TokenType type, const std::string& expected);
  bool match(TokenType type);
  Token peek();
  Token advance();
  
  // Parsers for specific constructs
  std::string parseLabel();
  std::unique_ptr<Instruction> parseInstruction();
  std::unique_ptr<Directive> parseDirective();
  std::unique_ptr<Operand> parseOperand();
  std::vector<std::string> parseParameters();
  std::unique_ptr<Operand> parseRegister();
  std::unique_ptr<Operand> parseImmediate();
  std::unique_ptr<Operand> parseMemoryRef();
  std::unique_ptr<Operand> parseLabelRef();
};

} // namespace casm