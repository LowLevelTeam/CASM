#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include "casm/lexer.hpp"
#include "casm/parser.hpp"
#include "casm/assembler.hpp"
#include <coil/coil.hpp>
#include <coil/stream.hpp>
#include <sstream>
#include <vector>
#include <string>
#include <memory>
#include <cstdio>

using namespace casm;
using namespace Catch::Matchers;

// Helper function to assemble a string directly
coil::Object assembleString(const std::string& source, std::vector<std::string>* errors = nullptr) {
    // Create assembler
    Assembler assembler;
    
    // Set verbose mode for debugging
    Assembler::Options options;
    options.verbose = true;
    assembler.setOptions(options);
    
    // Assemble source
    auto result = assembler.assembleSource(source, "test.casm");
    
    // Copy errors if requested
    if (errors) {
        *errors = assembler.getErrors();
    }
    
    return result.object;
}

// Initialize COIL library for tests
struct CoilTestFixture {
    CoilTestFixture() {
        coil::initialize();
    }
    
    ~CoilTestFixture() {
        coil::shutdown();
    }
};

TEST_CASE_METHOD(CoilTestFixture, "Basic assembler tests", "[assembler]") {
    SECTION("Empty source") {
        std::vector<std::string> errors;
        coil::Object obj = assembleString("", &errors);
        
        CHECK(errors.empty());
        CHECK(obj.getSectionCount() == 0);
    }
    
    SECTION("Invalid syntax") {
        std::vector<std::string> errors;
        coil::Object obj = assembleString("invalid syntax here", &errors);
        
        CHECK(!errors.empty());
    }
    
    SECTION("Simple NOP instruction") {
        std::string source = "nop";
        std::vector<std::string> errors;
        coil::Object obj = assembleString(source, &errors);
        
        CHECK(errors.empty());
        CHECK(obj.getSectionCount() > 0);
        
        // Should have .text section
        coil::u16 textIndex = obj.getSectionIndex(".text");
        REQUIRE(textIndex > 0);
        
        // Get .text section data
        auto* section = dynamic_cast<const coil::DataSection*>(obj.getSection(textIndex));
        REQUIRE(section != nullptr);
        
        // Should have some data (at least the NOP instruction)
        CHECK(!section->getData().empty());
        
        // First byte should be the NOP opcode
        CHECK(section->getData()[0] == static_cast<coil::u8>(coil::Opcode::Nop));
    }
}

TEST_CASE_METHOD(CoilTestFixture, "Sections and directives", "[assembler]") {
    SECTION("Multiple sections") {
        std::string source = R"(
            .section .text
            nop
            
            .section .data
            .i32 $id42
            
            .section .bss
            .zero 100
        )";
        
        std::vector<std::string> errors;
        coil::Object obj = assembleString(source, &errors);
        
        CHECK(errors.empty());
        
        // Should have all three sections
        CHECK(obj.getSectionIndex(".text") > 0);
        CHECK(obj.getSectionIndex(".data") > 0);
        CHECK(obj.getSectionIndex(".bss") > 0);
        
        // Check section flags
        auto* textSection = obj.getSection(obj.getSectionIndex(".text"));
        auto* dataSection = obj.getSection(obj.getSectionIndex(".data"));
        auto* bssSection = obj.getSection(obj.getSectionIndex(".bss"));
        
        REQUIRE(textSection != nullptr);
        REQUIRE(dataSection != nullptr);
        REQUIRE(bssSection != nullptr);
        
        // Text section should have code flag
        CHECK((textSection->getHeader().flags & static_cast<uint16_t>(coil::SectionFlag::Code)) != 0);
        
        // Data section should have write flag
        CHECK((dataSection->getHeader().flags & static_cast<uint16_t>(coil::SectionFlag::Write)) != 0);
        
        // BSS section should have write flag and be NoBits type
        CHECK((bssSection->getHeader().flags & static_cast<uint16_t>(coil::SectionFlag::Write)) != 0);
        CHECK(bssSection->getSectionType() == static_cast<coil::u8>(coil::SectionType::NoBits));
    }
    
    SECTION("Data directives") {
        std::string source = R"(
            .section .data
            .i8 $id1, $id2
            .i16 $id1000
            .i32 $id100000
            .i64 $id1000000000000
            .f32 $fd3.14
            .f64 $fd3.14159265359
            .ascii $"Hello"
            .asciiz $"World"
            .zero 10
        )";
        
        std::vector<std::string> errors;
        coil::Object obj = assembleString(source, &errors);
        
        CHECK(errors.empty());
        
        // Should have .data section
        coil::u16 dataIndex = obj.getSectionIndex(".data");
        REQUIRE(dataIndex > 0);
        
        // Get .data section data
        auto* section = dynamic_cast<const coil::DataSection*>(obj.getSection(dataIndex));
        REQUIRE(section != nullptr);
        
        // Section should have data
        CHECK(!section->getData().empty());
        
        // Calculate expected size:
        // .i8: 2 bytes
        // .i16: 2 bytes
        // .i32: 4 bytes
        // .i64: 8 bytes
        // .f32: 4 bytes
        // .f64: 8 bytes
        // .ascii "Hello": 5 bytes
        // .asciiz "World": 6 bytes (including null)
        // .zero: 10 bytes
        // Total: 49 bytes
        CHECK(section->getData().size() >= 49);
    }
    
    SECTION("Section with custom flags") {
        std::string source = R"(
            .section .custom ^Write ^Alloc
            .i32 $id1, $id2, $id3
        )";
        
        std::vector<std::string> errors;
        coil::Object obj = assembleString(source, &errors);
        
        CHECK(errors.empty());
        
        // Should have .custom section
        coil::u16 customIndex = obj.getSectionIndex(".custom");
        REQUIRE(customIndex > 0);
        
        // Check flags
        auto* section = obj.getSection(customIndex);
        REQUIRE(section != nullptr);
        
        // Should have Write and Alloc flags
        CHECK((section->getHeader().flags & static_cast<uint16_t>(coil::SectionFlag::Write)) != 0);
        CHECK((section->getHeader().flags & static_cast<uint16_t>(coil::SectionFlag::Alloc)) != 0);
        CHECK((section->getHeader().flags & static_cast<uint16_t>(coil::SectionFlag::Code)) == 0);
    }
}

TEST_CASE_METHOD(CoilTestFixture, "Symbols and labels", "[assembler]") {
    SECTION("Global symbols") {
        std::string source = R"(
            .section .text
            .global @main
            
            #main
              nop
              ret
        )";
        
        std::vector<std::string> errors;
        coil::Object obj = assembleString(source, &errors);
        
        CHECK(errors.empty());
        
        // Should have a symbol table
        REQUIRE(obj.getSymbolTable() != nullptr);
        
        // Should have main symbol
        coil::u16 mainIndex = obj.getSymbolIndex("main");
        REQUIRE(mainIndex > 0);
        
        // Check symbol properties
        const coil::Symbol* main = obj.getSymbol(mainIndex);
        REQUIRE(main != nullptr);
        
        // Should be in .text section
        CHECK(main->section_index == obj.getSectionIndex(".text"));
        
        // Should be global binding
        CHECK(main->binding == static_cast<coil::u8>(coil::SymbolBinding::Global));
    }
    
    SECTION("Multiple labels") {
        std::string source = R"(
            .section .text
            
            #start
              nop
              
            #middle
              add %r1, %r1, $id1
              
            #end
              ret
        )";
        
        std::vector<std::string> errors;
        coil::Object obj = assembleString(source, &errors);
        
        CHECK(errors.empty());
        
        // Should have a symbol table
        REQUIRE(obj.getSymbolTable() != nullptr);
        
        // Should have all three symbols
        CHECK(obj.getSymbolIndex("start") > 0);
        CHECK(obj.getSymbolIndex("middle") > 0);
        CHECK(obj.getSymbolIndex("end") > 0);
        
        // Offsets should be in order
        const coil::Symbol* start = obj.getSymbol(obj.getSymbolIndex("start"));
        const coil::Symbol* middle = obj.getSymbol(obj.getSymbolIndex("middle"));
        const coil::Symbol* end = obj.getSymbol(obj.getSymbolIndex("end"));
        
        REQUIRE(start != nullptr);
        REQUIRE(middle != nullptr);
        REQUIRE(end != nullptr);
        
        CHECK(start->value < middle->value);
        CHECK(middle->value < end->value);
    }
    
    SECTION("Label with directive") {
        std::string source = R"(
            .section .data
            
            #constants
              .i32 $id1, $id2, $id3, $id4
        )";
        
        std::vector<std::string> errors;
        coil::Object obj = assembleString(source, &errors);
        
        CHECK(errors.empty());
        
        // Should have constants symbol
        coil::u16 constantsIndex = obj.getSymbolIndex("constants");
        REQUIRE(constantsIndex > 0);
        
        // Symbol should be in .data section
        const coil::Symbol* constants = obj.getSymbol(constantsIndex);
        REQUIRE(constants != nullptr);
        CHECK(constants->section_index == obj.getSectionIndex(".data"));
        
        // Value should be 0 (start of the constants)
        CHECK(constants->value == 0);
    }
}

TEST_CASE_METHOD(CoilTestFixture, "Instructions", "[assembler]") {
    SECTION("Basic instructions") {
        std::string source = R"(
            .section .text
            
            nop             ; No operation
            mov %r1, %r2    ; Register to register
            mov %r1, $id42  ; Immediate to register
            add %r1, %r2, %r3
            sub %r1, %r2, $id10
            ret
        )";
        
        std::vector<std::string> errors;
        coil::Object obj = assembleString(source, &errors);
        
        CHECK(errors.empty());
        
        // Get text section
        coil::u16 textIndex = obj.getSectionIndex(".text");
        REQUIRE(textIndex > 0);
        
        auto* section = dynamic_cast<const coil::DataSection*>(obj.getSection(textIndex));
        REQUIRE(section != nullptr);
        
        // Should have data for all instructions
        const auto& data = section->getData();
        CHECK(!data.empty());
        
        // Check for NOP opcode in the first instruction
        CHECK(data[0] == static_cast<coil::u8>(coil::Opcode::Nop));
        
        // Check for MOV opcode in the second instruction
        // Assuming 8-byte instructions based on the encoding in assembler.cpp
        CHECK(data[8] == static_cast<coil::u8>(coil::Opcode::Mov));
        
        // Check for ADD opcode in the fourth instruction
        CHECK(data[24] == static_cast<coil::u8>(coil::Opcode::Add));
    }
    
    SECTION("Memory operations") {
        std::string source = R"(
            .section .text
            
            load %r1, [%r2]     ; Load from memory
            load %r1, [%r2+8]   ; Load with offset
            store [%r1], %r2    ; Store to memory
            store [%r1-4], %r2  ; Store with negative offset
        )";
        
        std::vector<std::string> errors;
        coil::Object obj = assembleString(source, &errors);
        
        CHECK(errors.empty());
        
        // Get text section
        coil::u16 textIndex = obj.getSectionIndex(".text");
        REQUIRE(textIndex > 0);
        
        auto* section = dynamic_cast<const coil::DataSection*>(obj.getSection(textIndex));
        REQUIRE(section != nullptr);
        
        // Should have data for all instructions
        const auto& data = section->getData();
        CHECK(!data.empty());
        
        // Check for LOAD opcode in the first instruction
        CHECK(data[0] == static_cast<coil::u8>(coil::Opcode::Load));
        
        // Check for STORE opcode in the third instruction
        // Assuming 8-byte instructions based on the encoding in assembler.cpp
        CHECK(data[16] == static_cast<coil::u8>(coil::Opcode::Store));
    }
    
    SECTION("Branch instructions with parameters") {
        std::string source = R"(
            .section .text
            
            #loop
              inc %r1
              cmp %r1, $id10
              br ^lt @loop    ; Branch if less than
              ret
        )";
        
        std::vector<std::string> errors;
        coil::Object obj = assembleString(source, &errors);
        
        CHECK(errors.empty());
        
        // Get text section
        coil::u16 textIndex = obj.getSectionIndex(".text");
        REQUIRE(textIndex > 0);
        
        auto* section = dynamic_cast<const coil::DataSection*>(obj.getSection(textIndex));
        REQUIRE(section != nullptr);
        
        // Should have data for all instructions
        const auto& data = section->getData();
        CHECK(!data.empty());
        
        // Find the BR instruction (third instruction)
        // Assuming 8-byte instructions based on the encoding in assembler.cpp
        CHECK(data[16] == static_cast<coil::u8>(coil::Opcode::Br));
        
        // Check flag byte (LT parameter)
        CHECK(data[17] == static_cast<coil::u8>(coil::InstrFlag0::LT));
    }
}

TEST_CASE_METHOD(CoilTestFixture, "Complete program examples", "[assembler]") {
    SECTION("Factorial example") {
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
        )";
        
        std::vector<std::string> errors;
        coil::Object obj = assembleString(source, &errors);
        
        CHECK(errors.empty());
        
        // Should have .text section
        coil::u16 textIndex = obj.getSectionIndex(".text");
        REQUIRE(textIndex > 0);
        
        // Should have factorial symbol
        coil::u16 factorialIndex = obj.getSymbolIndex("factorial");
        REQUIRE(factorialIndex > 0);
        
        // Should be global
        const coil::Symbol* factorial = obj.getSymbol(factorialIndex);
        REQUIRE(factorial != nullptr);
        CHECK(factorial->binding == static_cast<coil::u8>(coil::SymbolBinding::Global));
        
        // Should have base_case symbol
        coil::u16 baseCaseIndex = obj.getSymbolIndex("base_case");
        REQUIRE(baseCaseIndex > 0);
    }
    
    SECTION("Hello world example") {
        std::string source = R"(
            ; Hello world program
            .section .text
            .global @main
            
            #main
              ; Load address of hello string
              mov %r1, @hello
              
              ; Call print function
              call @print
              
              ; Exit
              ret
            
            #print
              ; Print function implementation (stub)
              ret
            
            .section .data
            #hello
              .asciiz $"Hello, world!"  ; Null-terminated string
        )";
        
        std::vector<std::string> errors;
        coil::Object obj = assembleString(source, &errors);
        
        CHECK(errors.empty());
        
        // Should have both .text and .data sections
        coil::u16 textIndex = obj.getSectionIndex(".text");
        coil::u16 dataIndex = obj.getSectionIndex(".data");
        REQUIRE(textIndex > 0);
        REQUIRE(dataIndex > 0);
        
        // Should have main and hello symbols
        coil::u16 mainIndex = obj.getSymbolIndex("main");
        coil::u16 helloIndex = obj.getSymbolIndex("hello");
        REQUIRE(mainIndex > 0);
        REQUIRE(helloIndex > 0);
        
        // main should be in .text, hello in .data
        const coil::Symbol* main = obj.getSymbol(mainIndex);
        const coil::Symbol* hello = obj.getSymbol(helloIndex);
        REQUIRE(main != nullptr);
        REQUIRE(hello != nullptr);
        
        CHECK(main->section_index == textIndex);
        CHECK(hello->section_index == dataIndex);
        
        // Data section should contain the string
        auto* dataSection = dynamic_cast<const coil::DataSection*>(obj.getSection(dataIndex));
        REQUIRE(dataSection != nullptr);
        
        const auto& data = dataSection->getData();
        REQUIRE(data.size() >= 14); // "Hello, world!" + null byte
        
        // Check for the string content
        std::string str;
        for (size_t i = hello->value; i < data.size() && data[i] != 0; ++i) {
            str += static_cast<char>(data[i]);
        }
        
        CHECK(str == "Hello, world!");
    }
}

TEST_CASE_METHOD(CoilTestFixture, "Error handling", "[assembler]") {
    SECTION("Undefined symbol") {
        std::string source = R"(
            .section .text
            
            ; Reference to undefined symbol
            jmp @nonexistent
        )";
        
        std::vector<std::string> errors;
        coil::Object obj = assembleString(source, &errors);
        
        // Should report an error for undefined symbol
        CHECK(!errors.empty());
        
        // Error message should mention the undefined symbol
        bool foundError = false;
        for (const auto& error : errors) {
            if (error.find("Undefined symbol") != std::string::npos && 
                error.find("nonexistent") != std::string::npos) {
                foundError = true;
                break;
            }
        }
        
        CHECK(foundError);
    }
    
    SECTION("Invalid directive") {
        std::string source = R"(
            .section .text
            
            ; Invalid directive
            .invalid_directive $id1, $id2
        )";
        
        std::vector<std::string> errors;
        coil::Object obj = assembleString(source, &errors);
        
        // Should report an error for invalid directive
        CHECK(!errors.empty());
        
        // Error message should mention the invalid directive
        bool foundError = false;
        for (const auto& error : errors) {
            if (error.find("Unknown directive") != std::string::npos && 
                error.find("invalid_directive") != std::string::npos) {
                foundError = true;
                break;
            }
        }
        
        CHECK(foundError);
    }
    
    SECTION("Invalid instruction") {
        std::string source = R"(
            .section .text
            
            ; Invalid instruction
            invalid_instruction %r1, %r2
        )";
        
        std::vector<std::string> errors;
        coil::Object obj = assembleString(source, &errors);
        
        // Should report an error for invalid instruction
        CHECK(!errors.empty());
        
        // Error message should mention the invalid instruction
        bool foundError = false;
        for (const auto& error : errors) {
            if (error.find("Unknown instruction") != std::string::npos && 
                error.find("invalid_instruction") != std::string::npos) {
                foundError = true;
                break;
            }
        }
        
        CHECK(foundError);
    }
}

TEST_CASE_METHOD(CoilTestFixture, "Complex examples", "[assembler]") {
    SECTION("Loop with branch instructions") {
        std::string source = R"(
            ; Sum numbers from 1 to 10
            .section .text
            .global @main
            
            #main
              mov %r1, $id0        ; Initialize sum to 0
              mov %r2, $id1        ; Initialize counter to 1
              
            #loop
              add %r1, %r1, %r2    ; Add counter to sum
              inc %r2              ; Increment counter
              cmp %r2, $id11       ; Compare with 11 (> 10)
              br ^lt @loop         ; Loop if less than
              ret                  ; Return with sum in r1
        )";
        
        std::vector<std::string> errors;
        coil::Object obj = assembleString(source, &errors);
        
        CHECK(errors.empty());
        
        // Should have .text section
        coil::u16 textIndex = obj.getSectionIndex(".text");
        REQUIRE(textIndex > 0);
        
        // Should have main and loop symbols
        coil::u16 mainIndex = obj.getSymbolIndex("main");
        coil::u16 loopIndex = obj.getSymbolIndex("loop");
        REQUIRE(mainIndex > 0);
        REQUIRE(loopIndex > 0);
        
        // Check relative positions
        const coil::Symbol* main = obj.getSymbol(mainIndex);
        const coil::Symbol* loop = obj.getSymbol(loopIndex);
        REQUIRE(main != nullptr);
        REQUIRE(loop != nullptr);
        
        CHECK(main->value < loop->value);
    }
    
    SECTION("Multiple data sections") {
        std::string source = R"(
            ; Program with multiple data sections
            .section .text
            .global @main
            
            #main
              ; Load data addresses
              mov %r1, @rodata_val
              mov %r2, @data_val
              
              ; Load values
              load %r3, [%r1]
              load %r4, [%r2]
              
              ; Add them
              add %r5, %r3, %r4
              
              ; Store result
              store [%r2], %r5
              
              ret
              
            .section .rodata
            #rodata_val
              .i32 $id100
              
            .section .data
            #data_val
              .i32 $id200
        )";
        
        std::vector<std::string> errors;
        coil::Object obj = assembleString(source, &errors);
        
        CHECK(errors.empty());
        
        // Should have all three sections
        coil::u16 textIndex = obj.getSectionIndex(".text");
        coil::u16 rodataIndex = obj.getSectionIndex(".rodata");
        coil::u16 dataIndex = obj.getSectionIndex(".data");
        REQUIRE(textIndex > 0);
        REQUIRE(rodataIndex > 0);
        REQUIRE(dataIndex > 0);
        
        // Check section flags
        auto* textSection = obj.getSection(textIndex);
        auto* rodataSection = obj.getSection(rodataIndex);
        auto* dataSection = obj.getSection(dataIndex);
        REQUIRE(textSection != nullptr);
        REQUIRE(rodataSection != nullptr);
        REQUIRE(dataSection != nullptr);
        
        // Text should have code flag
        CHECK((textSection->getHeader().flags & static_cast<uint16_t>(coil::SectionFlag::Code)) != 0);
        
        // Rodata should NOT have write flag
        CHECK((rodataSection->getHeader().flags & static_cast<uint16_t>(coil::SectionFlag::Write)) == 0);
        
        // Data should have write flag
        CHECK((dataSection->getHeader().flags & static_cast<uint16_t>(coil::SectionFlag::Write)) != 0);
        
        // Should have symbols
        coil::u16 rodataValIndex = obj.getSymbolIndex("rodata_val");
        coil::u16 dataValIndex = obj.getSymbolIndex("data_val");
        REQUIRE(rodataValIndex > 0);
        REQUIRE(dataValIndex > 0);
        
        // Symbols should be in their respective sections
        const coil::Symbol* rodataVal = obj.getSymbol(rodataValIndex);
        const coil::Symbol* dataVal = obj.getSymbol(dataValIndex);
        REQUIRE(rodataVal != nullptr);
        REQUIRE(dataVal != nullptr);
        
        CHECK(rodataVal->section_index == rodataIndex);
        CHECK(dataVal->section_index == dataIndex);
        
        // Data sections should have the correct values
        auto* rodataSection2 = dynamic_cast<const coil::DataSection*>(obj.getSection(rodataIndex));
        auto* dataSection2 = dynamic_cast<const coil::DataSection*>(obj.getSection(dataIndex));
        REQUIRE(rodataSection2 != nullptr);
        REQUIRE(dataSection2 != nullptr);
        
        const auto& rodataData = rodataSection2->getData();
        const auto& dataData = dataSection2->getData();
        
        // Check that sections have data
        CHECK(rodataData.size() >= 4); // At least 4 bytes for i32
        CHECK(dataData.size() >= 4);   // At least 4 bytes for i32
    }
}