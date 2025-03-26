#include "code_generator.h"
#include <iostream>
#include <iomanip>

CodeGenerator::CodeGenerator(SymbolTable& symbols) : symbols(symbols), currentOffset(0) {}

void CodeGenerator::generateInstruction(const Instruction& inst) {
    // Add debug information
    std::cout << "Generating instruction: opcode=0x" << std::hex << (int)inst.opcode 
              << " with " << std::dec << inst.operands.size() << " operands" << std::endl;
    
    // Generate binary for the instruction
    output.push_back(inst.opcode);
    output.push_back(static_cast<uint8_t>(inst.operands.size()));
    
    for (const auto& operand : inst.operands) {
        generateOperand(operand);
    }
    
    currentOffset = output.size();
    
    // Print the current output buffer size for debugging
    std::cout << "Current output size: " << output.size() << " bytes" << std::endl;
    
    // Print a hex dump of the last instruction
    size_t startOffset = output.size() >= 16 ? output.size() - 16 : 0;
    std::cout << "Last instruction hex dump:" << std::endl;
    for (size_t i = startOffset; i < output.size(); i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)output[i] << " ";
        if ((i - startOffset + 1) % 8 == 0) std::cout << std::endl;
    }
    std::cout << std::dec << std::endl;
}

void CodeGenerator::generateOperand(const Operand& operand) {
    // IMPORTANT: COIL spec defines type field in little-endian format
    // Correctly handle the casting and bit shifting
    uint16_t typeField = (static_cast<uint16_t>(operand.type.id) << 8) | operand.type.ext;
    
    // Debug output for the type field
    std::cout << "  Generating operand: type=0x" << std::hex << typeField 
              << " (main=0x" << static_cast<int>(static_cast<uint16_t>(operand.type.id)) 
              << ", ext=0x" << static_cast<int>(operand.type.ext) << ")" << std::dec << std::endl;
    
    // Write the type field in little-endian format
    output.push_back(typeField & 0xFF);        // Low byte (type extensions)
    output.push_back((typeField >> 8) & 0xFF); // High byte (main type)
    
    // Debug output for type-specific data
    if (!operand.type.type_data.empty()) {
        std::cout << "  Type-specific data (" << operand.type.type_data.size() << " bytes):";
        for (const auto& byte : operand.type.type_data) {
            std::cout << " " << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
        }
        std::cout << std::dec << std::endl;
    }
    
    // Generate type-specific data if present
    for (uint8_t byte : operand.type.type_data) {
        output.push_back(byte);
    }
    
    // Debug output for operand value
    if (!operand.value.empty()) {
        std::cout << "  Operand value (" << operand.value.size() << " bytes):";
        for (const auto& byte : operand.value) {
            std::cout << " " << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
        }
        std::cout << std::dec << std::endl;
    }
    
    // Generate operand value
    for (uint8_t byte : operand.value) {
        output.push_back(byte);
    }
}

std::vector<uint8_t> CodeGenerator::getBinaryOutput() {
    // Add COIL header at the beginning if not already present
    if (output.size() < 4 || output[0] != 'C' || output[1] != 'O' || output[2] != 'I' || output[3] != 'L') {
        std::vector<uint8_t> withHeader;
        
        // Simple COIL header
        withHeader.push_back('C');   // Magic number "COIL"
        withHeader.push_back('O');
        withHeader.push_back('I');
        withHeader.push_back('L');
        withHeader.push_back(1);     // Major version
        withHeader.push_back(0);     // Minor version
        withHeader.push_back(0);     // Patch version
        withHeader.push_back(0);     // Flags
        // Add 8 more bytes for a 16-byte header (simplified)
        for (int i = 0; i < 8; i++) {
            withHeader.push_back(0);
        }
        
        // Append the original output
        withHeader.insert(withHeader.end(), output.begin(), output.end());
        return withHeader;
    }
    
    return output;
}

uint64_t CodeGenerator::getCurrentOffset() const {
    return currentOffset;
}