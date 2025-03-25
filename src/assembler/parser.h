/* src/assembler/parser.h - COIL assembly parser */
#ifndef COIL_PARSER_H
#define COIL_PARSER_H

#include "lexer.h"
#include "../common/object.h"
#include "../common/types.h"
#include "../common/opcodes.h"

/* Operand structure */
typedef struct {
  coil_type_t type;            /* Operand type */
  int has_value;               /* Does operand have a value? */
  uint64_t immediate_value;    /* Immediate value (if applicable) */
  double float_value;          /* Floating point value (if applicable) */
  char symbol_name[64];        /* Symbol name (if applicable) */
  int is_memory_reference;     /* Is this a memory reference? */
  struct {
    char base_reg[16];         /* Base register */
    char index_reg[16];        /* Index register */
    int scale;                 /* Scale factor */
    int64_t displacement;      /* Displacement */
  } mem;                       /* Memory reference components (if applicable) */
} coil_operand_t;

/* Instruction structure */
typedef struct {
  uint8_t opcode;              /* Instruction opcode */
  coil_branch_cond_t condition; /* Condition code (if conditional) */
  int num_operands;            /* Number of operands */
  coil_operand_t operands[8];  /* Operands (max 8 for now) */
  char label[64];              /* Label (if this instruction has one) */
} coil_instruction_t;

/* Parser context */
typedef struct {
  coil_lexer_t lexer;          /* Lexer for tokens */
  coil_object_t* object;       /* Output object being built */
  coil_error_context_t* error; /* Error context */
  coil_section_t* current_section; /* Current section being assembled */
  int in_conditional;          /* Are we in a conditional block? */
  int conditional_depth;       /* Depth of conditional nesting */
  int skip_block;              /* Should we skip the current block? */
  int if_result;               /* Result of the last IF condition */
  
  /* Assembly state */
  uint8_t current_arch;        /* Current architecture */
  uint8_t current_arch_mode;   /* Current architecture mode */
  uint8_t current_proc;        /* Current processor */
} coil_parser_t;

/* Initialize a parser */
void coil_parser_init(coil_parser_t* parser, const char* input, size_t input_length, 
                     coil_object_t* object, coil_error_context_t* error);

/* Parse the entire input */
coil_error_t coil_parser_parse(coil_parser_t* parser);

/* Parse a single instruction */
coil_error_t coil_parser_parse_instruction(coil_parser_t* parser, coil_instruction_t* instr);

/* Parse a directive */
coil_error_t coil_parser_parse_directive(coil_parser_t* parser);

/* Parse an operand */
coil_error_t coil_parser_parse_operand(coil_parser_t* parser, coil_operand_t* operand);

/* Parse a label definition */
coil_error_t coil_parser_parse_label(coil_parser_t* parser);

/* Check if a token is a directive */
int coil_parser_is_directive(const char* token);

/* Check if a token is a register name */
int coil_parser_is_register(const char* token, char* reg_name, size_t reg_name_size);

/* Check if a token is a type name */
int coil_parser_is_type(const char* token, coil_type_t* type);

#endif /* COIL_PARSER_H */