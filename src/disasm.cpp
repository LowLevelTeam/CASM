/**
 * @file disasm.cpp
 * @brief Implementation of disassembler for COIL binary
 */

#include "casm/disasm.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace casm {

// Constructor
Disassembler::Disassembler() {
  initMaps();
}

// Initialize opcode, flag, and type maps
void Disassembler::initMaps() {
  // Initialize opcode map (inverse of code generator)
  opcodeMap[coil::Opcode::Nop] = "nop";
  opcodeMap[coil::Opcode::Br] = "br";
  opcodeMap[coil::Opcode::Jump] = "jump";
  opcodeMap[coil::Opcode::Call] = "call";
  opcodeMap[coil::Opcode::Ret] = "ret";
  opcodeMap[coil::Opcode::Load] = "load";
  opcodeMap[coil::Opcode::Store] = "store";
  opcodeMap[coil::Opcode::Push] = "push";
  opcodeMap[coil::Opcode::Pop] = "pop";
  opcodeMap[coil::Opcode::Add] = "add";
  opcodeMap[coil::Opcode::Sub] = "sub";
  opcodeMap[coil::Opcode::Mul] = "mul";
  opcodeMap[coil::Opcode::Div] = "div";
  opcodeMap[coil::Opcode::Rem] = "rem";
  opcodeMap[coil::Opcode::Inc] = "inc";
  opcodeMap[coil::Opcode::Dec] = "dec";
  opcodeMap[coil::Opcode::And] = "and";
  opcodeMap[coil::Opcode::Or] = "or";
  opcodeMap[coil::Opcode::Xor] = "xor";
  opcodeMap[coil::Opcode::Not] = "not";
  opcodeMap[coil::Opcode::Shl] = "shl";
  opcodeMap[coil::Opcode::Shr] = "shr";
  opcodeMap[coil::Opcode::Sar] = "sar";
  opcodeMap[coil::Opcode::Cmp] = "cmp";
  opcodeMap[coil::Opcode::Test] = "test";
  
  // Initialize flag map
  flagMap[coil::InstrFlag0::None] = "";
  flagMap[coil::InstrFlag0::EQ] = "eq";
  flagMap[coil::InstrFlag0::NEQ] = "neq";
  flagMap[coil::InstrFlag0::GT] = "gt";
  flagMap[coil::InstrFlag0::GTE] = "gte";
  flagMap[coil::InstrFlag0::LT] = "lt";
  flagMap[coil::InstrFlag0::LTE] = "lte";
  
  // Initialize type map
  typeMap[coil::ValueType::I8] = "i8";
  typeMap[coil::ValueType::I16] = "i16";
  typeMap[coil::ValueType::I32] = "i32";
  typeMap[coil::ValueType::I64] = "i64";
  typeMap[coil::ValueType::U8] = "u8";
  typeMap[coil::ValueType::U16] = "u16";
  typeMap[coil::ValueType::U32] = "u32";
  typeMap[coil::ValueType::U64] = "u64";
  typeMap[coil::ValueType::F32] = "f32";
  typeMap[coil::ValueType::F64] = "f64";
  typeMap[coil::ValueType::Ptr] = "ptr";
  typeMap[coil::ValueType::Void] = "void";
}

// Disassemble COIL object to CASM code
std::string Disassembler::disassemble(const coil::Object& obj) {
  // Clear previous state
  errors.clear();
  symbolMap.clear();
  
  // Build symbol map
  buildSymbolMap(obj);
  
  std::stringstream output;
  
  // Disassemble each section
  for (uint16_t i = 1; i <= obj.getSectionCount(); i++) {
    const coil::BaseSection* section = obj.getSection(i);
    if (!section) {
      continue;
    }
    
    // Get section name
    const char* name = obj.getString(section->getHeader().name);
    if (!name) {
      name = "unknown_section";
    }
    
    // Add section header
    output << "\n.section " << name << "\n";
    
    // Disassemble section content
    output << disassembleSection(section, obj);
  }
  
  return output.str();
}

// Get disassembly errors
const std::vector<std::string>& Disassembler::getErrors() const {
  return errors;
}

// Disassemble a section
std::string Disassembler::disassembleSection(const coil::BaseSection* section, const coil::Object& obj) {
  if (!section) {
    return "";
  }
  
  // Handle different section types
  switch (static_cast<coil::SectionType>(section->getHeader().type)) {
    case coil::SectionType::ProgBits: {
      // Check if it's a code section
      if (section->getHeader().flags & static_cast<uint16_t>(coil::SectionFlag::Code)) {
        return disassembleCodeSection(static_cast<const coil::DataSection*>(section), obj);
      } else {
        return disassembleDataSection(static_cast<const coil::DataSection*>(section), obj);
      }
    }
    case coil::SectionType::NoBits:
      // BSS section - just placeholder
      return "  ; BSS section\n";
    
    case coil::SectionType::SymTab:
    case coil::SectionType::StrTab:
      // Skip these sections
      return "  ; Special section\n";
    
    default:
      return "  ; Unknown section type\n";
  }
}

// Disassemble a code section
std::string Disassembler::disassembleCodeSection(const coil::DataSection* section, const coil::Object& obj) {
  if (!section) {
    return "";
  }
  
  std::stringstream output;
  
  // Get section data
  const auto& data = section->getData();
  
  // Calculate number of instructions
  size_t instrCount = data.size() / sizeof(coil::Instruction);
  
  // Check if empty
  if (instrCount == 0) {
    output << "  ; Empty code section\n";
    return output.str();
  }
  
  // Get pointer to instructions
  const coil::Instruction* instructions = reinterpret_cast<const coil::Instruction*>(data.data());
  
  // Process each instruction
  for (size_t i = 0; i < instrCount; i++) {
    // Check if this address has a symbol
    auto symbolIt = symbolMap.find(static_cast<uint32_t>(i));
    if (symbolIt != symbolMap.end()) {
      output << symbolIt->second << ":\n";
    }
    
    // Disassemble the instruction
    output << "  " << disassembleInstruction(instructions[i]) << "\n";
  }
  
  return output.str();
}

// Disassemble a data section
std::string Disassembler::disassembleDataSection(const coil::DataSection* section, const coil::Object& obj) {
  if (!section) {
    return "";
  }
  
  std::stringstream output;
  
  // Get section data
  const auto& data = section->getData();
  
  // Check if empty
  if (data.empty()) {
    output << "  ; Empty data section\n";
    return output.str();
  }
  
  // Output data in byte form - 16 bytes per line
  output << "  .byte ";
  
  for (size_t i = 0; i < data.size(); i++) {
    if (i > 0) {
      // Add comma separator
      output << ", ";
      
      // Break line every 16 bytes
      if (i % 16 == 0) {
        output << "\n  .byte ";
      }
    }
    
    // Output byte value
    output << "0x" << std::hex << std::setw(2) << std::setfill('0') 
           << static_cast<int>(data[i]) << std::dec;
  }
  
  output << "\n";
  return output.str();
}

// Disassemble an instruction
std::string Disassembler::disassembleInstruction(const coil::Instruction& instr) {
  std::stringstream output;
  
  // Get opcode string
  auto opcodeIt = opcodeMap.find(instr.opcode);
  if (opcodeIt == opcodeMap.end()) {
    output << "unknown_op";
  } else {
    output << opcodeIt->second;
  }
  
  // Add flag suffix if present
  auto flagIt = flagMap.find(instr.flag0);
  if (flagIt != flagMap.end() && !flagIt->second.empty()) {
    output << "." << flagIt->second;
  }
  
  // Process operands
  bool hasOperand = false;
  
  // Destination operand
  if (instr.dest.type != coil::OperandType::None) {
    output << " " << disassembleOperand(instr.dest);
    hasOperand = true;
  }
  
  // Source operand 1
  if (instr.src1.type != coil::OperandType::None) {
    if (hasOperand) {
      output << ", ";
    } else {
      output << " ";
      hasOperand = true;
    }
    output << disassembleOperand(instr.src1);
  }
  
  // Source operand 2
  if (instr.src2.type != coil::OperandType::None) {
    if (hasOperand) {
      output << ", ";
    } else {
      output << " ";
    }
    output << disassembleOperand(instr.src2);
  }
  
  return output.str();
}

// Disassemble an operand
std::string Disassembler::disassembleOperand(const coil::Operand& op) {
  std::stringstream output;
  
  switch (op.type) {
    case coil::OperandType::Reg:
      // Register operand
      output << "r" << op.reg;
      break;
    
    case coil::OperandType::Imm:
      // Immediate operand
      switch (op.value_type) {
        case coil::ValueType::I8:
          output << static_cast<int>(op.imm.i8_val);
          break;
        case coil::ValueType::I16:
          output << op.imm.i16_val;
          break;
        case coil::ValueType::I32:
          output << op.imm.i32_val;
          break;
        case coil::ValueType::I64:
          output << op.imm.i64_val;
          break;
        case coil::ValueType::U8:
          output << static_cast<unsigned int>(op.imm.u8_val);
          break;
        case coil::ValueType::U16:
          output << op.imm.u16_val;
          break;
        case coil::ValueType::U32:
          output << op.imm.u32_val;
          break;
        case coil::ValueType::U64:
          output << op.imm.u64_val;
          break;
        case coil::ValueType::F32:
          output << op.imm.f32_val;
          break;
        case coil::ValueType::F64:
          output << op.imm.f64_val;
          break;
        default:
          output << "0";
          break;
      }
      break;
    
    case coil::OperandType::Mem:
      // Memory operand
      output << "[r" << op.mem.base;
      if (op.mem.offset > 0) {
        output << "+" << op.mem.offset;
      } else if (op.mem.offset < 0) {
        output << op.mem.offset; // Negative sign will be included
      }
      output << "]";
      break;
    
    case coil::OperandType::Label:
      // Label operand
      {
        // Check if we have a symbol for this label
        auto symbolIt = symbolMap.find(op.label);
        if (symbolIt != symbolMap.end()) {
          output << symbolIt->second;
        } else {
          output << "L" << op.label; // Generate a label name
        }
      }
      break;
    
    default:
      output << "unknown_operand";
      break;
  }
  
  return output.str();
}

// Build a symbol map from object file
void Disassembler::buildSymbolMap(const coil::Object& obj) {
  // Clear any existing map
  symbolMap.clear();
  
  // Get symbol table
  const coil::SymbolSection* symtab = obj.getSymbolTable();
  if (!symtab) {
    return; // No symbol table
  }
  
  // Process all symbols
  const auto& symbols = symtab->getSymbols();
  for (size_t i = 0; i < symbols.size(); i++) {
    const coil::Symbol& sym = symbols[i];
    
    // Get symbol name
    const char* name = obj.getString(sym.name);
    if (!name) {
      continue;
    }
    
    // Add to map
    symbolMap[sym.value] = name;
  }
}

// Report an error
void Disassembler::error(const std::string& message) {
  errors.push_back(message);
}

} // namespace casm