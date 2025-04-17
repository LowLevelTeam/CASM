# Test configuration
set(TEST_SOURCES
  test_lexer.cpp
  test_parser.cpp
  test_assembler.cpp
)

# Build the test executable
add_executable(casm_tests ${TEST_SOURCES})
target_include_directories(casm_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Link against required libraries
target_link_libraries(casm_tests PRIVATE 
  # Link with the COIL library
  coil::coil
  
  # Link with Catch2
  Catch2::Catch2WithMain
  
  # Need to link with our source files directly, excluding main.cpp
  ${CMAKE_SOURCE_DIR}/src/token.cpp
  ${CMAKE_SOURCE_DIR}/src/lexer.cpp
  ${CMAKE_SOURCE_DIR}/src/parser.cpp
  ${CMAKE_SOURCE_DIR}/src/assembler.cpp
)

# Register tests with CTest
include(Catch)
catch_discover_tests(casm_tests)