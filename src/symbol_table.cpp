#include "casm/symbol_table.h"
#include <algorithm>

namespace casm {

// Scope implementation
Scope::Scope(std::string name, std::shared_ptr<Scope> parent)
  : name_(std::move(name)), parent_(parent) {
}

const std::string& Scope::getName() const {
  return name_;
}

std::shared_ptr<Scope> Scope::getParent() const {
  return parent_;
}

bool Scope::addSymbol(const SymbolInfo& symbol) {
  // Check if symbol already exists in this scope
  if (symbols_.find(symbol.name) != symbols_.end()) {
      return false; // Symbol already exists
  }
  
  symbols_[symbol.name] = symbol;
  return true;
}

bool Scope::hasSymbol(const std::string& name) const {
  return symbols_.find(name) != symbols_.end();
}

SymbolInfo* Scope::getSymbol(const std::string& name) {
  auto it = symbols_.find(name);
  if (it != symbols_.end()) {
      return &it->second;
  }
  
  return nullptr;
}

std::vector<SymbolInfo> Scope::getAllSymbols() const {
  std::vector<SymbolInfo> result;
  
  for (const auto& [name, info] : symbols_) {
      result.push_back(info);
  }
  
  return result;
}

// SymbolTable implementation
SymbolTable::SymbolTable() {
  // Create global scope
  globalScope_ = std::make_shared<Scope>("global");
  currentScope_ = globalScope_;
  scopeStack_.push_back(globalScope_);
}

void SymbolTable::enterScope(const std::string& name) {
  // Create new scope with current scope as parent
  std::string scopeName = currentScope_->getName() + "." + name;
  auto newScope = std::make_shared<Scope>(scopeName, currentScope_);
  
  // Update current scope
  currentScope_ = newScope;
  scopeStack_.push_back(newScope);
}

void SymbolTable::leaveScope() {
  // Ensure we don't leave the global scope
  if (scopeStack_.size() <= 1) {
      return;
  }
  
  // Remove current scope from stack
  scopeStack_.pop_back();
  
  // Update current scope
  currentScope_ = scopeStack_.back();
}

bool SymbolTable::addSymbol(const SymbolInfo& symbol) {
  return currentScope_->addSymbol(symbol);
}

bool SymbolTable::hasSymbol(const std::string& name) const {
  // Start with current scope
  std::shared_ptr<Scope> scope = currentScope_;
  
  // Check each scope in the hierarchy
  while (scope) {
      if (scope->hasSymbol(name)) {
          return true;
      }
      
      scope = scope->getParent();
  }
  
  return false;
}

SymbolInfo* SymbolTable::getSymbol(const std::string& name) {
  // Start with current scope
  std::shared_ptr<Scope> scope = currentScope_;
  
  // Check each scope in the hierarchy
  while (scope) {
      SymbolInfo* symbol = scope->getSymbol(name);
      if (symbol) {
          return symbol;
      }
      
      scope = scope->getParent();
  }
  
  return nullptr;
}

std::vector<SymbolInfo> SymbolTable::getAllSymbols() const {
  std::vector<SymbolInfo> result;
  
  // Process each scope
  for (const auto& scope : scopeStack_) {
      std::vector<SymbolInfo> scopeSymbols = scope->getAllSymbols();
      result.insert(result.end(), scopeSymbols.begin(), scopeSymbols.end());
  }
  
  return result;
}

std::vector<SymbolInfo> SymbolTable::getGlobalSymbols() const {
  return globalScope_->getAllSymbols();
}

std::shared_ptr<Scope> SymbolTable::getCurrentScope() const {
  return currentScope_;
}

std::shared_ptr<Scope> SymbolTable::getGlobalScope() const {
  return globalScope_;
}

uint16_t SymbolTable::addToCoilObject(coil::CoilObject& coilObj) {
  uint16_t symbolCount = 0;
  
  // Process each symbol in the symbol table
  for (const auto& symbolInfo : getAllSymbols()) {
      // Create COIL symbol
      coil::Symbol symbol;
      symbol.name = symbolInfo.name;
      symbol.name_length = static_cast<uint16_t>(symbolInfo.name.length());
      symbol.attributes = symbolInfo.attributes;
      symbol.value = symbolInfo.address;
      symbol.section_index = symbolInfo.sectionIndex;
      symbol.processor_type = 0; // Default processor type
      
      // Add symbol to COIL object
      coilObj.addSymbol(symbol);
      symbolCount++;
  }
  
  return symbolCount;
}

} // namespace casm