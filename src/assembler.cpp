#include <casm/assembler.hpp>
#include <casm/lexer.hpp>
#include <casm/parser.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace casm {

Assembler::Assembler() {
  // Initialize COIL library
  coil::initialize();
}

coil::Object Assembler::assemble(const std::vector<Statement>& statements) {
  // Clear any previous state
  m_sections.clear();
  m_currentSection.clear();
  m_globalSymbols.clear();
  m_labelReferences.clear();
  m_errors.clear();
  
  // First pass - collect labels and calculate section sizes
  collectLabels(statements);
  
  // Second pass - generate code
  generateCode(statements);
  
  // Resolve label references
  resolveReferences();
  
  // Create COIL object
  coil::Object obj = coil::Object::create();
  
  // Initialize string table
  obj.initStringTable();
  
  // Add sections to object
  for (const auto& [name, section] : m_sections) {
    // Add section name to string table
    u64 nameOffset = obj.addString(name);
    
    // Add section to object
    uint16_t flags = static_cast<uint16_t>(section.flags);
    uint8_t type = static_cast<uint8_t>(section.type);
    
    obj.addSection(nameOffset, flags, type, section.data.size(), section.data);
    
    log("Added section '" + name + "', size: " + std::to_string(section.data.size()) + 
        " bytes, flags: 0x" + std::to_string(flags));
  }
  
  // Initialize symbol table
  obj.initSymbolTable();
  
  // Add global symbols to object
  for (const auto& [label, sectionName] : m_globalSymbols) {
    // Get section index
    uint16_t sectionIndex = obj.getSectionIndex(sectionName);
    if (sectionIndex == 0) {
      error("Could not find section '" + sectionName + "' for global symbol '" + label + "'");
      continue;
    }
    
    // Get symbol offset
    size_t offset = 0;
    if (m_sections.count(sectionName) > 0 && m_sections[sectionName].labels.count(label) > 0) {
      offset = m_sections[sectionName].labels[label];
    } else {
      error("Could not find label '" + label + "' in section '" + sectionName + "'");
      continue;
    }
    
    // Add symbol name to string table
    u64 nameOffset = obj.addString(label);
    
    // Add symbol to object
    obj.addSymbol(
      nameOffset,                                          // name
      static_cast<u32>(offset),                            // value
      sectionIndex,                                        // section_index
      static_cast<u8>(coil::SymbolType::Func),            // type (assume function)
      static_cast<u8>(coil::SymbolBinding::Global)        // binding
    );
    
    log("Added global symbol '" + label + "' at offset " + std::to_string(offset) + 
        " in section '" + sectionName + "'");
  }
  
  return obj;
}

coil::Object Assembler::assembleSource(const std::string& source, const std::string& filename) {
  // Create lexer and parser
  Lexer lexer(filename, source);
  Parser parser(lexer);
  
  // Parse the source
  std::vector<Statement> statements = parser.parse();
  
  // Check for parser errors
  if (!parser.getErrors().empty()) {
    for (const auto& error : parser.getErrors()) {
      m_errors.push_back(error);
    }
    throw AssemblerException("Parser errors");
  }
  
  // Assemble the statements
  return assemble(statements);
}

void Assembler::collectLabels(const std::vector<Statement>& statements) {
  log("First pass - collecting labels");
  
  for (const auto& stmt : statements) {
    // Skip empty statements
    if (stmt.getType() == Statement::Type::Empty) {
      continue;
    }
    
    // Handle label statement
    if (stmt.getType() == Statement::Type::Label) {
      ensureCurrentSection();
      addLabel(stmt.getLabel());
      continue;
    }
    
    // Process directives that affect section layout
    if (stmt.getType() == Statement::Type::Directive) {
      const Directive* directive = stmt.getDirective();
      const std::string& name = directive->getName();
      
      // Handle section directive
      if (name == "section") {
        if (directive->getOperands().empty()) {
          error("Section directive requires a name operand");
          continue;
        }
        
        const Operand* op = directive->getOperands()[0].get();
        if (op->getType() != Operand::Type::Label && op->getType() != Operand::Type::Immediate) {
          error("Section name must be a label reference or immediate value");
          continue;
        }
        
        std::string sectionName;
        if (op->getType() == Operand::Type::Label) {
          sectionName = static_cast<const LabelOperand*>(op)->getLabel();
        } else {
          // Handle immediate string
          const ImmediateOperand* immOp = static_cast<const ImmediateOperand*>(op);
          const ImmediateValue& value = immOp->getValue();
          
          if (value.format == ImmediateFormat::String) {
            sectionName = std::get<std::string>(value.value);
          } else {
            std::ostringstream ss;
            if (value.format == ImmediateFormat::Character) {
              ss << std::get<char>(value.value);
            } else if (value.format == ImmediateFormat::Integer) {
              ss << std::get<i64>(value.value);
            } else if (value.format == ImmediateFormat::Float) {
              ss << std::get<f64>(value.value);
            }
            sectionName = ss.str();
          }
        }
        
        // Switch to the new section
        switchSection(sectionName);
        
        // Get section parameters (flags)
        for (size_t i = 1; i < directive->getOperands().size(); ++i) {
          const Operand* param = directive->getOperands()[i].get();
          if (param->getType() != Operand::Type::Label) {
            error("Section parameter must be a label reference");
            continue;
          }
          
          const std::string& paramName = static_cast<const LabelOperand*>(param)->getLabel();
          
          // Convert parameter name to section flag (case-insensitive)
          std::string paramNameLower = paramName;
          std::transform(paramNameLower.begin(), paramNameLower.end(), paramNameLower.begin(), ::tolower);
          
          if (paramNameLower == "progbits") {
            m_sections[m_currentSection].type = coil::SectionType::ProgBits;
          } else if (paramNameLower == "nobits") {
            m_sections[m_currentSection].type = coil::SectionType::NoBits;
          } else if (paramNameLower == "symtab") {
            m_sections[m_currentSection].type = coil::SectionType::SymTab;
          } else if (paramNameLower == "strtab") {
            m_sections[m_currentSection].type = coil::SectionType::StrTab;
          } else if (paramNameLower == "write") {
            m_sections[m_currentSection].flags = 
              m_sections[m_currentSection].flags | coil::SectionFlag::Write;
          } else if (paramNameLower == "code") {
            m_sections[m_currentSection].flags = 
              m_sections[m_currentSection].flags | coil::SectionFlag::Code;
          } else if (paramNameLower == "alloc") {
            m_sections[m_currentSection].flags = 
              m_sections[m_currentSection].flags | coil::SectionFlag::Alloc;
          } else {
            error("Unknown section parameter: " + paramName);
          }
        }
        
        continue;
      }
      
      // Handle global directive
      if (name == "global") {
        if (directive->getOperands().empty()) {
          error("Global directive requires a label operand");
          continue;
        }
        
        const Operand* op = directive->getOperands()[0].get();
        if (op->getType() != Operand::Type::Label) {
          error("Global symbol must be a label reference");
          continue;
        }
        
        const std::string& label = static_cast<const LabelOperand*>(op)->getLabel();
        addGlobalSymbol(label);
        continue;
      }
      
      // Handle data directives
      if (name == "i8" || name == "u8" || name == "i16" || name == "u16" || 
          name == "i32" || name == "u32" || name == "i64" || name == "u64" || 
          name == "f32" || name == "f64") {
        
        ensureCurrentSection();
        
        // Calculate size of data
        size_t elementSize = 0;
        if (name == "i8" || name == "u8") {
          elementSize = 1;
        } else if (name == "i16" || name == "u16") {
          elementSize = 2;
        } else if (name == "i32" || name == "u32" || name == "f32") {
          elementSize = 4;
        } else if (name == "i64" || name == "u64" || name == "f64") {
          elementSize = 8;
        }
        
        size_t dataSize = elementSize * directive->getOperands().size();
        m_sections[m_currentSection].currentOffset += dataSize;
        continue;
      }
      
      // Handle string directives
      if (name == "ascii" || name == "asciiz") {
        ensureCurrentSection();
        
        for (const auto& op : directive->getOperands()) {
          if (op->getType() != Operand::Type::Immediate) {
            error("String operand must be an immediate value");
            continue;
          }
          
          const ImmediateOperand* immOp = static_cast<const ImmediateOperand*>(op.get());
          const ImmediateValue& value = immOp->getValue();
          
          if (value.format != ImmediateFormat::String) {
            error("String operand must be a string literal");
            continue;
          }
          
          const std::string& str = std::get<std::string>(value.value);
          size_t strSize = str.size();
          
          // Add null terminator for asciiz
          if (name == "asciiz") {
            strSize++;
          }
          
          m_sections[m_currentSection].currentOffset += strSize;
        }
        
        continue;
      }
      
      // Handle zero directive
      if (name == "zero") {
        ensureCurrentSection();
        
        if (directive->getOperands().empty()) {
          error("Zero directive requires a size operand");
          continue;
        }
        
        const Operand* op = directive->getOperands()[0].get();
        if (op->getType() != Operand::Type::Immediate) {
          error("Zero size must be an immediate value");
          continue;
        }
        
        const ImmediateOperand* immOp = static_cast<const ImmediateOperand*>(op);
        const ImmediateValue& value = immOp->getValue();
        
        if (value.format != ImmediateFormat::Integer) {
          error("Zero size must be an integer");
          continue;
        }
        
        size_t zeroSize = static_cast<size_t>(std::get<i64>(value.value));
        m_sections[m_currentSection].currentOffset += zeroSize;
        
        continue;
      }
    }
    
    // Handle instruction
    if (stmt.getType() == Statement::Type::Instruction) {
      ensureCurrentSection();
      
      // Add label if present
      if (!stmt.getLabel().empty()) {
        addLabel(stmt.getLabel());
      }
      
      // Add instruction size estimate
      // For simplicity, we'll use a fixed size of 8 bytes per instruction
      // This will be refined in a real implementation
      m_sections[m_currentSection].currentOffset += 8;
    }
  }
}

void Assembler::generateCode(const std::vector<Statement>& statements) {
  log("Second pass - generating code");
  
  // Reset section offsets
  for (auto& [name, section] : m_sections) {
    section.currentOffset = 0;
    section.data.clear();
  }
  
  // Reset current section
  m_currentSection.clear();
  
  for (const auto& stmt : statements) {
    // Skip empty statements
    if (stmt.getType() == Statement::Type::Empty) {
      continue;
    }
    
    // Handle directives
    if (stmt.getType() == Statement::Type::Directive) {
      const Directive* directive = stmt.getDirective();
      processDirective(*directive, stmt.getLabel());
      continue;
    }
    
    // Handle instructions
    if (stmt.getType() == Statement::Type::Instruction) {
      const Instruction* instruction = stmt.getInstruction();
      processInstruction(*instruction, stmt.getLabel());
      continue;
    }
    
    // Handle label-only statements
    if (stmt.getType() == Statement::Type::Label) {
      ensureCurrentSection();
      addLabel(stmt.getLabel());
      continue;
    }
  }
}

void Assembler::ensureCurrentSection() {
  if (m_currentSection.empty()) {
    // Default to .text section
    switchSection(".text");
  }
}

void Assembler::switchSection(const std::string& name) {
  m_currentSection = name;
  
  // Create section if it doesn't exist
  if (m_sections.find(name) == m_sections.end()) {
    m_sections[name] = SectionContext{};
    m_sections[name].name = name;
    
    // Set default flags based on section name
    if (name == ".text") {
      m_sections[name].flags = coil::SectionFlag::Code | coil::SectionFlag::Alloc;
      m_sections[name].type = coil::SectionType::ProgBits;
    } else if (name == ".data") {
      m_sections[name].flags = coil::SectionFlag::Write | coil::SectionFlag::Alloc;
      m_sections[name].type = coil::SectionType::ProgBits;
    } else if (name == ".bss") {
      m_sections[name].flags = coil::SectionFlag::Write | coil::SectionFlag::Alloc;
      m_sections[name].type = coil::SectionType::NoBits;
    }
  }
  
  log("Switched to section '" + name + "'");
}

void Assembler::addLabel(const std::string& label) {
  ensureCurrentSection();
  
  // Add label to current section
  m_sections[m_currentSection].addLabel(label);
  log("Added label '" + label + "' at offset " + 
      std::to_string(m_sections[m_currentSection].currentOffset) + 
      " in section '" + m_currentSection + "'");
}

void Assembler::addGlobalSymbol(const std::string& label) {
  ensureCurrentSection();
  
  // Add global symbol
  m_globalSymbols[label] = m_currentSection;
  log("Added global symbol '" + label + "' in section '" + m_currentSection + "'");
}

void Assembler::addImmediate(const ImmediateValue& value, coil::ValueType type) {
  switch (type) {
    case coil::ValueType::I8:
    case coil::ValueType::U8:
      addByteImmediate(value);
      break;
    case coil::ValueType::I16:
    case coil::ValueType::U16:
      addShortImmediate(value);
      break;
    case coil::ValueType::I32:
    case coil::ValueType::U32:
      addWordImmediate(value);
      break;
    case coil::ValueType::I64:
    case coil::ValueType::U64:
      addLongImmediate(value);
      break;
    case coil::ValueType::F32:
      addFloatImmediate(value);
      break;
    case coil::ValueType::F64:
      addDoubleImmediate(value);
      break;
    default:
      error("Unsupported value type for immediate");
      break;
  }
}

void Assembler::addByteImmediate(const ImmediateValue& value) {
  ensureCurrentSection();
  
  uint8_t byteValue = 0;
  
  if (value.format == ImmediateFormat::Integer) {
    byteValue = static_cast<uint8_t>(std::get<i64>(value.value));
  } else if (value.format == ImmediateFormat::Character) {
    byteValue = static_cast<uint8_t>(std::get<char>(value.value));
  } else {
    error("Cannot convert immediate to byte");
    return;
  }
  
  // Add byte to section
  m_sections[m_currentSection].data.push_back(byteValue);
  m_sections[m_currentSection].currentOffset += 1;
}

void Assembler::addShortImmediate(const ImmediateValue& value) {
  ensureCurrentSection();
  
  uint16_t shortValue = 0;
  
  if (value.format == ImmediateFormat::Integer) {
    shortValue = static_cast<uint16_t>(std::get<i64>(value.value));
  } else if (value.format == ImmediateFormat::Character) {
    shortValue = static_cast<uint16_t>(std::get<char>(value.value));
  } else {
    error("Cannot convert immediate to short");
    return;
  }
  
  // Add short to section (little-endian)
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>(shortValue & 0xFF));
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>((shortValue >> 8) & 0xFF));
  m_sections[m_currentSection].currentOffset += 2;
}

void Assembler::addWordImmediate(const ImmediateValue& value) {
  ensureCurrentSection();
  
  uint32_t wordValue = 0;
  
  if (value.format == ImmediateFormat::Integer) {
    wordValue = static_cast<uint32_t>(std::get<i64>(value.value));
  } else if (value.format == ImmediateFormat::Character) {
    wordValue = static_cast<uint32_t>(std::get<char>(value.value));
  } else if (value.format == ImmediateFormat::Float) {
    float floatValue = static_cast<float>(std::get<f64>(value.value));
    std::memcpy(&wordValue, &floatValue, sizeof(float));
  } else {
    error("Cannot convert immediate to word");
    return;
  }
  
  // Add word to section (little-endian)
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>(wordValue & 0xFF));
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>((wordValue >> 8) & 0xFF));
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>((wordValue >> 16) & 0xFF));
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>((wordValue >> 24) & 0xFF));
  m_sections[m_currentSection].currentOffset += 4;
}

void Assembler::addLongImmediate(const ImmediateValue& value) {
  ensureCurrentSection();
  
  uint64_t longValue = 0;
  
  if (value.format == ImmediateFormat::Integer) {
    longValue = static_cast<uint64_t>(std::get<i64>(value.value));
  } else if (value.format == ImmediateFormat::Character) {
    longValue = static_cast<uint64_t>(std::get<char>(value.value));
  } else if (value.format == ImmediateFormat::Float) {
    double doubleValue = std::get<f64>(value.value);
    std::memcpy(&longValue, &doubleValue, sizeof(double));
  } else {
    error("Cannot convert immediate to long");
    return;
  }
  
  // Add long to section (little-endian)
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>(longValue & 0xFF));
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>((longValue >> 8) & 0xFF));
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>((longValue >> 16) & 0xFF));
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>((longValue >> 24) & 0xFF));
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>((longValue >> 32) & 0xFF));
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>((longValue >> 40) & 0xFF));
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>((longValue >> 48) & 0xFF));
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>((longValue >> 56) & 0xFF));
  m_sections[m_currentSection].currentOffset += 8;
}

void Assembler::addFloatImmediate(const ImmediateValue& value) {
  ensureCurrentSection();
  
  float floatValue = 0.0f;
  
  if (value.format == ImmediateFormat::Integer) {
    floatValue = static_cast<float>(std::get<i64>(value.value));
  } else if (value.format == ImmediateFormat::Float) {
    floatValue = static_cast<float>(std::get<f64>(value.value));
  } else {
    error("Cannot convert immediate to float");
    return;
  }
  
  // Add float to section (as 4 bytes)
  uint32_t bits;
  std::memcpy(&bits, &floatValue, sizeof(float));
  
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>(bits & 0xFF));
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>((bits >> 8) & 0xFF));
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>((bits >> 16) & 0xFF));
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>((bits >> 24) & 0xFF));
  m_sections[m_currentSection].currentOffset += 4;
}

void Assembler::addDoubleImmediate(const ImmediateValue& value) {
  ensureCurrentSection();
  
  double doubleValue = 0.0;
  
  if (value.format == ImmediateFormat::Integer) {
    doubleValue = static_cast<double>(std::get<i64>(value.value));
  } else if (value.format == ImmediateFormat::Float) {
    doubleValue = std::get<f64>(value.value);
  } else {
    error("Cannot convert immediate to double");
    return;
  }
  
  // Add double to section (as 8 bytes)
  uint64_t bits;
  std::memcpy(&bits, &doubleValue, sizeof(double));
  
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>(bits & 0xFF));
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>((bits >> 8) & 0xFF));
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>((bits >> 16) & 0xFF));
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>((bits >> 24) & 0xFF));
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>((bits >> 32) & 0xFF));
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>((bits >> 40) & 0xFF));
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>((bits >> 48) & 0xFF));
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>((bits >> 56) & 0xFF));
  m_sections[m_currentSection].currentOffset += 8;
}

void Assembler::addLabelReference(const std::string& label, size_t size, bool isRelative) {
  ensureCurrentSection();
  
  // Add a placeholder for the label reference
  for (size_t i = 0; i < size; ++i) {
    m_sections[m_currentSection].data.push_back(0);
  }
  
  // Add reference to be resolved later
  LabelReference ref;
  ref.label = label;
  ref.section = m_currentSection;
  ref.offset = m_sections[m_currentSection].currentOffset - size;
  ref.size = size;
  ref.isRelative = isRelative;
  
  m_labelReferences.push_back(ref);
  
  m_sections[m_currentSection].currentOffset += size;
}

void Assembler::addString(const std::string& str, bool nullTerminated) {
  ensureCurrentSection();
  
  // Add string to section
  for (char c : str) {
    m_sections[m_currentSection].data.push_back(static_cast<uint8_t>(c));
  }
  
  // Add null terminator if requested
  if (nullTerminated) {
    m_sections[m_currentSection].data.push_back(0);
  }
  
  m_sections[m_currentSection].currentOffset += str.size() + (nullTerminated ? 1 : 0);
}

void Assembler::processInstruction(const Instruction& instruction, const std::string& label) {
  ensureCurrentSection();
  
  // Add label if present
  if (!label.empty()) {
    addLabel(label);
  }
  
  // Get instruction name (convert to lowercase)
  std::string name = instruction.getName();
  std::transform(name.begin(), name.end(), name.begin(), ::tolower);
  
  // Create COIL instruction (simplified implementation)
  // In a real implementation, we would encode actual COIL instructions
  // For now, we'll just create a placeholder with the instruction name and operands
  
  // For demonstration purposes, we'll write a simple encoding:
  // - 1 byte for opcode
  // - 1 byte for number of operands
  // - Remaining bytes for operands (simplified)
  
  uint8_t opcode = 0;
  coil::InstrFlag0 flag = coil::InstrFlag0::None;
  
  // Get instruction parameters
  const auto& params = instruction.getParameters();
  for (const auto& param : params) {
    if (param == "eq") {
      flag = coil::InstrFlag0::EQ;
    } else if (param == "neq") {
      flag = coil::InstrFlag0::NEQ;
    } else if (param == "gt") {
      flag = coil::InstrFlag0::GT;
    } else if (param == "gte") {
      flag = coil::InstrFlag0::GTE;
    } else if (param == "lt") {
      flag = coil::InstrFlag0::LT;
    } else if (param == "lte") {
      flag = coil::InstrFlag0::LTE;
    }
  }
  
  // Determine opcode from instruction name
  if (name == "nop") {
    opcode = static_cast<uint8_t>(coil::Opcode::Nop);
  } else if (name == "jmp") {
    opcode = static_cast<uint8_t>(coil::Opcode::Jump);
  } else if (name == "br") {
    opcode = static_cast<uint8_t>(coil::Opcode::Br);
  } else if (name == "call") {
    opcode = static_cast<uint8_t>(coil::Opcode::Call);
  } else if (name == "ret") {
    opcode = static_cast<uint8_t>(coil::Opcode::Ret);
  } else if (name == "load") {
    opcode = static_cast<uint8_t>(coil::Opcode::Load);
  } else if (name == "store") {
    opcode = static_cast<uint8_t>(coil::Opcode::Store);
  } else if (name == "push") {
    opcode = static_cast<uint8_t>(coil::Opcode::Push);
  } else if (name == "pop") {
    opcode = static_cast<uint8_t>(coil::Opcode::Pop);
  } else if (name == "mov") {
    opcode = static_cast<uint8_t>(coil::Opcode::Mov);
  } else if (name == "add") {
    opcode = static_cast<uint8_t>(coil::Opcode::Add);
  } else if (name == "sub") {
    opcode = static_cast<uint8_t>(coil::Opcode::Sub);
  } else if (name == "mul") {
    opcode = static_cast<uint8_t>(coil::Opcode::Mul);
  } else if (name == "div") {
    opcode = static_cast<uint8_t>(coil::Opcode::Div);
  } else if (name == "rem") {
    opcode = static_cast<uint8_t>(coil::Opcode::Rem);
  } else if (name == "inc") {
    opcode = static_cast<uint8_t>(coil::Opcode::Inc);
  } else if (name == "dec") {
    opcode = static_cast<uint8_t>(coil::Opcode::Dec);
  } else if (name == "neg") {
    opcode = static_cast<uint8_t>(coil::Opcode::Neg);
  } else if (name == "and") {
    opcode = static_cast<uint8_t>(coil::Opcode::And);
  } else if (name == "or") {
    opcode = static_cast<uint8_t>(coil::Opcode::Or);
  } else if (name == "xor") {
    opcode = static_cast<uint8_t>(coil::Opcode::Xor);
  } else if (name == "not") {
    opcode = static_cast<uint8_t>(coil::Opcode::Not);
  } else if (name == "shl") {
    opcode = static_cast<uint8_t>(coil::Opcode::Shl);
  } else if (name == "shr") {
    opcode = static_cast<uint8_t>(coil::Opcode::Shr);
  } else if (name == "sar") {
    opcode = static_cast<uint8_t>(coil::Opcode::Sar);
  } else if (name == "cmp") {
    opcode = static_cast<uint8_t>(coil::Opcode::Cmp);
  } else if (name == "test") {
    opcode = static_cast<uint8_t>(coil::Opcode::Test);
  } else {
    error("Unknown instruction: " + name);
    return;
  }
  
  // Simplified encoding - just for demonstration
  
  // Write opcode
  m_sections[m_currentSection].data.push_back(opcode);
  
  // Write flag
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>(flag));
  
  // Write number of operands
  m_sections[m_currentSection].data.push_back(static_cast<uint8_t>(instruction.getOperands().size()));
  
  // Write operand types (simplified)
  for (const auto& operand : instruction.getOperands()) {
    uint8_t operandType = 0;
    
    switch (operand->getType()) {
      case Operand::Type::Register:
        operandType = 1;
        break;
      case Operand::Type::Immediate:
        operandType = 2;
        break;
      case Operand::Type::Memory:
        operandType = 3;
        break;
      case Operand::Type::Label:
        operandType = 4;
        break;
    }
    
    m_sections[m_currentSection].data.push_back(operandType);
    
    // Handle operand data (simplified)
    if (operand->getType() == Operand::Type::Register) {
      // Register operand - just write register number
      const RegisterOperand* regOp = static_cast<const RegisterOperand*>(operand.get());
      uint32_t regIndex = getRegisterIndex(regOp->getName());
      m_sections[m_currentSection].data.push_back(static_cast<uint8_t>(regIndex));
    } else if (operand->getType() == Operand::Type::Immediate) {
      // Immediate operand - write a 4-byte value
      const ImmediateOperand* immOp = static_cast<const ImmediateOperand*>(operand.get());
      const ImmediateValue& value = immOp->getValue();
      
      if (value.format == ImmediateFormat::Integer) {
        int32_t intValue = static_cast<int32_t>(std::get<i64>(value.value));
        addWordImmediate(value);
      } else {
        // Write a zero for non-integer immediates
        m_sections[m_currentSection].data.push_back(0);
        m_sections[m_currentSection].data.push_back(0);
        m_sections[m_currentSection].data.push_back(0);
        m_sections[m_currentSection].data.push_back(0);
      }
    } else if (operand->getType() == Operand::Type::Memory) {
      // Memory operand - write base register and offset
      const MemoryOperand* memOp = static_cast<const MemoryOperand*>(operand.get());
      const MemoryReference& memRef = memOp->getReference();
      
      uint32_t regIndex = getRegisterIndex(memRef.reg);
      m_sections[m_currentSection].data.push_back(static_cast<uint8_t>(regIndex));
      
      // Write offset as 4-byte value (little-endian)
      int32_t offset = static_cast<int32_t>(memRef.offset);
      m_sections[m_currentSection].data.push_back(static_cast<uint8_t>(offset & 0xFF));
      m_sections[m_currentSection].data.push_back(static_cast<uint8_t>((offset >> 8) & 0xFF));
      m_sections[m_currentSection].data.push_back(static_cast<uint8_t>((offset >> 16) & 0xFF));
      m_sections[m_currentSection].data.push_back(static_cast<uint8_t>((offset >> 24) & 0xFF));
    } else if (operand->getType() == Operand::Type::Label) {
      // Label operand - add a label reference
      const LabelOperand* labelOp = static_cast<const LabelOperand*>(operand.get());
      
      // For jump/branch/call instructions, use relative addressing
      bool isRelative = (name == "jmp" || name == "br" || name == "call");
      
      // Add a 4-byte label reference
      size_t currentSize = m_sections[m_currentSection].data.size();
      addLabelReference(labelOp->getLabel(), 4, isRelative);
      
      // Adjust currentOffset because addLabelReference adds 4 bytes
      m_sections[m_currentSection].currentOffset -= 4;
    }
  }
  
  // Update section offset (simplified - always use 8 bytes per instruction)
  m_sections[m_currentSection].currentOffset = m_sections[m_currentSection].data.size();
}

void Assembler::processDirective(const Directive& directive, const std::string& label) {
  // Get directive name
  const std::string& name = directive.getName();
  
  // Handle section directive
  if (name == "section") {
    if (directive.getOperands().empty()) {
      error("Section directive requires a name operand");
      return;
    }
    
    const Operand* op = directive.getOperands()[0].get();
    if (op->getType() != Operand::Type::Label && op->getType() != Operand::Type::Immediate) {
      error("Section name must be a label reference or immediate value");
      return;
    }
    
    std::string sectionName;
    if (op->getType() == Operand::Type::Label) {
      sectionName = static_cast<const LabelOperand*>(op)->getLabel();
    } else {
      // Handle immediate string
      const ImmediateOperand* immOp = static_cast<const ImmediateOperand*>(op);
      const ImmediateValue& value = immOp->getValue();
      
      if (value.format == ImmediateFormat::String) {
        sectionName = std::get<std::string>(value.value);
      } else {
        std::ostringstream ss;
        if (value.format == ImmediateFormat::Character) {
          ss << std::get<char>(value.value);
        } else if (value.format == ImmediateFormat::Integer) {
          ss << std::get<i64>(value.value);
        } else if (value.format == ImmediateFormat::Float) {
          ss << std::get<f64>(value.value);
        }
        sectionName = ss.str();
      }
    }
    
    // Switch to the new section
    switchSection(sectionName);
    
    // Get section parameters (flags)
    for (size_t i = 1; i < directive.getOperands().size(); ++i) {
      const Operand* param = directive.getOperands()[i].get();
      if (param->getType() != Operand::Type::Label) {
        error("Section parameter must be a label reference");
        continue;
      }
      
      const std::string& paramName = static_cast<const LabelOperand*>(param)->getLabel();
      
      // Convert parameter name to section flag (case-insensitive)
      std::string paramNameLower = paramName;
      std::transform(paramNameLower.begin(), paramNameLower.end(), paramNameLower.begin(), ::tolower);
      
      if (paramNameLower == "progbits") {
        m_sections[m_currentSection].type = coil::SectionType::ProgBits;
      } else if (paramNameLower == "nobits") {
        m_sections[m_currentSection].type = coil::SectionType::NoBits;
      } else if (paramNameLower == "symtab") {
        m_sections[m_currentSection].type = coil::SectionType::SymTab;
      } else if (paramNameLower == "strtab") {
        m_sections[m_currentSection].type = coil::SectionType::StrTab;
      } else if (paramNameLower == "write") {
        m_sections[m_currentSection].flags = 
          m_sections[m_currentSection].flags | coil::SectionFlag::Write;
      } else if (paramNameLower == "code") {
        m_sections[m_currentSection].flags = 
          m_sections[m_currentSection].flags | coil::SectionFlag::Code;
      } else if (paramNameLower == "alloc") {
        m_sections[m_currentSection].flags = 
          m_sections[m_currentSection].flags | coil::SectionFlag::Alloc;
      } else {
        error("Unknown section parameter: " + paramName);
      }
    }
    
    return;
  }
  
  // Add label if present
  if (!label.empty()) {
    addLabel(label);
  }
  
  // Handle global directive
  if (name == "global") {
    if (directive.getOperands().empty()) {
      error("Global directive requires a label operand");
      return;
    }
    
    const Operand* op = directive.getOperands()[0].get();
    if (op->getType() != Operand::Type::Label) {
      error("Global symbol must be a label reference");
      return;
    }
    
    const std::string& label = static_cast<const LabelOperand*>(op)->getLabel();
    addGlobalSymbol(label);
    return;
  }
  
  // Ensure we have a current section
  ensureCurrentSection();
  
  // Handle data directives
  if (name == "i8" || name == "u8" || name == "i16" || name == "u16" || 
      name == "i32" || name == "u32" || name == "i64" || name == "u64" || 
      name == "f32" || name == "f64") {
    
    // Determine value type
    coil::ValueType type = stringToValueType(name);
    
    // Process all operands as values
    for (const auto& op : directive.getOperands()) {
      if (op->getType() != Operand::Type::Immediate) {
        error("Data directive operand must be an immediate value");
        continue;
      }
      
      const ImmediateOperand* immOp = static_cast<const ImmediateOperand*>(op.get());
      addImmediate(immOp->getValue(), type);
    }
    
    return;
  }
  
  // Handle string directives
  if (name == "ascii" || name == "asciiz") {
    for (const auto& op : directive.getOperands()) {
      if (op->getType() != Operand::Type::Immediate) {
        error("String operand must be an immediate value");
        continue;
      }
      
      const ImmediateOperand* immOp = static_cast<const ImmediateOperand*>(op.get());
      const ImmediateValue& value = immOp->getValue();
      
      if (value.format != ImmediateFormat::String) {
        error("String operand must be a string literal");
        continue;
      }
      
      const std::string& str = std::get<std::string>(value.value);
      addString(str, name == "asciiz");
    }
    
    return;
  }
  
  // Handle zero directive
  if (name == "zero") {
    if (directive.getOperands().empty()) {
      error("Zero directive requires a size operand");
      return;
    }
    
    const Operand* op = directive.getOperands()[0].get();
    if (op->getType() != Operand::Type::Immediate) {
      error("Zero size must be an immediate value");
      return;
    }
    
    const ImmediateOperand* immOp = static_cast<const ImmediateOperand*>(op);
    const ImmediateValue& value = immOp->getValue();
    
    if (value.format != ImmediateFormat::Integer) {
      error("Zero size must be an integer");
      return;
    }
    
    size_t zeroSize = static_cast<size_t>(std::get<i64>(value.value));
    std::vector<uint8_t> zeros(zeroSize, 0);
    m_sections[m_currentSection].addData(zeros);
    
    return;
  }
  
  error("Unknown directive: " + name);
}

void Assembler::resolveReferences() {
  log("Resolving label references");
  
  for (const auto& ref : m_labelReferences) {
    // Find label in section
    if (m_sections.find(ref.section) == m_sections.end()) {
      error("Unknown section for label reference: " + ref.section);
      continue;
    }
    
    SectionContext& section = m_sections[ref.section];
    
    // Get label value
    size_t labelOffset = 0;
    bool found = false;
    
    // Check if it's a local label in the same section
    if (section.labels.find(ref.label) != section.labels.end()) {
      labelOffset = section.labels[ref.label];
      found = true;
    }
    
    // Check if it's a global symbol in another section
    if (!found && m_globalSymbols.find(ref.label) != m_globalSymbols.end()) {
      const std::string& symSection = m_globalSymbols[ref.label];
      if (m_sections.find(symSection) != m_sections.end() && 
          m_sections[symSection].labels.find(ref.label) != m_sections[symSection].labels.end()) {
        
        labelOffset = m_sections[symSection].labels[ref.label];
        found = true;
      }
    }
    
    if (!found) {
      error("Undefined label: " + ref.label);
      continue;
    }
    
    // Calculate value to write
    uint64_t value = labelOffset;
    
    // If relative addressing, adjust value
    if (ref.isRelative) {
      // Calculate relative offset
      // In real implementation, this would depend on instruction encoding
      // For simplicity, we'll calculate from the end of the reference
      int64_t relativeOffset = static_cast<int64_t>(labelOffset) - 
                              static_cast<int64_t>(ref.offset + ref.size);
      
      value = static_cast<uint64_t>(relativeOffset);
    }
    
    // Write value to section data
    if (ref.offset + ref.size > section.data.size()) {
      error("Label reference offset out of bounds");
      continue;
    }
    
    // Write value in little-endian format
    for (size_t i = 0; i < ref.size; ++i) {
      section.data[ref.offset + i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
    }
    
    log("Resolved label '" + ref.label + "' to value 0x" + 
        std::to_string(value) + " at offset " + std::to_string(ref.offset) + 
        " in section '" + ref.section + "'");
  }
}

coil::Operand Assembler::convertOperand(const Operand& operand, coil::ValueType defaultType) {
  switch (operand.getType()) {
    case Operand::Type::Register: {
      const RegisterOperand* regOp = static_cast<const RegisterOperand*>(&operand);
      uint32_t regIndex = getRegisterIndex(regOp->getName());
      return coil::createRegOp(regIndex, defaultType);
    }
    
    case Operand::Type::Immediate: {
      const ImmediateOperand* immOp = static_cast<const ImmediateOperand*>(&operand);
      
      // Convert immediate value to COIL value
      const ImmediateValue& value = immOp->getValue();
      
      if (value.format == ImmediateFormat::Integer) {
        return coil::createImmOpInt(std::get<i64>(value.value), defaultType);
      } else if (value.format == ImmediateFormat::Float) {
        return coil::createImmOpFp(std::get<f64>(value.value), defaultType);
      } else if (value.format == ImmediateFormat::Character) {
        return coil::createImmOpInt(std::get<char>(value.value), defaultType);
      } else {
        // Can't represent string as immediate operand
        error("String immediate not supported as operand");
        return coil::createImmOpInt(0, defaultType);
      }
    }
    
    case Operand::Type::Memory: {
      const MemoryOperand* memOp = static_cast<const MemoryOperand*>(&operand);
      const MemoryReference& memRef = memOp->getReference();
      
      uint32_t regIndex = getRegisterIndex(memRef.reg);
      return coil::createMemOp(regIndex, static_cast<int32_t>(memRef.offset), defaultType);
    }
    
    case Operand::Type::Label: {
      const LabelOperand* labelOp = static_cast<const LabelOperand*>(&operand);
      
      // Create label operand
      // In a real implementation, we would resolve the label to an index
      // For now, just use a placeholder value
      return coil::createLabelOp(0);
    }
    
    default:
      error("Unknown operand type");
      return coil::createImmOpInt(0, defaultType);
  }
}

uint32_t Assembler::getRegisterIndex(const std::string& name) {
  // Extract numeric part of register name
  std::string numStr = name;
  if (numStr.size() > 1 && numStr[0] == 'r') {
    numStr = numStr.substr(1);
  }
  
  try {
    return static_cast<uint32_t>(std::stoul(numStr));
  } catch (const std::exception& e) {
    error("Invalid register name: " + name);
    return 0;
  }
}

coil::ImmediateValue Assembler::convertImmediate(const ImmediateValue& value, coil::ValueType type) {
  coil::ImmediateValue result;
  
  switch (type) {
    case coil::ValueType::I8:
      result.i8_val = static_cast<int8_t>(std::get<i64>(value.value));
      break;
    case coil::ValueType::I16:
      result.i16_val = static_cast<int16_t>(std::get<i64>(value.value));
      break;
    case coil::ValueType::I32:
      result.i32_val = static_cast<int32_t>(std::get<i64>(value.value));
      break;
    case coil::ValueType::I64:
      result.i64_val = std::get<i64>(value.value);
      break;
    case coil::ValueType::U8:
      result.u8_val = static_cast<uint8_t>(std::get<i64>(value.value));
      break;
    case coil::ValueType::U16:
      result.u16_val = static_cast<uint16_t>(std::get<i64>(value.value));
      break;
    case coil::ValueType::U32:
      result.u32_val = static_cast<uint32_t>(std::get<i64>(value.value));
      break;
    case coil::ValueType::U64:
      result.u64_val = static_cast<uint64_t>(std::get<i64>(value.value));
      break;
    case coil::ValueType::F32:
      result.f32_val = static_cast<float>(std::get<f64>(value.value));
      break;
    case coil::ValueType::F64:
      result.f64_val = std::get<f64>(value.value);
      break;
    default:
      error("Unsupported value type for immediate");
      break;
  }
  
  return result;
}

coil::ValueType Assembler::stringToValueType(const std::string& typeStr) {
  if (typeStr == "i8") return coil::ValueType::I8;
  if (typeStr == "i16") return coil::ValueType::I16;
  if (typeStr == "i32") return coil::ValueType::I32;
  if (typeStr == "i64") return coil::ValueType::I64;
  if (typeStr == "u8") return coil::ValueType::U8;
  if (typeStr == "u16") return coil::ValueType::U16;
  if (typeStr == "u32") return coil::ValueType::U32;
  if (typeStr == "u64") return coil::ValueType::U64;
  if (typeStr == "f32") return coil::ValueType::F32;
  if (typeStr == "f64") return coil::ValueType::F64;
  
  error("Unknown type string: " + typeStr);
  return coil::ValueType::I32; // Default to I32
}

void Assembler::error(const std::string& message) {
  m_errors.push_back(message);
  std::cerr << "Error: " << message << std::endl;
}

void Assembler::log(const std::string& message) {
  if (m_verbose) {
    std::cout << message << std::endl;
  }
}

} // namespace casm