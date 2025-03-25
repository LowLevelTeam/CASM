/* include/coil_asm.h - Assembler interface */
#ifndef COIL_ASM_H
#define COIL_ASM_H

#include "coil.h"

/* COIL assembler context */
typedef struct coil_asm_context_t coil_asm_context_t;

/* Create a new assembler context */
coil_asm_context_t* coil_asm_create(void);

/* Free an assembler context */
void coil_asm_free(coil_asm_context_t* ctx);

/* Assemble COIL-ASM text into a COIL object */
coil_error_t coil_asm_assemble_string(coil_asm_context_t* ctx, const char* input, coil_object_t** obj);

/* Assemble COIL-ASM from a file into a COIL object */
coil_error_t coil_asm_assemble_file(coil_asm_context_t* ctx, const char* filename, coil_object_t** obj);

/* Get the last error message from the assembler */
const char* coil_asm_get_error(const coil_asm_context_t* ctx);

/* Get the current line number where an error occurred */
int coil_asm_get_error_line(const coil_asm_context_t* ctx);

/* Get the current column where an error occurred */
int coil_asm_get_error_column(const coil_asm_context_t* ctx);

#endif /* COIL_ASM_H */