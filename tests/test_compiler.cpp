#include "lexer.hpp"
#include "parser.hpp"
#include "semantic.hpp"
#include <iostream>
#include <sstream>

using namespace calpha;

void printTokens(const std::vector<Token>& tokens) {
    std::cout << "=== TOKENS ===" << std::endl;
    for (const auto& token : tokens) {
        std::cout << "Line " << token.line << ", Column " << token.column 
                  << ": " << Lexer::tokenTypeToString(token.type) 
                  << " [" << token.value << "]" << std::endl;
    }
    std::cout << std::endl;
}

void printSemanticAnalysis(SemanticAnalyzer& analyzer, bool success) {
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
    analyzer.printSymbolTable();
}

void printAST(const ASTNode* node, int depth = 0) {
    if (!node) return;
    
    std::string indent(depth * 2, ' ');
    std::cout << indent;
    
    switch (node->nodeType) {
        case NodeType::PROGRAM:
            std::cout << "Program" << std::endl;
            {
                const auto* prog = dynamic_cast<const Program*>(node);
                for (const auto& stmt : prog->statements) {
                    printAST(stmt.get(), depth + 1);
                }
            }
            break;
            
        case NodeType::VARIABLE_DECLARATION:
            std::cout << "VariableDeclaration" << std::endl;
            {
                const VariableDeclaration* decl = dynamic_cast<const VariableDeclaration*>(node);
                std::cout << indent << "  Type: ";
                printAST(decl->type.get(), 0);
                std::cout << indent << "  Name: " << decl->name << std::endl;
                if (decl->initializer) {
                    std::cout << indent << "  Initializer:" << std::endl;
                    printAST(decl->initializer.get(), depth + 2);
                }
            }
            break;
            
        case NodeType::ASSIGNMENT:
            std::cout << "Assignment" << std::endl;
            {
                const Assignment* assign = dynamic_cast<const Assignment*>(node);
                std::cout << indent << "  Target:" << std::endl;
                printAST(assign->target.get(), depth + 2);
                std::cout << indent << "  Value:" << std::endl;
                printAST(assign->value.get(), depth + 2);
            }
            break;
            
        case NodeType::BINARY_EXPRESSION:
            std::cout << "BinaryExpression" << std::endl;
            {
                const BinaryExpression* binExpr = dynamic_cast<const BinaryExpression*>(node);
                std::cout << indent << "  Operator: " << Lexer::tokenTypeToString(binExpr->operator_) << std::endl;
                std::cout << indent << "  Left:" << std::endl;
                printAST(binExpr->left.get(), depth + 2);
                std::cout << indent << "  Right:" << std::endl;
                printAST(binExpr->right.get(), depth + 2);
            }
            break;
            
        case NodeType::UNARY_EXPRESSION:
            std::cout << "UnaryExpression" << std::endl;
            {
                const UnaryExpression* unExpr = dynamic_cast<const UnaryExpression*>(node);
                std::cout << indent << "  Operator: " << Lexer::tokenTypeToString(unExpr->operator_) << std::endl;
                std::cout << indent << "  Operand:" << std::endl;
                printAST(unExpr->operand.get(), depth + 2);
            }
            break;
            
        case NodeType::IDENTIFIER:
            {
                const Identifier* id = dynamic_cast<const Identifier*>(node);
                std::cout << "Identifier: " << id->name << std::endl;
            }
            break;
            
        case NodeType::LITERAL:
            {
                const Literal* lit = dynamic_cast<const Literal*>(node);
                std::cout << "Literal: " << lit->value << " (" << Lexer::tokenTypeToString(lit->literalType) << ")" << std::endl;
            }
            break;
            
        case NodeType::BASIC_TYPE:
            {
                const BasicType* type = dynamic_cast<const BasicType*>(node);
                std::cout << "BasicType: " << Lexer::tokenTypeToString(type->baseType) << std::endl;
            }
            break;
            
        case NodeType::POINTER_TYPE:
            {
                const PointerType* type = dynamic_cast<const PointerType*>(node);
                std::cout << "PointerType -> ";
                printAST(type->pointsTo.get(), 0);
            }
            break;
            
        case NodeType::BLOCK_STATEMENT:
            std::cout << "BlockStatement" << std::endl;
            {
                const BlockStatement* block = dynamic_cast<const BlockStatement*>(node);
                for (const auto& stmt : block->statements) {
                    printAST(stmt.get(), depth + 1);
                }
            }
            break;
            
        case NodeType::EXPRESSION_STATEMENT:
            std::cout << "ExpressionStatement" << std::endl;
            {
                const ExpressionStatement* exprStmt = dynamic_cast<const ExpressionStatement*>(node);
                printAST(exprStmt->expression.get(), depth + 1);
            }
            break;
            
        case NodeType::FUNCTION_DECLARATION:
            std::cout << "FunctionDeclaration" << std::endl;
            {
                const FunctionDeclaration* funcDecl = dynamic_cast<const FunctionDeclaration*>(node);
                std::cout << indent << "  Name: " << funcDecl->name << std::endl;
                std::cout << indent << "  Return Type: ";
                printAST(funcDecl->returnType.get(), 0);
                std::cout << indent << "  Parameters (" << funcDecl->parameters.size() << "):" << std::endl;
                for (const auto& param : funcDecl->parameters) {
                    printAST(param.get(), depth + 2);
                }
                std::cout << indent << "  Body:" << std::endl;
                printAST(funcDecl->body.get(), depth + 1);
            }
            break;
            
        case NodeType::PARAMETER:
            std::cout << "Parameter" << std::endl;
            {
                const Parameter* param = dynamic_cast<const Parameter*>(node);
                std::cout << indent << "  Name: " << param->name << std::endl;
                std::cout << indent << "  Type: ";
                printAST(param->type.get(), 0);
            }
            break;
            
        case NodeType::RETURN_STATEMENT:
            std::cout << "ReturnStatement" << std::endl;
            {
                const ReturnStatement* retStmt = dynamic_cast<const ReturnStatement*>(node);
                if (retStmt->value) {
                    std::cout << indent << "  Value:" << std::endl;
                    printAST(retStmt->value.get(), depth + 1);
                } else {
                    std::cout << indent << "  No return value" << std::endl;
                }
            }
            break;
            
        case NodeType::IF_STATEMENT:
            std::cout << "IfStatement" << std::endl;
            {
                const IfStatement* ifStmt = dynamic_cast<const IfStatement*>(node);
                std::cout << indent << "  Condition:" << std::endl;
                printAST(ifStmt->condition.get(), depth + 1);
                std::cout << indent << "  Then:" << std::endl;
                printAST(ifStmt->thenStatement.get(), depth + 1);
                if (ifStmt->elseStatement) {
                    std::cout << indent << "  Else:" << std::endl;
                    printAST(ifStmt->elseStatement.get(), depth + 1);
                }
            }
            break;
            
        case NodeType::WHILE_STATEMENT:
            std::cout << "WhileStatement" << std::endl;
            {
                const WhileStatement* whileStmt = dynamic_cast<const WhileStatement*>(node);
                std::cout << indent << "  Condition:" << std::endl;
                printAST(whileStmt->condition.get(), depth + 1);
                std::cout << indent << "  Body:" << std::endl;
                printAST(whileStmt->body.get(), depth + 1);
            }
            break;
            
        case NodeType::FUNCTION_CALL:
            std::cout << "FunctionCall" << std::endl;
            {
                const FunctionCall* funcCall = dynamic_cast<const FunctionCall*>(node);
                std::cout << indent << "  Function: " << funcCall->functionName << std::endl;
                std::cout << indent << "  Arguments (" << funcCall->arguments.size() << "):" << std::endl;
                for (const auto& arg : funcCall->arguments) {
                    printAST(arg.get(), depth + 2);
                }
            }
            break;
            
        case NodeType::ARRAY_ALLOCATION:
            std::cout << "ArrayAllocation" << std::endl;
            {
                const ArrayAllocation* arrayAlloc = dynamic_cast<const ArrayAllocation*>(node);
                std::cout << indent << "  Element Type: ";
                printAST(arrayAlloc->elementType.get(), 0);
                std::cout << indent << "  Size:" << std::endl;
                printAST(arrayAlloc->size.get(), depth + 1);
            }
            break;
            
        case NodeType::ARRAY_ACCESS:
            std::cout << "ArrayAccess" << std::endl;
            {
                const ArrayAccess* arrayAccess = dynamic_cast<const ArrayAccess*>(node);
                std::cout << indent << "  Array:" << std::endl;
                printAST(arrayAccess->array.get(), depth + 1);
                std::cout << indent << "  Index:" << std::endl;
                printAST(arrayAccess->index.get(), depth + 1);
            }
            break;
            
        case NodeType::LAYOUT_DECLARATION:
            std::cout << "LayoutDeclaration" << std::endl;
            {
                const LayoutDeclaration* layoutDecl = dynamic_cast<const LayoutDeclaration*>(node);
                std::cout << indent << "  Name: " << layoutDecl->name << std::endl;
                std::cout << indent << "  Members (" << layoutDecl->members.size() << "):" << std::endl;
                for (const auto& member : layoutDecl->members) {
                    std::cout << indent << "    Member: " << member->name << " (";
                    printAST(member->type.get(), 0);
                    std::cout << ")" << std::endl;
                }
            }
            break;
            
        case NodeType::LAYOUT_TYPE:
            {
                const LayoutType* layoutType = dynamic_cast<const LayoutType*>(node);
                std::cout << "LayoutType: " << layoutType->layoutName << std::endl;
            }
            break;
            
        case NodeType::MEMBER_ACCESS:
            std::cout << "MemberAccess" << std::endl;
            {
                const auto* memberAccess = dynamic_cast<const MemberAccess*>(node);
                std::cout << indent << "  Object:" << std::endl;
                printAST(memberAccess->object.get(), depth + 1);
                std::cout << indent << "  Member: " << memberAccess->memberName << std::endl;
            }
            break;
            
        default:
            std::cout << "Unknown Node Type" << std::endl;
            break;
    }
}

int main() {
    // Test basic variables and function
    std::string code = "int a = 42;";
    code += "\n->int b = ->a;";
    code += "\nchar c = \'A\';";
    code += "\nint result = c + 5 * 2;";
    code += "\nfn int add(int x, int y) { ret x + y; };";
    code += "\nfn char getChar() { ret \"Z\"; };";
    code += "\nfn int test_if(int a, int b) { if (a == b) { ret a; } else { ret b; } };";
    code += "\nfn int countdown(int n) { while (n > 0) { n = n - 1; } ret n; };";
    code += "\n->int array = ~int[20];";
    code += "\narray[0] = 42;";
    code += "\nint sum = add(a, array[0]);";
    code += "\nchar letter = getChar();";
    code += "\nif (a > 5) { result = a * 2; } else { result = a + 1; }";
    code += "\nwhile (result > 0) { result = result - 1; }";

    // Add layout test
    code += "\nlayout example { int a; ->int b; };";
    code += "\nexample test;";
    code += "\ntest.a = 2;";
    code += "\ntest.b = ->test.a;";
    code += "\n->example test_ptr = ->test;";

    std::cout << "=== Testing C-Alpha Code with Layouts ===" << std::endl;
    std::cout << "Code: " << code << std::endl << std::endl;

    try {
        Lexer l(code);
        auto tokens = l.tokenize();
        printTokens(tokens);

        Parser p(tokens);
        auto ast = p.parseProgram();
        std::cout << "=== AST ===" << std::endl;
        printAST(ast.get());

        SemanticAnalyzer analyzer;
        bool success = analyzer.analyze(ast.get());
        printSemanticAnalysis(analyzer, success);

    } catch (const Parser::ParseError& e) {
        std::cout << "Parse Error: " << e.what() << " at line " << e.getLine() << ", column " << e.getColumn() << std::endl;
    }
    
    return 0;
} 