/**
 * @file test_parser.cpp
 * @brief Tests for the CASM parser
 */

#include <catch2/catch_test_macros.hpp>
#include "casm/parser.hpp"

// Helper function to parse a single line
static casm::Line parseLine(casm::Parser& parser, const std::string& text, size_t line_number = 1) {
  casm::Line line;
  casm::Result result = parser.parseLine(text, line_number, line);
  REQUIRE(result == casm::Result::Success);
  return line;
}

TEST_CASE("Parser empty line handling", "[parser]") {
  casm::Parser parser;
  
  SECTION("Empty line") {
    casm::Line line = parseLine(parser, "");
    CHECK(line.type == casm::LineType::Empty);
  }
  
  SECTION("Whitespace line") {
    casm::Line line = parseLine(parser, "    ");
    CHECK(line.type == casm::LineType::Empty);
  }
  
  SECTION("Comment line") {
    casm::Line line = parseLine(parser, "; This is a comment");
    CHECK(line.type == casm::LineType::Empty);
  }
  
  SECTION("Line with leading whitespace and comment") {
    casm::Line line = parseLine(parser, "   ; Whitespace and comment");
    CHECK(line.type == casm::LineType::Empty);
  }
}

TEST_CASE("Parser label handling", "[parser]") {
  casm::Parser parser;
  
  SECTION("Label only") {
    casm::Line line = parseLine(parser, "label:");
    CHECK(line.type == casm::LineType::Label);
    CHECK(line.label == "label");
  }
  
  SECTION("Label with whitespace") {
    casm::Line line = parseLine(parser, "  label:  ");
    CHECK(line.type == casm::LineType::Label);
    CHECK(line.label == "label");
  }
  
  SECTION("Label with comment") {
    casm::Line line = parseLine(parser, "label: ; A comment");
    CHECK(line.type == casm::LineType::Label);
    CHECK(line.label == "label");
  }
}

TEST_CASE("Parser instruction handling", "[parser]") {
  casm::Parser parser;
  
  SECTION("Simple instruction") {
    casm::Line line = parseLine(parser, "add r1, r2, r3");
    REQUIRE(line.type == casm::LineType::Instruction);
    CHECK(line.instruction->name == "add");
    CHECK(line.instruction->condition.empty());
    REQUIRE(line.instruction->operands.size() == 3);
    
    CHECK(line.instruction->operands[0].type == casm::Operand::Type::Register);
    CHECK(line.instruction->operands[0].reg.number == 1);
    
    CHECK(line.instruction->operands[1].type == casm::Operand::Type::Register);
    CHECK(line.instruction->operands[1].reg.number == 2);
    
    CHECK(line.instruction->operands[2].type == casm::Operand::Type::Register);
    CHECK(line.instruction->operands[2].reg.number == 3);
  }
  
  SECTION("Instruction with condition") {
    casm::Line line = parseLine(parser, "br.eq label");
    REQUIRE(line.type == casm::LineType::Instruction);
    CHECK(line.instruction->name == "br");
    CHECK(line.instruction->condition == "eq");
    REQUIRE(line.instruction->operands.size() == 1);
    
    CHECK(line.instruction->operands[0].type == casm::Operand::Type::Label);
    CHECK(*line.instruction->operands[0].label == "label");
  }
  
  SECTION("Instruction with label") {
    casm::Line line = parseLine(parser, "start: add r1, r2, r3");
    REQUIRE(line.type == casm::LineType::Instruction);
    CHECK(line.label == "start");
    CHECK(line.instruction->name == "add");
    REQUIRE(line.instruction->operands.size() == 3);
  }
  
  SECTION("Instruction with immediate value") {
    casm::Line line = parseLine(parser, "add r1, r2, 42");
    REQUIRE(line.type == casm::LineType::Instruction);
    CHECK(line.instruction->operands[2].type == casm::Operand::Type::Immediate);
    CHECK(line.instruction->operands[2].imm.i_value == 42);
  }
  
  SECTION("Instruction with hex immediate") {
    casm::Line line = parseLine(parser, "add r1, r2, 0xFF");
    REQUIRE(line.type == casm::LineType::Instruction);
    CHECK(line.instruction->operands[2].type == casm::Operand::Type::Immediate);
    CHECK(line.instruction->operands[2].imm.i_value == 255);
  }
  
  SECTION("Instruction with memory operand") {
    casm::Line line = parseLine(parser, "load r1, [r2+8]");
    REQUIRE(line.type == casm::LineType::Instruction);
    CHECK(line.instruction->name == "load");
    REQUIRE(line.instruction->operands.size() == 2);
    
    CHECK(line.instruction->operands[0].type == casm::Operand::Type::Register);
    CHECK(line.instruction->operands[0].reg.number == 1);
    
    CHECK(line.instruction->operands[1].type == casm::Operand::Type::Memory);
    CHECK(line.instruction->operands[1].mem.base.number == 2);
    CHECK(line.instruction->operands[1].mem.offset == 8);
  }
  
  SECTION("Instruction with negative offset memory operand") {
    casm::Line line = parseLine(parser, "load r1, [r2-4]");
    REQUIRE(line.type == casm::LineType::Instruction);
    CHECK(line.instruction->operands[1].type == casm::Operand::Type::Memory);
    CHECK(line.instruction->operands[1].mem.base.number == 2);
    CHECK(line.instruction->operands[1].mem.offset == -4);
  }
  
  SECTION("Instruction with typed register") {
    casm::Line line = parseLine(parser, "add r1.i64, r2.i64, r3.i64");
    REQUIRE(line.type == casm::LineType::Instruction);
    
    CHECK(line.instruction->operands[0].type == casm::Operand::Type::Register);
    CHECK(line.instruction->operands[0].reg.number == 1);
    CHECK(line.instruction->operands[0].reg.type == coil::ValueType::I64);
    CHECK(line.instruction->operands[0].reg.has_explicit_type == true);
  }
}

TEST_CASE("Parser directive handling", "[parser]") {
  casm::Parser parser;
  
  SECTION("Section directive") {
    casm::Line line = parseLine(parser, ".section .text");
    REQUIRE(line.type == casm::LineType::Directive);
    CHECK(line.directive->name == ".section");
    REQUIRE(line.directive->args.size() == 1);
    CHECK(line.directive->args[0].text == ".text");
  }
  
  SECTION("Global directive") {
    casm::Line line = parseLine(parser, ".global main");
    REQUIRE(line.type == casm::LineType::Directive);
    CHECK(line.directive->name == ".global");
    REQUIRE(line.directive->args.size() == 1);
    CHECK(line.directive->args[0].text == "main");
  }
  
  SECTION("Integer directive") {
    casm::Line line = parseLine(parser, ".i32 1, 2, 3, 4");
    REQUIRE(line.type == casm::LineType::Directive);
    CHECK(line.directive->name == ".i32");
    REQUIRE(line.directive->args.size() == 4);
    CHECK(line.directive->args[0].text == "1");
    CHECK(line.directive->args[1].text == "2");
    CHECK(line.directive->args[2].text == "3");
    CHECK(line.directive->args[3].text == "4");
  }
  
  SECTION("String directive") {
    casm::Line line = parseLine(parser, ".string \"Hello, world!\"");
    REQUIRE(line.type == casm::LineType::Directive);
    CHECK(line.directive->name == ".string");
    REQUIRE(line.directive->args.size() == 1);
    CHECK(line.directive->args[0].text == "\"Hello, world!\"");
  }
  
  SECTION("Directive with label") {
    casm::Line line = parseLine(parser, "message: .string \"Hello, world!\"");
    REQUIRE(line.type == casm::LineType::Directive);
    CHECK(line.label == "message");
    CHECK(line.directive->name == ".string");
    REQUIRE(line.directive->args.size() == 1);
  }
}

TEST_CASE("Parser error handling", "[parser]") {
  casm::Parser parser;
  casm::Line line;
  
  SECTION("Invalid instruction") {
    casm::Result result = parser.parseLine("invalid_instr r1, r2", 1, line);
    // This should still parse as an instruction with an unknown name
    CHECK(result == casm::Result::Success);
    REQUIRE(line.type == casm::LineType::Instruction);
    CHECK(line.instruction->name == "invalid_instr");
  }
  
  SECTION("Malformed memory reference") {
    casm::Result result = parser.parseLine("load r1, [r2", 1, line);
    CHECK(result != casm::Result::Success);
  }
  
  SECTION("Malformed register") {
    casm::Result result = parser.parseLine("add rx, r2, r3", 1, line);
    CHECK(result != casm::Result::Success);
  }
}