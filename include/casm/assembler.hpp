#pragma once
#include "casm/parser.hpp"
#include <coil/coil.hpp>
#include <coil/instr.hpp>
#include <coil/obj.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>
#include <optional>
#include <functional>

namespace casm {

/**
 * @brief Exception thrown during assembly process
 */
class AssemblyException : public CasmException {
public:
    AssemblyException(const std::string& message, const SourceLocation& location = SourceLocation())
        : CasmException("Assembly error: " + message), m_location(location) {}

    const SourceLocation& getLocation() const { return m_location; }

private:
    SourceLocation m_location;
};

/**
 * @brief Result of an assembly operation
 */
struct AssemblyResult {
    coil::Object object;           // The assembled COIL object
    std::vector<std::string> warnings;  // Any warnings generated during assembly
    
    bool success() const { return !object.getHeader().magic[0] == 0; }
};

/**
 * @brief Assembler that converts CASM statements to COIL binary
 */
class Assembler {
public:
    /**
     * @brief Configuration options for the assembler
     */
    struct Options {
        bool verbose = false;              // Enable verbose output
        bool optimize = false;             // Enable optimization
        bool allowUnresolvedSymbols = false; // Allow unresolved symbols (for linking)
        bool emitDebugInfo = false;        // Emit debug information
    };

    /**
     * @brief Construct an assembler with optional configuration
     * @param options Configuration options
     */
    explicit Assembler(const Options& options = Options());
    
    /**
     * @brief Assemble CASM statements into a COIL object file
     * @param statements Statements to assemble
     * @return Assembly result
     */
    AssemblyResult assemble(const std::vector<Statement>& statements);
    
    /**
     * @brief Assemble CASM source code into a COIL object file
     * @param source Source code to assemble
     * @param filename Source filename (for error reporting)
     * @return Assembly result
     */
    AssemblyResult assembleSource(const std::string& source, const std::string& filename = "<input>");
    
    /**
     * @brief Get assembler errors
     * @return Vector of error messages
     */
    const std::vector<std::string>& getErrors() const { return m_errors; }
    
    /**
     * @brief Set error handler
     * @param handler Function to call when errors occur
     */
    void setErrorHandler(std::function<void(const std::string&, const SourceLocation&)> handler) {
        m_errorHandler = std::move(handler);
    }
    
    /**
     * @brief Get current options
     * @return Current assembler options
     */
    const Options& getOptions() const { return m_options; }
    
    /**
     * @brief Update options
     * @param options New options
     */
    void setOptions(const Options& options) { m_options = options; }

private:
    // Forward declarations
    struct Symbol;
    struct Section;
    struct RelocationEntry;
    class AssemblyContext;
    
    /**
     * @brief Symbol information
     */
    struct Symbol {
        std::string name;          // Symbol name
        u64 value = 0;             // Symbol value (usually offset)
        std::string section;       // Section name
        coil::SymbolType type = coil::SymbolType::NoType;  // Symbol type
        coil::SymbolBinding binding = coil::SymbolBinding::Local; // Symbol binding
        bool defined = false;      // Whether symbol is defined
        SourceLocation location;   // Where symbol was defined/referenced
    };
    
    /**
     * @brief Relocation entry for symbol references
     */
    struct RelocationEntry {
        std::string symbolName;    // Referenced symbol name
        std::string sectionName;   // Section containing the reference
        size_t offset;             // Offset within section
        size_t size;               // Size of reference in bytes
        bool isRelative;           // Whether it's a PC-relative reference
        int64_t addend;            // Value to add to symbol value
        SourceLocation location;   // Where the reference occurred
    };
    
    /**
     * @brief Section data
     */
    struct Section {
        std::string name;          // Section name
        std::vector<u8> data;      // Section data
        size_t currentOffset = 0;  // Current offset in the section
        coil::SectionType type = coil::SectionType::ProgBits; // Section type
        coil::SectionFlag flags = coil::SectionFlag::None;   // Section flags
        size_t alignment = 1;      // Section alignment
        
        // Symbol table
        std::unordered_map<std::string, size_t> symbols;
        
        // Helper method to add data with alignment
        void addData(const std::vector<u8>& newData, size_t align = 1) {
            // Pad to alignment if needed
            if (align > 1) {
                size_t padding = (align - (currentOffset % align)) % align;
                data.insert(data.end(), padding, 0);
                currentOffset += padding;
            }
            
            data.insert(data.end(), newData.begin(), newData.end());
            currentOffset += newData.size();
        }
        
        // Helper method to add a single byte
        void addByte(u8 value) {
            data.push_back(value);
            currentOffset++;
        }
        
        // Add bytes with necessary padding for alignment
        void addAlignedBytes(const std::vector<u8>& bytes, size_t alignment) {
            // Add padding bytes if needed
            size_t padding = (alignment - (currentOffset % alignment)) % alignment;
            data.insert(data.end(), padding, 0);
            currentOffset += padding;
            
            // Add actual data
            data.insert(data.end(), bytes.begin(), bytes.end());
            currentOffset += bytes.size();
        }
    };
    
    /**
     * @brief Assembly context containing all state for the assembly process
     */
    class AssemblyContext {
    public:
        explicit AssemblyContext(const Options& options);
        
        // Section management
        void ensureSection(const std::string& name);
        void switchSection(const std::string& name);
        Section& getCurrentSection();
        Section* getSection(const std::string& name);
        
        // Symbol management
        void addSymbol(const std::string& name, const Symbol& symbol);
        Symbol* getSymbol(const std::string& name);
        void markSymbolDefined(const std::string& name, u64 value, const std::string& section);
        void addGlobalSymbol(const std::string& name);
        
        // Relocation management
        void addRelocation(const RelocationEntry& reloc);
        
        // Data operations
        void addImmediate(const ImmediateValue& value, coil::ValueType type);
        void addLabelReference(const std::string& label, size_t size, bool isRelative = false, int64_t addend = 0);
        void addString(const std::string& str, bool nullTerminated = false);
        
        // Get all sections
        const std::unordered_map<std::string, Section>& getSections() const { return m_sections; }
        
        // Get all symbols
        const std::unordered_map<std::string, Symbol>& getSymbols() const { return m_symbols; }
        
        // Get all relocations
        const std::vector<RelocationEntry>& getRelocations() const { return m_relocations; }
        
        // Current section name
        const std::string& getCurrentSectionName() const { return m_currentSection; }
        
        // Options
        const Options& getOptions() const { return m_options; }
        
    private:
        std::unordered_map<std::string, Section> m_sections;
        std::unordered_map<std::string, Symbol> m_symbols;
        std::vector<RelocationEntry> m_relocations;
        std::string m_currentSection;
        const Options& m_options;
    };
    
    // Implementation methods
    
    /**
     * @brief First pass - collect symbols, sections, and calculate sizes
     * @param statements Statements to process
     * @param ctx Assembly context
     */
    void collectSymbols(const std::vector<Statement>& statements, AssemblyContext& ctx);
    
    /**
     * @brief Second pass - generate code and resolve references
     * @param statements Statements to process
     * @param ctx Assembly context
     */
    void generateCode(const std::vector<Statement>& statements, AssemblyContext& ctx);
    
    /**
     * @brief Generate COIL object from assembly context
     * @param ctx Assembly context
     * @return COIL object
     */
    coil::Object generateObject(AssemblyContext& ctx);
    
    /**
     * @brief Process a directive
     * @param directive Directive to process
     * @param label Optional label
     * @param ctx Assembly context
     */
    void processDirective(const Directive& directive, const std::string& label, AssemblyContext& ctx);
    
    /**
     * @brief Process an instruction
     * @param instruction Instruction to process
     * @param label Optional label
     * @param ctx Assembly context
     */
    void processInstruction(const Instruction& instruction, const std::string& label, AssemblyContext& ctx);
    
    /**
     * @brief Encode an instruction to binary
     * @param instr COIL instruction
     * @return Encoded instruction bytes
     */
    std::vector<u8> encodeInstruction(const coil::Instruction& instr);
    
    /**
     * @brief Convert CASM operand to COIL operand
     * @param operand CASM operand
     * @param ctx Assembly context
     * @param defaultType Default value type
     * @return COIL operand and any relocations needed
     */
    coil::Operand convertOperand(const Operand& operand, AssemblyContext& ctx, 
                                coil::ValueType defaultType = coil::ValueType::I32);
    
    /**
     * @brief Convert value type string to COIL value type
     * @param typeStr Type string (e.g., "i32", "f64")
     * @return COIL value type
     */
    coil::ValueType stringToValueType(const std::string& typeStr);
    
    /**
     * @brief Get register index from name
     * @param name Register name (e.g., "r0", "r1")
     * @return Register index
     */
    uint32_t getRegisterIndex(const std::string& name);
    
    /**
     * @brief Report an error
     * @param message Error message
     * @param location Source location
     */
    void error(const std::string& message, const SourceLocation& location = SourceLocation());
    
    /**
     * @brief Log a message if verbose mode is enabled
     * @param message Message to log
     */
    void log(const std::string& message);
    
    // Member variables
    Options m_options;
    std::vector<std::string> m_errors;
    std::function<void(const std::string&, const SourceLocation&)> m_errorHandler;
};

} // namespace casm