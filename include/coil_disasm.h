/* include/coil_disasm.h - Disassembler interface */
#ifndef COIL_DISASM_H
#define COIL_DISASM_H

#include "coil.h"

/* COIL disassembler context */
typedef struct coil_disasm_context_t coil_disasm_context_t;

/* Create a new disassembler context */
coil_disasm_context_t* coil_disasm_create(void);

/* Free a disassembler context */
void coil_disasm_free(coil_disasm_context_t* ctx);

/* Disassemble a COIL object into COIL-ASM text */
coil_error_t coil_disasm_disassemble(const coil_disasm_context_t* ctx, const coil_object_t* obj, char** output);

/* Disassemble a COIL object to a file */
coil_error_t coil_disasm_disassemble_to_file(const coil_disasm_context_t* ctx, const coil_object_t* obj, const char* filename);

/* Get the last error message from the disassembler */
const char* coil_disasm_get_error(const coil_disasm_context_t* ctx);

#endif /* COIL_DISASM_H */