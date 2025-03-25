/* src/assembler/lexer.h - Tokenizer for COIL assembly */
#ifndef COIL_LEXER_H
#define COIL_LEXER_H

#include "../common/error.h"
#include <stdint.h>

/* Token types */
typedef enum {
  TOKEN_NONE,
  TOKEN_EOF,
  TOKEN_NEWLINE,
  TOKEN_IDENTIFIER,
  TOKEN_NUMBER,
  TOKEN_STRING,
  TOKEN_REGISTER,
  TOKEN_COMMA,
  TOKEN_COLON,
  TOKEN_SEMICOLON,
  TOKEN_EQUALS,
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_LBRACKET,
  TOKEN_RBRACKET,
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_ASTERISK,
  TOKEN_SLASH,
  TOKEN_PERCENT,
  TOKEN_DOT,
  TOKEN_TYPE
} coil_token_type_t;

/* Token structure */
typedef struct {
  coil_token_type_t type;  /* Token type */
  char value[256];         /* Token value as string */
  int line;                /* Line number */
  int column;              /* Column number */
  uint64_t num_value;      /* Numeric value (if applicable) */
  int is_float;            /* Flag for floating point numbers */
  double float_value;      /* Floating point value (if applicable) */
} coil_token_t;

/* Lexer context */
typedef struct {
  const char* input;       /* Input string */
  size_t input_length;     /* Total input length */
  size_t position;         /* Current position */
  int line;                /* Current line */
  int column;              /* Current column */
  coil_token_t current;    /* Current token */
  coil_error_context_t* error; /* Error context */
} coil_lexer_t;

/* Initialize a lexer with input */
void coil_lexer_init(coil_lexer_t* lexer, const char* input, size_t input_length, coil_error_context_t* error_ctx);

/* Get the next token without consuming it */
const coil_token_t* coil_lexer_peek(coil_lexer_t* lexer);

/* Consume the current token and get the next one */
const coil_token_t* coil_lexer_next(coil_lexer_t* lexer);

/* Match and consume a specific token type */
int coil_lexer_match(coil_lexer_t* lexer, coil_token_type_t type);

/* Expect a specific token type, error if not found */
int coil_lexer_expect(coil_lexer_t* lexer, coil_token_type_t type);

/* Skip to the end of the current line */
void coil_lexer_skip_line(coil_lexer_t* lexer);

#endif /* COIL_LEXER_H */