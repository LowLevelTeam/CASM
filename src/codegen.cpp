/**
 * @file codegen.cpp
 * @brief Implementation of code generator for CASM assembly language
 */

#include "casm/codegen.hpp"
#include <sstream>
#include <memory>
#include <algorithm>

#include <coil/err.hpp> // resultToString

namespace casm {

// Constructor
CodeGenerator::CodeGenerator() : currentSectionIndex(0) {
  initOpcodeMap();
}

// Initialize opcode map
void CodeGenerator::initOpcodeMap() {
  opcodeMap["nop"] = coil::Opcode::Nop;
  opcodeMap["br"] = coil::Opcode::Br;
  opcodeMap["jump"] = coil::Opcode::Jump;
  opcodeMap["call"] = coil::Opcode::Call;
  opcodeMap["ret"] = coil::Opcode::Ret;
  opcodeMap["load"] = coil::Opcode::Load;
  opcodeMap["store"] = coil::Opcode::Store;
  opcodeMap["push"] = coil::Opcode::Push;
  opcodeMap["pop"] = coil::Opcode::Pop;
  opcodeMap["add"] = coil::Opcode::Add;
  opcodeMap["sub"] = coil::Opcode::Sub;
  opcodeMap["mul"] = coil::Opcode::Mul;
  opcodeMap["div"] = coil::Opcode::Div;
  opcodeMap["rem"] = coil::Opcode::Rem;
  opcodeMap["inc"] = coil::Opcode::Inc;
  opcodeMap["dec"] = coil::Opcode::Dec;
  opcodeMap["and"] = coil::Opcode::And;
  opcodeMap["or"] = coil::Opcode::Or;
  opcodeMap["xor"] = coil::Opcode::Xor;
  opcodeMap["not"] = coil::Opcode::Not;
  opcodeMap["shl"] = coil::Opcode::Shl;
  opcodeMap["shr"] = coil::Opcode::Shr;
  opcodeMap["sar"] = coil::Opcode::Sar;
  opcodeMap["cmp"] = coil::Opcode::Cmp;
  opcodeMap["test"] = coil::Opcode::Test;
}

// Generate COIL object from statements
std::unique_ptr<coil::Object> CodeGenerator::generate(
    const std::vector<std::unique_ptr<Statement>>& statements) {
  // Clear previous state
  labels.clear();
  pendingLabels.clear();
  errors.clear();
  
  // Create a new COIL object
  obj = coil::Object::create();
  
  // Initialize string table (for section and symbol names)
  obj.initStringTable();
  
  // Process all statements
  for (const auto& stmt : statements) {
    coil::Result result = coil::Result::Success;
    
    switch (stmt->type) {
      case StatementType::Section:
        result = processSection(static_cast<const SectionStatement*>(stmt.get()));
        break;
      case StatementType::Label:
        result = processLabel(static_cast<const LabelStatement*>(stmt.get()));
        break;
      case StatementType::Instruction:
        result = processInstruction(static_cast<const InstructionStatement*>(stmt.get()));
        break;
      case StatementType::Directive:
        result = processDirective(static_cast<const DirectiveStatement*>(stmt.get()));
        break;
    }
    
    if (result != coil::Result::Success) {
      std::stringstream ss;
      ss << "Failed to process statement: " << coil::resultToString(result);
      error(ss.str(), stmt->line);
    }
  }
  
  // Finalize current section
  if (currentSectionIndex > 0) {
    finalizeCurrentSection();
  }
  
  // Resolve label references
  if (!resolveLabels()) {
    // Failed to resolve labels
    return nullptr;
  }
  
  // Return the generated object
  return std::make_unique<coil::Object>(std::move(obj));
}

// Get code generation errors
const std::vector<std::string>& CodeGenerator::getErrors() const {
  return errors;
}

// Process a section statement
coil::Result CodeGenerator::processSection(const SectionStatement* section) {
  // Finalize current section
  if (currentSectionIndex > 0) {
    coil::Result result = finalizeCurrentSection();
    if (result != coil::Result::Success) {
      return result;
    }
  }
  
  // Determine section type and flags
  coil::SectionType type = coil::SectionType::ProgBits;
  coil::SectionFlag flags = coil::SectionFlag::None;
  
  // Set appropriate flags based on section name
  if (section->name == ".text") {
    flags = coil::SectionFlag::Code | coil::SectionFlag::Alloc;
  } else if (section->name == ".data") {
    flags = coil::SectionFlag::Write | coil::SectionFlag::Alloc;
  } else if (section->name == ".bss") {
    type = coil::SectionType::NoBits;
    flags = coil::SectionFlag::Write | coil::SectionFlag::Alloc;
  } else if (section->name == ".rodata") {
    flags = coil::SectionFlag::Alloc;
  }
  
  // Create new section
  currentSectionIndex = addSection(section->name, type, flags);
  
  // Reset current instruction block
  currentBlock = coil::InstructionBlock();
  
  return coil::Result::Success;
}

// Process a label statement
coil::Result CodeGenerator::processLabel(const LabelStatement* label) {
  // Check if we're in a section
  if (currentSectionIndex == 0) {
    error("Label defined outside of a section", label->line);
    return coil::Result::BadState;
  }
  
  // Get current position in instruction block
  uint32_t position = currentBlock.getInstructionCount();
  
  // Add label to map
  labels[label->name] = position;
  
  return coil::Result::Success;
}

// Process an instruction statement
coil::Result CodeGenerator::processInstruction(const InstructionStatement* instr) {
  // Check if we're in a section
  if (currentSectionIndex == 0) {
    error("Instruction used outside of a section", instr->line);
    return coil::Result::BadState;
  }
  
  // Create COIL instruction
  coil::Instruction coilInstr = createInstruction(instr);
  
  // Add to current block
  uint32_t instrIndex = currentBlock.addInstruction(coilInstr);
  
  // Check for label operands to resolve later
  for (size_t i = 0; i < instr->operands.size(); i++) {
    const auto& op = instr->operands[i];
    if (op.type == InstructionOperand::Type::Label) {
      // Save for later resolution
      pendingLabels[instrIndex] = op.labelName;
    }
  }
  
  return coil::Result::Success;
}

// Create a COIL instruction
coil::Instruction CodeGenerator::createInstruction(const InstructionStatement* instr) {
  // Find opcode
  auto opcodeIt = opcodeMap.find(instr->opcode);
  if (opcodeIt == opcodeMap.end()) {
    std::stringstream ss;
    ss << "Unknown opcode: " << instr->opcode;
    error(ss.str(), instr->line);
    return coil::createInstr(coil::Opcode::Nop);
  }
  
  coil::Opcode opcode = opcodeIt->second;
  
  // Convert operands
  std::vector<coil::Operand> operands;
  for (const auto& op : instr->operands) {
    operands.push_back(convertOperand(op));
  }
  
  // Create instruction based on number of operands
  if (operands.empty()) {
    return coil::createInstr(opcode, instr->flag);
  } else if (operands.size() == 1) {
    return coil::createInstr(opcode, operands[0], instr->flag);
  } else if (operands.size() == 2) {
    return coil::createInstr(opcode, operands[0], operands[1], instr->flag);
  } else if (operands.size() == 3) {
    return coil::createInstr(opcode, operands[0], operands[1], operands[2], instr->flag);
  } else {
    error("Too many operands for instruction", instr->line);
    return coil::createInstr(coil::Opcode::Nop);
  }
}

// Convert instruction operand to COIL operand
coil::Operand CodeGenerator::convertOperand(const InstructionOperand& op) {
  switch (op.type) {
    case InstructionOperand::Type::Register:
      return coil::createRegOp(op.reg, op.valueType);
    
    case InstructionOperand::Type::Immediate:
      return coil::createImmOpInt(op.imm, op.valueType);
    
    case InstructionOperand::Type::FloatImmediate:
      return coil::createImmOpFp(op.fpImm, op.valueType);
    
    case InstructionOperand::Type::Memory:
      return coil::createMemOp(op.mem.base, op.mem.offset, op.valueType);
    
    case InstructionOperand::Type::Label:
      // Use placeholder, will be resolved later
      return coil::createLabelOp(0);
    
    default:
      // Invalid operand type
      return coil::createImmOpInt(0, coil::ValueType::I32);
  }
}

// Process a directive statement
coil::Result CodeGenerator::processDirective(const DirectiveStatement* directive) {
  // Special handling for .section directive
  if (directive->name == ".section") {
    if (directive->args.empty()) {
      error("Section directive requires a name", directive->line);
      return coil::Result::InvalidArg;
    }
    
    // Create a section statement and process it
    SectionStatement section(directive->args[0], directive->line);
    return processSection(&section);
  }
  
  // Check if current section exists for other directives
  if (currentSectionIndex == 0) {
    error("Directive used outside of a section", directive->line);
    return coil::Result::BadState;
  }
  
  // Handle different directive types
  if (directive->name == ".global" || directive->name == ".globl") {
    // Initialize symbol table if needed
    obj.initSymbolTable();
    
    // Process global symbols
    for (const auto& arg : directive->args) {
      // Add to symbol table as global
      uint64_t nameOffset = obj.addString(arg.c_str());
      uint16_t symIndex = obj.addSymbol(
        nameOffset, 
        0, // Value will be filled in later
        currentSectionIndex,
        static_cast<uint8_t>(coil::SymbolType::Func),
        static_cast<uint8_t>(coil::SymbolBinding::Global)
      );
      
      if (symIndex == 0) {
        error("Failed to add global symbol: " + arg, directive->line);
        return coil::Result::OutOfMemory;
      }
    }
  }
  else if (directive->name == ".byte" || directive->name == ".word" || 
           directive->name == ".dword" || directive->name == ".qword") {
    // Data directives - to be implemented
    // For now, just skip
  }
  else if (directive->name == ".float" || directive->name == ".double") {
    // Float data directives - to be implemented
    // For now, just skip
  }
  else if (directive->name == ".string") {
    // String directives - to be implemented
    // For now, just skip
  }
  else if (directive->name == ".align") {
    // Alignment directives - to be implemented
    // For now, just skip
  }
  
  return coil::Result::Success;
}

// Add a section to the object
uint16_t CodeGenerator::addSection(const std::string& name, coil::SectionType type, coil::SectionFlag flags) {
  // Add name to string table
  uint64_t nameOffset = obj.addString(name.c_str());
  if (nameOffset == 0) {
    error("Failed to add section name to string table: " + name, 0);
    return 0;
  }
  
  // Create section
  uint16_t index = obj.addSection(
    nameOffset,
    static_cast<uint16_t>(flags),
    static_cast<uint8_t>(type),
    0, // Initial size
    nullptr, // No initial data
    0
  );
  
  if (index == 0) {
    error("Failed to add section: " + name, 0);
    return 0;
  }
  
  return index;
}

// Finalize the current section
coil::Result CodeGenerator::finalizeCurrentSection() {
  if (currentSectionIndex == 0) {
    return coil::Result::Success; // No current section
  }
  
  // Get current section
  coil::BaseSection* section = obj.getSection(currentSectionIndex);
  if (!section) {
    error("Failed to get current section", 0);
    return coil::Result::NotFound;
  }
  
  // Check if it's a code section
  if (section->getHeader().flags & static_cast<uint16_t>(coil::SectionFlag::Code)) {
    // Convert instruction block to binary data
    const coil::Instruction* instructions = currentBlock.getData();
    uint32_t count = currentBlock.getInstructionCount();
    
    if (count > 0) {
      // Get section as data section
      auto dataSection = static_cast<coil::DataSection*>(section);
      
      // Add instructions to section data
      auto& data = dataSection->getData();
      size_t origSize = data.size();
      data.resize(origSize + count * sizeof(coil::Instruction));
      
      // Copy instructions
      std::memcpy(data.data() + origSize, instructions, count * sizeof(coil::Instruction));
    }
  }
  
  return coil::Result::Success;
}

// Resolve pending label references
bool CodeGenerator::resolveLabels() {
  bool success = true;
  
  // Get instruction sections
  for (uint16_t i = 1; i <= obj.getSectionCount(); i++) {
    coil::BaseSection* section = obj.getSection(i);
    if (!section || !(section->getHeader().flags & static_cast<uint16_t>(coil::SectionFlag::Code))) {
      continue; // Not a code section
    }
    
    // Get section data
    auto dataSection = static_cast<coil::DataSection*>(section);
    auto& data = dataSection->getData();
    
    // Calculate number of instructions
    size_t instrCount = data.size() / sizeof(coil::Instruction);
    
    // Get pointer to instructions
    coil::Instruction* instructions = reinterpret_cast<coil::Instruction*>(data.data());
    
    // Process each pending label
    for (const auto& [instrIndex, labelName] : pendingLabels) {
      // Check if index is valid for this section
      if (instrIndex >= instrCount) {
        continue;
      }
      
      // Find label
      auto labelIt = labels.find(labelName);
      if (labelIt == labels.end()) {
        std::stringstream ss;
        ss << "Undefined label: " << labelName;
        error(ss.str(), 0);
        success = false;
        continue;
      }
      
      uint32_t labelValue = labelIt->second;
      
      // Update the instruction with label value
      coil::Instruction& instr = instructions[instrIndex];
      
      // Update label operand based on position
      if (instr.dest.type == coil::OperandType::Label) {
        instr.dest.label = labelValue;
      } else if (instr.src1.type == coil::OperandType::Label) {
        instr.src1.label = labelValue;
      } else if (instr.src2.type == coil::OperandType::Label) {
        instr.src2.label = labelValue;
      }
    }
  }
  
  return success;
}

// Report an error
void CodeGenerator::error(const std::string& message, size_t line) {
  std::stringstream ss;
  ss << "Line " << line << ": " << message;
  errors.push_back(ss.str());
}

} // namespace casm