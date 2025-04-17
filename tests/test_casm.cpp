/**
 * @file test_casm.cpp
 * @brief Test cases for CASM compiler and disassembler
 */

#include <catch2/catch_test_macros.hpp>
#include "casm/casm.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>

// Helper function to read file to string
std::string readFile(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + filename);
  }
  
  return std::string(
    std::istreambuf_iterator<char>(file),
    std::istreambuf_iterator<char>()
  );
}

TEST_CASE("CASM basic assembly", "[casm]") {
  // Initialize COIL
  REQUIRE(coil::initialize() == coil::Result::Success);
  
  // Simple CASM source code
  std::string source = R"(
    .section .text
    .global main
    
    main:
      load r0, 42
      ret
  )";
  
  // Create CASM instance
  casm::CASM casm;
  
  // Assemble to temporary file
  std::string outFile = "test_out.coil";
  REQUIRE(casm.assemble(source, outFile));
  
  // Disassemble back
  std::string disasmFile = "test_out.casm";
  REQUIRE(casm.disassemble(outFile, disasmFile));
  
  // Cleanup
  std::filesystem::remove(outFile);
  std::filesystem::remove(disasmFile);
  
  coil::shutdown();
}

TEST_CASE("CASM roundtrip", "[casm]") {
  // Initialize COIL
  REQUIRE(coil::initialize() == coil::Result::Success);
  
  // Simple factorial program
  std::string source = R"(
    ; Calculate factorial
    .section .text
    .global factorial
    
    factorial:
      cmp r0, 1
      br.lte base_case
      
      push r0
      dec r0
      call factorial
      pop r1
      mul r0, r0, r1
      ret
      
    base_case:
      load r0, 1
      ret
  )";
  
  // Files to use for testing
  std::string asmFile = "factorial.casm";
  std::string binFile = "factorial.coil";
  std::string disasmFile = "factorial_disasm.casm";
  
  // Write source to file
  {
    std::ofstream file(asmFile);
    REQUIRE(file.is_open());
    file << source;
  }
  
  // Create CASM instance
  casm::CASM casm;
  
  // Assemble
  REQUIRE(casm.assemble(source, binFile));
  
  // Disassemble
  REQUIRE(casm.disassemble(binFile, disasmFile));
  
  // Read disassembled code
  std::string disasmCode = readFile(disasmFile);
  
  // Verify disassembly contains key elements
  CHECK(disasmCode.find("factorial") != std::string::npos);
  CHECK(disasmCode.find("base_case") != std::string::npos);
  CHECK(disasmCode.find("cmp") != std::string::npos);
  CHECK(disasmCode.find("mul") != std::string::npos);
  
  // Cleanup
  std::filesystem::remove(asmFile);
  std::filesystem::remove(binFile);
  std::filesystem::remove(disasmFile);
  
  coil::shutdown();
}