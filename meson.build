project('casm', 'cpp',
  version : '0.1.0',
  default_options : [
    'warning_level=3',
    'cpp_std=c++17',
    'buildtype=debugoptimized',
  ],
  meson_version : '>= 0.56.0'
)

# Find COIL dependency
coil_dep = dependency('coil', version : '>=0.1.0', required : true)

# Include directories
incdir = include_directories('include')

# Library sources
casm_lib_src = [
  'src/parser.cpp',
  'src/codegen.cpp',
  'src/assembler.cpp',
]

# Build optimizations (same as COIL library)
compiler = meson.get_compiler('cpp')
add_project_arguments('-fno-exceptions', '-fno-rtti', language : 'cpp')

# Build the library
casm_lib = library('casm',
  casm_lib_src,
  include_directories : incdir,
  dependencies : [coil_dep],
  install : true
)

# Build the executable
casm_exe = executable('casm',
  'src/main.cpp',
  include_directories : incdir,
  link_with : casm_lib,
  dependencies : [coil_dep],
  install : true
)

# Install headers
install_subdir('include/casm', install_dir : get_option('includedir'))

# Create dependency for test code
casm_dep = declare_dependency(
  include_directories : incdir,
  link_with : casm_lib,
  dependencies : [coil_dep]
)

# Generate pkg-config file
pkg_mod = import('pkgconfig')
pkg_mod.generate(
  libraries : casm_lib,
  version : '0.1.0',
  name : 'casm',
  filebase : 'casm',
  description : 'CASM (COIL Assembly) Assembler Library'
)

# Include the tests directory if it exists
if run_command('[', '-d', 'tests', ']', check: false).returncode() == 0
  subdir('tests')
endif