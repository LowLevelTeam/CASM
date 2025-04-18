#include <catch2/catch_all.hpp>
#include "casm/lexer.hpp"
#include "casm/parser.hpp"
#include <sstream>
#include <vector>
#include <iostream>

using namespace Catch;

TEST_CASE("Parser parses basic instructions", "[parser]") {
  std::string source = R"(
    mov %r1, %r2       ; Move instruction
    add %r1, %r2, %r3  ; Add instruction
    sub %r1, %r2, $id42 ; Subtract with immediate
  )";
  
  casm::Lexer lexer("test", source);
  casm::Parser parser(lexer);
  
  std::vector<casm::Statement> statements = parser.parse();
  
  // Check for parser errors
  REQUIRE(parser.getErrors().empty());
  
  // Filter out empty statements (e.g., blank lines or comment-only lines)
  std::vector<casm::Statement> filtered;
  for (const auto& stmt : statements) {
    if (stmt.getType() != casm::Statement::Type::Empty) {
      filtered.push_back(stmt);
    }
  }
  
  // Verify statements
  REQUIRE(filtered.size() == 3);
  
  // First instruction (mov)
  CHECK(filtered[0].getType() == casm::Statement::Type::Instruction);
  CHECK(filtered[0].getLabel().empty());
  
  auto* instr1 = filtered[0].getInstruction();
  REQUIRE(instr1 != nullptr);
  CHECK(instr1->getName() == "mov");
  CHECK(instr1->getParameters().empty());
  REQUIRE(instr1->getOperands().size() == 2);
  CHECK(instr1->getOperands()[0]->getType() == casm::Operand::Type::Register);
  CHECK(static_cast<const casm::RegisterOperand*>(instr1->getOperands()[0].get())->getName() == "r1");
  CHECK(instr1->getOperands()[1]->getType() == casm::Operand::Type::Register);
  CHECK(static_cast<const casm::RegisterOperand*>(instr1->getOperands()[1].get())->getName() == "r2");
  
  // Second instruction (add)
  CHECK(filtered[1].getType() == casm::Statement::Type::Instruction);
  CHECK(filtered[1].getLabel().empty());
  
  auto* instr2 = filtered[1].getInstruction();
  REQUIRE(instr2 != nullptr);
  CHECK(instr2->getName() == "add");
  CHECK(instr2->getParameters().empty());
  REQUIRE(instr2->getOperands().size() == 3);
  CHECK(instr2->getOperands()[0]->getType() == casm::Operand::Type::Register);
  CHECK(static_cast<const casm::RegisterOperand*>(instr2->getOperands()[0].get())->getName() == "r1");
  CHECK(instr2->getOperands()[1]->getType() == casm::Operand::Type::Register);
  CHECK(static_cast<const casm::RegisterOperand*>(instr2->getOperands()[1].get())->getName() == "r2");
  CHECK(instr2->getOperands()[2]->getType() == casm::Operand::Type::Register);
  CHECK(static_cast<const casm::RegisterOperand*>(instr2->getOperands()[2].get())->getName() == "r3");
  
  // Third instruction (sub)
  CHECK(filtered[2].getType() == casm::Statement::Type::Instruction);
  CHECK(filtered[2].getLabel().empty());
  
  auto* instr3 = filtered[2].getInstruction();
  REQUIRE(instr3 != nullptr);
  CHECK(instr3->getName() == "sub");
  CHECK(instr3->getParameters().empty());
  REQUIRE(instr3->getOperands().size() == 3);
  CHECK(instr3->getOperands()[0]->getType() == casm::Operand::Type::Register);
  CHECK(static_cast<const casm::RegisterOperand*>(instr3->getOperands()[0].get())->getName() == "r1");
  CHECK(instr3->getOperands()[1]->getType() == casm::Operand::Type::Register);
  CHECK(static_cast<const casm::RegisterOperand*>(instr3->getOperands()[1].get())->getName() == "r2");
  CHECK(instr3->getOperands()[2]->getType() == casm::Operand::Type::Immediate);
  
  auto* immediate = static_cast<const casm::ImmediateOperand*>(instr3->getOperands()[2].get());
  CHECK(immediate->getValue().format == casm::ImmediateFormat::Integer);
  CHECK(std::get<casm::i64>(immediate->getValue().value) == 42);
}

TEST_CASE("Parser parses labels and directives", "[parser]") {
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
  casm::Parser parser(lexer);
  
  std::vector<casm::Statement> statements = parser.parse();
  
  // Check for parser errors
  REQUIRE(parser.getErrors().empty());
  
  // Filter out empty statements
  std::vector<casm::Statement> filtered;
  for (const auto& stmt : statements) {
    if (stmt.getType() != casm::Statement::Type::Empty) {
      filtered.push_back(stmt);
    }
  }
  
  // Find section directive
  bool foundSectionDirective = false;
  for (const auto& stmt : filtered) {
    if (stmt.getType() == casm::Statement::Type::Directive) {
      auto* directive = stmt.getDirective();
      if (directive && directive->getName() == "section") {
        foundSectionDirective = true;
        REQUIRE(directive->getOperands().size() == 1);
        CHECK(directive->getOperands()[0]->getType() == casm::Operand::Type::Label);
        CHECK(static_cast<const casm::LabelOperand*>(directive->getOperands()[0].get())->getLabel() == ".text");
        break;
      }
    }
  }
  CHECK(foundSectionDirective);
  
  // Find main label and its instructions
  bool foundMainLabel = false;
  for (size_t i = 0; i < filtered.size(); ++i) {
    if (filtered[i].getType() == casm::Statement::Type::Label && 
        filtered[i].getLabel() == "main") {
      foundMainLabel = true;
      
      // Check mov instruction
      auto* instr = filtered[i + 1].getInstruction();
      REQUIRE(instr != nullptr);
      CHECK(instr->getName() == "mov");
      REQUIRE(instr->getOperands().size() == 2);
      CHECK(instr->getOperands()[0]->getType() == casm::Operand::Type::Register);
      CHECK(static_cast<const casm::RegisterOperand*>(instr->getOperands()[0].get())->getName() == "r1");
      CHECK(instr->getOperands()[1]->getType() == casm::Operand::Type::Immediate);
      
      // Check call instruction (next statement)
      if (i + 2 < filtered.size()) {
        auto* callInstr = filtered[i + 2].getInstruction();
        REQUIRE(callInstr != nullptr);
        CHECK(callInstr->getName() == "call");
        REQUIRE(callInstr->getOperands().size() == 1);
        CHECK(callInstr->getOperands()[0]->getType() == casm::Operand::Type::Label);
        CHECK(static_cast<const casm::LabelOperand*>(callInstr->getOperands()[0].get())->getLabel() == "factorial");
      }
      
      break;
    }
  }
  CHECK(foundMainLabel);
  
  // Find branch instruction with parameter
  bool foundBranchWithParameter = false;
  for (const auto& stmt : filtered) {
    if (stmt.getType() == casm::Statement::Type::Instruction) {
      auto* instr = stmt.getInstruction();
      if (instr && instr->getName() == "br" && !instr->getParameters().empty()) {
        foundBranchWithParameter = true;
        CHECK(instr->getParameters()[0] == "eq");
        REQUIRE(instr->getOperands().size() == 1);
        CHECK(instr->getOperands()[0]->getType() == casm::Operand::Type::Label);
        CHECK(static_cast<const casm::LabelOperand*>(instr->getOperands()[0].get())->getLabel() == "done");
        break;
      }
    }
  }
  CHECK(foundBranchWithParameter);
}

TEST_CASE("Parser parses memory operations", "[parser]") {
  std::string source = R"(
    load %r1, [%r2]     ; Load from memory
    load %r1, [%r2+8]   ; Load with positive offset
    store [%r1], %r2    ; Store to memory
  )";
  
  casm::Lexer lexer("test", source);
  casm::Parser parser(lexer);
  
  std::vector<casm::Statement> statements = parser.parse();
  
  // Check for parser errors
  REQUIRE(parser.getErrors().empty());
  
  // Filter out empty statements
  std::vector<casm::Statement> filtered;
  for (const auto& stmt : statements) {
    if (stmt.getType() != casm::Statement::Type::Empty) {
      filtered.push_back(stmt);
    }
  }
  
  REQUIRE(filtered.size() == 3);
  
  // First load instruction
  auto* load1 = filtered[0].getInstruction();
  REQUIRE(load1 != nullptr);
  CHECK(load1->getName() == "load");
  REQUIRE(load1->getOperands().size() == 2);
  CHECK(load1->getOperands()[0]->getType() == casm::Operand::Type::Register);
  CHECK(load1->getOperands()[1]->getType() == casm::Operand::Type::Memory);
  
  auto* memRef1 = static_cast<const casm::MemoryOperand*>(load1->getOperands()[1].get());
  CHECK(memRef1->getReference().reg == "r2");
  CHECK(memRef1->getReference().offset == 0);
  
  // Second load instruction (with offset)
  auto* load2 = filtered[1].getInstruction();
  REQUIRE(load2 != nullptr);
  CHECK(load2->getName() == "load");
  REQUIRE(load2->getOperands().size() == 2);
  CHECK(load2->getOperands()[0]->getType() == casm::Operand::Type::Register);
  CHECK(load2->getOperands()[1]->getType() == casm::Operand::Type::Memory);
  
  auto* memRef2 = static_cast<const casm::MemoryOperand*>(load2->getOperands()[1].get());
  CHECK(memRef2->getReference().reg == "r2");
  CHECK(memRef2->getReference().offset == 8);
  
  // Store instruction
  auto* store = filtered[2].getInstruction();
  REQUIRE(store != nullptr);
  CHECK(store->getName() == "store");
  REQUIRE(store->getOperands().size() == 2);
  CHECK(store->getOperands()[0]->getType() == casm::Operand::Type::Memory);
  CHECK(store->getOperands()[1]->getType() == casm::Operand::Type::Register);
  
  auto* memRef3 = static_cast<const casm::MemoryOperand*>(store->getOperands()[0].get());
  CHECK(memRef3->getReference().reg == "r1");
  CHECK(memRef3->getReference().offset == 0);
}

TEST_CASE("Parser parses data directives", "[parser]") {
  std::string source = R"(
    .section .data
    .i32 $id1, $id2, $id3, $id4     ; 32-bit integers
    .f64 $fd3.14, $fd2.71     ; 64-bit floats
    #string_data
    .ascii $"Hello"      ; ASCII string
    .asciiz $"World"     ; Null-terminated string
  )";
  
  casm::Lexer lexer("test", source);
  casm::Parser parser(lexer);
  
  std::vector<casm::Statement> statements = parser.parse();
  
  // Check for parser errors
  REQUIRE(parser.getErrors().empty());
  
  // Filter out empty statements
  std::vector<casm::Statement> filtered;
  for (const auto& stmt : statements) {
    if (stmt.getType() != casm::Statement::Type::Empty) {
      filtered.push_back(stmt);
    }
  }
  
  REQUIRE(filtered.size() >= 5); // section, i32, f64, label+ascii, asciiz
  
  // Check section directive
  CHECK(filtered[0].getType() == casm::Statement::Type::Directive);
  CHECK(filtered[0].getDirective()->getName() == "section");
  
  // Check i32 directive
  CHECK(filtered[1].getType() == casm::Statement::Type::Directive);
  CHECK(filtered[1].getDirective()->getName() == "i32");
  REQUIRE(filtered[1].getDirective()->getOperands().size() == 4);
  
  for (int i = 0; i < 4; ++i) {
    CHECK(filtered[1].getDirective()->getOperands()[i]->getType() == casm::Operand::Type::Immediate);
    auto* immediate = static_cast<const casm::ImmediateOperand*>(filtered[1].getDirective()->getOperands()[i].get());
    CHECK(immediate->getValue().format == casm::ImmediateFormat::Integer);
    CHECK(std::get<casm::i64>(immediate->getValue().value) == i + 1);
  }
  
  // Check f64 directive
  CHECK(filtered[2].getType() == casm::Statement::Type::Directive);
  CHECK(filtered[2].getDirective()->getName() == "f64");
  REQUIRE(filtered[2].getDirective()->getOperands().size() == 2);
  
  // Check first float value (3.14)
  CHECK(filtered[2].getDirective()->getOperands()[0]->getType() == casm::Operand::Type::Immediate);
  auto* float1 = static_cast<const casm::ImmediateOperand*>(filtered[2].getDirective()->getOperands()[0].get());
  CHECK(float1->getValue().format == casm::ImmediateFormat::Float);
  CHECK(std::get<casm::f64>(float1->getValue().value) == Approx(3.14));
  
  // Find label and string directives
  bool foundStringLabel = false;
  bool foundAscii = false;
  bool foundAsciiz = false;
  
  for (const auto& stmt : filtered) {
    if (stmt.getLabel() == "string_data") {
      foundStringLabel = true;
    }
    
    if (stmt.getType() == casm::Statement::Type::Directive) {
      auto* directive = stmt.getDirective();
      
      if (directive->getName() == "ascii") {
        foundAscii = true;
        REQUIRE(directive->getOperands().size() == 1);
        CHECK(directive->getOperands()[0]->getType() == casm::Operand::Type::Immediate);
        auto* str = static_cast<const casm::ImmediateOperand*>(directive->getOperands()[0].get());
        CHECK(str->getValue().format == casm::ImmediateFormat::String);
        CHECK(std::get<std::string>(str->getValue().value) == "Hello");
      }
      
      if (directive->getName() == "asciiz") {
        foundAsciiz = true;
        REQUIRE(directive->getOperands().size() == 1);
        CHECK(directive->getOperands()[0]->getType() == casm::Operand::Type::Immediate);
        auto* str = static_cast<const casm::ImmediateOperand*>(directive->getOperands()[0].get());
        CHECK(str->getValue().format == casm::ImmediateFormat::String);
        CHECK(std::get<std::string>(str->getValue().value) == "World");
      }
    }
  }
  
  CHECK(foundStringLabel);
  CHECK(foundAscii);
  CHECK(foundAsciiz);
}