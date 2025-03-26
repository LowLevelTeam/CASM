#pragma once
#include "types.h"
#include <string>
#include <unordered_map>

class SymbolTable {
public:
    bool addSymbol(const std::string& name, uint64_t value, SymbolType type);
    bool lookupSymbol(const std::string& name, Symbol& outSymbol);
    void defineLabel(const std::string& name, uint64_t offset);
    std::unordered_map<std::string, Symbol>& getSymbols();

private:
    std::unordered_map<std::string, Symbol> symbols;
};