/* src/disassembler/disasm.h - Main disassembler interface implementation */
#ifndef COIL_DISASM_IMPL_H
#define COIL_DISASM_IMPL_H

#include "../../include/coil_disasm.h"
#include "decoder.h"
#include "formatter.h"

/* COIL disassembler context */
struct coil_disasm_context_t {
  coil_error_context_t error;    /* Error context */
  coil_decoder_t decoder;        /* Decoder */
  coil_formatter_t formatter;    /* Formatter */
  int show_offsets;              /* Show instruction offsets */
  int show_bytes;                /* Show instruction bytes */
  int verbose;                   /* Verbose output with comments */
  int initialized;               /* Is the context initialized? */
};

#endif /* COIL_DISASM_IMPL_H */