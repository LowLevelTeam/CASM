/**
 * @file test_codegen.cpp
 * @brief Tests for the CASM code generator
 */

#include <catch2/catch_test_macros.hpp>
#include "casm/codegen.hpp"
#include "casm/parser.hpp"
#include "coil/obj.hpp"
#include <sstream>

// Helper function to generate code for a single instruction
static coil::Result generateInstruction(casm::CodeGenerator& codegen, const std::string& instruction) {
  casm::Parser parser;
  casm::Line line;
  
  coil::Result result = parser.parseLine(instruction, 1, line);
  if (result != coil::Result::Success) {
    return result;
  }
  
  return codegen.generateLine(line);
}

TEST_CASE("CodeGenerator basic functionality", "[codegen]") {
  casm::CodeGenerator codegen;
  
  SECTION("Initialization") {
    coil::Result result = codegen.initialize();
    CHECK(result == coil::Result::Success);
  }
  
  SECTION("Generate simple instruction") {
    coil::Result result = codegen.initialize();
    REQUIRE(result == coil::Result::Success);
    
    result = generateInstruction(codegen, "add r1, r2, r3");
    CHECK(result == coil::Result::Success);
    
    // Finalize and check object
    coil::Object obj;
    result = codegen.finalize(obj);
    CHECK(result == coil::Result::Success);
    
    // Check text section exists and has data
    uint16_t text_index = obj.getSectionIndex(".text", 5);
    REQUIRE(text_index > 0);
    
    const coil::BaseSection* section = obj.getSection(text_index);
    REQUIRE(section != nullptr);
    
    // Check section type
    CHECK(section->getSectionType() == static_cast<uint8_t>(coil::SectionType::ProgBits));
    
    // Text section should have instruction data
    const coil::DataSection* text_section = static_cast<const coil::DataSection*>(section);
    CHECK(text_section->getSize() > 0);
  }
}

TEST_CASE("CodeGenerator section handling", "[codegen]") {
  casm::CodeGenerator codegen;
  coil::Result result = codegen.initialize();
  REQUIRE(result == coil::Result::Success);
  
  SECTION("Change section") {
    casm::Parser parser;
    casm::Line line;
    
    // Parse section directive
    result = parser.parseLine(".section .data", 1, line);
    REQUIRE(result == coil::Result::Success);
    REQUIRE(line.type == casm::LineType::Directive);
    
    // Generate code for directive
    result = codegen.generateLine(line);
    CHECK(result == coil::Result::Success);
    
    // Add data to the section
    result = parser.parseLine(".i32 42", 2, line);
    REQUIRE(result == coil::Result::Success);
    
    result = codegen.generateLine(line);
    CHECK(result == coil::Result::Success);
    
    // Finalize and check object
    coil::Object obj;
    result = codegen.finalize(obj);
    CHECK(result == coil::Result::Success);
    
    // Check data section exists and has data
    uint16_t data_index = obj.getSectionIndex(".data", 5);
    REQUIRE(data_index > 0);
    
    const coil::BaseSection* section = obj.getSection(data_index);
    REQUIRE(section != nullptr);
    
    // Data section should have our integer
    const coil::DataSection* data_section = static_cast<const coil::DataSection*>(section);
    CHECK(data_section->getSize() >= 4); // At least 4 bytes for i32
  }
}

TEST_CASE("CodeGenerator symbol handling", "[codegen]") {
  casm::CodeGenerator codegen;
  coil::Result result = codegen.initialize();
  REQUIRE(result == coil::Result::Success);
  
  SECTION("Define label") {
    casm::Parser parser;
    casm::Line line;
    
    // Define a label
    result = parser.parseLine("main:", 1, line);
    REQUIRE(result == coil::Result::Success);
    REQUIRE(line.type == casm::LineType::Label);
    
    result = codegen.generateLine(line);
    CHECK(result == coil::Result::Success);
    
    // Add an instruction after the label
    result = parser.parseLine("add r1, r2, r3", 2, line);
    REQUIRE(result == coil::Result::Success);
    
    result = codegen.generateLine(line);
    CHECK(result == coil::Result::Success);
    
    // Finalize and check object
    coil::Object obj;
    result = codegen.finalize(obj);
    CHECK(result == coil::Result::Success);
    
    // Check symbol table
    REQUIRE(obj.getSymbolTable() != nullptr);
    
    // Symbol table should have our label
    uint16_t main_index = obj.getSymbolIndex("main", 4);
    REQUIRE(main_index > 0);
    
    const coil::Symbol* main_sym = obj.getSymbol(main_index);
    REQUIRE(main_sym != nullptr);
    
    // By default, labels are local
    CHECK(main_sym->binding == static_cast<uint8_t>(coil::SymbolBinding::Local));
  }
  
  SECTION("Global symbol") {
    casm::Parser parser;
    casm::Line line;
    
    // Declare symbol as global
    result = parser.parseLine(".global main", 1, line);
    REQUIRE(result == coil::Result::Success);
    
    result = codegen.generateLine(line);
    CHECK(result == coil::Result::Success);
    
    // Define the label
    result = parser.parseLine("main:", 2, line);
    REQUIRE(result == coil::Result::Success);
    
    result = codegen.generateLine(line);
    CHECK(result == coil::Result::Success);
    
    // Finalize and check object
    coil::Object obj;
    result = codegen.finalize(obj);
    CHECK(result == coil::Result::Success);
    
    // Check symbol table
    REQUIRE(obj.getSymbolTable() != nullptr);
    
    // Symbol table should have our label
    uint16_t main_index = obj.getSymbolIndex("main", 4);
    REQUIRE(main_index > 0);
    
    const coil::Symbol* main_sym = obj.getSymbol(main_index);
    REQUIRE(main_sym != nullptr);
    
    // Symbol should be global
    CHECK(main_sym->binding == static_cast<uint8_t>(coil::SymbolBinding::Global));
  }
}

TEST_CASE("CodeGenerator data directives", "[codegen]") {
  casm::CodeGenerator codegen;
  coil::Result result = codegen.initialize();
  REQUIRE(result == coil::Result::Success);
  
  SECTION("Integer directives") {
    casm::Parser parser;
    
    // Switch to data section
    casm::Line line;
    result = parser.parseLine(".section .data", 1, line);
    REQUIRE(result == coil::Result::Success);
    
    result = codegen.generateLine(line);
    CHECK(result == coil::Result::Success);
    
    // Add various integer data
    result = parser.parseLine(".i32 42, -42", 2, line);
    REQUIRE(result == coil::Result::Success);
    
    result = codegen.generateLine(line);
    CHECK(result == coil::Result::Success);
    
    // Finalize and check object
    coil::Object obj;
    result = codegen.finalize(obj);
    CHECK(result == coil::Result::Success);
    
    // Check data section
    uint16_t data_index = obj.getSectionIndex(".data", 5);
    REQUIRE(data_index > 0);
    
    const coil::BaseSection* section = obj.getSection(data_index);
    REQUIRE(section != nullptr);
    
    // Data section should have our integers
    const coil::DataSection* data_section = static_cast<const coil::DataSection*>(section);
    CHECK(data_section->getSize() >= 8); // 2 x 4 bytes for i32
  }
  
  SECTION("String directive") {
    casm::Parser parser;
    
    // Switch to data section
    casm::Line line;
    result = parser.parseLine(".section .data", 1, line);
    REQUIRE(result == coil::Result::Success);
    
    result = codegen.generateLine(line);
    CHECK(result == coil::Result::Success);
    
    // Add string data
    result = parser.parseLine(".string \"Hello, world!\"", 2, line);
    REQUIRE(result == coil::Result::Success);
    
    result = codegen.generateLine(line);
    CHECK(result == coil::Result::Success);
    
    // Finalize and check object
    coil::Object obj;
    result = codegen.finalize(obj);
    CHECK(result == coil::Result::Success);
    
    // Check data section
    uint16_t data_index = obj.getSectionIndex(".data", 5);
    REQUIRE(data_index > 0);
    
    const coil::BaseSection* section = obj.getSection(data_index);
    REQUIRE(section != nullptr);
    
    // Data section should have our string
    const coil::DataSection* data_section = static_cast<const coil::DataSection*>(section);
    CHECK(data_section->getSize() >= 14); // "Hello, world!" + null terminator
  }
}

TEST_CASE("CodeGenerator instruction encoding", "[codegen]") {
  casm::CodeGenerator codegen;
  coil::Result result = codegen.initialize();
  REQUIRE(result == coil::Result::Success);
  
  SECTION("Various instructions") {
    // Test various instruction types
    std::vector<std::string> instructions = {
      "nop",
      "add r1, r2, r3",
      "sub r1, r2, 42",
      "mul r1, r2, r3",
      "push r1",
      "pop r1",
      "load r1, [r2+8]",
      "store [r2], r1",
      "cmp r1, r2",
      "br.eq label",
      "jump r1",
      "call function"
    };
    
    for (const auto& instr : instructions) {
      result = generateInstruction(codegen, instr);
      CHECK(result == coil::Result::Success);
    }
    
    // Finalize and check object
    coil::Object obj;
    result = codegen.finalize(obj);
    CHECK(result == coil::Result::Success);
    
    // Check text section
    uint16_t text_index = obj.getSectionIndex(".text", 5);
    REQUIRE(text_index > 0);
    
    const coil::BaseSection* section = obj.getSection(text_index);
    REQUIRE(section != nullptr);
    
    // Text section should have all our instructions
    const coil::DataSection* text_section = static_cast<const coil::DataSection*>(section);
    CHECK(text_section->getSize() > 0);
    
    // Each instruction is sizeof(coil::Instruction) bytes
    CHECK(text_section->getSize() >= instructions.size() * sizeof(coil::Instruction));
  }
}