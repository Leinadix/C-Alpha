#include "lexer.hpp"
#include "parser.hpp"
#include "semantic.hpp"
#include <iostream>

using namespace calpha;

void testCode(const std::string& code, const std::string& testName) {
    std::cout << "=== " << testName << " ===" << std::endl;
    std::cout << "Code: " << code << std::endl << std::endl;
    
    try {
        Lexer lexer(code);
        auto tokens = lexer.tokenize();
        
        Parser parser(tokens);
        auto ast = parser.parseProgram();
        
        std::cout << "Parsing: SUCCESS" << std::endl;
        
        SemanticAnalyzer analyzer;
        bool success = analyzer.analyze(ast.get());
        
        if (success) {
            std::cout << "Semantic Analysis: SUCCESS" << std::endl;
        } else {
            std::cout << "Semantic Analysis: FAILED" << std::endl;
            analyzer.printErrors();
        }
        
        std::cout << "Symbol Table:" << std::endl;
        analyzer.printSymbolTable();
        
    } catch (const Parser::ParseError& e) {
        std::cout << "Parse Error: " << e.what() << " at line " << e.getLine() << ", column " << e.getColumn() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
    
    std::cout << std::endl << "===========================================" << std::endl << std::endl;
}

int main() {
    std::cout << "=== Debug Layout Array Access Issue ===" << std::endl << std::endl;
    
    // Test 1: Basic layout definition
    testCode("layout Point { int x; int y; };", "Basic Layout Definition");
    
    // Test 2: Array allocation with layout type
    testCode("layout Point { int x; int y; }; ->Point points = ~Point[3];", "Array Allocation with Layout Type");
    
    // Test 3: Array access with member access
    testCode("layout Point { int x; int y; }; ->Point points = ~Point[3]; points[0].x = 10;", "Array Access with Member Access");
    
    // Test 4: Full complex example
    testCode("layout Point { int x; int y; }; ->Point points = ~Point[3]; points[0].x = 10; points[0].y = 20; int x = points[0].x;", "Complex Array and Layout Operations");
    
    // Test 5: Basic array allocation for comparison
    testCode("->int nums = ~int[5]; nums[0] = 42;", "Basic Array Allocation and Access");
    
    return 0;
} 