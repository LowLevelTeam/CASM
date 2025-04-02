#include "casm/assembler.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstring>

void printUsage() {
  std::cout << "Usage: casm [options] input.casm -o output.coil\n";
  std::cout << "Options:\n";
  std::cout << "  -I <path>       : Include directory for header files\n";
  std::cout << "  -o <file>       : Specify output file\n";
  std::cout << "  -v              : Enable verbose output\n";
  std::cout << "  -h              : Display help message\n";
  std::cout << "  -d              : Enable debug information\n";
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
      printUsage();
      return 1;
  }
  
  // Default values
  std::string inputFile;
  std::string outputFile;
  std::vector<std::string> includePaths;
  bool verbose = false;
  bool generateDebugInfo = false;
  
  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-h") == 0) {
          printUsage();
          return 0;
      } else if (strcmp(argv[i], "-v") == 0) {
          verbose = true;
      } else if (strcmp(argv[i], "-d") == 0) {
          generateDebugInfo = true;
      } else if (strcmp(argv[i], "-I") == 0) {
          if (i + 1 < argc) {
              includePaths.push_back(argv[i + 1]);
              i++;
          } else {
              std::cerr << "Error: Missing path after -I\n";
              return 1;
          }
      } else if (strcmp(argv[i], "-o") == 0) {
          if (i + 1 < argc) {
              outputFile = argv[i + 1];
              i++;
          } else {
              std::cerr << "Error: Missing file after -o\n";
              return 1;
          }
      } else {
          // Assume it's the input file
          if (inputFile.empty()) {
              inputFile = argv[i];
          } else {
              std::cerr << "Error: Multiple input files specified\n";
              return 1;
          }
      }
  }
  
  // Check for required arguments
  if (inputFile.empty()) {
      std::cerr << "Error: No input file specified\n";
      return 1;
  }
  
  if (outputFile.empty()) {
      std::cerr << "Error: No output file specified (-o option)\n";
      return 1;
  }
  
  if (verbose) {
      std::cout << "Input file: " << inputFile << "\n";
      std::cout << "Output file: " << outputFile << "\n";
      
      if (!includePaths.empty()) {
          std::cout << "Include paths:\n";
          for (const auto& path : includePaths) {
              std::cout << "  " << path << "\n";
          }
      }
      
      if (generateDebugInfo) {
          std::cout << "Debug information enabled\n";
      }
  }
  
  // Create assembler and process the input file
  casm::Assembler assembler;
  bool success = assembler.assembleFile(inputFile, outputFile, includePaths, generateDebugInfo);
  
  if (!success) {
      if (verbose) {
          std::cout << "Assembly failed\n";
      }
      return 1;
  }
  
  if (verbose) {
      std::cout << "Assembly successful\n";
  }
  
  return 0;
}