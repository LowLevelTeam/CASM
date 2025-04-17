/**
* @file parser.hpp
* @brief Parser for CASM assembly language
*/

#pragma once
#include "casm/lexer.hpp"
#include <coil/instr.hpp>
#include <coil/obj.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace casm {

/**
* @brief Type of CASM statement
*/
enum class StatementType {
  Section,      ///< Section declaration
  Label,        ///< Label definition
  Instruction,  ///< Instruction statement
  Directive     ///< Assembler directive
};

/**
* @brief Base class for CASM statements
*/
struct Statement {
  StatementType type;  ///< Type of statement
  size_t line;        ///< Line number
  
  /**
  * @brief Virtual destructor
  */
  virtual ~Statement() = default;
  
  /**
  * @brief Create a new statement
  * @param type Statement type
  * @param line Line number
  */
  Statement(StatementType type, size_t line) : type(type), line(line) {}
};

/**
* @brief Section declaration statement
*/
struct SectionStatement : public Statement {
  std::string name;  ///< Section name
  
  /**
  * @brief Create a section statement
  * @param name Section name
  * @param line Line number
  */
  SectionStatement(const std::string& name, size_t line)
    : Statement(StatementType::Section, line), name(name) {}
};

/**
* @brief Label definition statement
*/
struct LabelStatement : public Statement {
  std::string name;  ///< Label name
  
  /**
  * @brief Create a label statement
  * @param name Label name
  * @param line Line number
  */
  LabelStatement(const std::string& name, size_t line)
    : Statement(StatementType::Label, line), name(name) {}
};

/**
* @brief Operand for an instruction
*/
struct InstructionOperand {
  enum class Type {
    Register,
    Immediate,
    FloatImmediate,
    Memory,
    Label
  };
  
  Type type;                     ///< Operand type
  coil::ValueType valueType;     ///< Value type
  
  union {
    uint32_t reg;                ///< Register number
    int64_t imm;                 ///< Immediate value
    double fpImm;                ///< Floating point immediate
    struct {
      uint32_t base;             ///< Base register
      int32_t offset;            ///< Memory offset
    } mem;                       ///< Memory reference
  };
  std::string labelName;         ///< Label name (not in union due to std::string)
  
  /**
  * @brief Create a register operand
  * @param reg Register number
  * @param vtype Value type
  * @return Register operand
  */
  static InstructionOperand createRegister(uint32_t reg, coil::ValueType vtype);
  
  /**
  * @brief Create an immediate operand
  * @param imm Immediate value
  * @param vtype Value type
  * @return Immediate operand
  */
  static InstructionOperand createImmediate(int64_t imm, coil::ValueType vtype);
  
  /**
  * @brief Create a floating-point immediate operand
  * @param imm Immediate value
  * @param vtype Value type
  * @return Float immediate operand
  */
  static InstructionOperand createFloatImmediate(double imm, coil::ValueType vtype);
  
  /**
  * @brief Create a memory operand
  * @param base Base register
  * @param offset Memory offset
  * @param vtype Value type
  * @return Memory operand
  */
  static InstructionOperand createMemory(uint32_t base, int32_t offset, coil::ValueType vtype);
  
  /**
  * @brief Create a label operand
  * @param name Label name
  * @return Label operand
  */
  static InstructionOperand createLabel(const std::string& name);
};

/**
* @brief Instruction statement
*/
struct InstructionStatement : public Statement {
  std::string opcode;                               ///< Instruction opcode
  coil::InstrFlag0 flag;                            ///< Instruction flag
  std::vector<InstructionOperand> operands;         ///< Instruction operands
  coil::ValueType valueType;                        ///< Value type for instruction
  
  /**
  * @brief Create an instruction statement
  * @param opcode Instruction opcode
  * @param flag Instruction flag
  * @param operands Instruction operands
  * @param valueType Value type for instruction
  * @param line Line number
  */
  InstructionStatement(
    const std::string& opcode,
    coil::InstrFlag0 flag,
    const std::vector<InstructionOperand>& operands,
    coil::ValueType valueType,
    size_t line
  ) : Statement(StatementType::Instruction, line),
      opcode(opcode), flag(flag), operands(operands),
      valueType(valueType) {}
};

/**
* @brief Directive statement
*/
struct DirectiveStatement : public Statement {
  std::string name;                ///< Directive name
  std::vector<std::string> args;   ///< Directive arguments
  
  /**
  * @brief Create a directive statement
  * @param name Directive name
  * @param args Directive arguments
  * @param line Line number
  */
  DirectiveStatement(
    const std::string& name,
    const std::vector<std::string>& args,
    size_t line
  ) : Statement(StatementType::Directive, line),
      name(name), args(args) {}
};

/**
* @brief Parser for CASM assembly language
* 
* Parses tokenized CASM code into an abstract syntax tree.
*/
class Parser {
public:
  /**
  * @brief Initialize parser with tokens
  * @param tokens Tokens from lexer
  */
  explicit Parser(const std::vector<Token>& tokens);
  
  /**
  * @brief Parse tokens into statements
  * @return Vector of parsed statements
  */
  std::vector<std::unique_ptr<Statement>> parse();
  
  /**
  * @brief Get parsing errors
  * @return Vector of error messages
  */
  const std::vector<std::string>& getErrors() const;
  
private:
  std::vector<Token> tokens;                      ///< Input tokens
  size_t current;                                 ///< Current token index
  std::vector<std::string> errors;                ///< Parsing errors
  std::unordered_map<std::string, coil::Opcode> opcodeMap;  ///< Map of opcode strings to enums
  std::unordered_map<std::string, coil::InstrFlag0> flagMap;  ///< Map of flag strings to enums
  std::unordered_map<std::string, coil::ValueType> typeMap;   ///< Map of type strings to enums
  
  /**
  * @brief Initialize opcode, flag, and type maps
  */
  void initMaps();
  
  /**
  * @brief Parse a single statement
  * @return Parsed statement
  */
  std::unique_ptr<Statement> parseStatement();
  
  /**
  * @brief Parse a section statement
  * @return Parsed section statement
  */
  std::unique_ptr<SectionStatement> parseSection();
  
  /**
  * @brief Parse a label statement
  * @return Parsed label statement
  */
  std::unique_ptr<LabelStatement> parseLabel();
  
  /**
  * @brief Parse an instruction statement
  * @return Parsed instruction statement
  */
  std::unique_ptr<InstructionStatement> parseInstruction();
  
  /**
  * @brief Parse a directive statement
  * @return Parsed directive statement
  */
  std::unique_ptr<DirectiveStatement> parseDirective();
  
  /**
  * @brief Parse an instruction operand
  * @return Parsed operand
  */
  InstructionOperand parseOperand();
  
  /**
  * @brief Get current token
  * @return Current token
  */
  Token peek() const;

  /**
  * @brief Get next token
  * @return Next token
  */
  Token peekNext() const;
  
  /**
  * @brief Advance to next token
  * @return Previous token
  */
  Token advance();
  
  /**
  * @brief Check if current token matches expected type
  * @param type Expected token type
  * @return True if matches
  */
  bool check(TokenType type) const;
  
  /**
  * @brief Match and consume token if it matches expected type
  * @param type Expected token type
  * @return True if token consumed
  */
  bool match(TokenType type);
  
  /**
  * @brief Consume token of expected type
  * @param type Expected token type
  * @param message Error message on failure
  * @return Consumed token
  */
  Token consume(TokenType type, const std::string& message);
  
  /**
  * @brief Report an error
  * @param token Token where error occurred
  * @param message Error message
  */
  void error(const Token& token, const std::string& message);
  
  /**
  * @brief Check if at end of tokens
  * @return True if at end
  */
  bool isAtEnd() const;
  
  /**
  * @brief Synchronize parser after error
  */
  void synchronize();
};

} // namespace casm