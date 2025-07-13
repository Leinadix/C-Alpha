#include <iostream>
#include <string>
#include <vector>
#include "lexer.hpp"
#include "parser.hpp"
#include "semantic.hpp"

using namespace calpha;

void testSemanticAnalysis(const std::string& code, const std::string& testName) {
    std::cout << "\n=== " << testName << " ===" << std::endl;
    std::cout << "Code: " << code << std::endl;
    
    try {
        // Tokenize
        Lexer lexer(code);
        std::vector<Token> tokens = lexer.tokenize();
        
        // Parse
        Parser parser(tokens);
        auto program = parser.parseProgram();
        
        if (!program) {
            std::cout << "Parse failed: program is null!" << std::endl;
            return;
        }
        
        if (program->statements.empty()) {
            std::cout << "Parse failed: no statements parsed!" << std::endl;
            return;
        }
        
        std::cout << "Parse successful: " << program->statements.size() << " statements" << std::endl;
        
        // Semantic Analysis
        SemanticAnalyzer analyzer;
        bool success = analyzer.analyze(program.get());
        
        if (success) {
            std::cout << "Semantic analysis: PASSED" << std::endl;
        } else {
            std::cout << "Semantic analysis: FAILED" << std::endl;
            analyzer.printErrors();
        }
        
        analyzer.printSymbolTable();
        
    } catch (const std::exception& e) {
        std::cout << "Exception during test: " << e.what() << std::endl;
    } catch (...) {
        std::cout << "Unknown exception during test!" << std::endl;
    }
    
    std::cout << std::endl;
}

int main() {
    std::cout << "C-Alpha Semantic Analyzer Test" << std::endl;
    std::cout << "===============================" << std::endl;

    // Test 1: Basic variable declaration and usage
    testSemanticAnalysis(
        "int x = 42; int y = x + 10;",
        "Basic Variable Declaration and Usage"
    );

    // Test 2: Undefined variable error
    testSemanticAnalysis(
        "int x = y + 10;",
        "Undefined Variable Error"
    );

    // Test 3: Type mismatch error
    testSemanticAnalysis(
        "int x = 42; char c = x;",
        "Type Mismatch Error"
    );

    // Test 4: Function declaration and call
    testSemanticAnalysis(
        "fn int add(int a, int b) { ret a + b; }; int result = add(5, 3);",
        "Function Declaration and Call"
    );

    // Test 5: Function call with wrong argument count
    testSemanticAnalysis(
        "fn int add(int a, int b) { ret a + b; }; int result = add(5);",
        "Function Call Wrong Argument Count"
    );

    // Test 6: Function call with wrong argument type
    testSemanticAnalysis(
        "fn int add(int a, int b) { ret a + b; }; char c = 'A'; int result = add(5, c);",
        "Function Call Wrong Argument Type"
    );

    // Test 7: Return type mismatch
    testSemanticAnalysis(
        "fn int getValue() { ret 'A'; };",
        "Return Type Mismatch"
    );

    // Test 8: Pointer operations
    testSemanticAnalysis(
        "int x = 42; ->int ptr = ->x; int value = <-ptr;",
        "Pointer Operations"
    );

    // Test 9: Array allocation and access
    testSemanticAnalysis(
        "->int arr = ~int[10]; int value = arr[0];",
        "Array Allocation and Access"
    );

    // Test 10: Complex expression with multiple types
    testSemanticAnalysis(
        "int a = 10; int b = 20; int result = (a + b) * 2 - 5;",
        "Complex Expression"
    );

    // Test 11: Nested scopes
    testSemanticAnalysis(
        "int x = 10; { int y = 20; x = x + y; } int z = x;",
        "Nested Scopes"
    );

    // Test 12: Variable shadowing
    testSemanticAnalysis(
        "int x = 10; { int x = 20; } int y = x;",
        "Variable Shadowing"
    );

    // Test 13: Use of uninitialized variable
    testSemanticAnalysis(
        "int x; int y = x + 10;",
        "Uninitialized Variable"
    );

    // Test 14: If statement with non-numeric condition
    testSemanticAnalysis(
        "->int ptr; if (ptr) { int x = 10; }",
        "If Statement Non-Numeric Condition"
    );

    // Test 15: Complete function with control flow
    testSemanticAnalysis(
        "fn int factorial(int n) { if (n <= 1) { ret 1; } else { ret n * factorial(n - 1); } };",
        "Complete Function with Control Flow"
    );

    // Test 16: Multiple function declarations
    testSemanticAnalysis(
        "fn int add(int a, int b) { ret a + b; }; fn int sub(int a, int b) { ret a - b; }; int result = add(10, sub(15, 5));",
        "Multiple Function Declarations"
    );

    // Test 17: Pointer to pointer
    testSemanticAnalysis(
        "int x = 42; ->int ptr1 = ->x; ->->int ptr2 = ->ptr1; int value = <-<-ptr2;",
        "Pointer to Pointer"
    );

    // Test 18: Array operations
    testSemanticAnalysis(
        "->int arr = ~int[10]; int value = arr[0];",
        "Array Operations"
    );
    
    // Test 19: Array with assignment and access
    testSemanticAnalysis(
        "int x = 10; ->int arr = ~int[5]; arr[0] = x; int value = arr[0];",
        "Array Assignment and Access"
    );

    // Test 20: Function redeclaration error
    testSemanticAnalysis(
        "fn int test() { ret 5; }; fn int test() { ret 10; };",
        "Function Redeclaration Error"
    );

    // Test 21: Variable redeclaration in same scope
    testSemanticAnalysis(
        "int x = 10; int x = 20;",
        "Variable Redeclaration Error"
    );

    return 0;
} 