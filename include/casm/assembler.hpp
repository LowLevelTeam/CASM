#pragma once
#include "casm/parser.hpp"
#include <coil/coil.hpp>
#include <coil/instr.hpp>
#include <coil/obj.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace casm {

/**
 * @brief Assembler that converts CASM statements to COIL binary
 */
class Assembler {
public:
  /**
   * @brief Construct an assembler
   */
  Assembler();
  
  /**
   * @brief Assemble CASM statements into a COIL object file
   * @param statements Statements to assemble
   * @return Assembled COIL object
   */
  coil::Object assemble(const std::vector<Statement>& statements);
  
  /**
   * @brief Assemble CASM source code into a COIL object file
   * @param source Source code to assemble
   * @param filename Source filename (for error reporting)
   * @return Assembled COIL object
   */
  coil::Object assembleSource(const std::string& source, const std::string& filename = "<input>");
  
  /**
   * @brief Get assembler errors
   * @return Vector of error messages
   */
  const std::vector<std::string>& getErrors() const { return m_errors; }
  
  /**
   * @brief Get whether assembler is in verbose mode
   * @return True if verbose mode is enabled
   */
  bool isVerbose() const { return m_verbose; }
  
  /**
   * @brief Set verbose mode
   * @param verbose True to enable verbose mode
   */
  void setVerbose(bool verbose) { m_verbose = verbose; }
  
private:
  struct SectionContext {
    std::vector<uint8_t> data;
    std::unordered_map<std::string, size_t> labels;
    size_t currentOffset = 0;
    
    // Section properties
    std::string name;
    coil::SectionType type = coil::SectionType::ProgBits;
    coil::SectionFlag flags = coil::SectionFlag::None;
    
    void addData(const std::vector<uint8_t>& newData) {
      data.insert(data.end(), newData.begin(), newData.end());
      currentOffset += newData.size();
    }
    
    void addLabel(const std::string& label) {
      labels[label] = currentOffset;
    }
  };
  
  struct LabelReference {
    std::string label;        // Label name
    std::string section;      // Section name
    size_t offset;            // Offset in section data
    size_t size;              // Size of reference (in bytes)
    bool isRelative;          // Whether reference is relative to current position
  };
  
  std::unordered_map<std::string, SectionContext> m_sections;
  std::string m_currentSection;
  std::unordered_map<std::string, std::string> m_globalSymbols;  // label -> section
  std::vector<LabelReference> m_labelReferences;
  std::vector<std::string> m_errors;
  bool m_verbose = false;
  
  // First pass - collect labels and section sizes
  void collectLabels(const std::vector<Statement>& statements);
  
  // Second pass - generate code
  void generateCode(const std::vector<Statement>& statements);
  
  // Helper methods
  void ensureCurrentSection();
  void switchSection(const std::string& name);
  void addLabel(const std::string& label);
  void addGlobalSymbol(const std::string& label);
  void addImmediate(const ImmediateValue& value, coil::ValueType type);
  void addByteImmediate(const ImmediateValue& value);
  void addShortImmediate(const ImmediateValue& value);
  void addWordImmediate(const ImmediateValue& value);
  void addLongImmediate(const ImmediateValue& value);
  void addFloatImmediate(const ImmediateValue& value);
  void addDoubleImmediate(const ImmediateValue& value);
  void addLabelReference(const std::string& label, size_t size, bool isRelative = false);
  void addString(const std::string& str, bool nullTerminated = false);
  
  // Convert statement to COIL instruction
  void processInstruction(const Instruction& instruction, const std::string& label);
  
  // Process directive
  void processDirective(const Directive& directive, const std::string& label);
  
  // Resolve label references
  void resolveReferences();
  
  // Convert operand to COIL operand
  coil::Operand convertOperand(const Operand& operand, coil::ValueType defaultType = coil::ValueType::I32);
  
  // Get register index from name
  uint32_t getRegisterIndex(const std::string& name);
  
  // Convert immediate value to COIL value
  coil::ImmediateValue convertImmediate(const ImmediateValue& value, coil::ValueType type);
  
  // Convert value type string to COIL value type
  coil::ValueType stringToValueType(const std::string& typeStr);
  
  // Log an error
  void error(const std::string& message);
  
  // Log a message if verbose mode is enabled
  void log(const std::string& message);
};

} // namespace casm