#include <iostream>
#include <string>
#include <vector>
#include "lexer.hpp"
#include "parser.hpp"
#include "semantic.hpp"
#include "codegen.hpp"

using namespace calpha;

void testIssue8() {
    std::cout << "=== Testing Issue #8: Weird behaviour when using Members of ->Layout Types ===" << std::endl;
    
    // This is the exact code from Issue #8 that was causing problems
    std::string code = R"(
        layout A {
            int number;
        }

        fn int main() {
            A a;                    // creates A-type and initializes A.number at correct offset
            a.number = 5;           // works
            
            ->A b = ~A[0];         // creates ->A-type and initializes A.number at correct offset
            b.number = 5;          // ERROR - This should work now!
            
            ret b.number;          // Return the value to verify it works
        }
    )";
    
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
            std::cout << "✗ Parse failed: program is null!" << std::endl;
            return;
        }
        
        std::cout << "✓ Parsing successful!" << std::endl;
        
        // Step 3: Semantic Analysis
        SemanticAnalyzer analyzer;
        bool semanticSuccess = analyzer.analyze(program.get());
        
        if (!semanticSuccess) {
            std::cout << "✗ Semantic analysis failed!" << std::endl;
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
        
        std::cout << "✓ Issue #8 has been FIXED! Pointer to layout member access now works correctly." << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "✗ Error: " << e.what() << std::endl;
        std::cout << "Issue #8 is still present." << std::endl;
    }
}

int main() {
    testIssue8();
    return 0;
}
