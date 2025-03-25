/* src/disassembler/decoder.h - COIL binary decoding */
#ifndef COIL_DECODER_H
#define COIL_DECODER_H

#include "../common/object.h"
#include "../common/types.h"
#include "../common/opcodes.h"
#include "../assembler/parser.h"  /* For instruction struct */

/* Decoder context */
typedef struct {
  const coil_object_t* object;    /* Input object */
  const coil_section_t* section;  /* Current section */
  coil_error_context_t* error;    /* Error context */
  uint32_t current_offset;        /* Current offset in section */
  const uint8_t* current_data;    /* Pointer to current data */
  size_t remaining_bytes;         /* Remaining bytes in section */
} coil_decoder_t;

/* Initialize a decoder */
void coil_decoder_init(coil_decoder_t* decoder, const coil_object_t* object, coil_error_context_t* error);

/* Decode an instruction */
coil_error_t coil_decoder_decode_instruction(coil_decoder_t* decoder, coil_instruction_t* instr);

/* Decode a type */
coil_error_t coil_decoder_decode_type(coil_decoder_t* decoder, coil_type_t* type);

/* Decode an operand */
coil_error_t coil_decoder_decode_operand(coil_decoder_t* decoder, coil_operand_t* operand);

/* Set the current section */
void coil_decoder_set_section(coil_decoder_t* decoder, const coil_section_t* section);

/* Set the current offset */
void coil_decoder_set_offset(coil_decoder_t* decoder, uint32_t offset);

/* Get the current offset */
uint32_t coil_decoder_get_offset(const coil_decoder_t* decoder);

/* Read a byte from the current position */
coil_error_t coil_decoder_read_u8(coil_decoder_t* decoder, uint8_t* value);

/* Read a 16-bit value from the current position */
coil_error_t coil_decoder_read_u16(coil_decoder_t* decoder, uint16_t* value);

/* Read a 32-bit value from the current position */
coil_error_t coil_decoder_read_u32(coil_decoder_t* decoder, uint32_t* value);

/* Read a 64-bit value from the current position */
coil_error_t coil_decoder_read_u64(coil_decoder_t* decoder, uint64_t* value);

/* Advance the decoder position */
coil_error_t coil_decoder_advance(coil_decoder_t* decoder, size_t bytes);

/* Get symbol name at offset */
const char* coil_decoder_find_symbol_at_offset(const coil_decoder_t* decoder, uint32_t offset);

#endif /* COIL_DECODER_H */