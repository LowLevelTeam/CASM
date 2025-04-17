/**
* @file casm.hpp
* @brief Main interface for CASM compiler
*/

#pragma once
#include "casm/lexer.hpp"
#include "casm/parser.hpp"
#include "casm/codegen.hpp"
#include "casm/disasm.hpp"
#include <coil/coil.hpp>
#include <string>
#include <vector>
#include <memory>

namespace casm {

/**
* @brief Main CASM interface
*/
class CASM {
public:
  /**
  * @brief Initialize CASM
  */
  CASM();
  
  /**
  * @brief Clean up resources
  */
  ~CASM();
  
  /**
  * @brief Assemble CASM code to COIL binary
  * @param source CASM source code
  * @param outputFile Output file path
  * @return True if successful
  */
  bool assemble(const std::string& source, const std::string& outputFile);
  
  /**
  * @brief Disassemble COIL binary to CASM code
  * @param inputFile Input file path
  * @param outputFile Output file path
  * @return True if successful
  */
  bool disassemble(const std::string& inputFile, const std::string& outputFile);
  
  /**
  * @brief Get errors from last operation
  * @return Vector of error messages
  */
  const std::vector<std::string>& getErrors() const;
  
private:
  std::vector<std::string> errors;  ///< Error messages
  
  /**
  * @brief Report an error
  * @param message Error message
  */
  void error(const std::string& message);
  
  /**
  * @brief Save COIL object to file
  * @param obj COIL object
  * @param filename Output file path
  * @return True if successful
  */
  bool saveObjectToFile(const coil::Object& obj, const std::string& filename);
  
  /**
  * @brief Load COIL object from file
  * @param filename Input file path
  * @return COIL object or nullptr on error
  */
  std::unique_ptr<coil::Object> loadObjectFromFile(const std::string& filename);
  
  /**
  * @brief Save CASM code to file
  * @param code CASM code
  * @param filename Output file path
  * @return True if successful
  */
  bool saveCodeToFile(const std::string& code, const std::string& filename);
  
  /**
  * @brief Load CASM code from file
  * @param filename Input file path
  * @return CASM code or empty string on error
  */
  std::string loadCodeFromFile(const std::string& filename);
};

} // namespace casm