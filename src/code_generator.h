#pragma once
#include "types.h"
#include "symbol_table.h"
#include <vector>

class CodeGenerator {
public:
    CodeGenerator(SymbolTable& symbols);
    void generateInstruction(const Instruction& inst);
    std::vector<uint8_t> getBinaryOutput();
    uint64_t getCurrentOffset() const;

private:
    SymbolTable& symbols;
    std::vector<uint8_t> output;
    uint64_t currentOffset;
    
    void generateOperand(const Operand& operand);
};