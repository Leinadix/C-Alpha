#include <iostream>
#include <string>
#include <vector>
#include "lexer.hpp"
#include "parser.hpp"
#include "semantic.hpp"

using namespace calpha;

void debugTokens(const std::string& code) {
    std::cout << "=== Token Debug ===" << std::endl;
    std::cout << "Code: " << code << std::endl;
    
    Lexer lexer(code);
    std::vector<Token> tokens = lexer.tokenize();
    
    for (size_t i = 0; i < tokens.size(); ++i) {
        std::cout << "Token " << i << ": '" << tokens[i].value 
                  << "' (type: " << (int)tokens[i].type 
                  << ", line: " << tokens[i].line 
                  << ", col: " << tokens[i].column << ")" << std::endl;
    }
    std::cout << std::endl;
}

void debugParsing(const std::string& code) {
    std::cout << "=== Parse Debug ===" << std::endl;
    std::cout << "Code: " << code << std::endl;
    
    try {
        Lexer lexer(code);
        std::vector<Token> tokens = lexer.tokenize();
        
        Parser parser(tokens);
        auto program = parser.parseProgram();
        
        if (program) {
            std::cout << "Parse successful: " << program->statements.size() << " statements" << std::endl;
            
            // Print AST structure
            for (size_t i = 0; i < program->statements.size(); ++i) {
                std::cout << "Statement " << i << ": NodeType " << (int)program->statements[i]->nodeType << std::endl;
            }
        } else {
            std::cout << "Parse failed!" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "C-Alpha Debug: Character Literals and Symbol Resolution" << std::endl;
    std::cout << "=======================================================" << std::endl;
    
    // Test 1: Simple character literal
    debugTokens("'A'");
    debugParsing("char c = 'A';");
    
    // Test 2: Function with character return
    debugTokens("fn int getValue() { ret 'A'; };");
    debugParsing("fn int getValue() { ret 'A'; };");
    
    // Test 3: Multi-statement with character
    debugTokens("char c = 'A'; int x = 5;");
    debugParsing("char c = 'A'; int x = 5;");
    
    // Test 4: The problematic case
    std::string problemCode = "fn int add(int a, int b) { ret a + b; }; char c = 'A'; int result = add(5, c);";
    debugTokens(problemCode);
    debugParsing(problemCode);
    
    // Test semantic analysis
    std::cout << "=== Semantic Debug ===" << std::endl;
    std::cout << "Code: " << problemCode << std::endl;
    
    try {
        Lexer lexer(problemCode);
        std::vector<Token> tokens = lexer.tokenize();
        
        Parser parser(tokens);
        auto program = parser.parseProgram();
        
        if (program) {
            SemanticAnalyzer analyzer;
            std::cout << "About to analyze " << program->statements.size() << " statements..." << std::endl;
            
            bool success = analyzer.analyze(program.get());
            
            if (success) {
                std::cout << "Semantic analysis: PASSED" << std::endl;
            } else {
                std::cout << "Semantic analysis: FAILED" << std::endl;
                analyzer.printErrors();
            }
        }
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    
    return 0;
} 