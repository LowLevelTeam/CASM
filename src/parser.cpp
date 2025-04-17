/**
 * @file parser.cpp
 * @brief Implementation of CASM parser
 */

#include "casm/parser.hpp"
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <cctype>

namespace casm {

// Default error callback function
static void defaultErrorCallback(const char* message, size_t line, size_t column, void* user_data) {
  (void)user_data; // Unused
  fprintf(stderr, "Error at line %zu, column %zu: %s\n", line, column, message);
}

Parser::Parser() 
  : error_callback(defaultErrorCallback), 
    error_user_data(nullptr) {
  // Initialize opcode map
  opcode_map["nop"] = coil::Opcode::Nop;
  opcode_map["br"] = coil::Opcode::Br;
  opcode_map["jump"] = coil::Opcode::Jump;
  opcode_map["call"] = coil::Opcode::Call;
  opcode_map["ret"] = coil::Opcode::Ret;
  
  opcode_map["load"] = coil::Opcode::Load;
  opcode_map["store"] = coil::Opcode::Store;
  opcode_map["push"] = coil::Opcode::Push;
  opcode_map["pop"] = coil::Opcode::Pop;
  
  opcode_map["add"] = coil::Opcode::Add;
  opcode_map["sub"] = coil::Opcode::Sub;
  opcode_map["mul"] = coil::Opcode::Mul;
  opcode_map["div"] = coil::Opcode::Div;
  opcode_map["rem"] = coil::Opcode::Rem;
  opcode_map["inc"] = coil::Opcode::Inc;
  opcode_map["dec"] = coil::Opcode::Dec;
  
  opcode_map["and"] = coil::Opcode::And;
  opcode_map["or"] = coil::Opcode::Or;
  opcode_map["xor"] = coil::Opcode::Xor;
  opcode_map["not"] = coil::Opcode::Not;
  opcode_map["shl"] = coil::Opcode::Shl;
  opcode_map["shr"] = coil::Opcode::Shr;
  opcode_map["sar"] = coil::Opcode::Sar;
  
  opcode_map["cmp"] = coil::Opcode::Cmp;
  opcode_map["test"] = coil::Opcode::Test;
  
  // Initialize condition map
  condition_map["eq"] = coil::InstrFlag0::EQ;
  condition_map["neq"] = coil::InstrFlag0::NEQ;
  condition_map["gt"] = coil::InstrFlag0::GT;
  condition_map["gte"] = coil::InstrFlag0::GTE;
  condition_map["lt"] = coil::InstrFlag0::LT;
  condition_map["lte"] = coil::InstrFlag0::LTE;
  
  // Initialize type map
  type_map["i8"] = coil::ValueType::I8;
  type_map["i16"] = coil::ValueType::I16;
  type_map["i32"] = coil::ValueType::I32;
  type_map["i64"] = coil::ValueType::I64;
  
  type_map["u8"] = coil::ValueType::U8;
  type_map["u16"] = coil::ValueType::U16;
  type_map["u32"] = coil::ValueType::U32;
  type_map["u64"] = coil::ValueType::U64;
  
  type_map["f32"] = coil::ValueType::F32;
  type_map["f64"] = coil::ValueType::F64;
  
  type_map["ptr"] = coil::ValueType::Ptr;
}

Parser::~Parser() {
  // Nothing to clean up
}

Result Parser::parseLine(const std::string& line, size_t line_number, Line& parsed_line) {
  std::vector<Token> tokens;
  
  // Tokenize the line
  Result result = tokenize(line, line_number, tokens);
  if (result != Result::Success) {
    return result;
  }
  
  // Skip empty lines and comments
  if (tokens.empty() || tokens[0].type == TokenType::Comment) {
    parsed_line = Line(); // Empty line
    return Result::Success;
  }
  
  // Parse the line
  size_t token_index = 0;
  std::string label;
  
  // Check for a label
  if (token_index < tokens.size() && tokens[token_index].type == TokenType::Identifier && 
      token_index + 1 < tokens.size() && tokens[token_index + 1].type == TokenType::Colon) {
    label = tokens[token_index].text;
    token_index += 2; // Skip label and colon
    
    // If only a label on the line, create a label-only line
    if (token_index >= tokens.size() || tokens[token_index].type == TokenType::Comment) {
      parsed_line = Line::makeLabel(label, line_number);
      return Result::Success;
    }
  }
  
  // Check for directive or instruction
  if (token_index < tokens.size()) {
    if (tokens[token_index].type == TokenType::Directive) {
      // Parse directive
      Directive* directive = new Directive();
      directive->line = line_number;
      result = parseDirective(tokens, token_index, *directive);
      
      if (result != Result::Success) {
        delete directive;
        return result;
      }
      
      parsed_line = Line::makeDirective(label, directive, line_number);
    } else if (tokens[token_index].type == TokenType::Identifier) {
      // Parse instruction
      Instruction* instruction = new Instruction();
      instruction->line = line_number;
      result = parseInstruction(tokens, token_index, *instruction);
      
      if (result != Result::Success) {
        delete instruction;
        return result;
      }
      
      parsed_line = Line::makeInstruction(label, instruction, line_number);
    } else {
      formatError("Expected directive or instruction, got '%s'", tokens[token_index].text.c_str());
      reportError(error_buffer, line_number, tokens[token_index].column);
      return Result::InvalidFormat;
    }
  }
  
  return Result::Success;
}

void Parser::setErrorCallback(void (*callback)(const char* message, size_t line, size_t column, void* user_data), void* user_data) {
  error_callback = callback ? callback : defaultErrorCallback;
  error_user_data = user_data;
}

const std::string& Parser::getLastError() const {
  return last_error;
}

Result Parser::tokenize(const std::string& line, size_t line_number, std::vector<Token>& tokens) {
  tokens.clear();
  
  size_t pos = 0;
  size_t len = line.length();
  
  while (pos < len) {
    char c = line[pos];
    
    // Skip whitespace
    if (isspace(c)) {
      pos++;
      continue;
    }
    
    // Handle comments
    if (c == ';') {
      Token token;
      token.type = TokenType::Comment;
      token.text = line.substr(pos);
      token.line = line_number;
      token.column = pos + 1;
      tokens.push_back(token);
      
      // Comment runs to end of line
      break;
    }
    
    // Handle directives (start with .)
    if (c == '.') {
      size_t start = pos;
      pos++; // Skip the .
      
      while (pos < len && (isalnum(line[pos]) || line[pos] == '_')) {
        pos++;
      }
      
      Token token;
      token.type = TokenType::Directive;
      token.text = line.substr(start, pos - start);
      token.line = line_number;
      token.column = start + 1;
      tokens.push_back(token);
      
      continue;
    }
    
    // Handle identifiers and registers
    if (isalpha(c) || c == '_') {
      size_t start = pos;
      
      // Check if this is a register (r0, r1, etc.)
      bool is_register = (c == 'r' && pos + 1 < len && isdigit(line[pos + 1]));
      
      pos++; // Skip first character
      
      // For identifiers, include letters, digits, underscores
      // For registers, only include digits and type suffix (.i32, .f64, etc.)
      if (is_register) {
        // Get register number
        while (pos < len && isdigit(line[pos])) {
          pos++;
        }
        
        // Check for type suffix
        if (pos < len && line[pos] == '.') {
          pos++; // Skip the dot
          
          // Parse type (i32, f64, etc.)
          while (pos < len && (isalnum(line[pos]) || line[pos] == '_')) {
            pos++;
          }
        }
      } else {
        // Parse identifier
        while (pos < len && (isalnum(line[pos]) || line[pos] == '_')) {
          pos++;
        }
      }
      
      Token token;
      token.type = is_register ? TokenType::Register : TokenType::Identifier;
      token.text = line.substr(start, pos - start);
      token.line = line_number;
      token.column = start + 1;
      tokens.push_back(token);
      
      continue;
    }
    
    // Handle numbers
    if (isdigit(c) || (c == '-' && pos + 1 < len && isdigit(line[pos + 1]))) {
      size_t start = pos;
      bool is_float = false;
      
      // Handle negative sign
      if (c == '-') {
        pos++;
      }
      
      // Parse integer part
      while (pos < len && isdigit(line[pos])) {
        pos++;
      }
      
      // Check for hexadecimal or binary prefix
      if (start + 1 < pos && line[start] == '0' && (line[start + 1] == 'x' || line[start + 1] == 'b')) {
        // Parse hex or binary number
        char prefix = line[start + 1];
        
        if (prefix == 'x') {
          // Hexadecimal
          while (pos < len && isxdigit(line[pos])) {
            pos++;
          }
        } else {
          // Binary
          while (pos < len && (line[pos] == '0' || line[pos] == '1')) {
            pos++;
          }
        }
      } else {
        // Check for floating point
        if (pos < len && line[pos] == '.') {
          is_float = true;
          pos++; // Skip the decimal point
          
          // Parse fractional part
          while (pos < len && isdigit(line[pos])) {
            pos++;
          }
        }
        
        // Check for exponent
        if (pos < len && (line[pos] == 'e' || line[pos] == 'E')) {
          is_float = true;
          pos++; // Skip the exponent marker
          
          // Handle sign
          if (pos < len && (line[pos] == '+' || line[pos] == '-')) {
            pos++;
          }
          
          // Parse exponent
          while (pos < len && isdigit(line[pos])) {
            pos++;
          }
        }
      }
      
      Token token;
      token.type = is_float ? TokenType::Float : TokenType::Integer;
      token.text = line.substr(start, pos - start);
      token.line = line_number;
      token.column = start + 1;
      tokens.push_back(token);
      
      continue;
    }
    
    // Handle string literals
    if (c == '"' || c == '\'') {
      char quote = c;
      size_t start = pos;
      pos++; // Skip opening quote
      
      // Find closing quote
      bool escaped = false;
      while (pos < len) {
        if (escaped) {
          escaped = false;
        } else if (line[pos] == '\\') {
          escaped = true;
        } else if (line[pos] == quote) {
          break;
        }
        pos++;
      }
      
      if (pos >= len) {
        formatError("Unterminated string literal");
        reportError(error_buffer, line_number, start + 1);
        return Result::InvalidFormat;
      }
      
      pos++; // Skip closing quote
      
      Token token;
      token.type = TokenType::String;
      token.text = line.substr(start, pos - start);
      token.line = line_number;
      token.column = start + 1;
      tokens.push_back(token);
      
      continue;
    }
    
    // Handle special characters
    TokenType type = TokenType::Invalid;
    switch (c) {
      case '[': type = TokenType::LBracket; break;
      case ']': type = TokenType::RBracket; break;
      case '+': type = TokenType::Plus; break;
      case '-': type = TokenType::Minus; break;
      case ',': type = TokenType::Comma; break;
      case ':': type = TokenType::Colon; break;
    }
    
    if (type != TokenType::Invalid) {
      Token token;
      token.type = type;
      token.text = std::string(1, c);
      token.line = line_number;
      token.column = pos + 1;
      tokens.push_back(token);
      pos++;
      continue;
    }
    
    // Unknown character
    formatError("Unexpected character: '%c'", c);
    reportError(error_buffer, line_number, pos + 1);
    return Result::InvalidFormat;
  }
  
  return Result::Success;
}

Result Parser::parseLabel(const std::vector<Token>& tokens, size_t& index, std::string& label) {
  if (index >= tokens.size() || tokens[index].type != TokenType::Identifier) {
    formatError("Expected label identifier");
    reportError(error_buffer, tokens[index].line, tokens[index].column);
    return Result::InvalidFormat;
  }
  
  label = tokens[index].text;
  index++;
  
  if (index >= tokens.size() || tokens[index].type != TokenType::Colon) {
    formatError("Expected colon after label");
    reportError(error_buffer, tokens[index - 1].line, tokens[index - 1].column);
    return Result::InvalidFormat;
  }
  
  index++;
  return Result::Success;
}

Result Parser::parseInstruction(const std::vector<Token>& tokens, size_t& index, Instruction& instruction) {
  if (index >= tokens.size() || tokens[index].type != TokenType::Identifier) {
    formatError("Expected instruction mnemonic");
    reportError(error_buffer, tokens[index].line, tokens[index].column);
    return Result::InvalidFormat;
  }
  
  // Split instruction mnemonic and condition
  std::string mnemonic = tokens[index].text;
  std::string condition;
  
  size_t dot_pos = mnemonic.find('.');
  if (dot_pos != std::string::npos) {
    condition = mnemonic.substr(dot_pos + 1);
    mnemonic = mnemonic.substr(0, dot_pos);
  }
  
  instruction.name = mnemonic;
  instruction.condition = condition;
  index++;
  
  // Parse operands
  while (index < tokens.size() && tokens[index].type != TokenType::Comment) {
    Operand operand;
    Result result = parseOperand(tokens, index, operand);
    if (result != Result::Success) {
      return result;
    }
    
    instruction.operands.push_back(operand);
    
    // Skip comma if present
    if (index < tokens.size() && tokens[index].type == TokenType::Comma) {
      index++;
    }
  }
  
  return Result::Success;
}

Result Parser::parseDirective(const std::vector<Token>& tokens, size_t& index, Directive& directive) {
  if (index >= tokens.size() || tokens[index].type != TokenType::Directive) {
    formatError("Expected directive");
    reportError(error_buffer, tokens[index].line, tokens[index].column);
    return Result::InvalidFormat;
  }
  
  directive.name = tokens[index].text;
  index++;
  
  // Parse directive arguments
  while (index < tokens.size() && tokens[index].type != TokenType::Comment) {
    directive.args.push_back(tokens[index]);
    index++;
    
    // Skip comma if present
    if (index < tokens.size() && tokens[index].type == TokenType::Comma) {
      index++;
    }
  }
  
  return Result::Success;
}

Result Parser::parseOperand(const std::vector<Token>& tokens, size_t& index, Operand& operand) {
  if (index >= tokens.size()) {
    formatError("Unexpected end of line while parsing operand");
    reportError(error_buffer, tokens.back().line, tokens.back().column);
    return Result::InvalidFormat;
  }
  
  // Check operand type
  switch (tokens[index].type) {
    case TokenType::Register: {
      // Register operand
      RegisterRef reg;
      Result result = parseRegister(tokens, index, reg);
      if (result != Result::Success) {
        return result;
      }
      
      operand = Operand::makeRegister(reg.number, reg.type, reg.has_explicit_type);
      return Result::Success;
    }
    
    case TokenType::LBracket: {
      // Memory operand
      MemoryRef mem;
      Result result = parseMemory(tokens, index, mem);
      if (result != Result::Success) {
        return result;
      }
      
      operand = Operand::makeMemory(mem.base, mem.offset, mem.type);
      return Result::Success;
    }
    
    case TokenType::Integer:
    case TokenType::Float: {
      // Immediate operand
      Result result = parseImmediate(tokens, index, operand);
      if (result != Result::Success) {
        return result;
      }
      
      return Result::Success;
    }
    
    case TokenType::String: {
      // String literal (for directives)
      formatError("String literals not allowed as operands");
      reportError(error_buffer, tokens[index].line, tokens[index].column);
      return Result::InvalidFormat;
    }
    
    case TokenType::Identifier: {
      // Label reference
      operand = Operand::makeLabel(tokens[index].text);
      index++;
      return Result::Success;
    }
    
    default:
      formatError("Unexpected token in operand: '%s'", tokens[index].text.c_str());
      reportError(error_buffer, tokens[index].line, tokens[index].column);
      return Result::InvalidFormat;
  }
}

Result Parser::parseRegister(const std::vector<Token>& tokens, size_t& index, RegisterRef& reg) {
  if (index >= tokens.size() || tokens[index].type != TokenType::Register) {
    formatError("Expected register");
    reportError(error_buffer, tokens[index].line, tokens[index].column);
    return Result::InvalidFormat;
  }
  
  std::string reg_text = tokens[index].text;
  index++;
  
  // Parse register number and type
  if (reg_text.size() < 2 || reg_text[0] != 'r' || !isdigit(reg_text[1])) {
    formatError("Invalid register format: '%s'", reg_text.c_str());
    reportError(error_buffer, tokens[index - 1].line, tokens[index - 1].column);
    return Result::InvalidFormat;
  }
  
  // Extract register number
  size_t type_pos = reg_text.find('.');
  std::string num_str = reg_text.substr(1, type_pos == std::string::npos ? std::string::npos : type_pos - 1);
  
  reg.number = std::stoul(num_str);
  reg.type = coil::ValueType::I32; // Default type
  reg.has_explicit_type = false;
  
  // Extract register type if specified
  if (type_pos != std::string::npos) {
    std::string type_str = reg_text.substr(type_pos + 1);
    auto it = type_map.find(type_str);
    if (it == type_map.end()) {
      formatError("Unknown register type: '%s'", type_str.c_str());
      reportError(error_buffer, tokens[index - 1].line, tokens[index - 1].column);
      return Result::InvalidFormat;
    }
    
    reg.type = it->second;
    reg.has_explicit_type = true;
  }
  
  return Result::Success;
}

Result Parser::parseMemory(const std::vector<Token>& tokens, size_t& index, MemoryRef& mem) {
  if (index >= tokens.size() || tokens[index].type != TokenType::LBracket) {
    formatError("Expected '[' for memory reference");
    reportError(error_buffer, tokens[index].line, tokens[index].column);
    return Result::InvalidFormat;
  }
  
  index++; // Skip [
  
  // Parse base register
  if (index >= tokens.size() || tokens[index].type != TokenType::Register) {
    formatError("Expected register as base for memory reference");
    reportError(error_buffer, tokens[index].line, tokens[index].column);
    return Result::InvalidFormat;
  }
  
  Result result = parseRegister(tokens, index, mem.base);
  if (result != Result::Success) {
    return result;
  }
  
  // Default offset and type
  mem.offset = 0;
  mem.type = mem.base.type;
  
  // Check for offset
  if (index < tokens.size() && (tokens[index].type == TokenType::Plus || tokens[index].type == TokenType::Minus)) {
    bool is_negative = tokens[index].type == TokenType::Minus;
    index++; // Skip +/-
    
    if (index >= tokens.size() || tokens[index].type != TokenType::Integer) {
      formatError("Expected integer offset after '%s'", is_negative ? "-" : "+");
      reportError(error_buffer, tokens[index].line, tokens[index].column);
      return Result::InvalidFormat;
    }
    
    int32_t offset = std::stoi(tokens[index].text);
    mem.offset = is_negative ? -offset : offset;
    index++; // Skip offset
  }
  
  // Check for closing bracket
  if (index >= tokens.size() || tokens[index].type != TokenType::RBracket) {
    formatError("Expected ']' to close memory reference");
    reportError(error_buffer, tokens[index].line, tokens[index].column);
    return Result::InvalidFormat;
  }
  
  index++; // Skip ]
  return Result::Success;
}

Result Parser::parseImmediate(const std::vector<Token>& tokens, size_t& index, Operand& imm) {
  if (index >= tokens.size() || (tokens[index].type != TokenType::Integer && tokens[index].type != TokenType::Float)) {
    formatError("Expected immediate value");
    reportError(error_buffer, tokens[index].line, tokens[index].column);
    return Result::InvalidFormat;
  }
  
  if (tokens[index].type == TokenType::Integer) {
    // Parse integer
    std::string int_str = tokens[index].text;
    int64_t value;
    
    // Check for hex or binary prefix
    if (int_str.size() > 2 && int_str[0] == '0') {
      if (int_str[1] == 'x') {
        // Hexadecimal
        value = std::stoll(int_str, nullptr, 16);
      } else if (int_str[1] == 'b') {
        // Binary
        value = std::stoll(int_str.substr(2), nullptr, 2);
      } else {
        // Decimal or octal
        value = std::stoll(int_str);
      }
    } else {
      // Decimal
      value = std::stoll(int_str);
    }
    
    imm = Operand::makeImmediate(value);
  } else {
    // Parse float
    double value = std::stod(tokens[index].text);
    imm = Operand::makeImmediateFloat(value);
  }
  
  index++;
  return Result::Success;
}

void Parser::reportError(const char* message, size_t line, size_t column) {
  last_error = message;
  error_callback(message, line, column, error_user_data);
}

void Parser::formatError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vsnprintf(error_buffer, sizeof(error_buffer), format, args);
  va_end(args);
}

} // namespace casm