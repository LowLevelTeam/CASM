/* src/common/error.h - Error handling */
#ifndef COIL_ERROR_H
#define COIL_ERROR_H

#include "../../include/coil.h"

/* Error context structure */
typedef struct {
  coil_error_t code;       /* Error code */
  int line;                /* Line number (if relevant) */
  int column;              /* Column number (if relevant) */
  char message[256];       /* Detailed error message */
} coil_error_context_t;

/* Initialize error context */
void coil_error_init(coil_error_context_t* ctx);

/* Set error information */
void coil_error_set(coil_error_context_t* ctx, coil_error_t code, int line, int column, const char* format, ...);

/* Get error message */
const char* coil_error_message(const coil_error_context_t* ctx);

/* Check if there is an error */
int coil_has_error(const coil_error_context_t* ctx);

/* Clear error */
void coil_error_clear(coil_error_context_t* ctx);

#endif /* COIL_ERROR_H */