#include "casm/lexer.h"
#include <cctype>
#include <algorithm>
#include <coil/instruction_set.h>

namespace casm {

// Static member initialization
std::unordered_map<std::string, TokenType> Lexer::keywords_;
std::unordered_map<std::string, TokenType> Lexer::directives_;
std::unordered_map<std::string, TokenType> Lexer::instructions_;
std::unordered_map<std::string, TokenType> Lexer::registers_;
bool Lexer::keywordMapsInitialized_ = Lexer::initializeKeywordMaps();

bool Lexer::initializeKeywordMaps() {
  // Initialize keywords
  keywords_["TYPE_INT8"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_INT16"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_INT32"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_INT64"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_UNT8"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_UNT16"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_UNT32"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_UNT64"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_FP16"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_FP32"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_FP64"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_FP128"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_V128"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_V256"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_V512"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_BIT"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_VOID"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_INT"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_UNT"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_FP"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_PTR"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_VAR"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_SYM"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_RGP"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_RFP"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_RV"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_STRUCT"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_PACK"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_UNION"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_ARRAY"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_ABICTL"] = TokenType::TYPE_SPECIFIER;
  keywords_["TYPE_PARAM0"] = TokenType::TYPE_SPECIFIER;
  
  // Initialize directives
  directives_["PROC"] = TokenType::DIRECTIVE;
  directives_["ARCH"] = TokenType::DIRECTIVE;
  directives_["MODE"] = TokenType::DIRECTIVE;
  directives_["ALIGN"] = TokenType::DIRECTIVE;
  directives_["SECTION"] = TokenType::DIRECTIVE;
  directives_["DATA"] = TokenType::DIRECTIVE;
  directives_["ABI"] = TokenType::DIRECTIVE;
  directives_["EXIT"] = TokenType::DIRECTIVE;
  directives_["SCOPEE"] = TokenType::DIRECTIVE;
  directives_["SCOPEL"] = TokenType::DIRECTIVE;
  directives_["IF"] = TokenType::DIRECTIVE;
  directives_["ELIF"] = TokenType::DIRECTIVE;
  directives_["ELSE"] = TokenType::DIRECTIVE;
  directives_["ENDIF"] = TokenType::DIRECTIVE;
  directives_["EXTERN"] = TokenType::DIRECTIVE;
  directives_["GLOBAL"] = TokenType::DIRECTIVE;
  directives_["INCLUDE"] = TokenType::DIRECTIVE;
  directives_["VERSION"] = TokenType::DIRECTIVE;
  
  // Initialize instructions (this is a partial list)
  instructions_["SYM"] = TokenType::INSTRUCTION;
  instructions_["BR"] = TokenType::INSTRUCTION;
  instructions_["CALL"] = TokenType::INSTRUCTION;
  instructions_["RET"] = TokenType::INSTRUCTION;
  instructions_["CMP"] = TokenType::INSTRUCTION;
  instructions_["MOV"] = TokenType::INSTRUCTION;
  instructions_["PUSH"] = TokenType::INSTRUCTION;
  instructions_["POP"] = TokenType::INSTRUCTION;
  instructions_["LEA"] = TokenType::INSTRUCTION;
  instructions_["VAR"] = TokenType::INSTRUCTION;
  instructions_["ADD"] = TokenType::INSTRUCTION;
  instructions_["SUB"] = TokenType::INSTRUCTION;
  instructions_["MUL"] = TokenType::INSTRUCTION;
  instructions_["DIV"] = TokenType::INSTRUCTION;
  instructions_["MOD"] = TokenType::INSTRUCTION;
  instructions_["INC"] = TokenType::INSTRUCTION;
  instructions_["DEC"] = TokenType::INSTRUCTION;
  instructions_["AND"] = TokenType::INSTRUCTION;
  instructions_["OR"] = TokenType::INSTRUCTION;
  instructions_["XOR"] = TokenType::INSTRUCTION;
  instructions_["NOT"] = TokenType::INSTRUCTION;
  instructions_["SHL"] = TokenType::INSTRUCTION;
  instructions_["SHR"] = TokenType::INSTRUCTION;
  instructions_["MEMCPY"] = TokenType::INSTRUCTION;
  instructions_["MEMSET"] = TokenType::INSTRUCTION;
  instructions_["MEMCMP"] = TokenType::INSTRUCTION;
  instructions_["SYSCALL"] = TokenType::INSTRUCTION;
  
  // Add all opcodes from coil::Opcode namespace
  for (uint16_t opcode = 0; opcode <= 0xFF; opcode++) {
      // Skip unknown opcodes
      if (!coil::InstructionSet::isValidOpcode(opcode)) {
          continue;
      }
      
      std::string opcodeName = coil::InstructionSet::getInstructionName(opcode);
      if (opcodeName != "UNKNOWN") {
          instructions_[opcodeName] = TokenType::INSTRUCTION;
      }
  }
  
  // Initialize x86-64 registers (example)
  registers_["RAX"] = TokenType::REGISTER_NAME;
  registers_["RBX"] = TokenType::REGISTER_NAME;
  registers_["RCX"] = TokenType::REGISTER_NAME;
  registers_["RDX"] = TokenType::REGISTER_NAME;
  registers_["RSI"] = TokenType::REGISTER_NAME;
  registers_["RDI"] = TokenType::REGISTER_NAME;
  registers_["RBP"] = TokenType::REGISTER_NAME;
  registers_["RSP"] = TokenType::REGISTER_NAME;
  registers_["R8"] = TokenType::REGISTER_NAME;
  registers_["R9"] = TokenType::REGISTER_NAME;
  registers_["R10"] = TokenType::REGISTER_NAME;
  registers_["R11"] = TokenType::REGISTER_NAME;
  registers_["R12"] = TokenType::REGISTER_NAME;
  registers_["R13"] = TokenType::REGISTER_NAME;
  registers_["R14"] = TokenType::REGISTER_NAME;
  registers_["R15"] = TokenType::REGISTER_NAME;
  
  registers_["EAX"] = TokenType::REGISTER_NAME;
  registers_["EBX"] = TokenType::REGISTER_NAME;
  registers_["ECX"] = TokenType::REGISTER_NAME;
  registers_["EDX"] = TokenType::REGISTER_NAME;
  registers_["ESI"] = TokenType::REGISTER_NAME;
  registers_["EDI"] = TokenType::REGISTER_NAME;
  registers_["EBP"] = TokenType::REGISTER_NAME;
  registers_["ESP"] = TokenType::REGISTER_NAME;
  
  registers_["XMM0"] = TokenType::REGISTER_NAME;
  registers_["XMM1"] = TokenType::REGISTER_NAME;
  registers_["XMM2"] = TokenType::REGISTER_NAME;
  registers_["XMM3"] = TokenType::REGISTER_NAME;
  registers_["XMM4"] = TokenType::REGISTER_NAME;
  registers_["XMM5"] = TokenType::REGISTER_NAME;
  registers_["XMM6"] = TokenType::REGISTER_NAME;
  registers_["XMM7"] = TokenType::REGISTER_NAME;
  
  return true;
}

Lexer::Lexer(std::string source, std::string filename)
  : source_(std::move(source)), filename_(std::move(filename)),
    position_(0), line_(1), column_(1) {
}

Token Lexer::getNextToken() {
  skipWhitespace();
  
  if (isAtEnd()) {
      return makeToken(TokenType::END_OF_FILE);
  }
  
  return scanToken();
}

std::vector<Token> Lexer::tokenize() {
  std::vector<Token> tokens;
  Token token;
  
  do {
      token = getNextToken();
      tokens.push_back(token);
  } while (!token.isEof());
  
  return tokens;
}

const std::string& Lexer::getFilename() const {
  return filename_;
}

unsigned int Lexer::getLine() const {
  return line_;
}

unsigned int Lexer::getColumn() const {
  return column_;
}

char Lexer::peek() const {
  if (isAtEnd()) {
      return '\0';
  }
  return source_[position_];
}

char Lexer::peekNext() const {
  if (position_ + 1 >= source_.length()) {
      return '\0';
  }
  return source_[position_ + 1];
}

char Lexer::advance() {
  char current = source_[position_++];
  
  if (current == '\n') {
      line_++;
      column_ = 1;
  } else {
      column_++;
  }
  
  return current;
}

bool Lexer::match(char expected) {
  if (isAtEnd() || source_[position_] != expected) {
      return false;
  }
  
  advance();
  return true;
}

void Lexer::skipWhitespace() {
  while (true) {
      char c = peek();
      switch (c) {
          case ' ':
          case '\t':
          case '\r':
              advance();
              break;
          case '\n':
              advance();
              break;
          case ';':
              skipComment();
              break;
          default:
              return;
      }
  }
}

void Lexer::skipComment() {
  // Skip until the end of the line
  while (peek() != '\n' && !isAtEnd()) {
      advance();
  }
}

Token Lexer::makeToken(TokenType type) {
  return Token(type, line_, column_);
}

Token Lexer::makeToken(TokenType type, const std::string& lexeme) {
  return Token(type, lexeme, line_, column_);
}

template<typename T>
Token Lexer::makeToken(TokenType type, T value, const std::string& lexeme) {
  return Token(type, value, lexeme, line_, column_);
}

Token Lexer::scanToken() {
  char c = advance();
  
  // Handle identifiers
  if (isAlpha(c) || c == '.') {
      return scanIdentifier();
  }
  
  // Handle numbers
  if (isDigit(c) || (c == '-' && isDigit(peek()))) {
      return scanNumber();
  }
  
  // Handle variable IDs (#n)
  if (c == '#') {
      return scanVarID();
  }
  
  // Handle strings
  if (c == '"') {
      return scanString();
  }
  
  // Handle punctuation
  switch (c) {
      case ':': return makeToken(TokenType::COLON, ":");
      case ',': return makeToken(TokenType::COMMA, ",");
      case ';': return makeToken(TokenType::SEMICOLON, ";");
      case '(': return makeToken(TokenType::OPEN_PAREN, "(");
      case ')': return makeToken(TokenType::CLOSE_PAREN, ")");
      case '[': return makeToken(TokenType::OPEN_BRACKET, "[");
      case ']': return makeToken(TokenType::CLOSE_BRACKET, "]");
      case '+': return makeToken(TokenType::PLUS, "+");
      case '-': return makeToken(TokenType::MINUS, "-");
      case '*': return makeToken(TokenType::ASTERISK, "*");
      case '=': return makeToken(TokenType::EQUAL, "=");
      case '|': return makeToken(TokenType::PIPE, "|");
  }
  
  // Invalid token
  std::string invalidToken(1, c);
  return makeToken(TokenType::INVALID, invalidToken);
}

Token Lexer::scanIdentifier() {
  size_t start = position_ - 1;
  
  while (isAlphaNumeric(peek()) || peek() == '_' || peek() == '.' || peek() == '-') {
      advance();
  }
  
  std::string lexeme = source_.substr(start, position_ - start);
  TokenType type = TokenType::IDENTIFIER;
  
  // Check if it's a keyword
  auto keywordIt = keywords_.find(lexeme);
  if (keywordIt != keywords_.end()) {
      type = keywordIt->second;
  }
  
  // Check if it's a directive
  auto directiveIt = directives_.find(lexeme);
  if (directiveIt != directives_.end()) {
      type = directiveIt->second;
  }
  
  // Check if it's an instruction
  auto instructionIt = instructions_.find(lexeme);
  if (instructionIt != instructions_.end()) {
      type = instructionIt->second;
  }
  
  // Check if it's a register
  auto registerIt = registers_.find(lexeme);
  if (registerIt != registers_.end()) {
      type = registerIt->second;
  }
  
  // Check if it ends with ':' to identify labels
  if (peek() == ':') {
      advance(); // consume the colon
      return makeToken(TokenType::LABEL, lexeme);
  }
  
  return makeToken(type, lexeme);
}

Token Lexer::scanNumber() {
  size_t start = position_ - 1;
  bool isFloat = false;
  bool isHex = false;
  bool isBinary = false;
  
  // Check for hex or binary prefix
  if (source_[start] == '0' && position_ < source_.length()) {
      if (peek() == 'x' || peek() == 'X') {
          isHex = true;
          advance(); // consume 'x'
      } else if (peek() == 'b' || peek() == 'B') {
          isBinary = true;
          advance(); // consume 'b'
      }
  }
  
  // Parse the rest of the number
  while (true) {
      char c = peek();
      
      if (isHex) {
          if (!isDigit(c) && !(c >= 'a' && c <= 'f') && !(c >= 'A' && c <= 'F')) {
              break;
          }
      } else if (isBinary) {
          if (c != '0' && c != '1') {
              break;
          }
      } else {
          if (c == '.') {
              // Only allow one decimal point
              if (isFloat) {
                  break;
              }
              isFloat = true;
          } else if (!isDigit(c)) {
              break;
          }
      }
      
      advance();
  }
  
  std::string lexeme = source_.substr(start, position_ - start);
  
  if (isFloat) {
      double value = std::stod(lexeme);
      return makeToken(TokenType::FLOAT_LITERAL, value, lexeme);
  } else {
      int64_t value;
      if (isHex) {
          value = std::stoll(lexeme.substr(2), nullptr, 16); // Skip "0x"
      } else if (isBinary) {
          value = std::stoll(lexeme.substr(2), nullptr, 2); // Skip "0b"
      } else {
          value = std::stoll(lexeme);
      }
      return makeToken(TokenType::INTEGER_LITERAL, value, lexeme);
  }
}

Token Lexer::scanVarID() {
  size_t start = position_ - 1; // Include the '#'
  
  while (isDigit(peek())) {
      advance();
  }
  
  if (position_ - start <= 1) {
      // Just '#' without digits
      return makeToken(TokenType::INVALID, "#");
  }
  
  std::string lexeme = source_.substr(start, position_ - start);
  int64_t value = std::stoll(lexeme.substr(1)); // Skip '#'
  
  return makeToken(TokenType::VAR_ID, value, lexeme);
}

Token Lexer::scanString() {
  size_t start = position_; // Start after opening quote
  
  while (peek() != '"' && !isAtEnd()) {
      // Allow escaping quotes
      if (peek() == '\\' && peekNext() == '"') {
          advance(); // Skip the escape character
      }
      advance();
  }
  
  if (isAtEnd()) {
      // Unterminated string
      return makeToken(TokenType::INVALID, "Unterminated string");
  }
  
  // Closing quote
  advance();
  
  std::string lexeme = source_.substr(start, position_ - start - 1);
  return makeToken(TokenType::STRING_LITERAL, lexeme, "\"" + lexeme + "\"");
}

bool Lexer::isAtEnd() const {
  return position_ >= source_.length();
}

bool Lexer::isDigit(char c) const {
  return c >= '0' && c <= '9';
}

bool Lexer::isAlpha(char c) const {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::isAlphaNumeric(char c) const {
  return isAlpha(c) || isDigit(c);
}

} // namespace casm