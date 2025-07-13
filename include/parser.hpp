#pragma once
#include "lexer.hpp"
#include <memory>
#include <vector>

namespace calpha {

// Forward declarations
class ASTNode;
class Expression;
class Statement;
class Type;

// AST Node Types
enum class NodeType {
    PROGRAM,
    VARIABLE_DECLARATION,
    ASSIGNMENT,
    BINARY_EXPRESSION,
    UNARY_EXPRESSION,
    IDENTIFIER,
    LITERAL,
    STRING_LITERAL,
    IF_STATEMENT,
    WHILE_STATEMENT,
    FUNCTION_DECLARATION,
    RETURN_STATEMENT,
    BLOCK_STATEMENT,
    EXPRESSION_STATEMENT,
    POINTER_TYPE,
    BASIC_TYPE,
    PARAMETER,
    FUNCTION_CALL,
    ARRAY_ALLOCATION,
    ARRAY_ACCESS,
    LAYOUT_DECLARATION,
    LAYOUT_TYPE,
    MEMBER_ACCESS,
    SYSCALL_EXPRESSION,
    TYPE_CAST,
    NAMESPACE_DECLARATION,  // Add namespace declaration node type
    NAMESPACE_ACCESS,       // Add namespace access node type
    IMPORT_STATEMENT        // Add import statement node type
};

// Base AST Node
class ASTNode {
public:
    NodeType nodeType;
    int line;
    int column;
    
    ASTNode(NodeType type, int line, int column) 
        : nodeType(type), line(line), column(column) {}
    
    virtual ~ASTNode() = default;
};

// Type system
class Type : public ASTNode {
public:
    Type(NodeType type, int line, int column) : ASTNode(type, line, column) {}
    virtual ~Type() = default;
    virtual std::string toString() const = 0;
};

class BasicType : public Type {
public:
    TokenType baseType; // INT or CHAR
    
    BasicType(TokenType type, int line, int column) 
        : Type(NodeType::BASIC_TYPE, line, column), baseType(type) {}
    
    std::string toString() const override {
        switch (baseType) {
            case TokenType::INT: return "int";
            case TokenType::CHAR: return "char";
            default: return "unknown";
        }
    }
};

class PointerType : public Type {
public:
    std::unique_ptr<Type> pointsTo;
    
    PointerType(std::unique_ptr<Type> pointsTo, int line, int column)
        : Type(NodeType::POINTER_TYPE, line, column), pointsTo(std::move(pointsTo)) {}
    
    std::string toString() const override {
        return "->" + pointsTo->toString();
    }
};

class LayoutType : public Type {
public:
    std::string layoutName;
    
    LayoutType(const std::string& name, int line, int column)
        : Type(NodeType::LAYOUT_TYPE, line, column), layoutName(name) {}
    
    std::string toString() const override {
        return layoutName;
    }
};

// Expressions
class Expression : public ASTNode {
public:
    Expression(NodeType type, int line, int column) : ASTNode(type, line, column) {}
    virtual std::string toString() const = 0;
};

class Literal : public Expression {
public:
    std::string value;
    TokenType literalType;
    
    Literal(const std::string& value, TokenType type, int line, int column)
        : Expression(NodeType::LITERAL, line, column), value(value), literalType(type) {}
    
    std::string toString() const override { return value; }
};

class StringLiteral : public Expression {
public:
    std::string value;
    
    StringLiteral(const std::string& value, int line, int column)
        : Expression(NodeType::STRING_LITERAL, line, column), value(value) {}
    
    std::string toString() const override { return "\"" + value + "\""; }
};

class Identifier : public Expression {
public:
    std::string name;
    
    Identifier(const std::string& name, int line, int column)
        : Expression(NodeType::IDENTIFIER, line, column), name(name) {}
    
    std::string toString() const override { return name; }
};

class BinaryExpression : public Expression {
public:
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    TokenType operator_;
    
    BinaryExpression(std::unique_ptr<Expression> left, std::unique_ptr<Expression> right, 
                    TokenType op, int line, int column)
        : Expression(NodeType::BINARY_EXPRESSION, line, column), 
          left(std::move(left)), right(std::move(right)), operator_(op) {}
    
    std::string toString() const override {
        return "(" + left->toString() + " " + getOperatorString(operator_) + " " + right->toString() + ")";
    }
private:
    std::string getOperatorString(TokenType op) const {
        switch (op) {
            case TokenType::PLUS: return "+";
            case TokenType::MINUS: return "-";
            case TokenType::MULTIPLY: return "*";
            case TokenType::DIVIDE: return "/";
            case TokenType::MODULO: return "%";
            case TokenType::EQUAL: return "==";
            case TokenType::NOT_EQUAL: return "!=";
            case TokenType::LESS_THAN: return "<";
            case TokenType::LESS_EQUAL: return "<=";
            case TokenType::GREATER_THAN: return ">";
            case TokenType::GREATER_EQUAL: return ">=";
            case TokenType::BITWISE_AND: return "&&";
            case TokenType::BITWISE_OR: return "||";
            default: return "?";
        }
    }
};

class UnaryExpression : public Expression {
public:
    std::unique_ptr<Expression> operand;
    TokenType operator_;
    
    UnaryExpression(std::unique_ptr<Expression> operand, TokenType op, int line, int column)
        : Expression(NodeType::UNARY_EXPRESSION, line, column), 
          operand(std::move(operand)), operator_(op) {}
    
    std::string toString() const override {
        return std::string(operator_ == TokenType::MINUS ? "-" : "!") + operand->toString();
    }
};

class FunctionCall : public Expression {
public:
    std::string functionName;
    std::vector<std::unique_ptr<Expression>> arguments;
    
    FunctionCall(const std::string& name, std::vector<std::unique_ptr<Expression>> args, 
                int line, int column)
        : Expression(NodeType::FUNCTION_CALL, line, column), 
          functionName(name), arguments(std::move(args)) {}
    
    std::string toString() const override {
        std::string result = functionName + "(";
        for (size_t i = 0; i < arguments.size(); ++i) {
            if (i > 0) result += ", ";
            result += arguments[i]->toString();
        }
        return result + ")";
    }
};

class ArrayAllocation : public Expression {
public:
    std::unique_ptr<Type> elementType;
    std::unique_ptr<Expression> size;
    
    ArrayAllocation(std::unique_ptr<Type> elementType, std::unique_ptr<Expression> size, 
                   int line, int column)
        : Expression(NodeType::ARRAY_ALLOCATION, line, column), 
          elementType(std::move(elementType)), size(std::move(size)) {}
    
    std::string toString() const override {
        return "new[" + size->toString() + "]";
    }
};

class ArrayAccess : public Expression {
public:
    std::unique_ptr<Expression> array;
    std::unique_ptr<Expression> index;
    
    ArrayAccess(std::unique_ptr<Expression> array, std::unique_ptr<Expression> index, 
               int line, int column)
        : Expression(NodeType::ARRAY_ACCESS, line, column), 
          array(std::move(array)), index(std::move(index)) {}
    
    std::string toString() const override {
        return array->toString() + "[" + index->toString() + "]";
    }
};

class MemberAccess : public Expression {
public:
    std::unique_ptr<Expression> object;
    std::string memberName;
    
    MemberAccess(std::unique_ptr<Expression> object, const std::string& memberName, 
                int line, int column)
        : Expression(NodeType::MEMBER_ACCESS, line, column), 
          object(std::move(object)), memberName(memberName) {}
    
    std::string toString() const override {
        return object->toString() + "." + memberName;
    }
};

class SyscallExpression : public Expression {
public:
    std::vector<std::unique_ptr<Expression>> arguments;
    
    SyscallExpression(std::vector<std::unique_ptr<Expression>> args, int line, int column)
        : Expression(NodeType::SYSCALL_EXPRESSION, line, column), 
          arguments(std::move(args)) {}
    
    std::string toString() const override {
        std::string result = "syscall(";
        for (size_t i = 0; i < arguments.size(); ++i) {
            if (i > 0) result += ", ";
            result += arguments[i]->toString();
        }
        return result + ")";
    }
};

class TypeCast : public Expression {
public:
    std::unique_ptr<Type> targetType;
    std::unique_ptr<Expression> expression;
    
    TypeCast(std::unique_ptr<Type> targetType, std::unique_ptr<Expression> expression, int line, int column)
        : Expression(NodeType::TYPE_CAST, line, column), 
          targetType(std::move(targetType)), expression(std::move(expression)) {}
    
    std::string toString() const override {
        return "<" + targetType->toString() + ">(" + expression->toString() + ")";
    }
};

// Statements
class Statement : public ASTNode {
public:
    Statement(NodeType type, int line, int column) : ASTNode(type, line, column) {}
};

class VariableDeclaration : public Statement {
public:
    std::unique_ptr<Type> type;
    std::string name;
    std::unique_ptr<Expression> initializer;
    
    VariableDeclaration(std::unique_ptr<Type> type, const std::string& name, 
                       std::unique_ptr<Expression> initializer, int line, int column)
        : Statement(NodeType::VARIABLE_DECLARATION, line, column), 
          type(std::move(type)), name(name), initializer(std::move(initializer)) {}
};

class Assignment : public Statement {
public:
    std::unique_ptr<Expression> target;
    std::unique_ptr<Expression> value;
    
    Assignment(std::unique_ptr<Expression> target, std::unique_ptr<Expression> value, 
              int line, int column)
        : Statement(NodeType::ASSIGNMENT, line, column), 
          target(std::move(target)), value(std::move(value)) {}
};

class BlockStatement : public Statement {
public:
    std::vector<std::unique_ptr<Statement>> statements;
    
    BlockStatement(int line, int column) : Statement(NodeType::BLOCK_STATEMENT, line, column) {}
};

class ExpressionStatement : public Statement {
public:
    std::unique_ptr<Expression> expression;
    
    ExpressionStatement(std::unique_ptr<Expression> expression, int line, int column)
        : Statement(NodeType::EXPRESSION_STATEMENT, line, column), 
          expression(std::move(expression)) {}
};

class IfStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> thenStatement;
    std::unique_ptr<Statement> elseStatement;
    
    IfStatement(std::unique_ptr<Expression> condition, 
               std::unique_ptr<Statement> thenStatement,
               std::unique_ptr<Statement> elseStatement, int line, int column)
        : Statement(NodeType::IF_STATEMENT, line, column), 
          condition(std::move(condition)), thenStatement(std::move(thenStatement)), 
          elseStatement(std::move(elseStatement)) {}
};

class WhileStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;
    
    WhileStatement(std::unique_ptr<Expression> condition, 
                  std::unique_ptr<Statement> body, int line, int column)
        : Statement(NodeType::WHILE_STATEMENT, line, column), 
          condition(std::move(condition)), body(std::move(body)) {}
};

class ReturnStatement : public Statement {
public:
    std::unique_ptr<Expression> value;
    
    ReturnStatement(std::unique_ptr<Expression> value, int line, int column)
        : Statement(NodeType::RETURN_STATEMENT, line, column), value(std::move(value)) {}
};

class Parameter : public ASTNode {
public:
    std::unique_ptr<Type> type;
    std::string name;
    
    Parameter(std::unique_ptr<Type> type, const std::string& name, int line, int column)
        : ASTNode(NodeType::PARAMETER, line, column), type(std::move(type)), name(name) {}
};

class FunctionDeclaration : public Statement {
public:
    std::unique_ptr<Type> returnType;
    std::string name;
    std::vector<std::unique_ptr<Parameter>> parameters;
    std::unique_ptr<BlockStatement> body;
    
    FunctionDeclaration(std::unique_ptr<Type> returnType, const std::string& name,
                       std::vector<std::unique_ptr<Parameter>> parameters,
                       std::unique_ptr<BlockStatement> body, int line, int column)
        : Statement(NodeType::FUNCTION_DECLARATION, line, column), 
          returnType(std::move(returnType)), name(name), 
          parameters(std::move(parameters)), body(std::move(body)) {}
};

class LayoutMember : public ASTNode {
public:
    std::unique_ptr<Type> type;
    std::string name;
    
    LayoutMember(std::unique_ptr<Type> type, const std::string& name, int line, int column)
        : ASTNode(NodeType::PARAMETER, line, column), type(std::move(type)), name(name) {}
};

class LayoutDeclaration : public Statement {
public:
    std::string name;
    std::vector<std::unique_ptr<LayoutMember>> members;
    
    LayoutDeclaration(const std::string& name, std::vector<std::unique_ptr<LayoutMember>> members,
                     int line, int column)
        : Statement(NodeType::LAYOUT_DECLARATION, line, column), 
          name(name), members(std::move(members)) {}
};

class Program : public ASTNode {
public:
    std::vector<std::unique_ptr<Statement>> statements;
    
    Program(int line, int column) : ASTNode(NodeType::PROGRAM, line, column) {}
};

// Add after other statement classes
class NamespaceDeclaration : public Statement {
public:
    std::string name;
    std::vector<std::unique_ptr<Statement>> statements;
    
    NamespaceDeclaration(const std::string& name, std::vector<std::unique_ptr<Statement>> statements,
                        int line, int column)
        : Statement(NodeType::NAMESPACE_DECLARATION, line, column),
          name(name), statements(std::move(statements)) {}
};

// Represents an import statement (e.g., import "file.calpha";)
class ImportStatement : public Statement {
public:
    std::string path;

    ImportStatement(const std::string& path, int line, int column)
        : Statement(NodeType::IMPORT_STATEMENT, line, column), path(path) {}

    std::string toString() const {
        return "ImportStatement(" + path + ")";
    }
};

// Add after other expression classes
class NamespaceAccess : public Expression {
public:
    std::string namespaceName;
    std::unique_ptr<Expression> member;
    
    NamespaceAccess(const std::string& namespaceName, std::unique_ptr<Expression> member, int line, int column)
        : Expression(NodeType::NAMESPACE_ACCESS, line, column), namespaceName(namespaceName), member(std::move(member)) {}

    std::string toString() const override {
        return namespaceName + "." + member->toString();
    }
};

// Parser class
class Parser {
private:
    std::vector<Token> tokens;
    size_t position;
    
    Token& currentToken();
    Token& peek(int offset = 1);
    bool isAtEnd();
    void advance();
    bool match(TokenType type);
    bool check(TokenType type);
    void consume(TokenType type, const std::string& message);
    
    // Parsing methods
    std::unique_ptr<Type> parseType();
    std::unique_ptr<Expression> parseExpression();
    std::unique_ptr<Expression> parseLogicalOr();
    std::unique_ptr<Expression> parseLogicalAnd();
    std::unique_ptr<Expression> parseEquality();
    std::unique_ptr<Expression> parseComparison();
    std::unique_ptr<Expression> parseBitwiseOr();
    std::unique_ptr<Expression> parseBitwiseXor();
    std::unique_ptr<Expression> parseBitwiseAnd();
    std::unique_ptr<Expression> parseTerm();
    std::unique_ptr<Expression> parseFactor();
    std::unique_ptr<Expression> parseUnary();
    std::unique_ptr<Expression> parsePrimary();
    
    std::unique_ptr<Statement> parseStatement();
    std::unique_ptr<Statement> parseVariableDeclaration();
    std::unique_ptr<Statement> parseAssignment();
    std::unique_ptr<Statement> parseIfStatement();
    std::unique_ptr<Statement> parseWhileStatement();
    std::unique_ptr<Statement> parseReturnStatement();
    std::unique_ptr<Statement> parseBlockStatement();
    std::unique_ptr<Statement> parseExpressionStatement();
    std::unique_ptr<Statement> parseFunctionDeclaration();
    std::unique_ptr<Statement> parseLayoutDeclaration();
    std::unique_ptr<Statement> parseNamespaceDeclaration();
    std::unique_ptr<Statement> parseImportStatement();
    
    // Helper methods
    std::unique_ptr<Parameter> parseParameter();
    std::vector<std::unique_ptr<Parameter>> parseParameterList();
    std::unique_ptr<LayoutMember> parseLayoutMember();
    std::vector<std::unique_ptr<LayoutMember>> parseLayoutMemberList();
    std::unique_ptr<Expression> parseFunctionCall(const std::string& functionName, int line, int column);
    std::unique_ptr<Expression> parseSyscallExpression(int line, int column);
    std::vector<std::unique_ptr<Expression>> parseArgumentList();
    std::unique_ptr<Expression> parseArrayAllocation();
    std::unique_ptr<Expression> parseArrayAccess(std::unique_ptr<Expression> array);

    class ParseError; // Forward declaration for error handling

    // Helper function to format error messages with line content and ANSI colors
    std::string formatErrorMessage(const ParseError& e, const std::string& message);

public:
    explicit Parser(const std::vector<Token>& tokens);
    
    std::unique_ptr<Program> parseProgram();
    
    // Error handling
    class ParseError : public std::exception {
    private:
        std::string message;
        int line, column;
        
    public:
        ParseError(const std::string& msg, int line, int column)
            : message(msg), line(line), column(column) {}
        
        const char* what() const noexcept override { return message.c_str(); }
        int getLine() const { return line; }
        int getColumn() const { return column; }
    };
};

} // namespace calpha 