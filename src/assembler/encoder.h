/* src/assembler/encoder.h - COIL binary encoding */
#ifndef COIL_ENCODER_H
#define COIL_ENCODER_H

#include "parser.h"
#include "../common/object.h"

/* Encoder context */
typedef struct {
  coil_object_t* object;       /* Output object */
  coil_section_t* current_section; /* Current section */
  coil_error_context_t* error; /* Error context */
  uint32_t current_offset;     /* Current offset in section */
} coil_encoder_t;

/* Initialize an encoder */
void coil_encoder_init(coil_encoder_t* encoder, coil_object_t* object, coil_error_context_t* error);

/* Encode an instruction */
coil_error_t coil_encoder_encode_instruction(coil_encoder_t* encoder, const coil_instruction_t* instr);

/* Encode a type */
coil_error_t coil_encoder_encode_type(coil_encoder_t* encoder, coil_type_t type);

/* Encode an operand */
coil_error_t coil_encoder_encode_operand(coil_encoder_t* encoder, const coil_operand_t* operand);

/* Encode a directive */
coil_error_t coil_encoder_encode_directive(coil_encoder_t* encoder, uint8_t directive, 
                                         const coil_operand_t* operands, int num_operands);

/* Set the current section */
void coil_encoder_set_section(coil_encoder_t* encoder, coil_section_t* section);

/* Get the current offset in the section */
uint32_t coil_encoder_get_offset(const coil_encoder_t* encoder);

/* Add a relocation */
coil_error_t coil_encoder_add_relocation(coil_encoder_t* encoder, uint32_t offset, 
                                       const char* symbol_name, uint8_t reloc_type);

/* Write raw data to the section */
coil_error_t coil_encoder_write(coil_encoder_t* encoder, const void* data, size_t size);

/* Write a byte to the section */
coil_error_t coil_encoder_write_u8(coil_encoder_t* encoder, uint8_t value);

/* Write a 16-bit value to the section */
coil_error_t coil_encoder_write_u16(coil_encoder_t* encoder, uint16_t value);

/* Write a 32-bit value to the section */
coil_error_t coil_encoder_write_u32(coil_encoder_t* encoder, uint32_t value);

/* Write a 64-bit value to the section */
coil_error_t coil_encoder_write_u64(coil_encoder_t* encoder, uint64_t value);

/* Align the current position to a boundary */
coil_error_t coil_encoder_align(coil_encoder_t* encoder, size_t alignment);

#endif /* COIL_ENCODER_H */