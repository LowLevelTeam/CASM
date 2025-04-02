#include "casm/assembler.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <coil/utils/validation.h>

namespace casm {

Assembler::Assembler() {
  // Initialize assembler
}

bool Assembler::assemble(const std::string& source, const std::string& sourceFilename,
                      const std::string& outputFilename, const std::vector<std::string>& includePaths,
                      bool generateDebugInfo) {
  // Clear previous errors
  errorHandler_.clear();
  
  // Create lexer and tokenize source
  Lexer lexer(source, sourceFilename);
  std::vector<Token> tokens = lexer.tokenize();
  
  // Create parser and parse tokens
  Parser parser(tokens, errorHandler_, sourceFilename);
  std::unique_ptr<ProgramNode> ast = parser.parse();
  
  // Check for errors
  if (errorHandler_.hasErrors(ErrorSeverity::ERROR)) {
      errorHandler_.printErrors();
      return false;
  }
  
  // Generate COIL binary
  CodeGenerator codeGen(std::move(ast), errorHandler_, sourceFilename);
  coil::CoilObject coilObj = codeGen.generate();
  
  // Check for errors
  if (errorHandler_.hasErrors(ErrorSeverity::ERROR)) {
      errorHandler_.printErrors();
      return false;
  }
  
  // Validate COIL object
  if (!validateCoilObject(coilObj)) {
      errorHandler_.printErrors();
      return false;
  }
  
  // Write output file
  if (!codeGen.writeToFile(coilObj, outputFilename)) {
      errorHandler_.addError(
          0x01000001, // Generic I/O error code
          "Failed to write output file: " + outputFilename,
          sourceFilename, 0, 0,
          ErrorSeverity::ERROR
      );
      errorHandler_.printErrors();
      return false;
  }
  
  return true;
}

bool Assembler::assembleFile(const std::string& sourceFilename, const std::string& outputFilename,
                          const std::vector<std::string>& includePaths, bool generateDebugInfo) {
  // Read source file
  std::string source = readFile(sourceFilename);
  if (source.empty()) {
      errorHandler_.addError(
          0x01000002, // Generic file read error code
          "Failed to read source file: " + sourceFilename,
          sourceFilename, 0, 0,
          ErrorSeverity::ERROR
      );
      errorHandler_.printErrors();
      return false;
  }
  
  // Assemble source
  return assemble(source, sourceFilename, outputFilename, includePaths, generateDebugInfo);
}

const ErrorHandler& Assembler::getErrorHandler() const {
  return errorHandler_;
}

std::string Assembler::readFile(const std::string& filename) {
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (!file) {
      return "";
  }
  
  // Get file size
  file.seekg(0, std::ios::end);
  size_t size = file.tellg();
  file.seekg(0, std::ios::beg);
  
  // Read file content
  std::string content(size, ' ');
  file.read(&content[0], size);
  
  return content;
}

std::string Assembler::resolveInclude(const std::string& includeFile, 
                                  const std::vector<std::string>& includePaths) {
  // First try directly
  if (std::filesystem::exists(includeFile)) {
      return includeFile;
  }
  
  // Try each include path
  for (const auto& path : includePaths) {
      std::string fullPath = path;
      if (!fullPath.empty() && fullPath.back() != '/' && fullPath.back() != '\\') {
          fullPath += '/';
      }
      fullPath += includeFile;
      
      if (std::filesystem::exists(fullPath)) {
          return fullPath;
      }
  }
  
  return ""; // Not found
}

bool Assembler::validateCoilObject(const coil::CoilObject& coilObj) {
  // Use COIL's validation utilities
  coil::utils::Validation validator;
  coil::ErrorManager& coilErrorManager = errorHandler_.getCoilErrorManager();
  
  bool isValid = coil::utils::Validation::validateCoilObject(coilObj, coilErrorManager);
  
  // Copy COIL errors to our error handler
  if (!isValid) {
      for (const auto& error : coilErrorManager.getErrors()) {
          ErrorSeverity severity = ErrorSeverity::ERROR;
          
          // Convert COIL severity to our severity
          switch (error.severity) {
              case coil::ErrorSeverity::WARNING:
                  severity = ErrorSeverity::WARNING;
                  break;
              case coil::ErrorSeverity::NOTE:
                  severity = ErrorSeverity::NOTE;
                  break;
              default:
                  severity = ErrorSeverity::ERROR;
                  break;
          }
          
          errorHandler_.addError(
              error.error_code,
              error.toString(),
              "", // Filename not provided by COIL validation
              error.line,
              error.column,
              severity
          );
      }
  }
  
  return isValid;
}

} // namespace casm