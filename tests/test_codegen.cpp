#include <iostream>
#include <string>
#include <vector>
#include "lexer.hpp"
#include "parser.hpp"
#include "semantic.hpp"
#include "codegen.hpp"

using namespace calpha;

void testCodeGeneration(const std::string& code, const std::string& testName) {
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
        
        // Step 4: Code Generation
        CodeGenerator codeGen(&analyzer);
        std::string alphaCode = codeGen.generate(program.get());
        
        std::cout << "Generated Alpha_TUI Assembly:" << std::endl;
        std::cout << "================================" << std::endl;
        std::cout << alphaCode << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Exception during code generation: " << e.what() << std::endl;
    } catch (...) {
        std::cout << "Unknown exception during code generation!" << std::endl;
    }
}

int main() {
    std::cout << "C-Alpha Code Generator Test" << std::endl;
    std::cout << "===========================" << std::endl;

    // Test 1: Simple variable declarations and arithmetic
    testCodeGeneration(
        "int x = 42; int y = 10; int result = x + y * 2;",
        "Basic Arithmetic"
    );

    // Test 2: Character literals
    testCodeGeneration(
        "char c = 'A'; int ascii = c + 1;",
        "Character Literals"
    );

    // Test 3: String literal assignment to char pointer
    testCodeGeneration(
        "->char text = \"Hello\"; char first = text[0]; char second = text[1];",
        "String Literal Assignment"
    );

    // Test 4: Escape sequences in string literals and character literals
    testCodeGeneration(
        "char newline = '\\n'; ->char message = \"Hello\\nWorld\\t!\"; char tab = '\\t';",
        "Escape Sequences"
    );

    // Test 5: Layout declaration (basic)
    testCodeGeneration(
        "layout Point { int x; int y; }; Point p; p.x = 10; p.y = 20;",
        "Layout Declaration"
    );

    // Test 6: More complex expressions
    testCodeGeneration(
        "int a = 5; int b = 3; int result = (a + b) * 2 - 1;",
        "Complex Expression"
    );

    // Test 7: Multiple operations
    testCodeGeneration(
        "int x = 10; int y = 20; int sum = x + y; int product = x * y; int diff = x - y;",
        "Multiple Operations"
    );

    // Test 8: If statement
    testCodeGeneration(
        "int x = 5; if (x > 3) { x = x + 1; } else { x = x - 1; }",
        "If Statement"
    );

    // Test 9: While loop
    testCodeGeneration(
        "int i = 0; while (i < 3) { i = i + 1; }",
        "While Loop"
    );

    // Test 10: Function declaration and call
    testCodeGeneration(
        "fn int add(int a, int b) { ret a + b; }; int result = add(5, 3);",
        "Function Declaration and Call"
    );

    // Test 11: Pointer operations
    testCodeGeneration(
        "int x = 42; ->int ptr = ->x; int value = <-ptr;",
        "Pointer Operations"
    );

    // Test 12: Comparison operations
    testCodeGeneration(
        "int a = 5; int b = 3; int equal = a == b; int greater = a > b;",
        "Comparison Operations"
    );

    // Test 13: Array operations
    testCodeGeneration(
        "->int arr = ~int[5]; arr[0] = 42; arr[1] = arr[0] + 10; int value = arr[1];",
        "Array Operations"
    );

    // Test 14: Layout member access
    testCodeGeneration(
        "layout example { int a; ->int b; }; example test; test.a = 2; test.b = ->test.a; int x = test.a;",
        "Layout Member Access"
    );

    // Test 15: Complex array and layout combination
    testCodeGeneration(
        "layout Point { int x; int y; }; ->Point points = ~Point[3]; points[0].x = 10; points[0].y = 20;",
        "Complex Array and Layout"
    );

    // Test 16: Nested member access with pointers
    testCodeGeneration(
        "layout Node { int data; ->Node next; }; Node n; n.data = 42; ->Node ptr = ->n; int value = ptr.data;",
        "Nested Member Access"
    );


    testCodeGeneration(
        ""
        "fn int fun (int a, int b, int c, int d, int e) {"
        "   ret a + b - c * d / e;"
        "};"
        ""
        "fn int main() {"
        "   int result = fun(1, 2, 3, 4, 5);"
        "};",
        "Multiple Complex Parameters in Function call");

    return 0;
} 