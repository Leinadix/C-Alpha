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
    
    // Test 5: Mixed layout and pointer members
    testLayoutCode(
        "layout Node { int data; ->Node next; }; "
        "fn int main() { "
        "   Node n; "
        "   n.data = 42; "
        "   ret n.data; "
        "}",
        "Mixed Layout and Pointer Members"
    );
    
    // Test 6: Array of layout types
    testLayoutCode(
        "layout Point { int x; int y; }; "
        "fn int main() { "
        "   ->Point points = ~Point[3]; "
        "   points[0].x = 10; "
        "   points[0].y = 20; "
        "   ret points[0].x; "
        "}",
        "Array of Layout Types"
    );
    
    // Test 7: Nested member access with multiple layouts
    testLayoutCode(
        "layout Vector { int x; int y; }; "
        "layout Entity { Vector pos; int health; }; "
        "fn int main() { "
        "   Entity e; "
        "   e.pos.x = 5; "
        "   e.pos.y = 10; "
        "   e.health = 100; "
        "   ret e.pos.x + e.pos.y + e.health; "
        "}",
        "Nested Layout Member Access"
    );
    
    // Test 8: Pointer to nested layout
    testLayoutCode(
        "layout Vector { int x; int y; }; "
        "layout Entity { Vector pos; int health; }; "
        "fn int main() { "
        "   ->Entity ptr = ~Entity[0]; "
        "   ptr.pos.x = 5; "
        "   ptr.pos.y = 10; "
        "   ptr.health = 100; "
        "   ret ptr.pos.x + ptr.pos.y + ptr.health; "
        "}",
        "Pointer to Nested Layout"
    );
    
    // Test 9: Complex layout with char members
    testLayoutCode(
        "layout Person { char grade; int age; }; "
        "fn int main() { "
        "   Person p; "
        "   p.grade = 'A'; "
        "   p.age = 25; "
        "   ret p.age; "
        "}",
        "Layout with Char Members"
    );
    
    // Test 10: Pointer to layout with char members
    testLayoutCode(
        "layout Person { char grade; int age; }; "
        "fn int main() { "
        "   ->Person ptr = ~Person[0]; "
        "   ptr.grade = 'A'; "
        "   ptr.age = 25; "
        "   ret ptr.age; "
        "}",
        "Pointer to Layout with Char Members"
    );
    
    std::cout << "\n=== Layout and Pointer Layout Types Test Suite Complete ===" << std::endl;
    
    return 0;
}