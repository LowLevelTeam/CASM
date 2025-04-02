#ifndef CASM_SYMBOL_TABLE_H
#define CASM_SYMBOL_TABLE_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <coil/binary_format.h>

namespace casm {

/**
* Symbol type enum
*/
enum class SymbolType {
  LABEL,
  FUNCTION,
  VARIABLE,
  DIRECTIVE,
  SECTION,
  UNKNOWN
};

/**
* Symbol information structure
*/
struct SymbolInfo {
  std::string name;
  SymbolType type;
  uint32_t address;
  uint16_t sectionIndex;
  uint32_t attributes;
  bool defined;
  
  SymbolInfo() 
      : type(SymbolType::UNKNOWN), address(0), sectionIndex(0), 
        attributes(0), defined(false) {}
  
  SymbolInfo(std::string name, SymbolType type, uint32_t address = 0, 
            uint16_t sectionIndex = 0, uint32_t attributes = 0, bool defined = false)
      : name(std::move(name)), type(type), address(address), 
        sectionIndex(sectionIndex), attributes(attributes), defined(defined) {}
};

/**
* Scope for managing symbols
*/
class Scope {
public:
  Scope(std::string name, std::shared_ptr<Scope> parent = nullptr);
  
  const std::string& getName() const;
  std::shared_ptr<Scope> getParent() const;
  
  bool addSymbol(const SymbolInfo& symbol);
  bool hasSymbol(const std::string& name) const;
  SymbolInfo* getSymbol(const std::string& name);
  
  std::vector<SymbolInfo> getAllSymbols() const;
  
private:
  std::string name_;
  std::shared_ptr<Scope> parent_;
  std::unordered_map<std::string, SymbolInfo> symbols_;
};

/**
* Symbol table for managing symbols across scopes
*/
class SymbolTable {
public:
  SymbolTable();
  
  void enterScope(const std::string& name);
  void leaveScope();
  
  bool addSymbol(const SymbolInfo& symbol);
  bool hasSymbol(const std::string& name) const;
  SymbolInfo* getSymbol(const std::string& name);
  
  std::vector<SymbolInfo> getAllSymbols() const;
  std::vector<SymbolInfo> getGlobalSymbols() const;
  
  std::shared_ptr<Scope> getCurrentScope() const;
  std::shared_ptr<Scope> getGlobalScope() const;
  
  uint16_t addToCoilObject(coil::CoilObject& coilObj);
  
private:
  std::shared_ptr<Scope> globalScope_;
  std::shared_ptr<Scope> currentScope_;
  std::vector<std::shared_ptr<Scope>> scopeStack_;
};

} // namespace casm

#endif // CASM_SYMBOL_TABLE_H