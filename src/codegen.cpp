/**
 * @file codegen.cpp
 * @brief Implementation of CASM code generator
 */

#include "casm/codegen.hpp"
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <cctype>
#include <algorithm>

namespace casm {

// Default error callback function
static void defaultErrorCallback(const char* message, size_t line, void* user_data) {
  (void)user_data; // Unused
  fprintf(stderr, "Error at line %zu: %s\n", line, message);
}

CodeGenerator::CodeGenerator() 
  : current_section(nullptr),
    error_callback(defaultErrorCallback),
    error_user_data(nullptr),
    initialized(false) {
  
  // Initialize opcode map (same as parser)
  opcode_map["nop"] = coil::Opcode::Nop;
  opcode_map["br"] = coil::Opcode::Br;
  opcode_map["jump"] = coil::Opcode::Jump;
  opcode_map["call"] = coil::Opcode::Call;
  opcode_map["ret"] = coil::Opcode::Ret;
  
  opcode_map["load"] = coil::Opcode::Load;
  opcode_map["store"] = coil::Opcode::Store;
  opcode_map["push"] = coil::Opcode::Push;
  opcode_map["pop"] = coil::Opcode::Pop;
  
  opcode_map["add"] = coil::Opcode::Add;
  opcode_map["sub"] = coil::Opcode::Sub;
  opcode_map["mul"] = coil::Opcode::Mul;
  opcode_map["div"] = coil::Opcode::Div;
  opcode_map["rem"] = coil::Opcode::Rem;
  opcode_map["inc"] = coil::Opcode::Inc;
  opcode_map["dec"] = coil::Opcode::Dec;
  
  opcode_map["and"] = coil::Opcode::And;
  opcode_map["or"] = coil::Opcode::Or;
  opcode_map["xor"] = coil::Opcode::Xor;
  opcode_map["not"] = coil::Opcode::Not;
  opcode_map["shl"] = coil::Opcode::Shl;
  opcode_map["shr"] = coil::Opcode::Shr;
  opcode_map["sar"] = coil::Opcode::Sar;
  
  opcode_map["cmp"] = coil::Opcode::Cmp;
  opcode_map["test"] = coil::Opcode::Test;
  
  // Initialize condition map (same as parser)
  condition_map["eq"] = coil::InstrFlag0::EQ;
  condition_map["neq"] = coil::InstrFlag0::NEQ;
  condition_map["gt"] = coil::InstrFlag0::GT;
  condition_map["gte"] = coil::InstrFlag0::GTE;
  condition_map["lt"] = coil::InstrFlag0::LT;
  condition_map["lte"] = coil::InstrFlag0::LTE;
}

CodeGenerator::~CodeGenerator() {
  // Nothing to clean up
}

Result CodeGenerator::initialize() {
  // Create default sections
  Section text_section;
  text_section.name = ".text";
  text_section.type = coil::SectionType::ProgBits;
  text_section.flags = coil::SectionFlag::Code | coil::SectionFlag::Alloc;
  
  Section data_section;
  data_section.name = ".data";
  data_section.type = coil::SectionType::ProgBits;
  data_section.flags = coil::SectionFlag::Write | coil::SectionFlag::Alloc;
  
  Section bss_section;
  bss_section.name = ".bss";
  bss_section.type = coil::SectionType::NoBits;
  bss_section.flags = coil::SectionFlag::Write | coil::SectionFlag::Alloc;
  
  // Add sections to map
  sections[".text"] = text_section;
  sections[".data"] = data_section;
  sections[".bss"] = bss_section;
  
  // Set current section to .text
  current_section = &sections[".text"];
  
  initialized = true;
  return Result::Success;
}

Result CodeGenerator::generateLine(const Line& line) {
  if (!initialized) {
    formatError("Code generator not initialized");
    reportError(error_buffer, line.line_number);
    return Result::BadState;
  }
  
  // Define label if present
  if (!line.label.empty()) {
    Result result = defineLabel(line.label, line.line_number);
    if (result != Result::Success) {
      return result;
    }
  }
  
  // Generate code based on line type
  switch (line.type) {
    case LineType::Empty:
    case LineType::Label:
      // Nothing to do for empty lines or label-only lines
      return Result::Success;
      
    case LineType::Directive:
      return processDirective(*line.directive, line.line_number);
      
    case LineType::Instruction:
      return generateInstruction(*line.instruction, line.line_number);
      
    default:
      formatError("Unknown line type");
      reportError(error_buffer, line.line_number);
      return Result::InvalidArg;
  }
}

Result CodeGenerator::finalize(coil::Object& obj) {
  if (!initialized) {
    formatError("Code generator not initialized");
    reportError(error_buffer, 0);
    return Result::BadState;
  }
  
  // Initialize the object
  obj = coil::Object::create();
  
  // Initialize string table
  Result result = obj.initStringTable();
  if (result != Result::Success) {
    formatError("Failed to initialize string table");
    reportError(error_buffer, 0);
    return result;
  }
  
  // Initialize symbol table
  result = obj.initSymbolTable();
  if (result != Result::Success) {
    formatError("Failed to initialize symbol table");
    reportError(error_buffer, 0);
    return result;
  }
  
  // Process each section
  for (auto& section_pair : sections) {
    Section& section = section_pair.second;
    
    // Add the section name to the string table
    uint64_t name_offset = obj.addString(section.name.c_str());
    if (name_offset == 0) {
      formatError("Failed to add section name to string table: %s", section.name.c_str());
      reportError(error_buffer, 0);
      return Result::IoError;
    }
    
    // Add the section to the object
    uint16_t section_index = obj.addSection(
      name_offset,
      static_cast<uint16_t>(section.flags),
      static_cast<uint8_t>(section.type),
      section.data.size(),
      section.data.data(),
      section.data.size()
    );
    
    if (section_index == 0) {
      formatError("Failed to add section to object: %s", section.name.c_str());
      reportError(error_buffer, 0);
      return Result::IoError;
    }
  }
  
  // Process each symbol
  for (auto& symbol_pair : symbols) {
    Symbol& symbol = symbol_pair.second;
    
    // Skip undefined symbols
    if (!symbol.is_defined) {
      formatError("Undefined symbol: %s", symbol.name.c_str());
      reportError(error_buffer, symbol.refs.empty() ? 0 : symbol.refs[0]);
      return Result::InvalidFormat;
    }
    
    // Add the symbol name to the string table
    uint64_t name_offset = obj.addString(symbol.name.c_str());
    if (name_offset == 0) {
      formatError("Failed to add symbol name to string table: %s", symbol.name.c_str());
      reportError(error_buffer, 0);
      return Result::IoError;
    }
    
    // Get section index
    uint16_t section_index = 0;
    for (auto& section_pair : sections) {
      if (section_pair.second.name == symbol.name) {
        // Section symbol
        std::string section_name = symbol.name;
        section_index = obj.getSectionIndex(section_name.c_str(), section_name.length());
        break;
      }
    }
    
    // Add the symbol to the object
    coil::SymbolBinding binding = symbol.is_global ? coil::SymbolBinding::Global : coil::SymbolBinding::Local;
    
    uint16_t symbol_index = obj.addSymbol(
      name_offset,
      symbol.value,
      section_index,
      static_cast<uint8_t>(symbol.type),
      static_cast<uint8_t>(binding)
    );
    
    if (symbol_index == 0) {
      formatError("Failed to add symbol to object: %s", symbol.name.c_str());
      reportError(error_buffer, 0);
      return Result::IoError;
    }
  }
  
  return Result::Success;
}

void CodeGenerator::setErrorCallback(void (*callback)(const char* message, size_t line, void* user_data), void* user_data) {
  error_callback = callback ? callback : defaultErrorCallback;
  error_user_data = user_data;
}

const std::string& CodeGenerator::getLastError() const {
  return last_error;
}

Result CodeGenerator::generateInstruction(const Instruction& instr, size_t instr_line) {
  if (!current_section) {
    formatError("No active section for instruction");
    reportError(error_buffer, instr_line);
    return Result::BadState;
  }
  
  // Check if this is a code section
  if (!(current_section->flags & coil::SectionFlag::Code)) {
    formatError("Instruction in non-code section: %s", current_section->name.c_str());
    reportError(error_buffer, instr_line);
    return Result::InvalidFormat;
  }
  
  // Look up opcode
  auto opcode_it = opcode_map.find(instr.name);
  if (opcode_it == opcode_map.end()) {
    formatError("Unknown instruction: %s", instr.name.c_str());
    reportError(error_buffer, instr_line);
    return Result::InvalidFormat;
  }
  
  coil::Opcode opcode = opcode_it->second;
  
  // Look up condition flag
  coil::InstrFlag0 flag = coil::InstrFlag0::None;
  if (!instr.condition.empty()) {
    auto flag_it = condition_map.find(instr.condition);
    if (flag_it == condition_map.end()) {
      formatError("Unknown condition: %s", instr.condition.c_str());
      reportError(error_buffer, instr_line);
      return Result::InvalidFormat;
    }
    flag = flag_it->second;
  }
  
  // Convert operands
  std::vector<coil::Operand> operands;
  for (const Operand& src_op : instr.operands) {
    coil::Operand dst_op;
    Result result = convertOperand(src_op, dst_op, instr_line);
    if (result != Result::Success) {
      return result;
    }
    operands.push_back(dst_op);
  }
  
  // Create instruction based on number of operands
  coil::Instruction coil_instr;
  switch (operands.size()) {
    case 0:
      coil_instr = coil::createInstr(opcode, flag);
      break;
      
    case 1:
      coil_instr = coil::createInstr(opcode, operands[0], flag);
      break;
      
    case 2:
      coil_instr = coil::createInstr(opcode, operands[0], operands[1], flag);
      break;
      
    case 3:
      coil_instr = coil::createInstr(opcode, operands[0], operands[1], operands[2], flag);
      break;
      
    default:
      formatError("Too many operands for instruction: %s", instr.name.c_str());
      reportError(error_buffer, instr_line);
      return Result::InvalidFormat;
  }
  
  // Add instruction to current section
  // For now, we'll just add the serialized instruction to the section data
  // In a real implementation, we'd use a more structured approach
  
  uint32_t instr_offset = current_section->current_offset;
  size_t instr_size = sizeof(coil::Instruction);
  
  // Ensure section data has enough space
  if (current_section->data.size() < instr_offset + instr_size) {
    current_section->data.resize(instr_offset + instr_size);
  }
  
  // Copy instruction to section data
  memcpy(current_section->data.data() + instr_offset, &coil_instr, instr_size);
  
  // Update section offset
  current_section->current_offset += instr_size;
  
  return Result::Success;
}

Result CodeGenerator::processDirective(const Directive& dir, size_t dir_line) {
  // Remove leading dot from directive name
  std::string name = dir.name.substr(1); // Skip the leading dot
  
  if (name == "section") {
    // Section directive: .section <name>
    if (dir.args.size() < 1) {
      formatError("Missing section name");
      reportError(error_buffer, dir_line);
      return Result::InvalidFormat;
    }
    
    std::string section_name = dir.args[0].text;
    
    // Check if section exists
    auto it = sections.find(section_name);
    if (it == sections.end()) {
      // Create new section with default flags
      Section section;
      section.name = section_name;
      
      if (section_name == ".text") {
        section.type = coil::SectionType::ProgBits;
        section.flags = coil::SectionFlag::Code | coil::SectionFlag::Alloc;
      } else if (section_name == ".data") {
        section.type = coil::SectionType::ProgBits;
        section.flags = coil::SectionFlag::Write | coil::SectionFlag::Alloc;
      } else if (section_name == ".bss") {
        section.type = coil::SectionType::NoBits;
        section.flags = coil::SectionFlag::Write | coil::SectionFlag::Alloc;
      } else {
        // Default to progbits
        section.type = coil::SectionType::ProgBits;
        section.flags = coil::SectionFlag::None;
      }
      
      sections[section_name] = section;
      current_section = &sections[section_name];
    } else {
      // Use existing section
      current_section = &it->second;
    }
    
    return Result::Success;
  } else if (name == "global" || name == "local") {
    // Symbol visibility directive: .global <symbol> or .local <symbol>
    if (dir.args.size() < 1) {
      formatError("Missing symbol name");
      reportError(error_buffer, dir_line);
      return Result::InvalidFormat;
    }
    
    std::string symbol_name = dir.args[0].text;
    bool is_global = (name == "global");
    
    // Update symbol visibility
    auto it = symbols.find(symbol_name);
    if (it == symbols.end()) {
      // Create new symbol
      Symbol symbol;
      symbol.name = symbol_name;
      symbol.is_global = is_global;
      symbol.type = coil::SymbolType::NoType;
      symbol.refs.push_back(dir_line);
      symbols[symbol_name] = symbol;
    } else {
      // Update existing symbol
      it->second.is_global = is_global;
      it->second.refs.push_back(dir_line);
    }
    
    return Result::Success;
  } else if (name == "align") {
    // Alignment directive: .align <value>
    if (dir.args.size() < 1 || dir.args[0].type != TokenType::Integer) {
      formatError("Missing or invalid alignment value");
      reportError(error_buffer, dir_line);
      return Result::InvalidFormat;
    }
    
    uint32_t alignment = std::stoul(dir.args[0].text);
    return alignSection(alignment);
  } else if (name == "i8" || name == "i16" || name == "i32" || name == "i64" ||
             name == "u8" || name == "u16" || name == "u32" || name == "u64") {
    // Integer directive: .i8/.i16/.i32/.i64/.u8/.u16/.u32/.u64 <values...>
    if (dir.args.empty()) {
      formatError("Missing values for integer directive");
      reportError(error_buffer, dir_line);
      return Result::InvalidFormat;
    }
    
    // Determine size and signedness
    uint32_t size = 0;
    bool is_signed = (name[0] == 'i');
    
    if (name.substr(1) == "8") size = 1;
    else if (name.substr(1) == "16") size = 2;
    else if (name.substr(1) == "32") size = 4;
    else if (name.substr(1) == "64") size = 8;
    
    // Process each value
    for (const Token& arg : dir.args) {
      if (arg.type != TokenType::Integer) {
        formatError("Invalid value for integer directive: %s", arg.text.c_str());
        reportError(error_buffer, dir_line);
        return Result::InvalidFormat;
      }
      
      int64_t value = std::stoll(arg.text);
      
      // Check value range
      if (is_signed) {
        if (size == 1 && (value < -128 || value > 127)) {
          formatError("Value %lld out of range for i8", (long long)value);
          reportError(error_buffer, dir_line);
          return Result::InvalidFormat;
        } else if (size == 2 && (value < -32768 || value > 32767)) {
          formatError("Value %lld out of range for i16", (long long)value);
          reportError(error_buffer, dir_line);
          return Result::InvalidFormat;
        } else if (size == 4 && (value < -2147483648LL || value > 2147483647LL)) {
          formatError("Value %lld out of range for i32", (long long)value);
          reportError(error_buffer, dir_line);
          return Result::InvalidFormat;
        }
      } else {
        if (value < 0) {
          formatError("Negative value %lld for unsigned type", (long long)value);
          reportError(error_buffer, dir_line);
          return Result::InvalidFormat;
        } else if (size == 1 && value > 255) {
          formatError("Value %lld out of range for u8", (long long)value);
          reportError(error_buffer, dir_line);
          return Result::InvalidFormat;
        } else if (size == 2 && value > 65535) {
          formatError("Value %lld out of range for u16", (long long)value);
          reportError(error_buffer, dir_line);
          return Result::InvalidFormat;
        } else if (size == 4 && value > 4294967295LL) {
          formatError("Value %lld out of range for u32", (long long)value);
          reportError(error_buffer, dir_line);
          return Result::InvalidFormat;
        }
      }
      
      // Ensure section data has enough space
      uint32_t value_offset = current_section->current_offset;
      if (current_section->data.size() < value_offset + size) {
        current_section->data.resize(value_offset + size);
      }
      
      // Copy value to section data
      if (size == 1) {
        current_section->data[value_offset] = static_cast<uint8_t>(value);
      } else if (size == 2) {
        uint16_t val16 = static_cast<uint16_t>(value);
        memcpy(current_section->data.data() + value_offset, &val16, size);
      } else if (size == 4) {
        uint32_t val32 = static_cast<uint32_t>(value);
        memcpy(current_section->data.data() + value_offset, &val32, size);
      } else if (size == 8) {
        uint64_t val64 = static_cast<uint64_t>(value);
        memcpy(current_section->data.data() + value_offset, &val64, size);
      }
      
      // Update section offset
      current_section->current_offset += size;
    }
    
    return Result::Success;
  } else if (name == "f32" || name == "f64") {
    // Float directive: .f32/.f64 <values...>
    if (dir.args.empty()) {
      formatError("Missing values for float directive");
      reportError(error_buffer, dir_line);
      return Result::InvalidFormat;
    }
    
    // Determine size
    uint32_t size = (name == "f32") ? 4 : 8;
    
    // Process each value
    for (const Token& arg : dir.args) {
      if (arg.type != TokenType::Float && arg.type != TokenType::Integer) {
        formatError("Invalid value for float directive: %s", arg.text.c_str());
        reportError(error_buffer, dir_line);
        return Result::InvalidFormat;
      }
      
      double value = std::stod(arg.text);
      
      // Ensure section data has enough space
      uint32_t value_offset = current_section->current_offset;
      if (current_section->data.size() < value_offset + size) {
        current_section->data.resize(value_offset + size);
      }
      
      // Copy value to section data
      if (size == 4) {
        float val32 = static_cast<float>(value);
        memcpy(current_section->data.data() + value_offset, &val32, size);
      } else if (size == 8) {
        memcpy(current_section->data.data() + value_offset, &value, size);
      }
      
      // Update section offset
      current_section->current_offset += size;
    }
    
    return Result::Success;
  } else if (name == "string") {
    // String directive: .string <string>
    if (dir.args.size() < 1 || dir.args[0].type != TokenType::String) {
      formatError("Missing or invalid string literal");
      reportError(error_buffer, dir_line);
      return Result::InvalidFormat;
    }
    
    // Extract string content (remove quotes)
    std::string text = dir.args[0].text;
    if (text.size() < 2) {
      formatError("Invalid string literal: %s", text.c_str());
      reportError(error_buffer, dir_line);
      return Result::InvalidFormat;
    }
    
    char quote = text[0];
    if (quote != '"' && quote != '\'') {
      formatError("Invalid string literal: %s", text.c_str());
      reportError(error_buffer, dir_line);
      return Result::InvalidFormat;
    }
    
    // Remove quotes and handle escape sequences
    std::string content;
    for (size_t i = 1; i < text.size() - 1; i++) {
      if (text[i] == '\\' && i + 1 < text.size() - 1) {
        // Handle escape sequence
        char next = text[++i];
        switch (next) {
          case 'n': content += '\n'; break;
          case 'r': content += '\r'; break;
          case 't': content += '\t'; break;
          case '0': content += '\0'; break;
          case '\\': content += '\\'; break;
          case '\'': content += '\''; break;
          case '"': content += '"'; break;
          default: content += next; break;
        }
      } else {
        content += text[i];
      }
    }
    
    // Add null terminator
    content += '\0';
    
    // Ensure section data has enough space
    uint32_t string_offset = current_section->current_offset;
    if (current_section->data.size() < string_offset + content.size()) {
      current_section->data.resize(string_offset + content.size());
    }
    
    // Copy string to section data
    memcpy(current_section->data.data() + string_offset, content.data(), content.size());
    
    // Update section offset
    current_section->current_offset += content.size();
    
    return Result::Success;
  } else if (name == "bytes") {
    // Bytes directive: .bytes <values...>
    if (dir.args.empty()) {
      formatError("Missing values for bytes directive");
      reportError(error_buffer, dir_line);
      return Result::InvalidFormat;
    }
    
    // Process each value
    for (const Token& arg : dir.args) {
      if (arg.type != TokenType::Integer) {
        formatError("Invalid value for bytes directive: %s", arg.text.c_str());
        reportError(error_buffer, dir_line);
        return Result::InvalidFormat;
      }
      
      int64_t value = std::stoll(arg.text, nullptr, 0);
      
      // Check value range
      if (value < 0 || value > 255) {
        formatError("Value %lld out of range for byte", (long long)value);
        reportError(error_buffer, dir_line);
        return Result::InvalidFormat;
      }
      
      // Ensure section data has enough space
      uint32_t value_offset = current_section->current_offset;
      if (current_section->data.size() < value_offset + 1) {
        current_section->data.resize(value_offset + 1);
      }
      
      // Copy value to section data
      current_section->data[value_offset] = static_cast<uint8_t>(value);
      
      // Update section offset
      current_section->current_offset++;
    }
    
    return Result::Success;
  } else if (name == "space") {
    // Space directive: .space <size>
    if (dir.args.size() < 1 || dir.args[0].type != TokenType::Integer) {
      formatError("Missing or invalid size value");
      reportError(error_buffer, dir_line);
      return Result::InvalidFormat;
    }
    
    uint32_t size = std::stoul(dir.args[0].text);
    
    // Ensure section data has enough space
    uint32_t space_offset = current_section->current_offset;
    if (current_section->data.size() < space_offset + size) {
      current_section->data.resize(space_offset + size);
    }
    
    // Zero-initialize the space
    memset(current_section->data.data() + space_offset, 0, size);
    
    // Update section offset
    current_section->current_offset += size;
    
    return Result::Success;
  } else {
    // Unknown directive
    formatError("Unknown directive: %s", dir.name.c_str());
    reportError(error_buffer, dir_line);
    return Result::InvalidFormat;
  }
}

Result CodeGenerator::defineLabel(const std::string& label, size_t label_line) {
  if (!current_section) {
    formatError("No active section for label: %s", label.c_str());
    reportError(error_buffer, label_line);
    return Result::BadState;
  }
  
  // Check if symbol already exists
  auto it = symbols.find(label);
  if (it != symbols.end()) {
    if (it->second.is_defined) {
      formatError("Symbol already defined: %s", label.c_str());
      reportError(error_buffer, label_line);
      return Result::AlreadyExists;
    }
    
    // Update existing symbol
    it->second.value = current_section->current_offset;
    it->second.section = 0; // TODO: Store section index
    it->second.is_defined = true;
    it->second.type = coil::SymbolType::Func; // Assume labels are functions by default
  } else {
    // Create new symbol
    Symbol symbol;
    symbol.name = label;
    symbol.value = current_section->current_offset;
    symbol.section = 0; // TODO: Store section index
    symbol.is_defined = true;
    symbol.is_global = false;
    symbol.type = coil::SymbolType::Func; // Assume labels are functions by default
    symbols[label] = symbol;
  }
  
  return Result::Success;
}

Result CodeGenerator::defineSymbol(const std::string& name, uint32_t value, uint16_t section_index, bool is_global, coil::SymbolType sym_type, size_t sym_line) {
  // Check if symbol already exists
  auto it = symbols.find(name);
  if (it != symbols.end()) {
    if (it->second.is_defined) {
      formatError("Symbol already defined: %s", name.c_str());
      reportError(error_buffer, sym_line);
      return Result::AlreadyExists;
    }
    
    // Update existing symbol
    it->second.value = value;
    it->second.section = section_index;
    it->second.is_defined = true;
    it->second.is_global = is_global;
    it->second.type = sym_type;
  } else {
    // Create new symbol
    Symbol symbol;
    symbol.name = name;
    symbol.value = value;
    symbol.section = section_index;
    symbol.is_defined = true;
    symbol.is_global = is_global;
    symbol.type = sym_type;
    symbols[name] = symbol;
  }
  
  return Result::Success;
}

uint16_t CodeGenerator::referenceSymbol(const std::string& name, size_t ref_line) {
  // Check if symbol exists
  auto it = symbols.find(name);
  if (it == symbols.end()) {
    // Create undefined symbol
    Symbol symbol;
    symbol.name = name;
    symbol.is_defined = false;
    symbol.refs.push_back(ref_line);
    symbols[name] = symbol;
    return 0;
  }
  
  // Add reference to existing symbol
  it->second.refs.push_back(ref_line);
  
  // Return symbol index (placeholder for now)
  return 1;
}

uint16_t CodeGenerator::getOrCreateSection(const std::string& name, coil::SectionType type, coil::SectionFlag flags) {
  // Check if section exists
  auto it = sections.find(name);
  if (it == sections.end()) {
    // Create new section
    Section section;
    section.name = name;
    section.type = type;
    section.flags = flags;
    sections[name] = section;
  }
  
  // Return section index (placeholder for now)
  return 1;
}

Result CodeGenerator::convertOperand(const Operand& src, coil::Operand& dst, size_t inst_line) {
  switch (src.type) {
    case Operand::Type::Register: {
      // Convert register operand
      dst = coil::createRegOp(src.reg.number, src.reg.type);
      return Result::Success;
    }
    
    case Operand::Type::Immediate: {
      // Convert immediate operand
      if (src.imm.type == coil::ValueType::F32 || src.imm.type == coil::ValueType::F64) {
        dst = coil::createImmOpFp(src.imm.f_value, src.imm.type);
      } else {
        dst = coil::createImmOpInt(src.imm.i_value, src.imm.type);
      }
      return Result::Success;
    }
    
    case Operand::Type::Memory: {
      // Convert memory operand
      dst = coil::createMemOp(src.mem.base.number, src.mem.offset, src.mem.type);
      return Result::Success;
    }
    
    case Operand::Type::Label: {
      // Convert label operand
      if (!src.label) {
        formatError("Null label reference");
        reportError(error_buffer, inst_line);
        return Result::InvalidArg;
      }
      
      // Reference the symbol (to track usage)
      uint16_t sym_index = referenceSymbol(*src.label, inst_line);
      
      // For now, just create a label operand with the raw index
      dst = coil::createLabelOp(sym_index);
      return Result::Success;
    }
    
    default:
      formatError("Unknown operand type: %d", static_cast<int>(src.type));
      reportError(error_buffer, inst_line);
      return Result::InvalidArg;
  }
}

Result CodeGenerator::alignSection(uint32_t alignment) {
  if (!current_section) {
    formatError("No active section for alignment");
    reportError(error_buffer, 0);
    return Result::BadState;
  }
  
  // Check if alignment is a power of 2
  if ((alignment & (alignment - 1)) != 0) {
    formatError("Alignment must be a power of 2: %u", alignment);
    reportError(error_buffer, 0);
    return Result::InvalidArg;
  }
  
  // Calculate aligned offset
  uint32_t current_offset = current_section->current_offset;
  uint32_t aligned_offset = (current_offset + alignment - 1) & ~(alignment - 1);
  uint32_t padding = aligned_offset - current_offset;
  
  if (padding > 0) {
    // Add padding bytes
    if (current_section->data.size() < aligned_offset) {
      current_section->data.resize(aligned_offset);
    }
    
    // Zero-initialize padding
    memset(current_section->data.data() + current_offset, 0, padding);
    
    // Update section offset
    current_section->current_offset = aligned_offset;
  }
  
  return Result::Success;
}

void CodeGenerator::reportError(const char* message, size_t line) {
  last_error = message;
  error_callback(message, line, error_user_data);
}

void CodeGenerator::formatError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vsnprintf(error_buffer, sizeof(error_buffer), format, args);
  va_end(args);
}

} // namespace casm