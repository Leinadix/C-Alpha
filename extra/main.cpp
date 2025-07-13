#include <fstream>
#include <iostream>
#include <string>
#include <filesystem>

#include <codegen.hpp>
#include <lexer.hpp>
#include <parser.hpp>
#include <semantic.hpp>
#include <preprocessor.hpp>


auto read_file(std::string_view path) -> std::string {
    constexpr auto read_size = std::size_t(4096);
    auto stream = std::ifstream(path.data());
    stream.exceptions(std::ios_base::badbit);

    if (not stream) {
        throw std::ios_base::failure("file does not exist");
    }

    auto out = std::string();
    auto buf = std::string(read_size, '\0');
    while (stream.read(& buf[0], read_size)) {
        out.append(buf, 0, stream.gcount());
    }
    out.append(buf, 0, stream.gcount());
    return out;
}


void printSemanticAnalysis(calpha::SemanticAnalyzer& analyzer, bool success) {
    std::cout << "=== SEMANTIC ANALYSIS ===" << std::endl;

    if (success) {
        std::cout << "Semantic Analysis: PASSED" << std::endl;
        std::cout << "No semantic errors found." << std::endl;
    } else {
        std::cout << "Semantic Analysis: FAILED" << std::endl;
        std::cout << "Semantic errors found:" << std::endl;
        analyzer.printErrors();
    }

    std::cout << std::endl;
    std::cout << "=== SYMBOL TABLE ===" << std::endl;
    analyzer.printSymbolTable();
    std::cout << std::endl;
}

#define Dsebug

// Args: 1. path/to/file.calpha
//       2. path/to/output.alpha

int main(int argc, char* argv[]) {

#ifndef Debug
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <path/to/file.calpha>"  " <path/to/output.alpha>"<< std::endl;
        return 1;
    }

    std::string filePath = argv[1];
    std::string outputPath = argv[2];
    std::cout << "Compiling C-Alpha file: " << filePath << std::endl;
#endif
#ifdef Debug

    std::string filePath = "../alpha/lib/std/bitwise.calpha";
    std::string outputPath = "../alpha/out.alpha";
#endif

    // Convert paths to absolute
    std::filesystem::path sourcePath = std::filesystem::absolute(filePath);
    std::filesystem::path outputAbsPath = std::filesystem::absolute(outputPath);
    
    auto content = read_file(sourcePath.string());
    
    // Preprocess the source file
    calpha::Preprocessor preprocessor(sourcePath.parent_path().string());
    try {
        content = preprocessor.process(content, sourcePath.string());
    } catch (const std::exception& e) {
        std::cerr << "Preprocessing error: " << e.what() << std::endl;
        return 1;
    }

    calpha::Lexer lexer(content, sourcePath.string());  // Pass the absolute path
    std::vector<calpha::Token> tokens = lexer.tokenize();

    calpha::Parser parser(tokens);
    auto program = parser.parseProgram();

    if (!program) {
        std::cout << "Parse failed: program is null!" << std::endl;
        return 1;
    }

    calpha::SemanticAnalyzer analyzer;
    bool semanticSuccess = analyzer.analyze(program.get());

    if (!semanticSuccess) {
        std::cout << "Semantic analysis failed!" << std::endl;
        analyzer.printErrors();
        return 1;
    }

    calpha::CodeGenerator codeGen(&analyzer);
    std::string alphaCode = codeGen.generate(program.get());

    // printSemanticAnalysis(analyzer, semanticSuccess);

    std::ofstream out(outputAbsPath.string());
    out << alphaCode;
    out.close();

    return 0;
}

