#include <catch2/catch_all.hpp>
#include "casm/lexer.hpp"
#include <sstream>
#include <vector>

using namespace Catch;

TEST_CASE("Lexer tokenizes basic instructions", "[lexer]") {
  std::string source = R"(
    mov %r1, %r2       ; Move instruction
    add %r1, %r2, %r3  ; Add instruction
    sub %r1, %r2, $id42 ; Subtract with immediate
  )";
  
  casm::Lexer lexer("test", source);
  std::vector<casm::Token> tokens = lexer.tokenize();
  
  // Filter out comments and end-of-line tokens for easier testing
  std::vector<casm::Token> filtered;
  for (const auto& token : tokens) {
    if (token.type != casm::TokenType::Comment && 
        token.type != casm::TokenType::EndOfLine) {
      filtered.push_back(token);
    }
  }
  
  // Verify token types and values
  REQUIRE(filtered.size() >= 9); // 3 instructions with 3+ tokens each
  
  // First instruction
  CHECK(filtered[0].type == casm::TokenType::Instruction);
  CHECK(filtered[0].value == "mov");
  CHECK(filtered[1].type == casm::TokenType::Register);
  CHECK(filtered[1].value == "r1");
  CHECK(filtered[2].type == casm::TokenType::Comma);
  CHECK(filtered[3].type == casm::TokenType::Register);
  CHECK(filtered[3].value == "r2");
  
  // Second instruction
  CHECK(filtered[4].type == casm::TokenType::Instruction);
  CHECK(filtered[4].value == "add");
  CHECK(filtered[5].type == casm::TokenType::Register);
  CHECK(filtered[5].value == "r1");
  CHECK(filtered[6].type == casm::TokenType::Comma);
  CHECK(filtered[7].type == casm::TokenType::Register);
  CHECK(filtered[7].value == "r2");
  CHECK(filtered[8].type == casm::TokenType::Comma);
  CHECK(filtered[9].type == casm::TokenType::Register);
  CHECK(filtered[9].value == "r3");
  
  // Third instruction
  CHECK(filtered[10].type == casm::TokenType::Instruction);
  CHECK(filtered[10].value == "sub");
  CHECK(filtered[11].type == casm::TokenType::Register);
  CHECK(filtered[11].value == "r1");
  CHECK(filtered[12].type == casm::TokenType::Comma);
  CHECK(filtered[13].type == casm::TokenType::Register);
  CHECK(filtered[13].value == "r2");
  CHECK(filtered[14].type == casm::TokenType::Comma);
  CHECK(filtered[15].type == casm::TokenType::Immediate);
  CHECK(filtered[15].value == "$id42");
}

TEST_CASE("Lexer tokenizes labels and directives", "[lexer]") {
  std::string source = R"(
    .section .text     ; Start text section
    
    #main              ; Main label
      mov %r1, $id0    ; Initialize r1
      call @factorial  ; Call factorial function
      ret              ; Return
      
    #factorial
      cmp %r1, $id0    ; Compare with zero
      br ^eq @done     ; Branch if equal
  )";
  
  casm::Lexer lexer("test", source);
  std::vector<casm::Token> tokens = lexer.tokenize();
  
  // Filter out comments and end-of-line tokens for easier testing
  std::vector<casm::Token> filtered;
  for (const auto& token : tokens) {
    if (token.type != casm::TokenType::Comment && 
        token.type != casm::TokenType::EndOfLine) {
      filtered.push_back(token);
    }
  }
  
  // Verify specific tokens
  CHECK(filtered[0].type == casm::TokenType::Directive);
  CHECK(filtered[0].value == "section");
  CHECK(filtered[1].type == casm::TokenType::LabelRef);
  CHECK(filtered[1].value == ".text");
  
  CHECK(filtered[2].type == casm::TokenType::Label);
  CHECK(filtered[2].value == "main");
  
  CHECK(filtered[3].type == casm::TokenType::Instruction);
  CHECK(filtered[3].value == "mov");
  CHECK(filtered[4].type == casm::TokenType::Register);
  CHECK(filtered[4].value == "r1");
  CHECK(filtered[5].type == casm::TokenType::Comma);
  CHECK(filtered[6].type == casm::TokenType::Immediate);
  CHECK(filtered[6].value == "$id0");
  
  CHECK(filtered[7].type == casm::TokenType::Instruction);
  CHECK(filtered[7].value == "call");
  CHECK(filtered[8].type == casm::TokenType::LabelRef);
  CHECK(filtered[8].value == "factorial");
  
  // Find the factorial label
  bool foundFactorialLabel = false;
  for (const auto& token : filtered) {
    if (token.type == casm::TokenType::Label && token.value == "factorial") {
      foundFactorialLabel = true;
      break;
    }
  }
  CHECK(foundFactorialLabel);
  
  // Find the branch instruction with parameter
  bool foundBranchInstruction = false;
  bool foundEqParameter = false;
  
  for (size_t i = 0; i < filtered.size(); ++i) {
    if (filtered[i].type == casm::TokenType::Instruction && filtered[i].value == "br") {
      foundBranchInstruction = true;
      
      // Check for parameter
      if (i + 1 < filtered.size() && 
          filtered[i + 1].type == casm::TokenType::Parameter && 
          filtered[i + 1].value == "eq") {
        foundEqParameter = true;
      }
      
      break;
    }
  }
  
  CHECK(foundBranchInstruction);
  CHECK(foundEqParameter);
}

TEST_CASE("Lexer tokenizes memory references", "[lexer]") {
  std::string source = R"(
    load %r1, [%r2]     ; Load from memory
    load %r1, [%r2+8]   ; Load with positive offset
    load %r1, [%r2-4]   ; Load with negative offset
    store [%r1], %r2    ; Store to memory
  )";
  
  casm::Lexer lexer("test", source);
  std::vector<casm::Token> tokens = lexer.tokenize();
  
  // Find memory reference tokens
  std::vector<casm::Token> memRefs;
  for (const auto& token : tokens) {
    if (token.type == casm::TokenType::MemoryRef) {
      memRefs.push_back(token);
    }
  }
  
  REQUIRE(memRefs.size() == 4);
  
  // Check memory references
  CHECK(memRefs[0].value == "[%r2]");
  CHECK(memRefs[0].memoryRef.has_value());
  CHECK(memRefs[0].memoryRef->reg == "r2");
  CHECK(memRefs[0].memoryRef->offset == 0);
  
  CHECK(memRefs[1].value == "[%r2+8]");
  CHECK(memRefs[1].memoryRef.has_value());
  CHECK(memRefs[1].memoryRef->reg == "r2");
  CHECK(memRefs[1].memoryRef->offset == 8);
  
  CHECK(memRefs[2].value == "[%r2-4]");
  CHECK(memRefs[2].memoryRef.has_value());
  CHECK(memRefs[2].memoryRef->reg == "r2");
  CHECK(memRefs[2].memoryRef->offset == -4);
  
  CHECK(memRefs[3].value == "[%r1]");
  CHECK(memRefs[3].memoryRef.has_value());
  CHECK(memRefs[3].memoryRef->reg == "r1");
  CHECK(memRefs[3].memoryRef->offset == 0);
}

TEST_CASE("Lexer tokenizes immediate values", "[lexer]") {
  std::string source = R"(
    mov %r1, $id42      ; Decimal integer
    mov %r2, $ix2A      ; Hex integer
    mov %r3, $ib1010    ; Binary integer
    mov %r4, $fd3.14    ; Decimal float
    mov %r5, $'A'       ; Character literal
    add %r6, %r7, $id-5 ; Negative integer
  )";
  
  casm::Lexer lexer("test", source);
  std::vector<casm::Token> tokens = lexer.tokenize();
  
  // Find immediate tokens
  std::vector<casm::Token> immediates;
  for (const auto& token : tokens) {
    if (token.type == casm::TokenType::Immediate) {
      immediates.push_back(token);
    }
  }
  
  REQUIRE(immediates.size() == 6);
  
  // Check immediate values
  CHECK(immediates[0].value == "$id42");
  CHECK(immediates[0].immediateValue.has_value());
  CHECK(immediates[0].immediateValue->format == casm::ImmediateFormat::Integer);
  CHECK(immediates[0].immediateValue->base == casm::ImmediateBase::Decimal);
  CHECK(std::get<casm::i64>(immediates[0].immediateValue->value) == 42);
  
  CHECK(immediates[1].value == "$ix2A");
  CHECK(immediates[1].immediateValue.has_value());
  CHECK(immediates[1].immediateValue->format == casm::ImmediateFormat::Integer);
  CHECK(immediates[1].immediateValue->base == casm::ImmediateBase::Hexadecimal);
  CHECK(std::get<casm::i64>(immediates[1].immediateValue->value) == 42); // 0x2A = 42
  
  CHECK(immediates[2].value == "$ib1010");
  CHECK(immediates[2].immediateValue.has_value());
  CHECK(immediates[2].immediateValue->format == casm::ImmediateFormat::Integer);
  CHECK(immediates[2].immediateValue->base == casm::ImmediateBase::Binary);
  CHECK(std::get<casm::i64>(immediates[2].immediateValue->value) == 10); // 1010 = 10
  
  CHECK(immediates[3].value == "$fd3.14");
  CHECK(immediates[3].immediateValue.has_value());
  CHECK(immediates[3].immediateValue->format == casm::ImmediateFormat::Float);
  CHECK(immediates[3].immediateValue->base == casm::ImmediateBase::Decimal);
  CHECK(std::get<casm::f64>(immediates[3].immediateValue->value) == Approx(3.14));
  
  CHECK(immediates[4].value == "$'A'");
  CHECK(immediates[4].immediateValue.has_value());
  CHECK(immediates[4].immediateValue->format == casm::ImmediateFormat::Character);
  CHECK(std::get<char>(immediates[4].immediateValue->value) == 'A');
  
  CHECK(immediates[5].value == "$id-5");
  CHECK(immediates[5].immediateValue.has_value());
  CHECK(immediates[5].immediateValue->format == casm::ImmediateFormat::Integer);
  CHECK(immediates[5].immediateValue->base == casm::ImmediateBase::Decimal);
  CHECK(std::get<casm::i64>(immediates[5].immediateValue->value) == -5);
}

TEST_CASE("Lexer tokenizes data directives", "[lexer]") {
  std::string source = R"(
    .section .data
    .i32 1, 2, 3, 4     ; 32-bit integers
    .f64 3.14, 2.71     ; 64-bit floats
    .ascii $"Hello"      ; ASCII string
    .asciiz $"World"     ; Null-terminated string
  )";
  
  casm::Lexer lexer("test", source);
  std::vector<casm::Token> tokens = lexer.tokenize();
  
  // Find directive tokens
  std::vector<casm::Token> directives;
  for (const auto& token : tokens) {
    if (token.type == casm::TokenType::Directive) {
      directives.push_back(token);
    }
  }
  
  REQUIRE(directives.size() == 5); // section, i32, f64, ascii, asciiz
  
  CHECK(directives[0].value == "section");
  CHECK(directives[1].value == "i32");
  CHECK(directives[2].value == "f64");
  CHECK(directives[3].value == "ascii");
  CHECK(directives[4].value == "asciiz");
  
  // Find string literals
  std::vector<casm::Token> strings;
  for (const auto& token : tokens) {
    if (token.type == casm::TokenType::Immediate && 
        token.immediateValue.has_value() && 
        token.immediateValue->format == casm::ImmediateFormat::String) {
      strings.push_back(token);
    }
  }
  
  REQUIRE(strings.size() == 2);
  CHECK(std::get<std::string>(strings[0].immediateValue->value) == "Hello");
  CHECK(std::get<std::string>(strings[1].immediateValue->value) == "World");
}