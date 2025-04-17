#include "casm/lexer.hpp"
#include "casm/parser.hpp"
#include "casm/assembler.hpp"
#include <coil/coil.hpp>
#include <coil/stream.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstring>

// Version information
constexpr const char* VERSION = "0.1.0";

// Print help message
void printHelp(const char* programName) {
  std::cout << "CASM Assembler v" << VERSION << std::endl;
  std::cout << "Usage: " << programName << " [options] input_file output_file" << std::endl;
  std::cout << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  -h, --help     Show this help message" << std::endl;
  std::cout << "  -v, --verbose  Enable verbose output" << std::endl;
  std::cout << std::endl;
  std::cout << "Examples:" << std::endl;
  std::cout << "  " << programName << " example.casm example.coil" << std::endl;
  std::cout << "  " << programName << " -v factorial.casm factorial.coil" << std::endl;
}

// Read the entire file into a string
std::string readFile(const std::string& filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Could not open file: " + filename);
  }
  
  // Get file size
  file.seekg(0, std::ios::end);
  size_t size = file.tellg();
  file.seekg(0, std::ios::beg);
  
  // Read the entire file
  std::string content(size, '\0');
  file.read(&content[0], size);
  
  return content;
}

int main(int argc, char* argv[]) {
  // Parse command-line arguments
  std::string inputFile;
  std::string outputFile;
  bool verbose = false;
  
  // Process arguments
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      printHelp(argv[0]);
      return 0;
    } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
      verbose = true;
    } else if (inputFile.empty()) {
      inputFile = argv[i];
    } else if (outputFile.empty()) {
      outputFile = argv[i];
    } else {
      std::cerr << "Error: Unexpected argument: " << argv[i] << std::endl;
      printHelp(argv[0]);
      return 1;
    }
  }
  
  // Validate arguments
  if (inputFile.empty() || outputFile.empty()) {
    std::cerr << "Error: Input and output files are required" << std::endl;
    printHelp(argv[0]);
    return 1;
  }
  
  try {
    // Initialize COIL library
    coil::initialize();
    
    if (verbose) {
      std::cout << "Reading input file: " << inputFile << std::endl;
    }
    
    // Read the input file
    std::string source = readFile(inputFile);
    
    if (verbose) {
      std::cout << "Parsing source file..." << std::endl;
    }
    
    // Create assembler
    casm::Assembler assembler;
    assembler.setVerbose(verbose);
    
    // Assemble the source
    coil::Object obj = assembler.assembleSource(source, inputFile);
    
    if (verbose) {
      std::cout << "Writing output file: " << outputFile << std::endl;
    }
    
    // Save the object to the output file
    coil::FileStream outStream(outputFile, coil::StreamMode::Write);
    obj.save(outStream);
    
    if (verbose) {
      std::cout << "Assembly completed successfully." << std::endl;
    }
    
    // Shutdown COIL library
    coil::shutdown();
    
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}