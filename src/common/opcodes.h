/* src/common/opcodes.h - Opcode definitions */
#ifndef COIL_OPCODES_H
#define COIL_OPCODES_H

#include <stdint.h>
#include "../../include/coil.h"

/* Opcode categories */
#define OP_CATEGORY_NOP       0x00      /* No Operation */
#define OP_CATEGORY_CF        0x01      /* Control Flow (0x01-0x0F) */
#define OP_CATEGORY_MEM       0x10      /* Memory Operations (0x10-0x2F) */
#define OP_CATEGORY_RESERVED  0x30      /* Reserved for Multi-Device Operations (0x30-0x4F) */
#define OP_CATEGORY_BIT       0x50      /* Bit Manipulation (0x50-0x5F) */
#define OP_CATEGORY_ARITH     0x60      /* Arithmetic (0x60-0x8F) */
#define OP_CATEGORY_VECTOR    0x90      /* Vector/Array Operations (0x90-0x9F) */
#define OP_CATEGORY_TYPE      0xA0      /* Type Instructions (0xA0-0xAF) */
#define OP_CATEGORY_DIRECTIVE 0xB0      /* Directive Instructions (0xB0-0xBF) */
#define OP_CATEGORY_ARCH      0xC0      /* Architecture/Processor-Specific Instructions (0xC0-0xFE) */
#define OP_CATEGORY_EXT       0xFF      /* COIL Processor-Specific Extensions */

/* Opcode definitions */
typedef enum {
  /* No Operation */
  OP_NOP       = 0x00,

  /* Control Flow Instructions (0x01-0x0F) */
  OP_SYM       = 0x01,
  OP_BR        = 0x02,
  OP_CALL      = 0x03,
  OP_RET       = 0x04,
  OP_CMP       = 0x05,
  OP_TEST      = 0x06,
  OP_JMP       = 0x07,
  OP_LOOP      = 0x08,
  OP_SWITCH    = 0x09,

  /* Memory Operations (0x10-0x2F) */
  OP_MOV       = 0x10,
  OP_PUSH      = 0x11,
  OP_POP       = 0x12,
  OP_LEA       = 0x13,
  OP_SCOPEE    = 0x14,
  OP_SCOPEL    = 0x15,
  OP_VAR       = 0x16,
  OP_MEMCPY    = 0x17,
  OP_MEMSET    = 0x18,
  OP_MEMCMP    = 0x19,
  OP_XCHG      = 0x1A,
  OP_CAS       = 0x1B,
  OP_PIN       = 0x2E,
  OP_UNPIN     = 0x2F,

  /* Bit Manipulation (0x50-0x5F) */
  OP_AND       = 0x50,
  OP_OR        = 0x51,
  OP_XOR       = 0x52,
  OP_NOT       = 0x53,
  OP_SHL       = 0x54,
  OP_SHR       = 0x55,
  OP_SAR       = 0x56,
  OP_ROL       = 0x57,
  OP_ROR       = 0x58,
  OP_POPCNT    = 0x59,
  OP_BSWAP     = 0x5A,

  /* Arithmetic Instructions (0x60-0x8F) */
  OP_ADD       = 0x60,
  OP_SUB       = 0x61,
  OP_MUL       = 0x62,
  OP_DIV       = 0x63,
  OP_MOD       = 0x64,
  OP_INC       = 0x65,
  OP_DEC       = 0x66,
  OP_NEG       = 0x67,
  OP_ABS       = 0x68,
  OP_SQRT      = 0x69,
  OP_FMA       = 0x6A,
  OP_MIN       = 0x7B,
  OP_MAX       = 0x7C,

  /* Vector/Array Operations (0x90-0x9F) */
  OP_VEXTRACT  = 0x9A,
  OP_VINSERT   = 0x9B,
  OP_VSHUFFLE  = 0x9C,
  OP_VCMP      = 0x9D,
  OP_VBLEND    = 0x9E,
  OP_VMASKMOV  = 0x9F,

  /* Type Instructions (0xA0-0xAF) */
  OP_TYPEOF    = 0xA0,
  OP_SIZEOF    = 0xA1,
  OP_ALIGNOF   = 0xA2,
  OP_CONVERT   = 0xA3,
  OP_CAST      = 0xA4,
  OP_STRUCT    = 0xA5,
  OP_GET       = 0xA6,
  OP_INDEX     = 0xA7,

  /* Directive Instructions (0xB0-0xBF) */
  OP_ARCH      = 0xB0,
  OP_PROC      = 0xB1,
  OP_ALIGN     = 0xB3,
  OP_SECTION   = 0xB4,
  OP_DATA      = 0xB5,
  OP_IF        = 0xB6,
  OP_ELIF      = 0xB7,
  OP_ELSE      = 0xB8,
  OP_ENDIF     = 0xB9,
  OP_ABI       = 0xBA
} coil_opcode_t;

/* Opcode information */
typedef struct {
  uint8_t opcode;          /* Opcode value */
  const char* mnemonic;    /* Instruction mnemonic */
  uint8_t min_operands;    /* Minimum number of operands */
  uint8_t max_operands;    /* Maximum number of operands */
  uint8_t flags;           /* Instruction flags */
} coil_opcode_info_t;

/* Opcode flags */
#define OP_FLAG_NONE       0x00  /* No special flags */
#define OP_FLAG_CONDITIONAL 0x01  /* Supports conditional execution */
#define OP_FLAG_DIRECTIVE  0x02  /* Assembler directive (not executable) */
#define OP_FLAG_ARCH_SPEC  0x04  /* Architecture-specific instruction */

/* Get information about an opcode */
const coil_opcode_info_t* coil_opcode_get_info(uint8_t opcode);

/* Get opcode from mnemonic */
coil_error_t coil_opcode_from_mnemonic(const char* mnemonic, uint8_t* opcode);

/* Check if opcode is valid */
int coil_opcode_is_valid(uint8_t opcode);

/* Check if opcode is in reserved range */
int coil_opcode_is_reserved(uint8_t opcode);

/* Check if an opcode can be conditional */
int coil_opcode_can_be_conditional(uint8_t opcode);

/* Append condition suffix to mnemonic */
void coil_opcode_add_condition(char* dest, size_t dest_size, const char* base_mnemonic, coil_branch_cond_t cond);

/* Parse mnemonic with potential condition suffix */
coil_error_t coil_opcode_parse_mnemonic(const char* mnemonic, uint8_t* opcode, coil_branch_cond_t* cond);

#endif /* COIL_OPCODES_H */