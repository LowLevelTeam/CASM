cmake_minimum_required(VERSION 3.14)
project(casm
  VERSION 0.1.0
  DESCRIPTION "COIL Assembly Language"
  LANGUAGES CXX
)

# C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(CASM_BUILD_TESTS "Build CASM tests" ON)
option(CASM_ENABLE_EXCEPTIONS "Enable C++ exceptions" ON)
option(CASM_ENABLE_RTTI "Enable C++ runtime type information" ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

# Conditionally disable exceptions and RTTI
if(NOT CASM_ENABLE_EXCEPTIONS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
endif()

if(NOT CASM_ENABLE_RTTI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

# Find COIL library
find_package(coil REQUIRED)

# Source files
set(CASM_SOURCES
  src/token.cpp
  src/lexer.cpp
  src/parser.cpp
  src/assembler.cpp
  src/main.cpp
)

# Headers
set(CASM_HEADERS
  include/casm/types.hpp
  include/casm/token.hpp
  include/casm/lexer.hpp
  include/casm/parser.hpp
  include/casm/assembler.hpp
)

# Create the executable
add_executable(casm ${CASM_SOURCES} ${CASM_HEADERS})
target_include_directories(casm
  PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Create a library target for tests to use
add_library(casml STATIC 
  src/token.cpp
  src/lexer.cpp
  src/parser.cpp
  src/assembler.cpp
)
target_include_directories(casml
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link against COIL library
target_link_libraries(casm PRIVATE coil::coil)
target_link_libraries(casml PUBLIC coil::coil)

# Version definitions
target_compile_definitions(casm PRIVATE 
  CASM_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
  CASM_VERSION_MINOR=${PROJECT_VERSION_MINOR}
  CASM_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# Tests
if(CASM_BUILD_TESTS)
  include(FetchContent)
  
  # Fetch Catch2 for testing
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.1.0
  )
  FetchContent_MakeAvailable(Catch2)
  
  # Enable testing
  enable_testing()
  
  # Add tests subdirectory
  add_subdirectory(tests)
endif()

# Installation
include(GNUInstallDirs)

# Install targets
install(TARGETS casm
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install headers
install(DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Package configuration
set(CPACK_PACKAGE_NAME "casm")
set(CPACK_PACKAGE_VENDOR "COIL Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "COIL Assembly Language")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

include(CPack)