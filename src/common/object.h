/* src/common/object.h - COIL object internal structure */
#ifndef COIL_OBJECT_H
#define COIL_OBJECT_H

#include "../../include/coil.h"
#include <stdint.h>

/* COIL section types */
typedef enum {
  SECTION_TEXT,        /* Code section */
  SECTION_DATA,        /* Initialized data section */
  SECTION_BSS,         /* Uninitialized data section */
  SECTION_SYMBOL,      /* Symbol table section */
  SECTION_RELOC,       /* Relocation section */
  SECTION_DEBUG,       /* Debug information section */
  SECTION_CUSTOM       /* Custom section */
} coil_section_type_t;

/* Section attributes */
#define SECTION_ATTR_EXEC  0x01  /* Executable */
#define SECTION_ATTR_WRITE 0x02  /* Writable */
#define SECTION_ATTR_READ  0x04  /* Readable */
#define SECTION_ATTR_INIT  0x08  /* Initialized data */
#define SECTION_ATTR_UNINIT 0x10 /* Uninitialized data */

/* Symbol types */
typedef enum {
  SYMBOL_UNKNOWN,      /* Unknown symbol type */
  SYMBOL_FUNCTION,     /* Function */
  SYMBOL_VARIABLE,     /* Variable */
  SYMBOL_SECTION,      /* Section */
  SYMBOL_FILE,         /* File */
  SYMBOL_LABEL,        /* Basic label */
  SYMBOL_EXTERNAL      /* External reference */
} coil_symbol_type_t;

/* Symbol visibility */
typedef enum {
  SYMBOL_VIS_LOCAL,    /* Symbol only visible in current file */
  SYMBOL_VIS_GLOBAL,   /* Symbol visible to all files */
  SYMBOL_VIS_WEAK,     /* Weakly defined symbol */
  SYMBOL_VIS_HIDDEN    /* Symbol not exported outside the file */
} coil_symbol_visibility_t;

/* COIL section */
typedef struct {
  char name[32];               /* Section name */
  coil_section_type_t type;    /* Section type */
  uint8_t attributes;          /* Section attributes */
  size_t size;                 /* Section size */
  size_t capacity;             /* Allocated capacity */
  uint8_t* data;               /* Section data */
  size_t alignment;            /* Section alignment */
} coil_section_t;

/* COIL symbol */
typedef struct {
  char name[64];               /* Symbol name */
  coil_symbol_type_t type;     /* Symbol type */
  coil_symbol_visibility_t visibility; /* Symbol visibility */
  uint32_t value;              /* Symbol value (usually offset) */
  uint32_t size;               /* Symbol size (if applicable) */
  int section_index;           /* Index of the section containing this symbol */
  uint32_t flags;              /* Symbol flags */
} coil_symbol_t;

/* COIL relocation */
typedef struct {
  uint32_t offset;             /* Offset in section */
  uint32_t symbol_idx;         /* Symbol index */
  uint8_t type;                /* Relocation type */
  int32_t addend;              /* Addend for the relocation */
} coil_relocation_t;

/* COIL file header */
typedef struct {
  char magic[4];               /* Magic number: "COIL" */
  uint8_t major_version;       /* Major version */
  uint8_t minor_version;       /* Minor version */
  uint8_t patch_version;       /* Patch version */
  uint8_t flags;               /* Format flags */
  uint32_t entry_point;        /* Entry point (if executable) */
  uint8_t arch;                /* Target architecture */
  uint8_t arch_mode;           /* Architecture mode */
  uint8_t proc;                /* Target processor */
  uint8_t reserved;            /* Reserved for future use */
} coil_header_t;

/* COIL object structure */
struct coil_object_t {
  coil_header_t header;        /* Object header */
  
  /* Sections */
  size_t num_sections;
  size_t sections_capacity;
  coil_section_t* sections;
  
  /* Symbols */
  size_t num_symbols;
  size_t symbols_capacity;
  coil_symbol_t* symbols;
  
  /* Relocations */
  size_t num_relocations;
  size_t relocations_capacity;
  coil_relocation_t* relocations;
  
  /* String table */
  size_t strtab_size;
  size_t strtab_capacity;
  char* strtab;
};

/* Add a section to the object */
coil_error_t coil_object_add_section(coil_object_t* obj, const char* name, coil_section_type_t type, uint8_t attributes);

/* Get a section by name */
coil_section_t* coil_object_get_section(coil_object_t* obj, const char* name);

/* Get a section by index */
coil_section_t* coil_object_get_section_by_index(coil_object_t* obj, size_t index);

/* Add data to a section */
coil_error_t coil_object_add_section_data(coil_object_t* obj, coil_section_t* section, const void* data, size_t size);

/* Add a symbol to the object */
coil_error_t coil_object_add_symbol(coil_object_t* obj, const char* name, coil_symbol_type_t type, 
                                  coil_symbol_visibility_t visibility, uint32_t value, uint32_t size,
                                  int section_index);

/* Get a symbol by name */
coil_symbol_t* coil_object_get_symbol(coil_object_t* obj, const char* name);

/* Get a symbol by index */
coil_symbol_t* coil_object_get_symbol_by_index(coil_object_t* obj, size_t index);

/* Add a relocation to the object */
coil_error_t coil_object_add_relocation(coil_object_t* obj, uint32_t offset, uint32_t symbol_idx,
                                       uint8_t type, int32_t addend);

/* Add a string to the string table */
uint32_t coil_object_add_string(coil_object_t* obj, const char* str);

/* Get a string from the string table */
const char* coil_object_get_string(coil_object_t* obj, uint32_t offset);

/* Initialize object with default header */
void coil_object_init_header(coil_object_t* obj);

#endif /* COIL_OBJECT_H */