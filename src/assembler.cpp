#include <casm/assembler.hpp>
#include <casm/lexer.hpp>
#include <casm/parser.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <array>
#include <functional>

namespace casm {

//
// Assembler implementation
//

Assembler::Assembler(const Options& options)
    : m_options(options) {
    // Initialize COIL library - this should already be initialized by the main program
    if (!coil::Library::instance().isInitialized()) {
        coil::initialize();
    }
}

AssemblyResult Assembler::assemble(const std::vector<Statement>& statements) {
    // Clear any previous state
    m_errors.clear();
    
    // Create assembly context
    AssemblyContext ctx(m_options);
    
    try {
        // First pass - collect symbols
        collectSymbols(statements, ctx);
        
        // Second pass - generate code
        generateCode(statements, ctx);
        
        // Generate object
        coil::Object obj = generateObject(ctx);
        
        // Return result
        AssemblyResult result;
        result.object = std::move(obj);
        return result;
    }
    catch (const AssemblyException& e) {
        error(e.what(), e.getLocation());
        AssemblyResult result;
        return result;
    }
    catch (const std::exception& e) {
        error(std::string("Unexpected error: ") + e.what());
        AssemblyResult result;
        return result;
    }
}

AssemblyResult Assembler::assembleSource(const std::string& source, const std::string& filename) {
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
        return AssemblyResult(); // Return empty result
    }
    
    // Assemble the statements
    return assemble(statements);
}

void Assembler::collectSymbols(const std::vector<Statement>& statements, AssemblyContext& ctx) {
    log("First pass - collecting symbols and calculating sizes");
    
    // Default to .text section if none specified
    ctx.ensureSection(".text");
    
    for (const auto& stmt : statements) {
        // Skip empty statements
        if (stmt.getType() == Statement::Type::Empty) {
            continue;
        }
        
        // Process label-only statements
        if (stmt.getType() == Statement::Type::Label) {
            Section& section = ctx.getCurrentSection();
            const std::string& label = stmt.getLabel();
            
            // Define the symbol at the current offset
            Symbol sym;
            sym.name = label;
            sym.value = section.currentOffset;
            sym.section = ctx.getCurrentSectionName();
            sym.type = coil::SymbolType::NoType;
            sym.binding = coil::SymbolBinding::Local;
            sym.defined = true;
            
            ctx.addSymbol(label, sym);
            continue;
        }
        
        // Process directives that affect section layout
        if (stmt.getType() == Statement::Type::Directive) {
            const Directive* directive = stmt.getDirective();
            if (!directive) continue;
            
            const std::string& name = directive->getName();
            
            // Handle section directive
            if (name == "section") {
                if (directive->getOperands().empty()) {
                    error("Section directive requires a name operand");
                    continue;
                }
                
                // Get section name
                std::string sectionName;
                const Operand* op = directive->getOperands()[0].get();
                
                if (op->getType() == Operand::Type::Label) {
                    sectionName = static_cast<const LabelOperand*>(op)->getLabel();
                } else if (op->getType() == Operand::Type::Immediate) {
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
                } else {
                    error("Section name must be a label reference or immediate value");
                    continue;
                }
                
                // Switch to the new section
                ctx.switchSection(sectionName);
                
                // Process section flags and attributes
                for (size_t i = 1; i < directive->getOperands().size(); ++i) {
                    const Operand* param = directive->getOperands()[i].get();
                    if (param->getType() != Operand::Type::Label) {
                        error("Section parameter must be a label reference");
                        continue;
                    }
                    
                    const std::string& paramName = static_cast<const LabelOperand*>(param)->getLabel();
                    std::string paramNameLower = paramName;
                    std::transform(paramNameLower.begin(), paramNameLower.end(), paramNameLower.begin(), ::tolower);
                    
                    Section& section = ctx.getCurrentSection();
                    
                    // Set section type
                    if (paramNameLower == "progbits") {
                        section.type = coil::SectionType::ProgBits;
                    } else if (paramNameLower == "nobits") {
                        section.type = coil::SectionType::NoBits;
                    } else if (paramNameLower == "symtab") {
                        section.type = coil::SectionType::SymTab;
                    } else if (paramNameLower == "strtab") {
                        section.type = coil::SectionType::StrTab;
                    }
                    // Set section flags
                    else if (paramNameLower == "write") {
                        section.flags = section.flags | coil::SectionFlag::Write;
                    } else if (paramNameLower == "code") {
                        section.flags = section.flags | coil::SectionFlag::Code;
                    } else if (paramNameLower == "alloc") {
                        section.flags = section.flags | coil::SectionFlag::Alloc;
                    } else if (paramNameLower == "merge") {
                        section.flags = section.flags | coil::SectionFlag::Merge;
                    } else if (paramNameLower == "tls") {
                        section.flags = section.flags | coil::SectionFlag::TLS;
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
                ctx.addGlobalSymbol(label);
                continue;
            }
            
            // Handle data directives - estimate size
            if (name == "i8" || name == "u8") {
                ctx.getCurrentSection().currentOffset += directive->getOperands().size();
                continue;
            }
            
            if (name == "i16" || name == "u16") {
                ctx.getCurrentSection().currentOffset += directive->getOperands().size() * 2;
                continue;
            }
            
            if (name == "i32" || name == "u32" || name == "f32") {
                ctx.getCurrentSection().currentOffset += directive->getOperands().size() * 4;
                continue;
            }
            
            if (name == "i64" || name == "u64" || name == "f64") {
                ctx.getCurrentSection().currentOffset += directive->getOperands().size() * 8;
                continue;
            }
            
            // Handle string directives
            if (name == "ascii" || name == "asciiz") {
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
                    
                    ctx.getCurrentSection().currentOffset += strSize;
                }
                continue;
            }
            
            // Handle zero directive (reserve space)
            if (name == "zero") {
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
                ctx.getCurrentSection().currentOffset += zeroSize;
                continue;
            }
            
            // Handle align directive
            if (name == "align") {
                if (directive->getOperands().empty()) {
                    error("Align directive requires an alignment operand");
                    continue;
                }
                
                const Operand* op = directive->getOperands()[0].get();
                if (op->getType() != Operand::Type::Immediate) {
                    error("Alignment must be an immediate value");
                    continue;
                }
                
                const ImmediateOperand* immOp = static_cast<const ImmediateOperand*>(op);
                const ImmediateValue& value = immOp->getValue();
                
                if (value.format != ImmediateFormat::Integer) {
                    error("Alignment must be an integer");
                    continue;
                }
                
                size_t alignment = static_cast<size_t>(std::get<i64>(value.value));
                if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
                    error("Alignment must be a power of 2");
                    continue;
                }
                
                // Calculate padding needed
                Section& section = ctx.getCurrentSection();
                size_t padding = (alignment - (section.currentOffset % alignment)) % alignment;
                section.currentOffset += padding;
                continue;
            }
            
            // Add label to section if present with directive
            if (!stmt.getLabel().empty()) {
                Section& section = ctx.getCurrentSection();
                const std::string& label = stmt.getLabel();
                
                // Define the symbol at the current offset
                Symbol sym;
                sym.name = label;
                sym.value = section.currentOffset;
                sym.section = ctx.getCurrentSectionName();
                sym.type = coil::SymbolType::NoType;
                sym.binding = coil::SymbolBinding::Local;
                sym.defined = true;
                
                ctx.addSymbol(label, sym);
            }
        }
        
        // Process instructions - calculate size
        if (stmt.getType() == Statement::Type::Instruction) {
            Section& section = ctx.getCurrentSection();
            
            // Add label if present
            if (!stmt.getLabel().empty()) {
                const std::string& label = stmt.getLabel();
                
                // Define the symbol at the current offset
                Symbol sym;
                sym.name = label;
                sym.value = section.currentOffset;
                sym.section = ctx.getCurrentSectionName();
                sym.type = coil::SymbolType::NoType;
                sym.binding = coil::SymbolBinding::Local;
                sym.defined = true;
                
                ctx.addSymbol(label, sym);
            }
            
            // Estimate instruction size - will be refined in code generation
            section.currentOffset += 8; // Conservative estimate
        }
    }
    
    log("Symbol collection complete");
}

void Assembler::generateCode(const std::vector<Statement>& statements, AssemblyContext& ctx) {
    log("Second pass - generating code");
    
    // Reset section data
    auto& sections = const_cast<std::unordered_map<std::string, Section>&>(ctx.getSections());
    for (auto& [name, section] : sections) {
        section.data.clear();
        section.currentOffset = 0;
    }
    
    // Reset current section
    ctx.switchSection(".text");
    
    for (const auto& stmt : statements) {
        // Skip empty statements
        if (stmt.getType() == Statement::Type::Empty) {
            continue;
        }
        
        // Process label-only statements
        if (stmt.getType() == Statement::Type::Label) {
            // Update symbol value to current offset
            Symbol* sym = ctx.getSymbol(stmt.getLabel());
            if (sym) {
                sym->value = ctx.getCurrentSection().currentOffset;
                sym->defined = true;
                sym->section = ctx.getCurrentSectionName();
            }
            continue;
        }
        
        // Process directives
        if (stmt.getType() == Statement::Type::Directive) {
            const Directive* directive = stmt.getDirective();
            if (!directive) continue;
            
            // Process directive
            processDirective(*directive, stmt.getLabel(), ctx);
            continue;
        }
        
        // Process instructions
        if (stmt.getType() == Statement::Type::Instruction) {
            const Instruction* instruction = stmt.getInstruction();
            if (!instruction) continue;
            
            // Process instruction
            processInstruction(*instruction, stmt.getLabel(), ctx);
            continue;
        }
    }
    
    log("Code generation complete");
}

coil::Object Assembler::generateObject(AssemblyContext& ctx) {
    log("Generating COIL object");
    
    // Create COIL object
    coil::Object obj = coil::Object::create();
    
    // Initialize string table
    obj.initStringTable();
    
    // Add section names to string table and create sections
    for (const auto& [name, section] : ctx.getSections()) {
        // Skip empty sections
        if ((section.type != coil::SectionType::NoBits && section.data.empty()) ||
            (section.type == coil::SectionType::NoBits && section.currentOffset == 0)) {
            continue;
        }
        
        // Add section name to string table
        u64 nameOffset = obj.addString(name);
        
        // Add section to object
        uint16_t flags = static_cast<uint16_t>(section.flags);
        uint8_t type = static_cast<uint8_t>(section.type);
        
        obj.addSection(nameOffset, flags, type, section.data.size(), section.data);
        
        log("Added section '" + name + "', size: " + std::to_string(section.data.size()) + 
            " bytes, type: " + std::to_string(type) + ", flags: 0x" + 
            std::to_string(flags));
    }
    
    // Initialize symbol table
    obj.initSymbolTable();
    
    // Add symbols to object
    for (const auto& [name, symbol] : ctx.getSymbols()) {
        // Skip symbols that weren't defined if we don't allow unresolved symbols
        if (!symbol.defined && !ctx.getOptions().allowUnresolvedSymbols) {
            error("Undefined symbol: " + name);
            continue;
        }
        
        // Skip local symbols in sections that weren't added
        auto section = ctx.getSection(symbol.section);
        if (!section) {
            continue;
        }
        
        // Get section index
        uint16_t sectionIndex = obj.getSectionIndex(symbol.section);
        if (sectionIndex == 0) {
            error("Could not find section '" + symbol.section + "' for symbol '" + name + "'");
            continue;
        }
        
        // Add symbol name to string table
        u64 nameOffset = obj.addString(name);
        
        // Add symbol to object
        obj.addSymbol(
            nameOffset,                                    // name
            static_cast<u32>(symbol.value),                // value
            sectionIndex,                                  // section_index
            static_cast<u8>(symbol.type),                  // type
            static_cast<u8>(symbol.binding)                // binding
        );
        
        log("Added symbol '" + name + "' at offset " + std::to_string(symbol.value) + 
            " in section '" + symbol.section + "'");
    }
    
    log("COIL object generation complete");
    return obj;
}

void Assembler::processDirective(const Directive& directive, const std::string& label, AssemblyContext& ctx) {
    const std::string& name = directive.getName();
    
    // Handle section directive
    if (name == "section") {
        if (directive.getOperands().empty()) {
            error("Section directive requires a name operand");
            return;
        }
        
        // Get section name
        std::string sectionName;
        const Operand* op = directive.getOperands()[0].get();
        
        if (op->getType() == Operand::Type::Label) {
            sectionName = static_cast<const LabelOperand*>(op)->getLabel();
        } else if (op->getType() == Operand::Type::Immediate) {
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
        } else {
            error("Section name must be a label reference or immediate value");
            return;
        }
        
        // Switch to the new section
        ctx.switchSection(sectionName);
        
        // Process section flags and attributes
        for (size_t i = 1; i < directive.getOperands().size(); ++i) {
            const Operand* param = directive.getOperands()[i].get();
            if (param->getType() != Operand::Type::Label) {
                error("Section parameter must be a label reference");
                continue;
            }
            
            const std::string& paramName = static_cast<const LabelOperand*>(param)->getLabel();
            std::string paramNameLower = paramName;
            std::transform(paramNameLower.begin(), paramNameLower.end(), paramNameLower.begin(), ::tolower);
            
            Section& section = ctx.getCurrentSection();
            
            // Set section type
            if (paramNameLower == "progbits") {
                section.type = coil::SectionType::ProgBits;
            } else if (paramNameLower == "nobits") {
                section.type = coil::SectionType::NoBits;
            } else if (paramNameLower == "symtab") {
                section.type = coil::SectionType::SymTab;
            } else if (paramNameLower == "strtab") {
                section.type = coil::SectionType::StrTab;
            }
            // Set section flags
            else if (paramNameLower == "write") {
                section.flags = section.flags | coil::SectionFlag::Write;
            } else if (paramNameLower == "code") {
                section.flags = section.flags | coil::SectionFlag::Code;
            } else if (paramNameLower == "alloc") {
                section.flags = section.flags | coil::SectionFlag::Alloc;
            } else if (paramNameLower == "merge") {
                section.flags = section.flags | coil::SectionFlag::Merge;
            } else if (paramNameLower == "tls") {
                section.flags = section.flags | coil::SectionFlag::TLS;
            } else {
                error("Unknown section parameter: " + paramName);
            }
        }
        
        return;
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
        ctx.addGlobalSymbol(label);
        return;
    }
    
    // Add label if present
    if (!label.empty()) {
        Symbol* sym = ctx.getSymbol(label);
        if (sym) {
            sym->value = ctx.getCurrentSection().currentOffset;
            sym->defined = true;
            sym->section = ctx.getCurrentSectionName();
        }
    }
    
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
            ctx.addImmediate(immOp->getValue(), type);
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
            ctx.addString(str, name == "asciiz");
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
        std::vector<u8> zeros(zeroSize, 0);
        ctx.getCurrentSection().addData(zeros);
        
        return;
    }
    
    // Handle align directive
    if (name == "align") {
        if (directive.getOperands().empty()) {
            error("Align directive requires an alignment operand");
            return;
        }
        
        const Operand* op = directive.getOperands()[0].get();
        if (op->getType() != Operand::Type::Immediate) {
            error("Alignment must be an immediate value");
            return;
        }
        
        const ImmediateOperand* immOp = static_cast<const ImmediateOperand*>(op);
        const ImmediateValue& value = immOp->getValue();
        
        if (value.format != ImmediateFormat::Integer) {
            error("Alignment must be an integer");
            return;
        }
        
        size_t alignment = static_cast<size_t>(std::get<i64>(value.value));
        if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
            error("Alignment must be a power of 2");
            return;
        }
        
        // Calculate padding needed
        Section& section = ctx.getCurrentSection();
        size_t padding = (alignment - (section.currentOffset % alignment)) % alignment;
        std::vector<u8> zeros(padding, 0);
        section.addData(zeros);
        
        return;
    }
    
    error("Unknown directive: " + name);
}

void Assembler::processInstruction(const Instruction& instruction, const std::string& label, AssemblyContext& ctx) {
    // Add label if present
    if (!label.empty()) {
        Symbol* sym = ctx.getSymbol(label);
        if (sym) {
            sym->value = ctx.getCurrentSection().currentOffset;
            sym->defined = true;
            sym->section = ctx.getCurrentSectionName();
            sym->type = coil::SymbolType::Func;  // Assume function
        }
    }
    
    // Get instruction name (convert to lowercase)
    std::string name = instruction.getName();
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    
    // Get instruction parameters
    const auto& params = instruction.getParameters();
    
    // Convert parameters to flags
    coil::InstrFlag0 flag0 = coil::InstrFlag0::None;
    for (const auto& param : params) {
        if (param == "eq") flag0 = coil::InstrFlag0::EQ;
        else if (param == "neq") flag0 = coil::InstrFlag0::NEQ;
        else if (param == "gt") flag0 = coil::InstrFlag0::GT;
        else if (param == "gte") flag0 = coil::InstrFlag0::GTE;
        else if (param == "lt") flag0 = coil::InstrFlag0::LT;
        else if (param == "lte") flag0 = coil::InstrFlag0::LTE;
    }
    
    // Map instruction name to COIL opcode
    coil::Opcode opcode;
    
    if (name == "nop") opcode = coil::Opcode::Nop;
    else if (name == "jmp") opcode = coil::Opcode::Jump;
    else if (name == "br") opcode = coil::Opcode::Br;
    else if (name == "call") opcode = coil::Opcode::Call;
    else if (name == "ret") opcode = coil::Opcode::Ret;
    else if (name == "load") opcode = coil::Opcode::Load;
    else if (name == "store") opcode = coil::Opcode::Store;
    else if (name == "push") opcode = coil::Opcode::Push;
    else if (name == "pop") opcode = coil::Opcode::Pop;
    else if (name == "mov") opcode = coil::Opcode::Mov;
    else if (name == "add") opcode = coil::Opcode::Add;
    else if (name == "sub") opcode = coil::Opcode::Sub;
    else if (name == "mul") opcode = coil::Opcode::Mul;
    else if (name == "div") opcode = coil::Opcode::Div;
    else if (name == "rem") opcode = coil::Opcode::Rem;
    else if (name == "inc") opcode = coil::Opcode::Inc;
    else if (name == "dec") opcode = coil::Opcode::Dec;
    else if (name == "neg") opcode = coil::Opcode::Neg;
    else if (name == "and") opcode = coil::Opcode::And;
    else if (name == "or") opcode = coil::Opcode::Or;
    else if (name == "xor") opcode = coil::Opcode::Xor;
    else if (name == "not") opcode = coil::Opcode::Not;
    else if (name == "shl") opcode = coil::Opcode::Shl;
    else if (name == "shr") opcode = coil::Opcode::Shr;
    else if (name == "sar") opcode = coil::Opcode::Sar;
    else if (name == "cmp") opcode = coil::Opcode::Cmp;
    else if (name == "test") opcode = coil::Opcode::Test;
    else if (name == "cvt") opcode = coil::Opcode::Cvt;
    else {
        error("Unknown instruction: " + name);
        return;
    }
    
    // Create COIL instruction based on operand count
    coil::Instruction coilInstr;
    coilInstr.opcode = opcode;
    coilInstr.flag0 = flag0;
    
    const auto& operands = instruction.getOperands();
    size_t opCount = operands.size();
    
    if (opCount == 0) {
        // No operands (e.g., nop, ret)
    }
    else if (opCount == 1) {
        // One operand (e.g., push, pop, jmp)
        coilInstr.dest = convertOperand(*operands[0], ctx);
    }
    else if (opCount == 2) {
        // Two operands (e.g., mov, load, store)
        coilInstr.dest = convertOperand(*operands[0], ctx);
        coilInstr.src1 = convertOperand(*operands[1], ctx);
    }
    else if (opCount == 3) {
        // Three operands (e.g., add, sub, mul)
        coilInstr.dest = convertOperand(*operands[0], ctx);
        coilInstr.src1 = convertOperand(*operands[1], ctx);
        coilInstr.src2 = convertOperand(*operands[2], ctx);
    }
    else {
        error("Too many operands for instruction: " + name);
        return;
    }
    
    // Encode the instruction
    std::vector<u8> encoded = encodeInstruction(coilInstr);
    
    // Add to section
    ctx.getCurrentSection().addData(encoded);
}

std::vector<u8> Assembler::encodeInstruction(const coil::Instruction& instr) {
    // Simple encoding for COIL instruction
    // In a real implementation, this would be more sophisticated
    
    // Encode the instruction into bytes
    std::vector<u8> encoded;
    
    // First byte: opcode
    encoded.push_back(static_cast<u8>(instr.opcode));
    
    // Second byte: flags
    encoded.push_back(static_cast<u8>(instr.flag0));
    
    // Encode operand types (dest, src1, src2)
    u8 opTypes = 0;
    opTypes |= static_cast<u8>(instr.dest.type) << 4;
    opTypes |= static_cast<u8>(instr.src1.type) << 2;
    opTypes |= static_cast<u8>(instr.src2.type);
    encoded.push_back(opTypes);
    
    // Reserve another byte for future use or alignment
    encoded.push_back(0);
    
    // Encode operands
    auto encodeOperand = [&encoded](const coil::Operand& op) {
        switch (op.type) {
            case coil::OperandType::Reg: {
                // Encode register index (4 bytes for alignment)
                encoded.push_back(op.reg & 0xFF);
                encoded.push_back((op.reg >> 8) & 0xFF);
                encoded.push_back((op.reg >> 16) & 0xFF);
                encoded.push_back((op.reg >> 24) & 0xFF);
                break;
            }
            case coil::OperandType::Imm: {
                // Encode immediate value based on type
                switch (op.value_type) {
                    case coil::ValueType::I8:
                    case coil::ValueType::U8:
                        encoded.push_back(op.imm.i8_val);
                        // Pad for alignment
                        encoded.push_back(0);
                        encoded.push_back(0);
                        encoded.push_back(0);
                        break;
                    case coil::ValueType::I16:
                    case coil::ValueType::U16:
                        encoded.push_back(op.imm.i16_val & 0xFF);
                        encoded.push_back((op.imm.i16_val >> 8) & 0xFF);
                        // Pad for alignment
                        encoded.push_back(0);
                        encoded.push_back(0);
                        break;
                    case coil::ValueType::I32:
                    case coil::ValueType::U32:
                    case coil::ValueType::F32:
                        encoded.push_back(op.imm.i32_val & 0xFF);
                        encoded.push_back((op.imm.i32_val >> 8) & 0xFF);
                        encoded.push_back((op.imm.i32_val >> 16) & 0xFF);
                        encoded.push_back((op.imm.i32_val >> 24) & 0xFF);
                        break;
                    case coil::ValueType::I64:
                    case coil::ValueType::U64:
                    case coil::ValueType::F64:
                        encoded.push_back(op.imm.i64_val & 0xFF);
                        encoded.push_back((op.imm.i64_val >> 8) & 0xFF);
                        encoded.push_back((op.imm.i64_val >> 16) & 0xFF);
                        encoded.push_back((op.imm.i64_val >> 24) & 0xFF);
                        // For 64-bit values, we truncate to 32 bits for now
                        break;
                    default:
                        // For other types, encode as zeros
                        encoded.push_back(0);
                        encoded.push_back(0);
                        encoded.push_back(0);
                        encoded.push_back(0);
                        break;
                }
                break;
            }
            case coil::OperandType::Mem: {
                // Encode memory reference (base register and offset)
                encoded.push_back(op.mem.base & 0xFF);
                encoded.push_back((op.mem.base >> 8) & 0xFF);
                encoded.push_back(op.mem.offset & 0xFF);
                encoded.push_back((op.mem.offset >> 8) & 0xFF);
                break;
            }
            case coil::OperandType::Label: {
                // Encode label reference (index)
                encoded.push_back(op.label & 0xFF);
                encoded.push_back((op.label >> 8) & 0xFF);
                encoded.push_back((op.label >> 16) & 0xFF);
                encoded.push_back((op.label >> 24) & 0xFF);
                break;
            }
            default:
                // For None or other types, encode as zeros
                encoded.push_back(0);
                encoded.push_back(0);
                encoded.push_back(0);
                encoded.push_back(0);
                break;
        }
    };
    
    // Encode operands if present
    if (instr.dest.type != coil::OperandType::None) {
        encodeOperand(instr.dest);
    }
    
    if (instr.src1.type != coil::OperandType::None) {
        encodeOperand(instr.src1);
    }
    
    if (instr.src2.type != coil::OperandType::None) {
        encodeOperand(instr.src2);
    }
    
    return encoded;
}

coil::Operand Assembler::convertOperand(const Operand& operand, AssemblyContext& ctx, coil::ValueType defaultType) {
    switch (operand.getType()) {
        case Operand::Type::Register: {
            const RegisterOperand* regOp = static_cast<const RegisterOperand*>(&operand);
            uint32_t regIndex = getRegisterIndex(regOp->getName());
            return coil::createRegOp(regIndex, defaultType);
        }
        
        case Operand::Type::Immediate: {
            const ImmediateOperand* immOp = static_cast<const ImmediateOperand*>(&operand);
            const ImmediateValue& value = immOp->getValue();
            
            // Convert immediate value based on format
            if (value.format == ImmediateFormat::Integer) {
                return coil::createImmOpInt(std::get<i64>(value.value), defaultType);
            } else if (value.format == ImmediateFormat::Float) {
                return coil::createImmOpFp(std::get<f64>(value.value), 
                    defaultType == coil::ValueType::F32 || defaultType == coil::ValueType::F64 
                    ? defaultType : coil::ValueType::F64);
            } else if (value.format == ImmediateFormat::Character) {
                return coil::createImmOpInt(std::get<char>(value.value), defaultType);
            } else {
                // Can't represent string as immediate operand directly
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
            const std::string& labelName = labelOp->getLabel();
            
            // Add a label reference
            // For COIL, we need to create a proper label reference with relocation
            // In a real implementation, this would involve relocation entries
            
            Symbol* sym = ctx.getSymbol(labelName);
            if (sym && sym->defined) {
                // Symbol is already defined
                if (sym->section == ctx.getCurrentSectionName()) {
                    // Same section, can use relative addressing
                    i64 relativeOffset = sym->value - ctx.getCurrentSection().currentOffset - 4;
                    return coil::createImmOpInt(relativeOffset, coil::ValueType::I32);
                } else {
                    // Different section, need absolute reference
                    return coil::createImmOpInt(sym->value, coil::ValueType::I32);
                }
            } else {
                // Symbol not yet defined or from another section
                // Add a relocation entry
                RelocationEntry reloc;
                reloc.symbolName = labelName;
                reloc.sectionName = ctx.getCurrentSectionName();
                reloc.offset = ctx.getCurrentSection().currentOffset;
                reloc.size = 4;  // 32-bit reference
                reloc.isRelative = false;
                reloc.addend = 0;
                
                ctx.addRelocation(reloc);
                
                // Return a placeholder value
                return coil::createImmOpInt(0, coil::ValueType::I32);
            }
        }
        
        default:
            error("Unknown operand type");
            return coil::createImmOpInt(0, defaultType);
    }
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

void Assembler::error(const std::string& message, const SourceLocation& location) {
    std::ostringstream ss;
    
    if (!location.filename.empty()) {
        ss << location.filename << ":";
        
        if (location.line > 0) {
            ss << location.line << ":";
            
            if (location.column > 0) {
                ss << location.column << ":";
            }
        }
        
        ss << " ";
    }
    
    ss << message;
    std::string fullMessage = ss.str();
    
    m_errors.push_back(fullMessage);
    
    // Call error handler if set
    if (m_errorHandler) {
        m_errorHandler(message, location);
    }
    
    // Log to stderr
    if (m_options.verbose) {
        std::cerr << "Error: " << fullMessage << std::endl;
    }
}

void Assembler::log(const std::string& message) {
    if (m_options.verbose) {
        std::cout << message << std::endl;
    }
}

//
// AssemblyContext implementation
//

Assembler::AssemblyContext::AssemblyContext(const Options& options)
    : m_options(options) {
    // Initialize with empty state
}

void Assembler::AssemblyContext::ensureSection(const std::string& name) {
    if (m_sections.find(name) == m_sections.end()) {
        switchSection(name);
    } else {
        m_currentSection = name;
    }
}

void Assembler::AssemblyContext::switchSection(const std::string& name) {
    m_currentSection = name;
    
    // Create section if it doesn't exist
    if (m_sections.find(name) == m_sections.end()) {
        Section section;
        section.name = name;
        
        // Set default flags based on section name
        if (name == ".text") {
            section.flags = coil::SectionFlag::Code | coil::SectionFlag::Alloc;
            section.type = coil::SectionType::ProgBits;
        } else if (name == ".data") {
            section.flags = coil::SectionFlag::Write | coil::SectionFlag::Alloc;
            section.type = coil::SectionType::ProgBits;
        } else if (name == ".bss") {
            section.flags = coil::SectionFlag::Write | coil::SectionFlag::Alloc;
            section.type = coil::SectionType::NoBits;
        } else if (name == ".rodata") {
            section.flags = coil::SectionFlag::Alloc;
            section.type = coil::SectionType::ProgBits;
        }
        
        m_sections[name] = section;
    }
}

Assembler::Section& Assembler::AssemblyContext::getCurrentSection() {
    if (m_currentSection.empty()) {
        ensureSection(".text");
    }
    
    return m_sections[m_currentSection];
}

Assembler::Section* Assembler::AssemblyContext::getSection(const std::string& name) {
    auto it = m_sections.find(name);
    if (it != m_sections.end()) {
        return &it->second;
    }
    return nullptr;
}

void Assembler::AssemblyContext::addSymbol(const std::string& name, const Symbol& symbol) {
    // Check if symbol already exists
    auto it = m_symbols.find(name);
    if (it != m_symbols.end()) {
        // Update existing symbol if it's not defined yet
        if (!it->second.defined) {
            it->second = symbol;
        } else if (symbol.defined) {
            // Symbol already defined - error
            throw AssemblyException("Symbol already defined: " + name, symbol.location);
        }
    } else {
        // Add new symbol
        m_symbols[name] = symbol;
    }
}

Assembler::Symbol* Assembler::AssemblyContext::getSymbol(const std::string& name) {
    auto it = m_symbols.find(name);
    if (it != m_symbols.end()) {
        return &it->second;
    }
    return nullptr;
}

void Assembler::AssemblyContext::markSymbolDefined(const std::string& name, u64 value, const std::string& section) {
    // Get or create symbol
    Symbol& sym = m_symbols[name];
    
    // Update symbol info
    sym.name = name;
    sym.value = value;
    sym.section = section;
    sym.defined = true;
}

void Assembler::AssemblyContext::addGlobalSymbol(const std::string& name) {
    // Get or create symbol
    Symbol& sym = m_symbols[name];
    
    // Mark as global
    sym.name = name;
    sym.binding = coil::SymbolBinding::Global;
}

void Assembler::AssemblyContext::addRelocation(const RelocationEntry& reloc) {
    m_relocations.push_back(reloc);
}

void Assembler::AssemblyContext::addImmediate(const ImmediateValue& value, coil::ValueType type) {
    Section& section = getCurrentSection();
    
    switch (type) {
        case coil::ValueType::I8:
        case coil::ValueType::U8: {
            uint8_t byteValue = 0;
            
            if (value.format == ImmediateFormat::Integer) {
                byteValue = static_cast<uint8_t>(std::get<i64>(value.value));
            } else if (value.format == ImmediateFormat::Character) {
                byteValue = static_cast<uint8_t>(std::get<char>(value.value));
            }
            
            section.addByte(byteValue);
            break;
        }
        
        case coil::ValueType::I16:
        case coil::ValueType::U16: {
            uint16_t shortValue = 0;
            
            if (value.format == ImmediateFormat::Integer) {
                shortValue = static_cast<uint16_t>(std::get<i64>(value.value));
            } else if (value.format == ImmediateFormat::Character) {
                shortValue = static_cast<uint16_t>(std::get<char>(value.value));
            }
            
            section.addByte(shortValue & 0xFF);
            section.addByte((shortValue >> 8) & 0xFF);
            break;
        }
        
        case coil::ValueType::I32:
        case coil::ValueType::U32: {
            uint32_t wordValue = 0;
            
            if (value.format == ImmediateFormat::Integer) {
                wordValue = static_cast<uint32_t>(std::get<i64>(value.value));
            } else if (value.format == ImmediateFormat::Character) {
                wordValue = static_cast<uint32_t>(std::get<char>(value.value));
            } else if (value.format == ImmediateFormat::Float) {
                float floatValue = static_cast<float>(std::get<f64>(value.value));
                std::memcpy(&wordValue, &floatValue, sizeof(float));
            }
            
            section.addByte(wordValue & 0xFF);
            section.addByte((wordValue >> 8) & 0xFF);
            section.addByte((wordValue >> 16) & 0xFF);
            section.addByte((wordValue >> 24) & 0xFF);
            break;
        }
        
        case coil::ValueType::I64:
        case coil::ValueType::U64: {
            uint64_t longValue = 0;
            
            if (value.format == ImmediateFormat::Integer) {
                longValue = static_cast<uint64_t>(std::get<i64>(value.value));
            } else if (value.format == ImmediateFormat::Character) {
                longValue = static_cast<uint64_t>(std::get<char>(value.value));
            } else if (value.format == ImmediateFormat::Float) {
                double doubleValue = std::get<f64>(value.value);
                std::memcpy(&longValue, &doubleValue, sizeof(double));
            }
            
            section.addByte(longValue & 0xFF);
            section.addByte((longValue >> 8) & 0xFF);
            section.addByte((longValue >> 16) & 0xFF);
            section.addByte((longValue >> 24) & 0xFF);
            section.addByte((longValue >> 32) & 0xFF);
            section.addByte((longValue >> 40) & 0xFF);
            section.addByte((longValue >> 48) & 0xFF);
            section.addByte((longValue >> 56) & 0xFF);
            break;
        }
        
        case coil::ValueType::F32: {
            float floatValue = 0.0f;
            
            if (value.format == ImmediateFormat::Integer) {
                floatValue = static_cast<float>(std::get<i64>(value.value));
            } else if (value.format == ImmediateFormat::Float) {
                floatValue = static_cast<float>(std::get<f64>(value.value));
            }
            
            // Add float to section (as 4 bytes)
            uint32_t bits;
            std::memcpy(&bits, &floatValue, sizeof(float));
            
            section.addByte(bits & 0xFF);
            section.addByte((bits >> 8) & 0xFF);
            section.addByte((bits >> 16) & 0xFF);
            section.addByte((bits >> 24) & 0xFF);
            break;
        }
        
        case coil::ValueType::F64: {
            double doubleValue = 0.0;
            
            if (value.format == ImmediateFormat::Integer) {
                doubleValue = static_cast<double>(std::get<i64>(value.value));
            } else if (value.format == ImmediateFormat::Float) {
                doubleValue = std::get<f64>(value.value);
            }
            
            // Add double to section (as 8 bytes)
            uint64_t bits;
            std::memcpy(&bits, &doubleValue, sizeof(double));
            
            section.addByte(bits & 0xFF);
            section.addByte((bits >> 8) & 0xFF);
            section.addByte((bits >> 16) & 0xFF);
            section.addByte((bits >> 24) & 0xFF);
            section.addByte((bits >> 32) & 0xFF);
            section.addByte((bits >> 40) & 0xFF);
            section.addByte((bits >> 48) & 0xFF);
            section.addByte((bits >> 56) & 0xFF);
            break;
        }
        
        default:
            throw AssemblyException("Unsupported value type for immediate");
    }
}

void Assembler::AssemblyContext::addLabelReference(const std::string& label, size_t size, bool isRelative, int64_t addend) {
    Section& section = getCurrentSection();
    
    // Add placeholder bytes for the reference
    std::vector<u8> placeholder(size, 0);
    section.addData(placeholder);
    
    // Add relocation entry
    RelocationEntry reloc;
    reloc.symbolName = label;
    reloc.sectionName = m_currentSection;
    reloc.offset = section.currentOffset - size;
    reloc.size = size;
    reloc.isRelative = isRelative;
    reloc.addend = addend;
    
    m_relocations.push_back(reloc);
}

void Assembler::AssemblyContext::addString(const std::string& str, bool nullTerminated) {
    Section& section = getCurrentSection();
    
    // Add string data
    for (char c : str) {
        section.addByte(static_cast<u8>(c));
    }
    
    // Add null terminator if requested
    if (nullTerminated) {
        section.addByte(0);
    }
}

} // namespace casm