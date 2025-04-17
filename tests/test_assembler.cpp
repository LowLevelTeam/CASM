#include <catch2/catch_test_macros.hpp>
#include "casm/lexer.hpp"
#include "casm/parser.hpp"
#include "casm/assembler.hpp"
#include <coil/coil.hpp>
#include <coil/stream.hpp>
#include <sstream>
#include <vector>

// Initialize COIL library for tests
struct CoilTestFixture {
  CoilTestFixture() {
    coil::initialize();
  }
  
  ~CoilTestFixture() {
    coil::shutdown();
  }
};

TEST_CASE_METHOD(CoilTestFixture, "Assembler assembles basic program", "[assembler]") {
  std::string source = R"(
    .section .text
    
    #main
      mov %r1, $id42   ; Load constant
      add %r1, %r1, %r2 ; Add registers
      ret              ; Return
  )";
  
  // Create assembler
  casm::Assembler assembler;
  
  // Assemble source
  coil::Object obj = assembler.assembleSource(source, "test.casm");
  
  // Check for assembler errors
  REQUIRE(assembler.getErrors().empty());
  
  // The object should have at least one section (.text)
  CHECK(obj.getSectionCount() > 0);
  
  // Check sections
  coil::u16 textSectionIndex = obj.getSectionIndex(".text");
  REQUIRE(textSectionIndex > 0);
  
  const coil::BaseSection* textSection = obj.getSection(textSectionIndex);
  REQUIRE(textSection != nullptr);
  
  // Text section should have code flag
  CHECK((textSection->getHeader().flags & static_cast<uint16_t>(coil::SectionFlag::Code)) != 0);
  
  // The text section should have data
  const coil::DataSection* dataSection = dynamic_cast<const coil::DataSection*>(textSection);
  REQUIRE(dataSection != nullptr);
  CHECK(!dataSection->getData().empty());
  
  // Test string table and symbol table exist
  CHECK(obj.getStringTable() != nullptr);
  CHECK(obj.getSymbolTable() != nullptr);
}

TEST_CASE_METHOD(CoilTestFixture, "Assembler handles sections and global symbols", "[assembler]") {
  std::string source = R"(
    .section .text
    .global @main
    
    #main
      call @helper
      ret
      
    #helper
      mov %r1, $id100
      ret
      
    .section .data
    #constants
      .i32 1, 2, 3, 4
      .asciiz "Hello, world!"
  )";
  
  // Create assembler
  casm::Assembler assembler;
  
  // Assemble source
  coil::Object obj = assembler.assembleSource(source, "test.casm");
  
  // Check for assembler errors
  REQUIRE(assembler.getErrors().empty());
  
  // Should have .text and .data sections
  coil::u16 textSectionIndex = obj.getSectionIndex(".text");
  coil::u16 dataSectionIndex = obj.getSectionIndex(".data");
  
  REQUIRE(textSectionIndex > 0);
  REQUIRE(dataSectionIndex > 0);
  
  // Check section types and flags
  const coil::BaseSection* textSection = obj.getSection(textSectionIndex);
  const coil::BaseSection* dataSection = obj.getSection(dataSectionIndex);
  
  REQUIRE(textSection != nullptr);
  REQUIRE(dataSection != nullptr);
  
  CHECK(textSection->getSectionType() == static_cast<coil::u8>(coil::SectionType::ProgBits));
  CHECK(dataSection->getSectionType() == static_cast<coil::u8>(coil::SectionType::ProgBits));
  
  CHECK((textSection->getHeader().flags & static_cast<uint16_t>(coil::SectionFlag::Code)) != 0);
  CHECK((dataSection->getHeader().flags & static_cast<uint16_t>(coil::SectionFlag::Write)) != 0);
  
  // Check symbol table for main symbol
  coil::u16 mainSymbolIndex = obj.getSymbolIndex("main");
  REQUIRE(mainSymbolIndex > 0);
  
  const coil::Symbol* mainSymbol = obj.getSymbol(mainSymbolIndex);
  REQUIRE(mainSymbol != nullptr);
  
  CHECK(mainSymbol->section_index == textSectionIndex);
  CHECK(mainSymbol->binding == static_cast<coil::u8>(coil::SymbolBinding::Global));
  
  // Data section should have data
  const coil::DataSection* dataSecData = dynamic_cast<const coil::DataSection*>(dataSection);
  REQUIRE(dataSecData != nullptr);
  CHECK(dataSecData->getData().size() > 16); // At least 4 ints (16 bytes) + string
}

TEST_CASE_METHOD(CoilTestFixture, "Assembler handles data directives", "[assembler]") {
  std::string source = R"(
    .section .data
    
    ; Integer data
    #integers
      .i8  1, -1
      .i16 1000, -1000
      .i32 100000, -100000
      .i64 1000000000000, -1000000000000
      
    ; Unsigned integer data
    #unsigned
      .u8  255
      .u16 65535
      .u32 4294967295
      .u64 18446744073709551615
      
    ; Floating point data
    #floats
      .f32 3.14159
      .f64 2.71828
      
    ; String data
    #strings
      .ascii "Hello"
      .asciiz "World"
      
    ; Zero data
    #zeros
      .zero 10
  )";
  
  // Create assembler
  casm::Assembler assembler;
  
  // Assemble source
  coil::Object obj = assembler.assembleSource(source, "test.casm");
  
  // Check for assembler errors
  REQUIRE(assembler.getErrors().empty());
  
  // Should have .data section
  coil::u16 dataSectionIndex = obj.getSectionIndex(".data");
  REQUIRE(dataSectionIndex > 0);
  
  // Check data section
  const coil::BaseSection* dataSection = obj.getSection(dataSectionIndex);
  REQUIRE(dataSection != nullptr);
  
  // Data section should have data
  const coil::DataSection* dataSecData = dynamic_cast<const coil::DataSection*>(dataSection);
  REQUIRE(dataSecData != nullptr);
  
  const std::vector<coil::u8>& data = dataSecData->getData();
  REQUIRE(!data.empty());
  
  // Check section size (rough estimate)
  // The section should contain:
  // - 2 i8 values (2 bytes)
  // - 2 i16 values (4 bytes)
  // - 2 i32 values (8 bytes)
  // - 2 i64 values (16 bytes)
  // - 1 u8 value (1 byte)
  // - 1 u16 value (2 bytes)
  // - 1 u32 value (4 bytes)
  // - 1 u64 value (8 bytes)
  // - 1 f32 value (4 bytes)
  // - 1 f64 value (8 bytes)
  // - "Hello" string (5 bytes)
  // - "World" string + null terminator (6 bytes)
  // - Zero padding (10 bytes)
  // Total: ~78 bytes
  
  CHECK(data.size() >= 70);
}

TEST_CASE_METHOD(CoilTestFixture, "Assembler resolves label references", "[assembler]") {
  std::string source = R"(
    .section .text
    
    #main
      jmp @loop          ; Jump to loop
      
    #data_ref
      load %r1, @data    ; Load data address
      ret
      
    #loop
      inc %r1            ; Increment r1
      cmp %r1, $id10     ; Compare with 10
      br ^lt @loop       ; Loop if less than 10
      ret                ; Return
      
    .section .data
    #data
      .i32 1, 2, 3, 4    ; Some data
  )";
  
  // Create assembler
  casm::Assembler assembler;
  
  // Assemble source
  coil::Object obj = assembler.assembleSource(source, "test.casm");
  
  // Check for assembler errors
  REQUIRE(assembler.getErrors().empty());
  
  // Should have .text and .data sections
  coil::u16 textSectionIndex = obj.getSectionIndex(".text");
  coil::u16 dataSectionIndex = obj.getSectionIndex(".data");
  
  REQUIRE(textSectionIndex > 0);
  REQUIRE(dataSectionIndex > 0);
  
  // Both sections should have data
  const coil::DataSection* textSection = dynamic_cast<const coil::DataSection*>(obj.getSection(textSectionIndex));
  const coil::DataSection* dataSection = dynamic_cast<const coil::DataSection*>(obj.getSection(dataSectionIndex));
  
  REQUIRE(textSection != nullptr);
  REQUIRE(dataSection != nullptr);
  
  CHECK(!textSection->getData().empty());
  CHECK(!dataSection->getData().empty());
  
  // Assembler should have resolved all references without errors
  CHECK(assembler.getErrors().empty());
}

TEST_CASE_METHOD(CoilTestFixture, "Assembler handles complete factorial example", "[assembler]") {
  std::string source = R"(
    ; Calculate factorial of n
    .section .text
    .global @factorial
    
    #factorial
      ; r1 = input value
      ; returns factorial in r1
      cmp %r1, $id0
      br ^eq @base_case
      
      ; Factorial(n) = n * Factorial(n-1)
      push %r1              ; Save n
      dec %r1               ; n-1
      call @factorial       ; Compute Factorial(n-1)
      mov %r2, %r1          ; r2 = Factorial(n-1)
      pop %r1               ; Restore n
      mul %r1, %r1, %r2     ; r1 = n * Factorial(n-1)
      ret
      
    #base_case
      mov %r1, $id1         ; Factorial(0) = 1
      ret
      
    .section .data
    #factorial_input
      .i32 5
  )";
  
  // Create assembler
  casm::Assembler assembler;
  
  // Assemble source
  coil::Object obj = assembler.assembleSource(source, "factorial.casm");
  
  // Check for assembler errors
  REQUIRE(assembler.getErrors().empty());
  
  // Should have .text and .data sections
  coil::u16 textSectionIndex = obj.getSectionIndex(".text");
  coil::u16 dataSectionIndex = obj.getSectionIndex(".data");
  
  REQUIRE(textSectionIndex > 0);
  REQUIRE(dataSectionIndex > 0);
  
  // Check symbol table for factorial symbol
  coil::u16 factorialSymbolIndex = obj.getSymbolIndex("factorial");
  REQUIRE(factorialSymbolIndex > 0);
  
  const coil::Symbol* factorialSymbol = obj.getSymbol(factorialSymbolIndex);
  REQUIRE(factorialSymbol != nullptr);
  
  CHECK(factorialSymbol->section_index == textSectionIndex);
  CHECK(factorialSymbol->binding == static_cast<coil::u8>(coil::SymbolBinding::Global));
  
  // Both sections should have data
  const coil::DataSection* textSection = dynamic_cast<const coil::DataSection*>(obj.getSection(textSectionIndex));
  const coil::DataSection* dataSection = dynamic_cast<const coil::DataSection*>(obj.getSection(dataSectionIndex));
  
  REQUIRE(textSection != nullptr);
  REQUIRE(dataSection != nullptr);
  
  CHECK(!textSection->getData().empty());
  CHECK(!dataSection->getData().empty());
  
  // Check that data section contains the factorial input value (5)
  const std::vector<coil::u8>& data = dataSection->getData();
  REQUIRE(data.size() >= 4); // At least 4 bytes for i32
  
  // Check the i32 value if it's 5 (little-endian)
  if (data.size() >= 4) {
    uint32_t value = (static_cast<uint32_t>(data[3]) << 24) |
                     (static_cast<uint32_t>(data[2]) << 16) |
                     (static_cast<uint32_t>(data[1]) << 8) |
                     static_cast<uint32_t>(data[0]);
    CHECK(value == 5);
  }
}