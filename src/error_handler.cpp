#include "casm/error_handler.h"
#include <iostream>
#include <sstream>
#include <iomanip>

namespace casm {

std::string ErrorInfo::toString() const {
  std::stringstream ss;
  
  // Error severity
  switch (severity) {
      case ErrorSeverity::ERROR:
          ss << "error";
          break;
      case ErrorSeverity::WARNING:
          ss << "warning";
          break;
      case ErrorSeverity::NOTE:
          ss << "note";
          break;
  }
  
  // Location information
  if (!filename.empty()) {
      ss << ":" << filename;
      
      if (line > 0) {
          ss << ":" << line;
          
          if (column > 0) {
              ss << ":" << column;
          }
      }
  }
  
  // Error code and message
  ss << ": [0x" << std::hex << std::setw(8) << std::setfill('0') << error_code << "] " 
      << message;
  
  return ss.str();
}

ErrorHandler::ErrorHandler() {
  // Initialize the error manager
}

void ErrorHandler::addError(uint32_t errorCode, const std::string& message,
                          const std::string& filename, unsigned int line, unsigned int column,
                          ErrorSeverity severity) {
  ErrorInfo error;
  error.error_code = errorCode;
  error.message = message;
  error.filename = filename;
  error.line = line;
  error.column = column;
  error.severity = severity;
  
  errors_.push_back(error);
  
  // Convert to COIL error severity
  coil::ErrorSeverity coilSeverity = coil::ErrorSeverity::ERROR;
  switch (severity) {
      case ErrorSeverity::WARNING:
          coilSeverity = coil::ErrorSeverity::WARNING;
          break;
      case ErrorSeverity::NOTE:
          coilSeverity = coil::ErrorSeverity::NOTE;
          break;
      default:
          coilSeverity = coil::ErrorSeverity::ERROR;
          break;
  }
  
  // Add to COIL error manager
  coilErrorManager_.addError(
      errorCode,
      message,
      coilSeverity,
      0, // location (not used directly)
      0, // file_id (not used directly)
      line,
      column
  );
}

void ErrorHandler::addError(uint32_t errorCode, const std::string& message,
                          const Token& token, const std::string& filename,
                          ErrorSeverity severity) {
  addError(errorCode, message, filename, token.line, token.column, severity);
}

bool ErrorHandler::hasErrors() const {
  return !errors_.empty();
}

bool ErrorHandler::hasErrors(ErrorSeverity severity) const {
  for (const auto& error : errors_) {
      if (error.severity == severity) {
          return true;
      }
  }
  
  return false;
}

const std::vector<ErrorInfo>& ErrorHandler::getErrors() const {
  return errors_;
}

void ErrorHandler::printErrors() const {
  for (const auto& error : errors_) {
      std::cerr << error.toString() << std::endl;
  }
}

void ErrorHandler::clear() {
  errors_.clear();
  coilErrorManager_.clear();
}

coil::ErrorManager& ErrorHandler::getCoilErrorManager() {
  return coilErrorManager_;
}

} // namespace casm