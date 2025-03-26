#include "symbol_table.h"

bool SymbolTable::addSymbol(const std::string& name, uint64_t value, SymbolType type) {
    auto it = symbols.find(name);
    if (it != symbols.end() && it->second.defined) {
        return false;  // Symbol already defined
    }
    
    symbols[name] = Symbol{name, value, type, true};
    return true;
}

bool SymbolTable::lookupSymbol(const std::string& name, Symbol& outSymbol) {
    auto it = symbols.find(name);
    if (it == symbols.end()) {
        return false;
    }
    
    outSymbol = it->second;
    return true;
}

void SymbolTable::defineLabel(const std::string& name, uint64_t offset) {
    auto it = symbols.find(name);
    if (it != symbols.end()) {
        it->second.value = offset;
        it->second.defined = true;
    } else {
        symbols[name] = Symbol{name, offset, SymbolType::LABEL, true};
    }
}

std::unordered_map<std::string, Symbol>& SymbolTable::getSymbols() {
    return symbols;
}