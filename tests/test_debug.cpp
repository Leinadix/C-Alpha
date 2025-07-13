#include "lexer.hpp"
#include "parser.hpp"
#include "semantic.hpp"
#include <iostream>

using namespace calpha;

int main() {
    std::cout << "=== Debug Layout Array Access Issue ===" << std::endl;
    
    // Test the specific problematic code
    std::string code = "layout Point { int x; int y; }; ->Point points = ~Point[3]; points[0].x = 10;";
    
    std::cout << "Code: " << code << std::endl << std::endl;
    
    try {
        Lexer lexer(code);
        auto tokens = lexer.tokenize();
        
        std::cout << "=== Tokens ===" << std::endl;
        for (const auto& token : tokens) {
            std::cout << "Type: " << static_cast<int>(token.type) << ", Value: " << token.value 
                      << ", Line: " << token.line << ", Col: " << token.column << std::endl;
        }
        std::cout << std::endl;
        
        Parser parser(tokens);
        auto ast = parser.parseProgram();
        
        std::cout << "=== Parsing Success ===" << std::endl;
        
        SemanticAnalyzer analyzer;
        bool success = analyzer.analyze(ast.get());
        
        std::cout << "=== Semantic Analysis ===" << std::endl;
        if (success) {
            std::cout << "Success: No semantic errors found!" << std::endl;
        } else {
            std::cout << "Errors found:" << std::endl;
            analyzer.printErrors();
        }
        
        std::cout << std::endl;
        analyzer.printSymbolTable();
        
    } catch (const Parser::ParseError& e) {
        std::cout << "Parse Error: " << e.what() << " at line " << e.getLine() << ", column " << e.getColumn() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
    
    return 0;
} 