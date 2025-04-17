/**
 * @file main.cpp
 * @brief CASM assembler command-line interface
 */

#include "casm/assembler.hpp"
#include "coil/coil.hpp"
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>

// Print usage information
void printUsage(const char* program_name) {
  printf("CASM - COIL Assembly Language Assembler\n");
  printf("Usage: %s [options] input_file\n", program_name);
  printf("Options:\n");
  printf("  -o, --output FILE    Specify output file (default is input file with .coil extension)\n");
  printf("  -v, --verbose        Enable verbose output\n");
  printf("  -d, --debug          Enable debug output\n");
  printf("  -h, --help           Show this help message\n");
}

int main(int argc, char** argv) {
  // Initialize COIL library
  coil::Result result = coil::initialize();
  if (result != coil::Result::Success) {
    fprintf(stderr, "Failed to initialize COIL library\n");
    return 1;
  }
  
  // Parse command-line arguments
  std::string input_file;
  std::string output_file;
  bool verbose = false;
  bool debug = false;
  
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      printUsage(argv[0]);
      return 0;
    } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
      verbose = true;
    } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
      debug = true;
    } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
      if (i + 1 < argc) {
        output_file = argv[++i];
      } else {
        fprintf(stderr, "Error: Missing output file after %s\n", argv[i]);
        return 1;
      }
    } else if (argv[i][0] == '-') {
      fprintf(stderr, "Error: Unknown option: %s\n", argv[i]);
      printUsage(argv[0]);
      return 1;
    } else {
      // Assume it's the input file
      if (input_file.empty()) {
        input_file = argv[i];
      } else {
        fprintf(stderr, "Error: Multiple input files specified\n");
        printUsage(argv[0]);
        return 1;
      }
    }
  }
  
  // Check if input file was specified
  if (input_file.empty()) {
    fprintf(stderr, "Error: No input file specified\n");
    printUsage(argv[0]);
    return 1;
  }
  
  // If output file not specified, derive from input file
  if (output_file.empty()) {
    // Replace .casm extension with .coil, or append .coil if no extension
    size_t dot_pos = input_file.find_last_of('.');
    if (dot_pos != std::string::npos) {
      output_file = input_file.substr(0, dot_pos) + ".coil";
    } else {
      output_file = input_file + ".coil";
    }
  }
  
  // Configure assembler
  casm::AssemblerConfig config;
  config.verbose = verbose;
  config.debug = debug;
  
  // Create assembler
  casm::Assembler assembler(config);
  
  // Assemble the file
  coil::Object obj;
  result = assembler.assembleFile(input_file, obj);
  if (result != coil::Result::Success) {
    fprintf(stderr, "Assembly failed: %s\n", assembler.getLastError().c_str());
    return 1;
  }
  
  // Save the object file
  result = assembler.saveObjectFile(obj, output_file);
  if (result != coil::Result::Success) {
    fprintf(stderr, "Failed to save object file: %s\n", assembler.getLastError().c_str());
    return 1;
  }
  
  if (verbose) {
    printf("Successfully assembled %s to %s\n", input_file.c_str(), output_file.c_str());
  }
  
  // Clean up COIL library
  coil::shutdown();
  
  return 0;
}