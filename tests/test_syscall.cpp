#include <iostream>
#include <string>
#include "lexer.hpp"
#include "parser.hpp"
#include "semantic.hpp"
#include "codegen.hpp"

using namespace calpha;

int main() {
    // Test syscall implementation
    std::string code = "syscall(0, 1, 2, 3, 4, 5, 8);";
    
    std::cout << "=== Testing Syscall Implementation ===" << std::endl;
    std::cout << "Code: " << code << std::endl << std::endl;
    
    try {
        // Tokenize
        Lexer lexer(code);
        auto tokens = lexer.tokenize();
        
        std::cout << "=== Tokens ===" << std::endl;
        for (const auto& token : tokens) {
            std::cout << "Token: '" << token.value << "' Type: " << Lexer::tokenTypeToString(token.type) << std::endl;
        }
        std::cout << std::endl;
        
        // Parse
        Parser parser(tokens);
        auto program = parser.parseProgram();
        
        std::cout << "=== Parsing ===" << std::endl;
        if (program && !program->statements.empty()) {
            std::cout << "Parse successful: " << program->statements.size() << " statements" << std::endl;
            auto stmt = program->statements[0].get();
            if (stmt->nodeType == NodeType::EXPRESSION_STATEMENT) {
                auto exprStmt = dynamic_cast<const ExpressionStatement*>(stmt);
                if (exprStmt->expression->nodeType == NodeType::SYSCALL_EXPRESSION) {
                    auto syscallExpr = dynamic_cast<const SyscallExpression*>(exprStmt->expression.get());
                    std::cout << "Found syscall expression with " << syscallExpr->arguments.size() << " arguments" << std::endl;
                }
            }
        }
        std::cout << std::endl;
        
        // Semantic analysis
        SemanticAnalyzer analyzer;
        bool semanticSuccess = analyzer.analyze(program.get());
        
        std::cout << "=== Semantic Analysis ===" << std::endl;
        if (semanticSuccess) {
            std::cout << "Semantic analysis passed!" << std::endl;
        } else {
            std::cout << "Semantic analysis failed with " << analyzer.getErrors().size() << " errors:" << std::endl;
            for (const auto& error : analyzer.getErrors()) {
                std::cout << "  Error: " << error.message << " at line " << error.line << ", column " << error.column << std::endl;
            }
        }
        std::cout << std::endl;
        
        // Code generation
        CodeGenerator codegen(&analyzer);
        std::string assembly = codegen.generate(program.get());
        
        std::cout << "=== Generated Assembly ===" << std::endl;
        std::cout << assembly << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
    
    return 0;
} 