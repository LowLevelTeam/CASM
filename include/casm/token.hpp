#pragma once
#include "casm/types.hpp"
#include <string>
#include <optional>

namespace casm {

// Token types for lexical analysis
enum class TokenType {
  Label,            // #label
  Instruction,      // add, mov, etc.
  Directive,        // .section, .global, etc.
  Register,         // %r0, %r1, etc.
  Immediate,        // $ix10, $id42, etc.
  MemoryRef,        // [%r1], [%r1+8], etc.
  LabelRef,         // @label
  Parameter,        // ^eq, ^lt, etc.
  Comma,            // ,
  Comment,          // ; comment
  EndOfLine,        // Newline
  EndOfFile,        // End of input
  Error             // Invalid token
};

// Token structure
struct Token {
  TokenType type;
  std::string value;
  SourceLocation location;
  
  // Additional data for specific token types
  std::optional<ImmediateValue> immediateValue;
  std::optional<MemoryReference> memoryRef;
  
  Token() = default;
  
  Token(TokenType type, std::string value, SourceLocation location)
    : type(type), value(std::move(value)), location(std::move(location)) {}
    
  std::string toString() const;
  
  // Helper methods for creating tokens
  static Token makeLabel(const std::string& name, const SourceLocation& location);
  static Token makeInstruction(const std::string& name, const SourceLocation& location);
  static Token makeDirective(const std::string& name, const SourceLocation& location);
  static Token makeRegister(const std::string& name, const SourceLocation& location);
  static Token makeImmediate(const std::string& value, const SourceLocation& location);
  static Token makeMemoryRef(const std::string& expr, const SourceLocation& location);
  static Token makeLabelRef(const std::string& name, const SourceLocation& location);
  static Token makeParameter(const std::string& name, const SourceLocation& location);
  static Token makeComma(const SourceLocation& location);
  static Token makeComment(const std::string& text, const SourceLocation& location);
  static Token makeEndOfLine(const SourceLocation& location);
  static Token makeEndOfFile(const SourceLocation& location);
  static Token makeError(const std::string& message, const SourceLocation& location);
};

// Helper functions
const char* tokenTypeToString(TokenType type);

} // namespace casm