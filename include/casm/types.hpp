#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <memory>

namespace casm {

// Basic numeric types
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;

// Source location information
struct SourceLocation {
  std::string filename;
  size_t line;
  size_t column;
  
  SourceLocation(std::string filename = "", size_t line = 0, size_t column = 0)
    : filename(std::move(filename)), line(line), column(column) {}
    
  std::string toString() const;
};

// Immediate value formats
enum class ImmediateFormat {
  Integer,      // Integer value
  Float,        // Floating point value
  Character,    // Character value
  String        // String value
};

// Immediate base
enum class ImmediateBase {
  Decimal,      // Decimal (base 10)
  Hexadecimal,  // Hexadecimal (base 16)
  Binary,       // Binary (base 2)
  Octal         // Octal (base 8)
};

// Memory reference
struct MemoryReference {
  std::string reg;    // Register name
  i64 offset;     // Offset value
  
  MemoryReference(std::string reg = "", i64 offset = 0)
    : reg(std::move(reg)), offset(offset) {}
};

// Immediate value
struct ImmediateValue {
  ImmediateFormat format;
  ImmediateBase base;
  std::variant<i64, f64, char, std::string> value;
  
  // Constructors for different types
  static ImmediateValue createInteger(i64 value, ImmediateBase base = ImmediateBase::Decimal);
  static ImmediateValue createFloat(f64 value);
  static ImmediateValue createChar(char value);
  static ImmediateValue createString(std::string value);
  
  std::string toString() const;
};

// Parse immediate value from string
std::optional<ImmediateValue> parseImmediate(const std::string& str);

// Parse memory reference from string
std::optional<MemoryReference> parseMemoryRef(const std::string& str);

// Exception classes
class CasmException : public std::runtime_error {
public:
  explicit CasmException(const std::string& message)
    : std::runtime_error(message) {}
};

class LexerException : public CasmException {
public:
  explicit LexerException(const std::string& message)
    : CasmException("Lexer error: " + message) {}
};

class ParserException : public CasmException {
public:
  explicit ParserException(const std::string& message)
    : CasmException("Parser error: " + message) {}
};

class AssemblerException : public CasmException {
public:
  explicit AssemblerException(const std::string& message)
    : CasmException("Assembler error: " + message) {}
};

} // namespace casm