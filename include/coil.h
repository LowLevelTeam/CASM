/* include/coil.h - Main COIL interface */
#ifndef COIL_H
#define COIL_H

#include <stdint.h>
#include <stddef.h>

/* COIL version */
#define COIL_VERSION_MAJOR 1
#define COIL_VERSION_MINOR 0
#define COIL_VERSION_PATCH 0

/* COIL error codes - simplified from err.md to focus on assembler/disassembler specific errors */
typedef enum {
  COIL_SUCCESS                 = 0,    /* Operation completed successfully */
  
  /* Format errors (1xxx) */
  COIL_ERR_INVALID_MAGIC       = 1001, /* Invalid magic number */
  COIL_ERR_UNSUPPORTED_VERSION = 1002, /* Unsupported version */
  COIL_ERR_CORRUPTED_HEADER    = 1003, /* Corrupted header */
  COIL_ERR_INVALID_SECTION     = 1004, /* Invalid section */
  COIL_ERR_INVALID_SYMBOL      = 1005, /* Invalid symbol table */
  
  /* Instruction errors (2xxx) */
  COIL_ERR_INVALID_OPCODE      = 2001, /* Invalid opcode */
  COIL_ERR_INVALID_OPERAND_COUNT = 2002, /* Invalid operand count */
  COIL_ERR_INVALID_OPERAND_TYPE = 2003, /* Invalid operand type */
  COIL_ERR_TYPE_MISMATCH       = 2004, /* Type mismatch */
  COIL_ERR_MISSING_OPERAND     = 2005, /* Missing operand */
  COIL_ERR_EXTRA_OPERAND       = 2006, /* Extra operand */
  COIL_ERR_INVALID_INSTRUCTION = 2007, /* Invalid instruction format */
  COIL_ERR_RESERVED_INSTRUCTION = 2008, /* Reserved instruction used */
  
  /* Type system errors (3xxx) */
  COIL_ERR_INVALID_TYPE        = 3001, /* Invalid type encoding */
  COIL_ERR_INCOMPATIBLE_TYPES  = 3002, /* Incompatible types */
  
  /* Symbol errors (5xxx) */
  COIL_ERR_UNDEFINED_SYMBOL    = 5001, /* Undefined symbol */
  COIL_ERR_DUPLICATE_SYMBOL    = 5002, /* Duplicate symbol definition */
  
  /* Assembler specific errors (9xxx) */
  COIL_ERR_SYNTAX_ERROR        = 9001, /* General syntax error */
  COIL_ERR_UNEXPECTED_TOKEN    = 9002, /* Unexpected token */
  COIL_ERR_FILE_IO_ERROR       = 9003, /* File I/O error */
  COIL_ERR_BUFFER_OVERFLOW     = 9004, /* Buffer overflow */
  COIL_ERR_INTERNAL_ERROR      = 9999  /* Internal error */
} coil_error_t;

/* Get string representation of an error code */
const char* coil_error_string(coil_error_t error);

/* COIL object structure - represents a COIL binary file */
typedef struct coil_object_t coil_object_t;

/* Create a new empty COIL object */
coil_object_t* coil_object_create(void);

/* Free a COIL object */
void coil_object_free(coil_object_t* obj);

/* Load a COIL object from a file */
coil_error_t coil_object_load(const char* filename, coil_object_t** obj);

/* Save a COIL object to a file */
coil_error_t coil_object_save(const coil_object_t* obj, const char* filename);

#endif /* COIL_H */