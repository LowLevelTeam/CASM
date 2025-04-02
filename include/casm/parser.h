#ifndef CASM_PARSER_H
#define CASM_PARSER_H

#include "token.h"
#include "error_handler.h"
#include "symbol_table.h"
#include <vector>
#include <memory>
#include <optional>
#include <variant>
#include <coil/type_system.h>

namespace casm {

/**
* AST node types
*/
enum class NodeType {
  PROGRAM,
  INSTRUCTION,
  DIRECTIVE,
  LABEL,
  OPERAND,
  EXPRESSION
};

/**
* Base AST node
*/
class ASTNode {
public:
  virtual ~ASTNode() = default;
  virtual NodeType getType() const = 0;
};

/**
* Operand node
*/
class OperandNode : public ASTNode {
public:
  enum class OperandType {
      REGISTER,
      IMMEDIATE,
      MEMORY,
      VARIABLE,
      LABEL,
      TYPE,
      EXPRESSION
  };
  
  OperandNode(OperandType operandType, Token token, uint16_t type = 0);
  
  OperandType getOperandType() const;
  const Token& getToken() const;
  uint16_t getCoilType() const;
  void setCoilType(uint16_t type);
  
  NodeType getType() const override;
  
private:
  OperandType operandType_;
  Token token_;
  uint16_t coilType_;
};

/**
* Instruction node
*/
class InstructionNode : public ASTNode {
public:
  InstructionNode(Token instruction);
  
  void addOperand(std::unique_ptr<OperandNode> operand);
  
  const Token& getInstruction() const;
  const std::vector<std::unique_ptr<OperandNode>>& getOperands() const;
  
  NodeType getType() const override;
  
private:
  Token instruction_;
  std::vector<std::unique_ptr<OperandNode>> operands_;
};

/**
* Directive node
*/
class DirectiveNode : public ASTNode {
public:
  DirectiveNode(Token directive);
  
  void addOperand(std::unique_ptr<OperandNode> operand);
  
  const Token& getDirective() const;
  const std::vector<std::unique_ptr<OperandNode>>& getOperands() const;
  
  NodeType getType() const override;
  
private:
  Token directive_;
  std::vector<std::unique_ptr<OperandNode>> operands_;
};

/**
* Label node
*/
class LabelNode : public ASTNode {
public:
  LabelNode(Token label);
  
  const Token& getLabel() const;
  
  NodeType getType() const override;
  
private:
  Token label_;
};

/**
* Program node (root of AST)
*/
class ProgramNode : public ASTNode {
public:
  ProgramNode();
  
  void addNode(std::unique_ptr<ASTNode> node);
  
  const std::vector<std::unique_ptr<ASTNode>>& getNodes() const;
  
  NodeType getType() const override;
  
private:
  std::vector<std::unique_ptr<ASTNode>> nodes_;
};

/**
* Parser class for parsing tokens into an AST
*/
class Parser {
public:
  /**
    * Constructor
    * @param tokens Tokens to parse
    * @param errorHandler Error handler
    * @param filename Source filename
    */
  Parser(std::vector<Token> tokens, ErrorHandler& errorHandler, const std::string& filename);
  
  /**
    * Parse tokens into an AST
    * @return Program node (root of AST)
    */
  std::unique_ptr<ProgramNode> parse();
  
  /**
    * Get the symbol table
    * @return Reference to the symbol table
    */
  SymbolTable& getSymbolTable();
  
private:
  std::vector<Token> tokens_;
  ErrorHandler& errorHandler_;
  std::string filename_;
  size_t current_;
  SymbolTable symbolTable_;
  unsigned int currentSection_;
  
  Token peek() const;
  Token previous() const;
  Token advance();
  bool check(TokenType type) const;
  bool match(TokenType type);
  bool isAtEnd() const;
  
  bool consume(TokenType type, const std::string& message);
  
  void synchronize();
  
  std::unique_ptr<LabelNode> parseLabel();
  std::unique_ptr<InstructionNode> parseInstruction();
  std::unique_ptr<DirectiveNode> parseDirective();
  std::unique_ptr<OperandNode> parseOperand();
  
  void handleDirective(const DirectiveNode& directive);
  void defineLabel(const Token& label);
  void defineSection(const DirectiveNode& directive);
  
  std::optional<uint16_t> parseType(const Token& token);
  
  void error(const Token& token, const std::string& message);
};

} // namespace casm

#endif // CASM_PARSER_H