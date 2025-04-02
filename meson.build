project('casm', 'cpp',
  version : '1.0.0',
  default_options : ['cpp_std=c++17', 'warning_level=3']
)

# Include directories
include_dirs = include_directories('include')

# Source files
sources = [
  'src/main.cpp',
  'src/assembler.cpp',
  'src/lexer.cpp',
  'src/parser.cpp',
  'src/code_generator.cpp',
  'src/symbol_table.cpp',
  'src/error_handler.cpp',
  'src/directive_handler.cpp',
]

# Find libcoil-dev
libcoil_dep = dependency('libcoil-dev')

# Executable target
casm_exe = executable('casm',
  sources,
  include_directories : include_dirs,
  dependencies : [libcoil_dep],
  install : true
)

# Installation
install_headers(
  'include/casm/assembler.h',
  'include/casm/lexer.h',
  'include/casm/parser.h',
  'include/casm/code_generator.h',
  'include/casm/symbol_table.h',
  'include/casm/error_handler.h',
  'include/casm/directive_handler.h',
  'include/casm/token.h',
  subdir : 'casm'
)