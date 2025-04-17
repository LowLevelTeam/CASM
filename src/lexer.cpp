#include "casm/lexer.hpp"
#include <sstream>
#include <cctype>
#include <unordered_set>
#include <algorithm>

namespace casm {

// Known instruction names
static const std::unordered_set<std::string> KNOWN_INSTRUCTIONS = {
  "nop", "jmp", "br", "call", "ret", 
  "load", "store", "push", "pop", "mov",
  "add", "sub", "mul", "div", "rem", "inc", "dec", "neg",
  "and", "or", "xor", "not", "shl", "shr", "sar",
  "cmp", "test"
};

// Known directive names (without the leading '.')
static const std::unordered_set<std::string> KNOWN_DIRECTIVES = {
  "section", "global", "local", 
  "i8", "i16", "i32", "i64", 
  "u8", "u16", "u32", "u64", 
  "f32", "f64", 
  "ascii", "asciiz", "zero"
};

// Known parameter names (without the leading '^')
static const std::unordered_set<std::string> KNOWN_PARAMETERS = {
  "eq", "neq", "gt", "gte", "lt", "lte", 
  "progbits", "code", "write", "nobits", "alloc"
};

Lexer::Lexer(std::string filename, std::istream& input)
  : m_filename(std::move(filename)), m_position(0), m_line(1), m_column(1) {
  // Read the entire input stream into m_source
  std::stringstream buffer;
  buffer << input.rdbuf();
  m_source = buffer.str();
}

Lexer::Lexer(std::string filename, const std::string& source)
  : m_filename(std::move(filename)), m_source(source), m_position(0), m_line(1), m_column(1) {
}

std::vector<Token> Lexer::tokenize() {
  std::vector<Token> tokens;
  
  Token token;
  do {
    token = nextToken();
    tokens.push_back(token);
  } while (token.type != TokenType::EndOfFile);
  
  return tokens;
}

Token Lexer::nextToken() {
  // If we have buffered tokens, return the first one
  if (!m_tokenBuffer.empty()) {
    Token token = m_tokenBuffer.front();
    m_tokenBuffer.erase(m_tokenBuffer.begin());
    return token;
  }
  
  // Skip whitespace
  skipWhitespace();
  
  // Check for end of file
  if (isAtEnd()) {
    return Token::makeEndOfFile(currentLocation());
  }
  
  // Check for newline
  if (current() == '\n') {
    Token token = Token::makeEndOfLine(currentLocation());
    advance();
    return token;
  }
  
  // Scan the next token
  return scanToken();
}

Token Lexer::peekToken() {
  // If we have buffered tokens, return the first one without removing it
  if (!m_tokenBuffer.empty()) {
    return m_tokenBuffer.front();
  }
  
  // Otherwise, get the next token and buffer it
  Token token = nextToken();
  m_tokenBuffer.push_back(token);
  return token;
}

void Lexer::skipWhitespace() {
  while (!isAtEnd()) {
    char c = current();
    if (c == ' ' || c == '\t' || c == '\r') {
      advance();
    } else {
      break;
    }
  }
}

SourceLocation Lexer::currentLocation() const {
  return SourceLocation(m_filename, m_line, m_column);
}

void Lexer::advance(size_t count) {
  for (size_t i = 0; i < count && m_position < m_source.size(); ++i) {
    char c = m_source[m_position];
    m_position++;
    
    if (c == '\n') {
      m_line++;
      m_column = 1;
    } else {
      m_column++;
    }
  }
}

bool Lexer::isAtEnd() const {
  return m_position >= m_source.size();
}

char Lexer::current() const {
  if (isAtEnd()) {
    return '\0';
  }
  return m_source[m_position];
}

char Lexer::peek(size_t offset) const {
  if (m_position + offset >= m_source.size()) {
    return '\0';
  }
  return m_source[m_position + offset];
}

bool Lexer::match(const std::string& str) {
  if (m_position + str.size() > m_source.size()) {
    return false;
  }
  
  for (size_t i = 0; i < str.size(); ++i) {
    if (m_source[m_position + i] != str[i]) {
      return false;
    }
  }
  
  advance(str.size());
  return true;
}

bool Lexer::match(char c) {
  if (isAtEnd() || current() != c) {
    return false;
  }
  
  advance();
  return true;
}

Token Lexer::scanToken() {
  char c = current();
  
  // Label (#label)
  if (c == '#') {
    return scanLabel();
  }
  
  // Directive (.directive)
  if (c == '.') {
    return scanDirective();
  }
  
  // Register (%reg)
  if (c == '%') {
    return scanRegister();
  }
  
  // Immediate ($imm)
  if (c == '$') {
    return scanImmediate();
  }
  
  // Memory reference ([reg+offset])
  if (c == '[') {
    return scanMemoryRef();
  }
  
  // Label reference (@label)
  if (c == '@') {
    return scanLabelRef();
  }
  
  // Parameter (^param)
  if (c == '^') {
    return scanParameter();
  }
  
  // Comment (;comment)
  if (c == ';') {
    return scanComment();
  }
  
  // Comma
  if (c == ',') {
    Token token = Token::makeComma(currentLocation());
    advance();
    return token;
  }
  
  // Try to recognize an instruction (no prefix)
  if (std::isalpha(c)) {
    return scanInstruction();
  }
  
  // Unknown character
  std::ostringstream ss;
  ss << "Unexpected character: '" << c << "'";
  Token token = Token::makeError(ss.str(), currentLocation());
  advance(); // Skip the invalid character
  return token;
}

Token Lexer::scanLabel() {
  SourceLocation location = currentLocation();
  advance(); // Skip '#'
  
  std::string name;
  while (!isAtEnd() && (std::isalnum(current()) || current() == '_')) {
    name += current();
    advance();
  }
  
  if (name.empty()) {
    return Token::makeError("Empty label name", location);
  }
  
  return Token::makeLabel(name, location);
}

Token Lexer::scanDirective() {
  SourceLocation location = currentLocation();
  advance(); // Skip '.'
  
  std::string name;
  while (!isAtEnd() && (std::isalnum(current()) || current() == '_')) {
    name += current();
    advance();
  }
  
  if (name.empty()) {
    return Token::makeError("Empty directive name", location);
  }
  
  // Verify that it's a known directive
  if (KNOWN_DIRECTIVES.find(name) == KNOWN_DIRECTIVES.end()) {
    return Token::makeError("Unknown directive: ." + name, location);
  }
  
  return Token::makeDirective(name, location);
}

Token Lexer::scanInstruction() {
  SourceLocation location = currentLocation();
  
  std::string name;
  while (!isAtEnd() && (std::isalnum(current()) || current() == '_')) {
    name += current();
    advance();
  }
  
  if (name.empty()) {
    return Token::makeError("Empty instruction name", location);
  }
  
  // Verify that it's a known instruction
  if (KNOWN_INSTRUCTIONS.find(name) == KNOWN_INSTRUCTIONS.end()) {
    return Token::makeError("Unknown instruction: " + name, location);
  }
  
  return Token::makeInstruction(name, location);
}

Token Lexer::scanRegister() {
  SourceLocation location = currentLocation();
  advance(); // Skip '%'
  
  std::string name;
  while (!isAtEnd() && (std::isalnum(current()) || current() == '_')) {
    name += current();
    advance();
  }
  
  if (name.empty()) {
    return Token::makeError("Empty register name", location);
  }
  
  // Validate register format (should start with 'r' followed by a number)
  if (name[0] != 'r' || name.size() == 1 || !std::isdigit(name[1])) {
    return Token::makeError("Invalid register format: %" + name, location);
  }
  
  return Token::makeRegister(name, location);
}

Token Lexer::scanImmediate() {
  SourceLocation location = currentLocation();
  std::string value;
  value += current(); // Include the '$' in the value
  advance();
  
  // Character literal ('x')
  if (current() == '\'') {
    value += current();
    advance();
    
    // Handle escaped character
    if (current() == '\\') {
      value += current();
      advance();
      if (isAtEnd()) {
        return Token::makeError("Unterminated character literal", location);
      }
      value += current();
      advance();
    } else if (!isAtEnd()) {
      value += current();
      advance();
    } else {
      return Token::makeError("Unterminated character literal", location);
    }
    
    if (current() != '\'') {
      return Token::makeError("Unterminated character literal", location);
    }
    value += current();
    advance();
    
    return Token::makeImmediate(value, location);
  }
  
  // String literal ("string")
  if (current() == '\"') {
    value += current();
    advance();
    
    while (!isAtEnd() && current() != '\"') {
      if (current() == '\\') {
        value += current();
        advance();
        if (isAtEnd()) {
          return Token::makeError("Unterminated string literal", location);
        }
      }
      value += current();
      advance();
    }
    
    if (current() != '\"') {
      return Token::makeError("Unterminated string literal", location);
    }
    value += current();
    advance();
    
    return Token::makeImmediate(value, location);
  }
  
  // Integer/Float format specifier (id, ix, etc.)
  if ((current() == 'i' || current() == 'f') && 
      (peek(1) == 'd' || peek(1) == 'x' || peek(1) == 'b' || peek(1) == 'o')) {
    value += current(); // Format (i/f)
    advance();
    value += current(); // Base (d/x/b/o)
    advance();
    
    // Collect the digits
    bool hasDecimal = false;
    while (!isAtEnd() && (std::isalnum(current()) || current() == '.' || current() == '_')) {
      if (current() == '.') {
        if (hasDecimal) {
          return Token::makeError("Multiple decimal points in number", location);
        }
        hasDecimal = true;
      }
      value += current();
      advance();
    }
    
    return Token::makeImmediate(value, location);
  }
  
  // Legacy format (without explicit format/base)
  while (!isAtEnd() && (std::isalnum(current()) || current() == '.' || current() == '_' || 
         current() == '+' || current() == '-')) {
    value += current();
    advance();
  }
  
  return Token::makeImmediate(value, location);
}

Token Lexer::scanMemoryRef() {
  SourceLocation location = currentLocation();
  std::string expr;
  expr += current(); // Include the '[' in the expression
  advance();
  
  // Collect everything until the closing bracket
  int bracketDepth = 1;
  while (!isAtEnd() && bracketDepth > 0) {
    if (current() == '[') {
      bracketDepth++;
    } else if (current() == ']') {
      bracketDepth--;
    }
    
    expr += current();
    advance();
    
    if (bracketDepth == 0) {
      break;
    }
  }
  
  if (bracketDepth > 0) {
    return Token::makeError("Unterminated memory reference", location);
  }
  
  return Token::makeMemoryRef(expr, location);
}

Token Lexer::scanLabelRef() {
  SourceLocation location = currentLocation();
  advance(); // Skip '@'
  
  std::string name;
  while (!isAtEnd() && (std::isalnum(current()) || current() == '_')) {
    name += current();
    advance();
  }
  
  if (name.empty()) {
    return Token::makeError("Empty label reference", location);
  }
  
  return Token::makeLabelRef(name, location);
}

Token Lexer::scanParameter() {
  SourceLocation location = currentLocation();
  advance(); // Skip '^'
  
  std::string name;
  while (!isAtEnd() && (std::isalnum(current()) || current() == '_')) {
    name += current();
    advance();
  }
  
  if (name.empty()) {
    return Token::makeError("Empty parameter name", location);
  }
  
  // Convert to lowercase for case-insensitive comparison
  std::string nameLower = name;
  std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
  
  // Verify that it's a known parameter (case-insensitive)
  if (KNOWN_PARAMETERS.find(nameLower) == KNOWN_PARAMETERS.end()) {
    return Token::makeError("Unknown parameter: ^" + name, location);
  }
  
  return Token::makeParameter(nameLower, location);
}

Token Lexer::scanComment() {
  SourceLocation location = currentLocation();
  advance(); // Skip ';'
  
  std::string text;
  while (!isAtEnd() && current() != '\n') {
    text += current();
    advance();
  }
  
  return Token::makeComment(text, location);
}

} // namespace casm