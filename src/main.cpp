#include "lexer.h"
#include "parser.h"
#include "symbol_table.h"
#include "code_generator.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: Could not open input file " << filename << std::endl;
        exit(1);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void writeFile(const std::string& filename, const std::vector<uint8_t>& data) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Could not open output file " << filename << std::endl;
        exit(1);
    }
    
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    if (file.fail()) {
        std::cerr << "Error: Failed to write to output file " << filename << std::endl;
        exit(1);
    }
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: coilasm input.casm output.coil" << std::endl;
        return 1;
    }
    
    std::string inputFile = argv[1];
    std::string outputFile = argv[2];
    
    // Read the input file
    std::string source = readFile(inputFile);
    
    // Set up the assembler components
    Lexer lexer(source);
    SymbolTable symbols;
    CodeGenerator codeGen(symbols);
    Parser parser(lexer, symbols, codeGen);
    
    // Parse the file
    if (!parser.parseFile()) {
        std::cerr << "Error: Parsing failed" << std::endl;
        return 1;
    }
    
    // Write the output
    std::vector<uint8_t> output = codeGen.getBinaryOutput();
    writeFile(outputFile, output);
    
    std::cout << "Assembly successful: " << inputFile << " -> " << outputFile << std::endl;
    
    return 0;
}