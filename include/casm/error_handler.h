#ifndef CASM_ERROR_HANDLER_H
#define CASM_ERROR_HANDLER_H

#include <string>
#include <vector>
#include "token.h"
#include <coil/error_codes.h>

namespace casm {

/**
* Error severity enum
*/
enum class ErrorSeverity {
  ERROR,
  WARNING,
  NOTE
};

/**
* Error info structure
*/
struct ErrorInfo {
  uint32_t error_code;
  std::string message;
  std::string filename;
  unsigned int line;
  unsigned int column;
  ErrorSeverity severity;
  
  std::string toString() const;
};

/**
* Error handler class for managing and reporting errors
*/
class ErrorHandler {
public:
  /**
    * Constructor
    */
  ErrorHandler();
  
  /**
    * Add an error
    * @param errorCode Error code
    * @param message Error message
    * @param filename Source filename
    * @param line Line number
    * @param column Column number
    * @param severity Error severity
    */
  void addError(uint32_t errorCode, const std::string& message,
                const std::string& filename, unsigned int line, unsigned int column,
                ErrorSeverity severity = ErrorSeverity::ERROR);
  
  /**
    * Add an error at token location
    * @param errorCode Error code
    * @param message Error message
    * @param token Token where error occurred
    * @param filename Source filename
    * @param severity Error severity
    */
  void addError(uint32_t errorCode, const std::string& message,
                const Token& token, const std::string& filename,
                ErrorSeverity severity = ErrorSeverity::ERROR);
  
  /**
    * Check if there are any errors
    * @return True if there are errors, false otherwise
    */
  bool hasErrors() const;
  
  /**
    * Check if there are errors of a specific severity
    * @param severity Error severity
    * @return True if there are errors of the specified severity, false otherwise
    */
  bool hasErrors(ErrorSeverity severity) const;
  
  /**
    * Get all errors
    * @return Vector of all errors
    */
  const std::vector<ErrorInfo>& getErrors() const;
  
  /**
    * Print all errors to stderr
    */
  void printErrors() const;
  
  /**
    * Clear all errors
    */
  void clear();
  
  /**
    * Get the coil::ErrorManager
    * @return Reference to the coil::ErrorManager
    */
  coil::ErrorManager& getCoilErrorManager();
  
private:
  std::vector<ErrorInfo> errors_;
  coil::ErrorManager coilErrorManager_;
};

} // namespace casm

#endif // CASM_ERROR_HANDLER_H