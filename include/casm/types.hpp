/**
 * @file types.hpp
 * @brief Common type definitions for CASM
 */

#pragma once
#include "coil/types.hpp"
#include "coil/instr.hpp"
#include "coil/err.hpp"
#include "coil/obj.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace casm {

/**
 * @brief Result codes for operations
 */
using Result = coil::Result;

/**
 * @brief Token types for the parser
 */
enum class TokenType {
  Invalid,      ///< Invalid token
  Identifier,   ///< Identifier or label
  Register,     ///< Register reference
  Integer,      ///< Integer literal
  Float,        ///< Float literal
  String,       ///< String literal
  LBracket,     ///< Left bracket [
  RBracket,     ///< Right bracket ]
  Plus,         ///< Plus +
  Minus,        ///< Minus -
  Comma,        ///< Comma ,
  Colon,        ///< Colon :
  Directive,    ///< Directive (starts with .)
  Comment,      ///< Comment (starts with ;)
  EOL           ///< End of line
};

/**
 * @brief Single token from the lexer
 */
struct Token {
  TokenType type;    ///< Token type
  std::string text;  ///< Token text
  size_t line;       ///< Line number
  size_t column;     ///< Column number
};

/**
 * @brief Register reference with type information
 */
struct RegisterRef {
  uint32_t number;         ///< Register number
  coil::ValueType type;    ///< Register value type
  bool has_explicit_type;  ///< Whether type was explicitly specified
};

/**
 * @brief Memory reference 
 */
struct MemoryRef {
  RegisterRef base;       ///< Base register
  int32_t offset;         ///< Offset from base
  coil::ValueType type;   ///< Memory access type
};

/**
 * @brief Operand in assembly language
 */
struct Operand {
  enum class Type {
    None,            ///< No operand
    Register,        ///< Register operand
    Immediate,       ///< Immediate value
    Memory,          ///< Memory reference
    Label            ///< Label reference
  };
  
  Type type;  ///< Operand type
  
  union {
    RegisterRef reg;       ///< Register reference
    
    struct {
      int64_t i_value;       ///< Integer immediate value
      double f_value;        ///< Float immediate value
      coil::ValueType type;  ///< Value type
    } imm;                   ///< Immediate value
    
    MemoryRef mem;         ///< Memory reference
    std::string* label;    ///< Label reference (pointer to avoid union issues)
  };
  
  // Constructor for default operand
  Operand() : type(Type::None), imm{0, 0.0, coil::ValueType::I32} {}
  
  // Destructor to clean up label if needed
  ~Operand() {
    if (type == Type::Label && label != nullptr) {
      delete label;
    }
  }
  
  // Copy constructor
  Operand(const Operand& other) : type(other.type) {
    switch (type) {
      case Type::Register:
        reg = other.reg;
        break;
      case Type::Immediate:
        imm = other.imm;
        break;
      case Type::Memory:
        mem = other.mem;
        break;
      case Type::Label:
        label = other.label ? new std::string(*other.label) : nullptr;
        break;
      default:
        break;
    }
  }
  
  // Move constructor
  Operand(Operand&& other) noexcept : type(other.type) {
    switch (type) {
      case Type::Register:
        reg = other.reg;
        break;
      case Type::Immediate:
        imm = other.imm;
        break;
      case Type::Memory:
        mem = other.mem;
        break;
      case Type::Label:
        label = other.label;
        other.label = nullptr;
        break;
      default:
        break;
    }
    other.type = Type::None;
  }
  
  // Assignment operator
  Operand& operator=(const Operand& other) {
    if (this == &other) return *this;
    
    // Clean up existing label if needed
    if (type == Type::Label && label != nullptr) {
      delete label;
      label = nullptr;
    }
    
    type = other.type;
    switch (type) {
      case Type::Register:
        reg = other.reg;
        break;
      case Type::Immediate:
        imm = other.imm;
        break;
      case Type::Memory:
        mem = other.mem;
        break;
      case Type::Label:
        label = other.label ? new std::string(*other.label) : nullptr;
        break;
      default:
        break;
    }
    
    return *this;
  }
  
  // Move assignment operator
  Operand& operator=(Operand&& other) noexcept {
    if (this == &other) return *this;
    
    // Clean up existing label if needed
    if (type == Type::Label && label != nullptr) {
      delete label;
      label = nullptr;
    }
    
    type = other.type;
    switch (type) {
      case Type::Register:
        reg = other.reg;
        break;
      case Type::Immediate:
        imm = other.imm;
        break;
      case Type::Memory:
        mem = other.mem;
        break;
      case Type::Label:
        label = other.label;
        other.label = nullptr;
        break;
      default:
        break;
    }
    
    other.type = Type::None;
    return *this;
  }
  
  // Create register operand
  static Operand makeRegister(uint32_t number, coil::ValueType type = coil::ValueType::I32, bool explicit_type = false) {
    Operand op;
    op.type = Type::Register;
    op.reg.number = number;
    op.reg.type = type;
    op.reg.has_explicit_type = explicit_type;
    return op;
  }
  
  // Create immediate integer operand
  static Operand makeImmediate(int64_t value, coil::ValueType type = coil::ValueType::I32) {
    Operand op;
    op.type = Type::Immediate;
    op.imm.i_value = value;
    op.imm.type = type;
    return op;
  }
  
  // Create immediate float operand
  static Operand makeImmediateFloat(double value, coil::ValueType type = coil::ValueType::F32) {
    Operand op;
    op.type = Type::Immediate;
    op.imm.f_value = value;
    op.imm.type = type;
    return op;
  }
  
  // Create memory operand
  static Operand makeMemory(const RegisterRef& base, int32_t offset = 0, coil::ValueType type = coil::ValueType::I32) {
    Operand op;
    op.type = Type::Memory;
    op.mem.base = base;
    op.mem.offset = offset;
    op.mem.type = type;
    return op;
  }
  
  // Create label operand
  static Operand makeLabel(const std::string& name) {
    Operand op;
    op.type = Type::Label;
    op.label = new std::string(name);
    return op;
  }
};

/**
 * @brief Instruction in assembly language
 */
struct Instruction {
  std::string name;       ///< Instruction name (e.g., "add", "jump")
  std::string condition;  ///< Conditional suffix (e.g., "eq", "lt")
  std::vector<Operand> operands;  ///< Operands
  size_t line;            ///< Line number in source
};

/**
 * @brief Directive in assembly language
 */
struct Directive {
  std::string name;      ///< Directive name (e.g., "section", "global")
  std::vector<Token> args;  ///< Directive arguments
  size_t line;           ///< Line number in source
};

/**
 * @brief Types of assembly line
 */
enum class LineType {
  Empty,       ///< Empty line or comment
  Instruction, ///< Instruction
  Directive,   ///< Directive
  Label        ///< Label declaration
};

/**
 * @brief Parsed line of assembly code
 */
struct Line {
  LineType type;          ///< Line type
  std::string label;      ///< Label (if any)
  size_t line_number;     ///< Line number in source
  
  union {
    Instruction* instruction;  ///< Instruction (if type is Instruction)
    Directive* directive;      ///< Directive (if type is Directive)
  };
  
  // Constructor for empty line
  Line() : type(LineType::Empty), instruction(nullptr), line_number(0) {}
  
  // Destructor
  ~Line() {
    switch (type) {
      case LineType::Instruction:
        delete instruction;
        break;
      case LineType::Directive:
        delete directive;
        break;
      default:
        break;
    }
  }
  
  // Copy constructor
  Line(const Line& other) : type(other.type), label(other.label), line_number(other.line_number) {
    switch (type) {
      case LineType::Instruction:
        instruction = new Instruction(*other.instruction);
        break;
      case LineType::Directive:
        directive = new Directive(*other.directive);
        break;
      default:
        instruction = nullptr;
        break;
    }
  }
  
  // Move constructor
  Line(Line&& other) noexcept : type(other.type), label(std::move(other.label)), line_number(other.line_number) {
    switch (type) {
      case LineType::Instruction:
        instruction = other.instruction;
        other.instruction = nullptr;
        break;
      case LineType::Directive:
        directive = other.directive;
        other.directive = nullptr;
        break;
      default:
        instruction = nullptr;
        break;
    }
    other.type = LineType::Empty;
  }
  
  // Assignment operator
  Line& operator=(const Line& other) {
    if (this == &other) return *this;
    
    // Clean up existing resources
    switch (type) {
      case LineType::Instruction:
        delete instruction;
        break;
      case LineType::Directive:
        delete directive;
        break;
      default:
        break;
    }
    
    type = other.type;
    label = other.label;
    line_number = other.line_number;
    
    switch (type) {
      case LineType::Instruction:
        instruction = new Instruction(*other.instruction);
        break;
      case LineType::Directive:
        directive = new Directive(*other.directive);
        break;
      default:
        instruction = nullptr;
        break;
    }
    
    return *this;
  }
  
  // Move assignment operator
  Line& operator=(Line&& other) noexcept {
    if (this == &other) return *this;
    
    // Clean up existing resources
    switch (type) {
      case LineType::Instruction:
        delete instruction;
        break;
      case LineType::Directive:
        delete directive;
        break;
      default:
        break;
    }
    
    type = other.type;
    label = std::move(other.label);
    line_number = other.line_number;
    
    switch (type) {
      case LineType::Instruction:
        instruction = other.instruction;
        other.instruction = nullptr;
        break;
      case LineType::Directive:
        directive = other.directive;
        other.directive = nullptr;
        break;
      default:
        instruction = nullptr;
        break;
    }
    
    other.type = LineType::Empty;
    return *this;
  }
  
  // Create instruction line
  static Line makeInstruction(const std::string& label, Instruction* instr, size_t line_number) {
    Line line;
    line.type = LineType::Instruction;
    line.label = label;
    line.instruction = instr;
    line.line_number = line_number;
    return line;
  }
  
  // Create directive line
  static Line makeDirective(const std::string& label, Directive* dir, size_t line_number) {
    Line line;
    line.type = LineType::Directive;
    line.label = label;
    line.directive = dir;
    line.line_number = line_number;
    return line;
  }
  
  // Create label-only line
  static Line makeLabel(const std::string& label, size_t line_number) {
    Line line;
    line.type = LineType::Label;
    line.label = label;
    line.instruction = nullptr;
    line.line_number = line_number;
    return line;
  }
};

/**
 * @brief Symbol information for assembler
 */
struct Symbol {
  std::string name;            ///< Symbol name
  uint32_t value;              ///< Symbol value
  uint16_t section;            ///< Section index
  bool is_defined;             ///< Whether symbol is defined
  bool is_global;              ///< Whether symbol is global
  coil::SymbolType type;       ///< Symbol type
  std::vector<size_t> refs;    ///< References to this symbol (line numbers)
  
  Symbol() : value(0), section(0), is_defined(false), is_global(false), type(coil::SymbolType::NoType) {}
};

/**
 * @brief Section information for assembler
 */
struct Section {
  std::string name;            ///< Section name
  coil::SectionType type;      ///< Section type
  coil::SectionFlag flags;     ///< Section flags
  std::vector<uint8_t> data;   ///< Section data
  uint32_t current_offset;     ///< Current offset in section
  
  Section() : type(coil::SectionType::Null), flags(coil::SectionFlag::None), current_offset(0) {}
};

} // namespace casm