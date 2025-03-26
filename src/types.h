#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

// Type enumerations
enum class TypeID : uint16_t {
    // Integer types
    TYPE_INT8    = 0x01,
    TYPE_INT16   = 0x02,
    TYPE_INT32   = 0x03,
    TYPE_INT64   = 0x04,
    TYPE_INT128  = 0x05,
    
    // Unsigned integer types
    TYPE_UNT8    = 0x10,
    TYPE_UNT16   = 0x12,
    TYPE_UNT32   = 0x13,
    TYPE_UNT64   = 0x14,
    TYPE_UNT128  = 0x15,
    
    // Floating point types
    TYPE_FP32    = 0x25,
    TYPE_FP64    = 0x26,
    
    // Vector types
    TYPE_V128    = 0x30,
    TYPE_V256    = 0x31,
    TYPE_V512    = 0x32,

    // Optimized Types
    TYPE_BIT = 0x40,
    
    // Special types
    TYPE_VAR     = 0x90,
    TYPE_SYM     = 0x91,
    TYPE_RGP     = 0x92,
    
    // Platform dependent types
    TYPE_INT     = 0xA0,
    TYPE_UNT     = 0xA1,
    TYPE_FP      = 0xA2,
    TYPE_PTR     = 0xA6,
    
    // Composite types
    TYPE_STRUCT  = 0xD0,
    TYPE_ARRAY   = 0xD3,
    
    // Parameter types
    TYPE_PARAM5  = 0xF0,
    TYPE_PARAM0  = 0xFE,
    TYPE_ABICTL  = 0xF8,
    
    // Void type
    TYPE_VOID    = 0xFF
};

// Type extensions
enum class TypeExt : uint8_t {
    NONE         = 0x00,
    CONST        = 0x01,
    VOLATILE     = 0x02,
    VOID         = 0x10,
    IMM          = 0x20,
    VAR          = 0x40,
    SYM          = 0x80
};

// Token types for lexer
enum class TokenType {
    END_OF_FILE,
    IDENTIFIER,
    NUMBER,
    STRING,
    SYMBOL,
    COLON,
    COMMA,
    LBRACKET,
    RBRACKET,
    EQUALS,
    PLUS,
    MINUS,
    ASTERISK,
    COMMENT,
    DIRECTIVE,
    INSTRUCTION,
    TYPE,
    REGISTER,
    NEWLINE,
    LPAREN,
    RPAREN,
    UNKNOWN
};

// Symbol types
enum class SymbolType {
    UNDEFINED,
    LABEL,
    FUNCTION,
    VARIABLE,
    EXTERNAL
};

// Data structures
struct Token {
    TokenType type;
    std::string value;
    size_t line;
    size_t column;
};

struct Type {
    TypeID id;
    uint8_t ext;
    std::vector<uint8_t> type_data;
};

struct Operand {
    Type type;
    std::vector<uint8_t> value;
};

struct Instruction {
    uint8_t opcode;
    std::vector<Operand> operands;
};

struct Symbol {
    std::string name;
    uint64_t value;
    SymbolType type;
    bool defined;
};