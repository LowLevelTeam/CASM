/* src/assembler/asm.h - Main assembler interface implementation */
#ifndef COIL_ASM_IMPL_H
#define COIL_ASM_IMPL_H

#include "../../include/coil_asm.h"
#include "parser.h"
#include "encoder.h"

/* COIL assembler context */
struct coil_asm_context_t {
  coil_error_context_t error;  /* Error context */
  coil_parser_t parser;        /* Parser */
  coil_encoder_t encoder;      /* Encoder */
  int initialized;             /* Is the context initialized? */
};

#endif /* COIL_ASM_IMPL_H */