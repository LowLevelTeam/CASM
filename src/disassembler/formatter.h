/* src/disassembler/formatter.h - ASM output formatting */
#ifndef COIL_FORMATTER_H
#define COIL_FORMATTER_H

#include "../assembler/parser.h"  /* For instruction struct */
#include "../common/object.h"

/* Output buffer */
typedef struct {
  char* buffer;            /* Output buffer */
  size_t length;           /* Current length */
  size_t capacity;         /* Total capacity */
} coil_output_buffer_t;

/* Formatter context */
typedef struct {
  coil_output_buffer_t output;   /* Output buffer */
  const coil_object_t* object;   /* Input object */
  coil_error_context_t* error;   /* Error context */
  int show_offsets;              /* Show instruction offsets */
  int show_bytes;                /* Show instruction bytes */
  int verbose;                   /* Verbose output with comments */
} coil_formatter_t;

/* Initialize a formatter */
void coil_formatter_init(coil_formatter_t* formatter, const coil_object_t* object, 
                        coil_error_context_t* error);

/* Clean up a formatter */
void coil_formatter_cleanup(coil_formatter_t* formatter);

/* Format an instruction */
coil_error_t coil_formatter_format_instruction(coil_formatter_t* formatter, 
                                             const coil_instruction_t* instr, 
                                             uint32_t offset, 
                                             const uint8_t* bytes, 
                                             size_t byte_count);

/* Format a type */
coil_error_t coil_formatter_format_type(coil_formatter_t* formatter, coil_type_t type, char* buffer, size_t size);

/* Format an operand */
coil_error_t coil_formatter_format_operand(coil_formatter_t* formatter, const coil_operand_t* operand, 
                                         char* buffer, size_t size);

/* Format a directive */
coil_error_t coil_formatter_format_directive(coil_formatter_t* formatter, uint8_t directive, 
                                           const coil_operand_t* operands, int num_operands);

/* Format a label */
coil_error_t coil_formatter_format_label(coil_formatter_t* formatter, const char* label);

/* Format a section header */
coil_error_t coil_formatter_format_section(coil_formatter_t* formatter, const coil_section_t* section);

/* Format a comment */
coil_error_t coil_formatter_format_comment(coil_formatter_t* formatter, const char* comment);

/* Append a string to the output */
coil_error_t coil_formatter_append(coil_formatter_t* formatter, const char* text);

/* Append a formatted string to the output */
coil_error_t coil_formatter_appendf(coil_formatter_t* formatter, const char* format, ...);

/* Get the formatted output */
const char* coil_formatter_get_output(const coil_formatter_t* formatter);

/* Get the output length */
size_t coil_formatter_get_length(const coil_formatter_t* formatter);

#endif /* COIL_FORMATTER_H */