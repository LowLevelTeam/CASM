/**
 * @file assembler.cpp
 * @brief Implementation of CASM assembler
 */

#include "casm/assembler.hpp"
#include "coil/stream.hpp"
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <fstream>
#include <sstream>

namespace casm {

// Default error callback function
static void defaultErrorCallback(const char* message, size_t line, const char* source, void* user_data) {
  (void)user_data; // Unused
  if (line > 0) {
    fprintf(stderr, "%s:%zu: Error: %s\n", source, line, message);
  } else {
    fprintf(stderr, "%s: Error: %s\n", source, message);
  }
}

// Static callback instances for parser and codegen
static Assembler* g_current_assembler = nullptr;

void Assembler::parserErrorCallback(const char* message, size_t line, size_t column, void* user_data) {
  (void)user_data; // Unused
  if (g_current_assembler) {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%s (column %zu)", message, column);
    g_current_assembler->reportError(buffer, line);
  }
}

void Assembler::codegenErrorCallback(const char* message, size_t line, void* user_data) {
  (void)user_data; // Unused
  if (g_current_assembler) {
    g_current_assembler->reportError(message, line);
  }
}

Assembler::Assembler(const AssemblerConfig& config)
  : config(config),
    error_callback(defaultErrorCallback),
    error_user_data(nullptr) {
  
  // Set up error callbacks for parser and code generator
  parser.setErrorCallback(parserErrorCallback, nullptr);
  codegen.setErrorCallback(codegenErrorCallback, nullptr);
}

Assembler::~Assembler() {
  // Clean up if this was the current assembler
  if (g_current_assembler == this) {
    g_current_assembler = nullptr;
  }
}

Result Assembler::assemble(const std::string& source, const std::string& source_name, coil::Object& obj) {
  // Set current assembler for error callbacks
  g_current_assembler = this;
  current_source = source_name;
  
  // Initialize code generator
  Result result = codegen.initialize();
  if (result != Result::Success) {
    return result;
  }
  
  // Split source into lines
  std::istringstream stream(source);
  std::string line;
  size_t line_number = 0;
  
  // Parse each line
  while (std::getline(stream, line)) {
    line_number++;
    
    Line parsed_line;
    result = parser.parseLine(line, line_number, parsed_line);
    if (result != Result::Success) {
      return result;
    }
    
    // Generate code for the line
    if (parsed_line.type != LineType::Empty) {
      result = codegen.generateLine(parsed_line);
      if (result != Result::Success) {
        return result;
      }
    }
  }
  
  // Finalize code generation
  result = codegen.finalize(obj);
  if (result != Result::Success) {
    return result;
  }
  
  return Result::Success;
}

Result Assembler::assembleFile(const std::string& filename, coil::Object& obj) {
  // Read the file into memory
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (!file) {
    formatError("Failed to open file: %s", filename.c_str());
    reportError(error_buffer, 0);
    return Result::IoError;
  }
  
  // Read file contents into string
  std::ostringstream content;
  content << file.rdbuf();
  
  // Assemble the content
  return assemble(content.str(), filename, obj);
}

Result Assembler::saveObjectFile(const coil::Object& obj, const std::string& filename) {
  // Create file stream
  coil::FileStream stream(filename.c_str(), coil::StreamMode::Write);
  if (!stream.isOpen()) {
    formatError("Failed to create output file: %s", filename.c_str());
    reportError(error_buffer, 0);
    return Result::IoError;
  }
  
  // Save object to stream
  Result result = obj.save(stream);
  if (result != Result::Success) {
    formatError("Failed to save object file: %s", filename.c_str());
    reportError(error_buffer, 0);
    return result;
  }
  
  return Result::Success;
}

const std::string& Assembler::getLastError() const {
  return last_error;
}

void Assembler::setErrorCallback(void (*callback)(const char* message, size_t line, const char* source, void* user_data), void* user_data) {
  error_callback = callback ? callback : defaultErrorCallback;
  error_user_data = user_data;
}

void Assembler::reportError(const char* message, size_t line) {
  last_error = message;
  error_callback(message, line, current_source.c_str(), error_user_data);
}

void Assembler::formatError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vsnprintf(error_buffer, sizeof(error_buffer), format, args);
  va_end(args);
}

} // namespace casm