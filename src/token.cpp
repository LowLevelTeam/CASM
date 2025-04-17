#include <casm/token.hpp>
#include <sstream>
#include <cctype>
#include <regex>

namespace casm {

std::string SourceLocation::toString() const {
  std::ostringstream ss;
  if (!filename.empty()) {
    ss << filename << ":";
  }
  ss << line << ":" << column;
  return ss.str();
}

ImmediateValue ImmediateValue::createInteger(i64 value, ImmediateBase base) {
  ImmediateValue iv;
  iv.format = ImmediateFormat::Integer;
  iv.base = base;
  iv.value = value;
  return iv;
}

ImmediateValue ImmediateValue::createFloat(f64 value) {
  ImmediateValue iv;
  iv.format = ImmediateFormat::Float;
  iv.base = ImmediateBase::Decimal;
  iv.value = value;
  return iv;
}

ImmediateValue ImmediateValue::createChar(char value) {
  ImmediateValue iv;
  iv.format = ImmediateFormat::Character;
  iv.base = ImmediateBase::Decimal;
  iv.value = value;
  return iv;
}

ImmediateValue ImmediateValue::createString(std::string value) {
  ImmediateValue iv;
  iv.format = ImmediateFormat::String;
  iv.base = ImmediateBase::Decimal;
  iv.value = std::move(value);
  return iv;
}

std::string ImmediateValue::toString() const {
  std::ostringstream ss;
  
  switch (format) {
    case ImmediateFormat::Integer:
      ss << "Integer(";
      if (base == ImmediateBase::Decimal) {
        ss << "decimal, " << std::get<i64>(value);
      } else if (base == ImmediateBase::Hexadecimal) {
        ss << "hex, 0x" << std::hex << std::get<i64>(value);
      } else if (base == ImmediateBase::Binary) {
        ss << "binary, 0b" << std::get<i64>(value); // Not ideal, should convert to binary
      } else if (base == ImmediateBase::Octal) {
        ss << "octal, 0" << std::oct << std::get<i64>(value);
      }
      ss << ")";
      break;
      
    case ImmediateFormat::Float:
      ss << "Float(" << std::get<f64>(value) << ")";
      break;
      
    case ImmediateFormat::Character:
      ss << "Char('" << std::get<char>(value) << "')";
      break;
      
    case ImmediateFormat::String:
      ss << "String(\"" << std::get<std::string>(value) << "\")";
      break;
  }
  
  return ss.str();
}

std::optional<ImmediateValue> parseImmediate(const std::string& str) {
  // Skip the initial '$' if present
  std::string value = str;
  if (!value.empty() && value[0] == '$') {
    value = value.substr(1);
  }
  
  if (value.empty()) {
    return std::nullopt;
  }
  
  // Character literal
  if (value.size() >= 3 && value[0] == '\'' && value[value.size()-1] == '\'') {
    if (value.size() == 3) {
      return ImmediateValue::createChar(value[1]);
    } else if (value.size() == 4 && value[1] == '\\') {
      // Escaped character
      char c = value[2];
      switch (c) {
        case 'n': return ImmediateValue::createChar('\n');
        case 't': return ImmediateValue::createChar('\t');
        case 'r': return ImmediateValue::createChar('\r');
        case '0': return ImmediateValue::createChar('\0');
        case '\\': return ImmediateValue::createChar('\\');
        case '\'': return ImmediateValue::createChar('\'');
        case '\"': return ImmediateValue::createChar('\"');
        default: return ImmediateValue::createChar(c);
      }
    }
    return std::nullopt; // Invalid character literal
  }
  
  // String literal
  if (value.size() >= 2 && value[0] == '\"' && value[value.size()-1] == '\"') {
    std::string strValue = value.substr(1, value.size() - 2);
    // TODO: Handle escape sequences
    return ImmediateValue::createString(strValue);
  }
  
  // Integer or float in format: [i|f][x|d|b|o]value
  if (value.size() >= 3 && (value[0] == 'i' || value[0] == 'f')) {
    char format = value[0];
    char base = value[1];
    std::string numStr = value.substr(2);
    
    if (format == 'i') {
      // Integer
      try {
        i64 intValue = 0;
        
        if (base == 'd') {
          // Decimal
          intValue = std::stoll(numStr, nullptr, 10);
          return ImmediateValue::createInteger(intValue, ImmediateBase::Decimal);
        } else if (base == 'x') {
          // Hexadecimal
          intValue = std::stoll(numStr, nullptr, 16);
          return ImmediateValue::createInteger(intValue, ImmediateBase::Hexadecimal);
        } else if (base == 'b') {
          // Binary
          intValue = std::stoll(numStr, nullptr, 2);
          return ImmediateValue::createInteger(intValue, ImmediateBase::Binary);
        } else if (base == 'o') {
          // Octal
          intValue = std::stoll(numStr, nullptr, 8);
          return ImmediateValue::createInteger(intValue, ImmediateBase::Octal);
        }
      } catch (const std::exception& e) {
        return std::nullopt; // Invalid number format
      }
    } else if (format == 'f') {
      // Float
      try {
        f64 floatValue = std::stod(numStr);
        return ImmediateValue::createFloat(floatValue);
      } catch (const std::exception& e) {
        return std::nullopt; // Invalid number format
      }
    }
  }
  
  // Try to parse as a plain integer or float
  try {
    // Check if it's a float (contains decimal point)
    if (value.find('.') != std::string::npos) {
      f64 floatValue = std::stod(value);
      return ImmediateValue::createFloat(floatValue);
    } else {
      // Try as integer
      i64 intValue = std::stoll(value);
      return ImmediateValue::createInteger(intValue);
    }
  } catch (const std::exception& e) {
    // Not a valid number
  }
  
  return std::nullopt;
}

std::optional<MemoryReference> parseMemoryRef(const std::string& str) {
  // Format should be [%reg] or [%reg+offset] or [%reg-offset]
  // First, remove the outer brackets
  if (str.size() < 3 || str[0] != '[' || str[str.size() - 1] != ']') {
    return std::nullopt;
  }
  
  std::string content = str.substr(1, str.size() - 2);
  
  // Find the register
  size_t regStart = content.find('%');
  if (regStart == std::string::npos) {
    return std::nullopt;
  }
  
  // Extract register name
  size_t regEnd = regStart + 1;
  while (regEnd < content.size() && (std::isalnum(content[regEnd]) || content[regEnd] == '_')) {
    regEnd++;
  }
  
  std::string regName = content.substr(regStart + 1, regEnd - regStart - 1);
  
  // Check for offset
  i64 offset = 0;
  if (regEnd < content.size()) {
    char sign = content[regEnd];
    if (sign == '+' || sign == '-') {
      std::string offsetStr = content.substr(regEnd + 1);
      try {
        offset = std::stoll(offsetStr);
        if (sign == '-') {
          offset = -offset;
        }
      } catch (const std::exception& e) {
        return std::nullopt; // Invalid offset
      }
    }
  }
  
  return MemoryReference(regName, offset);
}

const char* tokenTypeToString(TokenType type) {
  switch (type) {
    case TokenType::Label: return "Label";
    case TokenType::Instruction: return "Instruction";
    case TokenType::Directive: return "Directive";
    case TokenType::Register: return "Register";
    case TokenType::Immediate: return "Immediate";
    case TokenType::MemoryRef: return "MemoryRef";
    case TokenType::LabelRef: return "LabelRef";
    case TokenType::Parameter: return "Parameter";
    case TokenType::Comma: return "Comma";
    case TokenType::Comment: return "Comment";
    case TokenType::EndOfLine: return "EndOfLine";
    case TokenType::EndOfFile: return "EndOfFile";
    case TokenType::Error: return "Error";
    default: return "Unknown";
  }
}

std::string Token::toString() const {
  std::ostringstream ss;
  ss << tokenTypeToString(type) << "('" << value << "', at " << location.toString() << ")";
  
  if (immediateValue) {
    ss << " " << immediateValue->toString();
  }
  
  if (memoryRef) {
    ss << " MemRef(reg=" << memoryRef->reg << ", offset=" << memoryRef->offset << ")";
  }
  
  return ss.str();
}

Token Token::makeLabel(const std::string& name, const SourceLocation& location) {
  return Token(TokenType::Label, name, location);
}

Token Token::makeInstruction(const std::string& name, const SourceLocation& location) {
  return Token(TokenType::Instruction, name, location);
}

Token Token::makeDirective(const std::string& name, const SourceLocation& location) {
  return Token(TokenType::Directive, name, location);
}

Token Token::makeRegister(const std::string& name, const SourceLocation& location) {
  return Token(TokenType::Register, name, location);
}

Token Token::makeImmediate(const std::string& value, const SourceLocation& location) {
  Token token(TokenType::Immediate, value, location);
  token.immediateValue = parseImmediate(value);
  return token;
}

Token Token::makeMemoryRef(const std::string& expr, const SourceLocation& location) {
  Token token(TokenType::MemoryRef, expr, location);
  token.memoryRef = parseMemoryRef(expr);
  return token;
}

Token Token::makeLabelRef(const std::string& name, const SourceLocation& location) {
  return Token(TokenType::LabelRef, name, location);
}

Token Token::makeParameter(const std::string& name, const SourceLocation& location) {
  return Token(TokenType::Parameter, name, location);
}

Token Token::makeComma(const SourceLocation& location) {
  return Token(TokenType::Comma, ",", location);
}

Token Token::makeComment(const std::string& text, const SourceLocation& location) {
  return Token(TokenType::Comment, text, location);
}

Token Token::makeEndOfLine(const SourceLocation& location) {
  return Token(TokenType::EndOfLine, "\n", location);
}

Token Token::makeEndOfFile(const SourceLocation& location) {
  return Token(TokenType::EndOfFile, "", location);
}

Token Token::makeError(const std::string& message, const SourceLocation& location) {
  return Token(TokenType::Error, message, location);
}

} // namespace casm