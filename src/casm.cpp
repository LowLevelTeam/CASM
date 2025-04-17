/**
 * @file casm.cpp
 * @brief Main application for CASM compiler
 */

#include "casm/casm.hpp"
#include <coil/coil.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// Print usage information
void printUsage() {
  std::cout << "CASM - COIL Assembly Compiler and Disassembler\n\n"
            << "Usage:\n"
            << "  casm assemble <input.casm> [-o <output.coil>]\n"
            << "  casm disassemble <input.coil> [-o <output.casm>]\n"
            << "  casm --help\n\n"
            << "Options:\n"
            << "  -o, --output <file>  Specify output file (default: output.coil/output.casm)\n"
            << "  -h, --help           Show this help message\n"
            << "  -v, --version        Show version information\n";
}

// Print version information
void printVersion() {
  std::cout << "CASM version 0.1.0\n"
            << "COIL Assembly Compiler and Disassembler\n";
}

// Read entire file to string
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

int main(int argc, char* argv[]) {
  // Parse command line arguments
  std::vector<std::string> args;
  for (int i = 1; i < argc; i++) {
    args.push_back(argv[i]);
  }
  
  // Handle help and version flags
  if (args.empty() || args[0] == "-h" || args[0] == "--help") {
    printUsage();
    return 0;
  }
  
  if (args[0] == "-v" || args[0] == "--version") {
    printVersion();
    return 0;
  }
  
  // Initialize COIL
  if (coil::initialize() != coil::Result::Success) {
    std::cerr << "Failed to initialize COIL library\n";
    return 1;
  }
  
  // Create CASM instance
  casm::CASM casm;
  
  try {
    // Process commands
    if (args[0] == "assemble") {
      if (args.size() < 2) {
        std::cerr << "Error: No input file specified for assembly\n";
        printUsage();
        return 1;
      }
      
      std::string inputFile = args[1];
      std::string outputFile = "output.coil";
      
      // Check for output file option
      for (size_t i = 2; i < args.size(); i++) {
        if ((args[i] == "-o" || args[i] == "--output") && i + 1 < args.size()) {
          outputFile = args[i + 1];
          i++;
        }
      }
      
      // Read input file
      std::string source;
      try {
        source = readFile(inputFile);
      } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
      }
      
      // Assemble source
      if (!casm.assemble(source, outputFile)) {
        for (const auto& error : casm.getErrors()) {
          std::cerr << "Error: " << error << "\n";
        }
        return 1;
      }
      
      std::cout << "Successfully assembled " << inputFile << " to " << outputFile << "\n";
    }
    else if (args[0] == "disassemble") {
      if (args.size() < 2) {
        std::cerr << "Error: No input file specified for disassembly\n";
        printUsage();
        return 1;
      }
      
      std::string inputFile = args[1];
      std::string outputFile = "output.casm";
      
      // Check for output file option
      for (size_t i = 2; i < args.size(); i++) {
        if ((args[i] == "-o" || args[i] == "--output") && i + 1 < args.size()) {
          outputFile = args[i + 1];
          i++;
        }
      }
      
      // Disassemble binary
      if (!casm.disassemble(inputFile, outputFile)) {
        for (const auto& error : casm.getErrors()) {
          std::cerr << "Error: " << error << "\n";
        }
        return 1;
      }
      
      std::cout << "Successfully disassembled " << inputFile << " to " << outputFile << "\n";
    }
    else {
      std::cerr << "Error: Unknown command '" << args[0] << "'\n";
      printUsage();
      return 1;
    }
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
  
  // Clean up COIL
  coil::shutdown();
  
  return 0;
}