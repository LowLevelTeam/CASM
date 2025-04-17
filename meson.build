project('casm', 'cpp',
  version : '0.1.0',
  default_options : [
    'warning_level=3',
    'cpp_std=c++17',
    'buildtype=debugoptimized',
  ],
  meson_version : '>= 0.56.0'
)

# Include directories
incdir = include_directories('include')

# Dependencies
coil_dep = dependency('coil')

# Library sources
casm_src = [
  'src/lexer.cpp',
  'src/parser.cpp',
  'src/codegen.cpp',
  'src/disasm.cpp',
  'src/casm_lib.cpp',
]

# Build the library
casm_lib = library('casm',
  casm_src,
  include_directories : incdir,
  dependencies : coil_dep,
  install : true
)

# Create dependency for other code
casm_dep = declare_dependency(
  include_directories : incdir,
  link_with : casm_lib
)

# Main application
casm_app = executable('casm',
  'src/casm.cpp',
  dependencies : [casm_dep, coil_dep],
  install : true
)

# Generate pkg-config file
pkg_mod = import('pkgconfig')
pkg_mod.generate(
  libraries : casm_lib,
  version : '0.1.0',
  name : 'casm',
  filebase : 'casm',
  description : 'COIL Assembly compiler and disassembler'
)

# Include the tests directory
subdir('tests')

# Install example files
install_data(
  'examples/factorial.casm',
  install_dir: get_option('datadir') / 'casm' / 'examples'
)