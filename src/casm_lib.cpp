/**
 * @file casm_lib.cpp
 * @brief Implementation of main CASM interface
 */

#include "casm/casm.hpp"
#include "casm/lexer.hpp"
#include "casm/parser.hpp"
#include "casm/codegen.hpp"
#include "casm/disasm.hpp"
#include <coil/stream.hpp>
#include <coil/err.hpp>
#include <fstream>
#include <sstream>

namespace casm {

// Constructor
CASM::CASM() {
  // Nothing to initialize
}

// Destructor
CASM::~CASM() {
  // Nothing to clean up
}

// Report an error
void CASM::error(const std::string& message) {
  errors.push_back(message);
}

// Get errors
const std::vector<std::string>& CASM::getErrors() const {
  return errors;
}

// Assemble CASM code to COIL binary
bool CASM::assemble(const std::string& source, const std::string& outputFile) {
  // Clear any previous errors
  errors.clear();
  
  try {
    // Tokenize the source code
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();
    
    // Parse the tokens into statements
    Parser parser(tokens);
    auto statements = parser.parse();
    
    // Check for parsing errors
    if (!parser.getErrors().empty()) {
      for (const auto& error : parser.getErrors()) {
        this->error("Parse error: " + error);
      }
      return false;
    }
    
    // Generate COIL code
    CodeGenerator codegen;
    auto obj = codegen.generate(statements);
    
    // Check for code generation errors
    if (!codegen.getErrors().empty()) {
      for (const auto& error : codegen.getErrors()) {
        this->error("Code generation error: " + error);
      }
      return false;
    }
    
    // Save the object to file
    if (!saveObjectToFile(*obj, outputFile)) {
      return false;
    }
    
    return true;
  }
  catch (const std::exception& e) {
    this->error(std::string("Exception during assembly: ") + e.what());
    return false;
  }
}

// Disassemble COIL binary to CASM code
bool CASM::disassemble(const std::string& inputFile, const std::string& outputFile) {
  // Clear any previous errors
  errors.clear();
  
  try {
    // Load the object from file
    auto obj = loadObjectFromFile(inputFile);
    if (!obj) {
      return false;
    }
    
    // Disassemble the object
    Disassembler disasm;
    std::string code = disasm.disassemble(*obj);
    
    // Check for disassembly errors
    if (!disasm.getErrors().empty()) {
      for (const auto& error : disasm.getErrors()) {
        this->error("Disassembly error: " + error);
      }
      return false;
    }
    
    // Save the code to file
    if (!saveCodeToFile(code, outputFile)) {
      return false;
    }
    
    return true;
  }
  catch (const std::exception& e) {
    this->error(std::string("Exception during disassembly: ") + e.what());
    return false;
  }
}

// Save COIL object to file
bool CASM::saveObjectToFile(const coil::Object& obj, const std::string& filename) {
  try {
    // Create a file stream
    coil::FileStream stream(filename.c_str(), coil::StreamMode::Write);
    
    // Save the object
    coil::Result result = obj.save(stream);
    if (result != coil::Result::Success) {
      this->error("Failed to save object to file: " + std::string(coil::resultToString(result)));
      return false;
    }
    
    return true;
  }
  catch (const std::exception& e) {
    this->error(std::string("Exception saving object to file: ") + e.what());
    return false;
  }
}

// Load COIL object from file
std::unique_ptr<coil::Object> CASM::loadObjectFromFile(const std::string& filename) {
  try {
    // Create a file stream
    coil::FileStream stream(filename.c_str(), coil::StreamMode::Read);
    
    // Check if file is open
    if (!stream.isOpen()) {
      this->error("Failed to open file: " + filename);
      return nullptr;
    }
    
    // Create a new object
    auto obj = std::make_unique<coil::Object>();
    
    // Load the object
    coil::Result result = obj->load(stream);
    if (result != coil::Result::Success) {
      this->error("Failed to load object from file: " + std::string(coil::resultToString(result)));
      return nullptr;
    }
    
    return obj;
  }
  catch (const std::exception& e) {
    this->error(std::string("Exception loading object from file: ") + e.what());
    return nullptr;
  }
}

// Save CASM code to file
bool CASM::saveCodeToFile(const std::string& code, const std::string& filename) {
  try {
    // Open file for writing
    std::ofstream file(filename);
    if (!file.is_open()) {
      this->error("Failed to open file for writing: " + filename);
      return false;
    }
    
    // Write code to file
    file << code;
    
    // Check for errors
    if (file.fail()) {
      this->error("Failed to write to file: " + filename);
      return false;
    }
    
    file.close();
    return true;
  }
  catch (const std::exception& e) {
    this->error(std::string("Exception saving code to file: ") + e.what());
    return false;
  }
}

// Load CASM code from file
std::string CASM::loadCodeFromFile(const std::string& filename) {
  try {
    // Open file for reading
    std::ifstream file(filename);
    if (!file.is_open()) {
      this->error("Failed to open file for reading: " + filename);
      return "";
    }
    
    // Read entire file into string
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    // Check for errors
    if (file.fail()) {
      this->error("Failed to read from file: " + filename);
      return "";
    }
    
    file.close();
    return buffer.str();
  }
  catch (const std::exception& e) {
    this->error(std::string("Exception loading code from file: ") + e.what());
    return "";
  }
}

} // namespace casm