#include <iostream>
#include <string>
#include <vector>
#include "lexer.hpp"
#include "parser.hpp"
#include "semantic.hpp"
#include "codegen.hpp"

using namespace calpha;

void testLayoutCode(const std::string& code, const std::string& testName) {
    std::cout << "\n=== " << testName << " ===" << std::endl;
    std::cout << "C-Alpha Code:" << std::endl;
    std::cout << code << std::endl;
    std::cout << std::endl;
    
    try {
        // Step 1: Lexical Analysis
        Lexer lexer(code);
        std::vector<Token> tokens = lexer.tokenize();
        
        // Step 2: Syntactic Analysis
        Parser parser(tokens);
        auto program = parser.parseProgram();
        
        if (!program) {
            std::cout << "Parse failed: program is null!" << std::endl;
            return;
        }
        
        // Step 3: Semantic Analysis
        SemanticAnalyzer analyzer;
        bool semanticSuccess = analyzer.analyze(program.get());
        
        if (!semanticSuccess) {
            std::cout << "Semantic analysis failed!" << std::endl;
            analyzer.printErrors();
            return;
        }
        
        std::cout << "✓ Semantic analysis passed!" << std::endl;
        
        // Step 4: Code Generation
        CodeGenerator codeGen(&analyzer);
        std::string generatedCode = codeGen.generate(program.get());
        
        std::cout << "✓ Code generation passed!" << std::endl;
        std::cout << "Generated Assembly:" << std::endl;
        std::cout << generatedCode << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "✗ Error: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "=== Layout and Pointer Layout Types Test Suite ===" << std::endl;
    std::cout << "Testing Issue #7: Add special Test for Layout and ->Layout Types" << std::endl;
    std::cout << "Testing Issue #8: Weird behaviour when using Members of ->Layout Types" << std::endl;
    
    // Test 1: Basic layout declaration and direct member access
    testLayoutCode(
        "layout A { int number; }; "
        "fn int main() { "
        "   A a; "
        "   a.number = 5; "
        "   ret a.number; "
        "}",
        "Basic Layout Direct Member Access"
    );
    
    // Test 2: Pointer to layout member access (Issue #8 case)
    testLayoutCode(
        "layout A { int number; }; "
        "fn int main() { "
        "   ->A b = ~A[0]; "
        "   b.number = 5; "
        "   ret b.number; "
        "}",
        "Pointer to Layout Member Access (Issue #8)"
    );
    
    // Test 3: Multiple members in layout
    testLayoutCode(
        "layout Point { int x; int y; }; "
        "fn int main() { "
        "   Point p; "
        "   p.x = 10; "
        "   p.y = 20; "
        "   ret p.x + p.y; "
        "}",
        "Multiple Members Layout"
    );
    
    // Test 4: Multiple members with pointer to layout
    testLayoutCode(
        "layout Point { int x; int y; }; "
        "fn int main() { "
        "   ->Point ptr = ~Point[0]; "
        "   ptr.x = 10; "
        "   ptr.y = 20; "
        "   ret ptr.x + ptr.y; "
        "}",
        "Multiple Members Pointer to Layout"
    );
    
    std::cout << "\n=== Layout and Pointer Layout Types Test Suite Complete ===" << std::endl;
    
    return 0;
}
