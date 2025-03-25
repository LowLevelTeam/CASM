# COIL Tools Makefile

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -O2
LDFLAGS = -lm

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
BIN_DIR = bin

# Source files
COMMON_SRCS = $(wildcard $(SRC_DIR)/common/*.c)
ASM_SRCS = $(wildcard $(SRC_DIR)/assembler/*.c)
DISASM_SRCS = $(wildcard $(SRC_DIR)/disassembler/*.c)
MAIN_SRC = $(SRC_DIR)/main.c

# Object files
COMMON_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(COMMON_SRCS))
ASM_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(ASM_SRCS))
DISASM_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(DISASM_SRCS))
MAIN_OBJ = $(BUILD_DIR)/main.o

# Target executable
TARGET = $(BIN_DIR)/coiltool

# Default target
all: $(TARGET)

# Create build directories
$(BUILD_DIR)/common $(BUILD_DIR)/assembler $(BUILD_DIR)/disassembler $(BIN_DIR):
	mkdir -p $@

# Compile common sources
$(BUILD_DIR)/common/%.o: $(SRC_DIR)/common/%.c | $(BUILD_DIR)/common
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Compile assembler sources
$(BUILD_DIR)/assembler/%.o: $(SRC_DIR)/assembler/%.c | $(BUILD_DIR)/assembler
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Compile disassembler sources
$(BUILD_DIR)/disassembler/%.o: $(SRC_DIR)/disassembler/%.c | $(BUILD_DIR)/disassembler
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Compile main source
$(MAIN_OBJ): $(MAIN_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Link everything
$(TARGET): $(COMMON_OBJS) $(ASM_OBJS) $(DISASM_OBJS) $(MAIN_OBJ) | $(BIN_DIR)
	$(CC) -o $@ $^ $(LDFLAGS)

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Install the program (to /usr/local/bin by default)
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

# Uninstall the program
uninstall:
	rm -f /usr/local/bin/$(notdir $(TARGET))

.PHONY: all clean install uninstall