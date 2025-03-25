/* src/main.c - Command line interface for COIL tools */
#include "../include/coil.h"
#include "../include/coil_asm.h"
#include "../include/coil_disasm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERSION_STRING "COIL Tools v1.0.0"

/* Print usage information */
static void print_usage(const char* program_name) {
  printf("Usage: %s [options] <input-file> [output-file]\n\n", program_name);
  printf("Options:\n");
  printf("  -a, --assemble     Assemble COIL-ASM file to COIL binary\n");
  printf("  -d, --disassemble  Disassemble COIL binary to COIL-ASM\n");
  printf("  -o, --offsets      Show instruction offsets in disassembly\n");
  printf("  -b, --bytes        Show instruction bytes in disassembly\n");
  printf("  -v, --verbose      Verbose output\n");
  printf("  -h, --help         Show this help message\n");
  printf("  --version          Show version information\n");
  printf("\n");
  printf("If no mode is specified, the tool guesses based on the input file extension.\n");
  printf("Files ending in .casm are assembled, files ending in .coil are disassembled.\n");
}

/* Print version information */
static void print_version(void) {
  printf("%s\n", VERSION_STRING);
  printf("Copyright (c) 2025\n");
  printf("License: Public Domain (Unlicense)\n");
}

/* Read an entire file into memory */
static char* read_file(const char* filename, size_t* size) {
  FILE* file = fopen(filename, "rb");
  if (!file) {
    return NULL;
  }
  
  /* Get file size */
  fseek(file, 0, SEEK_END);
  *size = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  /* Allocate buffer */
  char* buffer = (char*)malloc(*size + 1);
  if (!buffer) {
    fclose(file);
    return NULL;
  }
  
  /* Read data */
  size_t read_size = fread(buffer, 1, *size, file);
  fclose(file);
  
  if (read_size != *size) {
    free(buffer);
    return NULL;
  }
  
  /* Null-terminate the buffer */
  buffer[*size] = '\0';
  
  return buffer;
}

/* Write a string to a file */
static int write_file(const char* filename, const char* data, size_t size) {
  FILE* file = fopen(filename, "wb");
  if (!file) {
    return 0;
  }
  
  size_t written = fwrite(data, 1, size, file);
  fclose(file);
  
  return written == size;
}

/* Assemble COIL-ASM file to COIL binary */
static int assemble_file(const char* input_file, const char* output_file, int verbose) {
  size_t size;
  char* input = read_file(input_file, &size);
  if (!input) {
    fprintf(stderr, "Error: Could not read input file '%s'\n", input_file);
    return 1;
  }
  
  if (verbose) {
    printf("Assembling %s to %s...\n", input_file, output_file);
  }
  
  coil_asm_context_t* ctx = coil_asm_create();
  if (!ctx) {
    fprintf(stderr, "Error: Could not create assembler context\n");
    free(input);
    return 1;
  }
  
  coil_object_t* obj = NULL;
  coil_error_t err = coil_asm_assemble_string(ctx, input, &obj);
  
  if (err != COIL_SUCCESS) {
    fprintf(stderr, "Error: %s (line %d, column %d)\n", 
            coil_asm_get_error(ctx), 
            coil_asm_get_error_line(ctx),
            coil_asm_get_error_column(ctx));
    coil_asm_free(ctx);
    free(input);
    return 1;
  }
  
  err = coil_object_save(obj, output_file);
  if (err != COIL_SUCCESS) {
    fprintf(stderr, "Error: Could not save output file '%s'\n", output_file);
    coil_object_free(obj);
    coil_asm_free(ctx);
    free(input);
    return 1;
  }
  
  if (verbose) {
    printf("Successfully assembled to %s\n", output_file);
  }
  
  coil_object_free(obj);
  coil_asm_free(ctx);
  free(input);
  
  return 0;
}

/* Disassemble COIL binary to COIL-ASM */
static int disassemble_file(const char* input_file, const char* output_file, 
                          int show_offsets, int show_bytes, int verbose) {
  coil_object_t* obj = NULL;
  coil_error_t err = coil_object_load(input_file, &obj);
  
  if (err != COIL_SUCCESS) {
    fprintf(stderr, "Error: Could not load COIL file '%s'\n", input_file);
    return 1;
  }
  
  if (verbose) {
    printf("Disassembling %s to %s...\n", input_file, output_file);
  }
  
  coil_disasm_context_t* ctx = coil_disasm_create();
  if (!ctx) {
    fprintf(stderr, "Error: Could not create disassembler context\n");
    coil_object_free(obj);
    return 1;
  }
  
  /* Configure disassembler options */
  ctx->show_offsets = show_offsets;
  ctx->show_bytes = show_bytes;
  ctx->verbose = verbose;
  
  char* output = NULL;
  err = coil_disasm_disassemble(ctx, obj, &output);
  
  if (err != COIL_SUCCESS) {
    fprintf(stderr, "Error: %s\n", coil_disasm_get_error(ctx));
    coil_disasm_free(ctx);
    coil_object_free(obj);
    return 1;
  }
  
  if (!write_file(output_file, output, strlen(output))) {
    fprintf(stderr, "Error: Could not write output file '%s'\n", output_file);
    free(output);
    coil_disasm_free(ctx);
    coil_object_free(obj);
    return 1;
  }
  
  if (verbose) {
    printf("Successfully disassembled to %s\n", output_file);
  }
  
  free(output);
  coil_disasm_free(ctx);
  coil_object_free(obj);
  
  return 0;
}

/* Main entry point */
int main(int argc, char* argv[]) {
  /* Default settings */
  int mode_assemble = -1;  /* -1 = auto, 0 = disassemble, 1 = assemble */
  int show_offsets = 0;
  int show_bytes = 0;
  int verbose = 0;
  const char* input_file = NULL;
  const char* output_file = NULL;
  
  /* Parse command line arguments */
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      /* Option */
      if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--assemble") == 0) {
        mode_assemble = 1;
      } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--disassemble") == 0) {
        mode_assemble = 0;
      } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--offsets") == 0) {
        show_offsets = 1;
      } else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--bytes") == 0) {
        show_bytes = 1;
      } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
        verbose = 1;
      } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
      } else if (strcmp(argv[i], "--version") == 0) {
        print_version();
        return 0;
      } else {
        fprintf(stderr, "Unknown option: %s\n", argv[i]);
        print_usage(argv[0]);
        return 1;
      }
    } else {
      /* File */
      if (!input_file) {
        input_file = argv[i];
      } else if (!output_file) {
        output_file = argv[i];
      } else {
        fprintf(stderr, "Too many file arguments\n");
        print_usage(argv[0]);
        return 1;
      }
    }
  }
  
  /* Check if input file was provided */
  if (!input_file) {
    fprintf(stderr, "No input file specified\n");
    print_usage(argv[0]);
    return 1;
  }
  
  /* Auto-detect mode if not specified */
  if (mode_assemble == -1) {
    size_t len = strlen(input_file);
    if (len > 5 && strcmp(input_file + len - 5, ".casm") == 0) {
      mode_assemble = 1;
    } else if (len > 5 && strcmp(input_file + len - 5, ".coil") == 0) {
      mode_assemble = 0;
    } else {
      fprintf(stderr, "Could not determine mode from file extension\n");
      print_usage(argv[0]);
      return 1;
    }
  }
  
  /* Generate default output filename if not specified */
  char default_output[1024];
  if (!output_file) {
    size_t len = strlen(input_file);
    strncpy(default_output, input_file, sizeof(default_output) - 1);
    default_output[sizeof(default_output) - 1] = '\0';
    
    if (mode_assemble) {
      /* Change .casm to .coil */
      if (len > 5 && strcmp(input_file + len - 5, ".casm") == 0) {
        strcpy(default_output + len - 5, ".coil");
      } else {
        strcat(default_output, ".coil");
      }
    } else {
      /* Change .coil to .casm */
      if (len > 5 && strcmp(input_file + len - 5, ".coil") == 0) {
        strcpy(default_output + len - 5, ".casm");
      } else {
        strcat(default_output, ".casm");
      }
    }
    
    output_file = default_output;
  }
  
  /* Perform the requested operation */
  if (mode_assemble) {
    return assemble_file(input_file, output_file, verbose);
  } else {
    return disassemble_file(input_file, output_file, show_offsets, show_bytes, verbose);
  }
}