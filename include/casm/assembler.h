#ifndef CASM_ASSEMBLER_H
#define CASM_ASSEMBLER_H

#include "lexer.h"
#include "parser.h"
#include "code_generator.h"
#include "error_handler.h"
#include <coil/binary_format.h>
#include <string>
#include <vector>
#include <memory>

namespace casm {

/**
* Assembler class for assembling CASM source code into COIL binary format
*/
class Assembler {
public:
  /**
    * Constructor
    */
  Assembler();
  
  /**
    * Assemble CASM source code into COIL binary format
    * @param source Source code
    * @param sourceFilename Source filename
    * @param outputFilename Output filename
    * @param includePaths Include paths for finding included files
    * @param generateDebugInfo Whether to generate debug information
    * @return True if successful, false otherwise
    */
  bool assemble(const std::string& source, const std::string& sourceFilename,
                const std::string& outputFilename, const std::vector<std::string>& includePaths = {},
                bool generateDebugInfo = false);
  
  /**
    * Assemble CASM source file into COIL binary format
    * @param sourceFilename Source filename
    * @param outputFilename Output filename
    * @param includePaths Include paths for finding included files
    * @param generateDebugInfo Whether to generate debug information
    * @return True if successful, false otherwise
    */
  bool assembleFile(const std::string& sourceFilename, const std::string& outputFilename,
                    const std::vector<std::string>& includePaths = {},
                    bool generateDebugInfo = false);
  
  /**
    * Get the error handler
    * @return Reference to the error handler
    */
  const ErrorHandler& getErrorHandler() const;
  
private:
  ErrorHandler errorHandler_;
  
  std::string readFile(const std::string& filename);
  std::string resolveInclude(const std::string& includeFile, const std::vector<std::string>& includePaths);
  
  bool validateCoilObject(const coil::CoilObject& coilObj);
};

} // namespace casm

#endif // CASM_ASSEMBLER_H