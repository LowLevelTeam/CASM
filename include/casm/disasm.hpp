/**
* @file disasm.hpp
* @brief Disassembler for COIL binary
*/

#pragma once
#include <coil/instr.hpp>
#include <coil/obj.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace casm {

/**
* @brief Disassembler for COIL binary
* 
* Converts COIL binary back to CASM assembly language.
*/
class Disassembler {
public:
  /**
  * @brief Initialize disassembler
  */
  Disassembler();
  
  /**
  * @brief Disassemble COIL object to CASM code
  * @param obj COIL object to disassemble
  * @return Disassembled CASM code
  */
  std::string disassemble(const coil::Object& obj);
  
  /**
  * @brief Get disassembly errors
  * @return Vector of error messages
  */
  const std::vector<std::string>& getErrors() const;
  
private:
  std::vector<std::string> errors;                         ///< Disassembly errors
  std::unordered_map<coil::Opcode, std::string> opcodeMap; ///< Map of opcodes to strings
  std::unordered_map<coil::InstrFlag0, std::string> flagMap; ///< Map of flags to strings
  std::unordered_map<coil::ValueType, std::string> typeMap;  ///< Map of value types to strings
  std::unordered_map<uint32_t, std::string> symbolMap;      ///< Map of addresses to symbol names
  
  /**
  * @brief Initialize opcode, flag, and type maps
  */
  void initMaps();
  
  /**
  * @brief Disassemble a section
  * @param section Section to disassemble
  * @param obj COIL object
  * @return Disassembled section code
  */
  std::string disassembleSection(const coil::BaseSection* section, const coil::Object& obj);
  
  /**
  * @brief Disassemble a code section
  * @param section Section to disassemble
  * @param obj COIL object
  * @return Disassembled code
  */
  std::string disassembleCodeSection(const coil::DataSection* section, const coil::Object& obj);
  
  /**
  * @brief Disassemble a data section
  * @param section Section to disassemble
  * @param obj COIL object
  * @return Disassembled data
  */
  std::string disassembleDataSection(const coil::DataSection* section, const coil::Object& obj);
  
  /**
  * @brief Disassemble an instruction
  * @param instr Instruction to disassemble
  * @return Disassembled instruction code
  */
  std::string disassembleInstruction(const coil::Instruction& instr);
  
  /**
  * @brief Disassemble an operand
  * @param op Operand to disassemble
  * @return Disassembled operand code
  */
  std::string disassembleOperand(const coil::Operand& op);
  
  /**
  * @brief Build a symbol map from object file
  * @param obj COIL object
  */
  void buildSymbolMap(const coil::Object& obj);
  
  /**
  * @brief Report an error
  * @param message Error message
  */
  void error(const std::string& message);
};

} // namespace casm