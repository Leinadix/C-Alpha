#include "parser.hpp"
#include <iostream>
#include <sstream> // Required for std::ostringstream
#include <stdexcept>

namespace calpha {

Parser::Parser(const std::vector<Token> &tokens) : tokens(tokens), position(0) {
}

Token &Parser::currentToken() {
    if (position >= tokens.size()) {
        static Token eofToken(TokenType::END_OF_FILE, "", 0, 0);
        return eofToken;
    }
    if (tokens.empty()) {
        static Token eofToken(TokenType::END_OF_FILE, "", 0, 0);
        return eofToken;
    }
    return tokens[position];
}

Token &Parser::peek(int offset) {
    size_t pos = position + offset;
    if (pos >= tokens.size()) {
        static Token eofToken(TokenType::END_OF_FILE, "", 0, 0);
        return eofToken;
    }
    return tokens[pos];
}

bool Parser::isAtEnd() {
    return currentToken().type == TokenType::END_OF_FILE;
}

void Parser::advance() {
    if (!isAtEnd())
        position++;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) {
    if (isAtEnd())
        return false;
    return currentToken().type == type;
}

void Parser::consume(TokenType type, const std::string &message) {
    if (check(type)) {
        advance();
        return;
    }

    // Safe token access
    int line = 1, column = 1;
    if (position < tokens.size()) {
        line = tokens[position].line;
        column = tokens[position].column;
    }

    throw ParseError(message, line, column);
}

std::unique_ptr<Type> Parser::parseType() {
    int startLine = currentToken().line;
    int startColumn = currentToken().column;

    // Handle pointer prefix (->)
    if (match(TokenType::REFERENCE)) {
        try {
            // Recursively parse the type that this points to
            auto pointsTo = parseType();
            if (!pointsTo) {
                throw ParseError("Invalid type after '->' in pointer type",
                                 currentToken().line, currentToken().column);
            }
            return std::make_unique<PointerType>(std::move(pointsTo), startLine,
                                                 startColumn);
        } catch (const ParseError &e) {
            // Add more context to the error
            throw ParseError("Invalid pointer type: " + std::string(e.what()) +
                                 "\nExpected a valid type after '->'",
                             startLine, startColumn);
        }
    }

    // Handle basic types
    if (match(TokenType::INT)) {
        return std::make_unique<BasicType>(TokenType::INT, startLine,
                                           startColumn);
    }

    if (match(TokenType::CHAR)) {
        return std::make_unique<BasicType>(TokenType::CHAR, startLine,
                                           startColumn);
    }

    // Handle layout types (layout name used as type)
    if (check(TokenType::IDENTIFIER)) {
        std::string typeName = currentToken().value;
        advance();

        // Check for namespace qualification (e.g., heap.ArrayHeader)
        if (check(TokenType::DOT)) {
            advance(); // consume the dot
            if (!check(TokenType::IDENTIFIER)) {
                throw ParseError("Expected type name after namespace in '" +
                                     typeName + ".'",
                                 currentToken().line, currentToken().column);
            }
            typeName = typeName + "." + currentToken().value;
            advance();
        }

        return std::make_unique<LayoutType>(typeName, startLine, startColumn);
    }

    // Provide more helpful error messages
    if (check(TokenType::STRING_LITERAL)) {
        throw ParseError("String literals cannot be used as types. Use 'char' "
                         "for character type or '->char' for string pointer",
                         startLine, startColumn);
    }

    if (check(TokenType::INTEGER)) {
        throw ParseError("Integer literals cannot be used as types. Use 'int' "
                         "for integer type",
                         startLine, startColumn);
    }

    // Check if we're in a type cast context
    bool inTypeCast = false;
    if (position >= 2) {
        TokenType prevToken = tokens[position - 1].type;
        if (prevToken == TokenType::LESS_THAN) {
            inTypeCast = true;
        }
    }

    if (inTypeCast) {
        throw ParseError("Expected type after '<' in type cast. Valid types "
                         "are: int, char, layout name, or pointer type (->)",
                         startLine, startColumn);
    } else {
        throw ParseError("Expected type (int, char, layout name, or pointer "
                         "type starting with '->')",
                         startLine, startColumn);
    }
}

std::unique_ptr<Expression> Parser::parseExpression() {
    return parseLogicalOr();
}

std::unique_ptr<Expression> Parser::parseLogicalOr() {
    auto expr = parseLogicalAnd();

    while (match(TokenType::BITWISE_OR)) {
        TokenType op = tokens[position - 1].type;
        int line = tokens[position - 1].line;
        int column = tokens[position - 1].column;
        auto right = parseLogicalAnd();
        expr = std::make_unique<BinaryExpression>(
            std::move(expr), std::move(right), op, line, column);
    }

    return expr;
}

std::unique_ptr<Expression> Parser::parseLogicalAnd() {
    auto expr = parseEquality();

    while (match(TokenType::BITWISE_AND)) {
        TokenType op = tokens[position - 1].type;
        int line = tokens[position - 1].line;
        int column = tokens[position - 1].column;
        auto right = parseEquality();
        expr = std::make_unique<BinaryExpression>(
            std::move(expr), std::move(right), op, line, column);
    }

    return expr;
}

std::unique_ptr<Expression> Parser::parseEquality() {
    auto expr = parseComparison();

    while (match(TokenType::EQUAL) || match(TokenType::NOT_EQUAL)) {
        TokenType op = tokens[position - 1].type;
        int line = tokens[position - 1].line;
        int column = tokens[position - 1].column;
        auto right = parseComparison();
        expr = std::make_unique<BinaryExpression>(
            std::move(expr), std::move(right), op, line, column);
    }

    return expr;
}

std::unique_ptr<Expression> Parser::parseComparison() {
    auto expr = parseBitwiseOr();

    while (match(TokenType::GREATER_THAN) || match(TokenType::GREATER_EQUAL) ||
           match(TokenType::LESS_THAN) || match(TokenType::LESS_EQUAL)) {
        TokenType op = tokens[position - 1].type;
        int line = tokens[position - 1].line;
        int column = tokens[position - 1].column;
        auto right = parseBitwiseOr();
        expr = std::make_unique<BinaryExpression>(
            std::move(expr), std::move(right), op, line, column);
    }

    return expr;
}

std::unique_ptr<Expression> Parser::parseBitwiseOr() {
    auto expr = parseBitwiseXor();

    while (match(TokenType::BITWISE_OR)) {
        TokenType op = tokens[position - 1].type;
        int line = tokens[position - 1].line;
        int column = tokens[position - 1].column;
        auto right = parseBitwiseXor();
        expr = std::make_unique<BinaryExpression>(
            std::move(expr), std::move(right), op, line, column);
    }

    return expr;
}

std::unique_ptr<Expression> Parser::parseBitwiseXor() {
    auto expr = parseBitwiseAnd();

    while (match(TokenType::BITWISE_XOR)) {
        TokenType op = tokens[position - 1].type;
        int line = tokens[position - 1].line;
        int column = tokens[position - 1].column;
        auto right = parseBitwiseAnd();
        expr = std::make_unique<BinaryExpression>(
            std::move(expr), std::move(right), op, line, column);
    }

    return expr;
}

std::unique_ptr<Expression> Parser::parseBitwiseAnd() {
    auto expr = parseTerm();

    while (match(TokenType::BITWISE_AND)) {
        TokenType op = tokens[position - 1].type;
        int line = tokens[position - 1].line;
        int column = tokens[position - 1].column;
        auto right = parseTerm();
        expr = std::make_unique<BinaryExpression>(
            std::move(expr), std::move(right), op, line, column);
    }

    return expr;
}

std::unique_ptr<Expression> Parser::parseTerm() {
    auto expr = parseFactor();

    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        TokenType op = tokens[position - 1].type;
        int line = tokens[position - 1].line;
        int column = tokens[position - 1].column;
        auto right = parseFactor();
        expr = std::make_unique<BinaryExpression>(
            std::move(expr), std::move(right), op, line, column);
    }

    return expr;
}

std::unique_ptr<Expression> Parser::parseFactor() {
    auto expr = parseUnary();

    while (match(TokenType::MULTIPLY) || match(TokenType::DIVIDE) ||
           match(TokenType::MODULO)) {
        TokenType op = tokens[position - 1].type;
        int line = tokens[position - 1].line;
        int column = tokens[position - 1].column;
        auto right = parseUnary();
        expr = std::make_unique<BinaryExpression>(
            std::move(expr), std::move(right), op, line, column);
    }

    return expr;
}

std::unique_ptr<Expression> Parser::parseUnary() {
    // Handle special case for ~ (could be bitwise NOT or array allocation)
    if (check(TokenType::BITWISE_NOT)) {
        // Look ahead to determine if this is array allocation (~Type[size]) or
        // bitwise NOT
        if (position + 1 < tokens.size()) {
            TokenType nextToken = tokens[position + 1].type;
            // If ~ is followed by a type token, it's array allocation
            // This includes basic types (int, char), identifiers (layout
            // names), and pointer types (->)
            if (nextToken == TokenType::INT || nextToken == TokenType::CHAR ||
                nextToken == TokenType::IDENTIFIER ||
                nextToken == TokenType::REFERENCE) {
                // This is array allocation, let parsePrimary handle it
                return parsePrimary();
            }
        }
        // Otherwise, treat as bitwise NOT
        TokenType op = currentToken().type;
        int line = currentToken().line;
        int column = currentToken().column;
        advance(); // consume the ~
        auto expr = parseUnary();
        return std::make_unique<UnaryExpression>(std::move(expr), op, line,
                                                 column);
    }

    // Handle special case for -> (could be reference operator or start of
    // pointer type)
    if (check(TokenType::REFERENCE)) {
        // Look ahead to see if this is followed by a type token
        if (position + 1 < tokens.size()) {
            TokenType nextToken = tokens[position + 1].type;
            // If -> is followed by a type token, this might be a type, not a
            // unary operator
            if (nextToken == TokenType::INT || nextToken == TokenType::CHAR ||
                (nextToken == TokenType::IDENTIFIER &&
                 position + 2 < tokens.size() &&
                 tokens[position + 2].type != TokenType::LEFT_PAREN)) {
                // This looks like a type (->int, ->char, ->LayoutName but not
                // ->functionName()) Check if we're in a type cast context
                bool inTypeCast = false;
                if (position >= 2) {
                    TokenType prevToken = tokens[position - 1].type;
                    if (prevToken == TokenType::LESS_THAN) {
                        inTypeCast = true;
                    }
                }

                if (inTypeCast) {
                    // Let parseType handle it by returning to parsePrimary
                    return parsePrimary();
                }

                // Otherwise, it's an error in expression context
                throw ParseError(
                    "Unexpected pointer type in expression context. Did you "
                    "mean to use this in a variable declaration?",
                    currentToken().line, currentToken().column);
            }
        }
        // Otherwise, treat as reference operator
        TokenType op = currentToken().type;
        int line = currentToken().line;
        int column = currentToken().column;
        advance(); // consume the ->
        auto expr = parseUnary();
        return std::make_unique<UnaryExpression>(std::move(expr), op, line,
                                                 column);
    }

    if (match(TokenType::MINUS) || match(TokenType::DEREFERENCE)) {
        TokenType op = tokens[position - 1].type;
        int line = tokens[position - 1].line;
        int column = tokens[position - 1].column;
        auto expr = parseUnary();
        return std::make_unique<UnaryExpression>(std::move(expr), op, line,
                                                 column);
    }

    return parsePrimary();
}

std::unique_ptr<Expression> Parser::parsePrimary() {
    std::unique_ptr<Expression> expr;

    // First, parse a "prefix" expression
    if (match(TokenType::INTEGER)) {
        Token &token = tokens[position - 1];
        expr = std::make_unique<Literal>(token.value, TokenType::INTEGER,
                                         token.line, token.column);
    } else if (match(TokenType::CHARACTER)) {
        Token &token = tokens[position - 1];
        expr = std::make_unique<Literal>(token.value, TokenType::CHARACTER,
                                         token.line, token.column);
    } else if (match(TokenType::STRING_LITERAL)) {
        Token &token = tokens[position - 1];
        expr = std::make_unique<StringLiteral>(token.value, token.line,
                                               token.column);
    } else if (match(TokenType::LESS_THAN)) {
        int startLine = tokens[position - 1].line;
        int startColumn = tokens[position - 1].column;
        try {
            auto targetType = parseType();
            consume(TokenType::GREATER_THAN,
                    "Expected '>' after type in type cast");
            consume(TokenType::LEFT_PAREN, "Expected '(' after type cast");
            auto expression = parseExpression();
            consume(TokenType::RIGHT_PAREN,
                    "Expected ')' after cast expression");
            expr = std::make_unique<TypeCast>(std::move(targetType),
                                              std::move(expression), startLine,
                                              startColumn);
        } catch (const ParseError &e) {
            throw ParseError("Type cast error: " + std::string(e.what()),
                             startLine, startColumn);
        }
    } else if (match(TokenType::BITWISE_NOT)) {
        expr = parseArrayAllocation();
    } else if (check(TokenType::SYSCALL)) {
        int line = currentToken().line;
        int column = currentToken().column;
        advance();
        expr = parseSyscallExpression(line, column);
    } else if (check(TokenType::LEFT_BRACE)) {
        // Layout initialization: { value1, value2, ... }
        expr = parseLayoutInitialization();
    } else if (check(TokenType::IDENTIFIER)) {
        Token &token = tokens[position];
        expr =
            std::make_unique<Identifier>(token.value, token.line, token.column);
        advance();
    } else if (match(TokenType::LEFT_PAREN)) {
        expr = parseExpression();
        consume(TokenType::RIGHT_PAREN, "Expected ')' after expression");
    } else {
        throw ParseError("Expected expression", currentToken().line,
                         currentToken().column);
    }

    // Now, loop and parse any "postfix" operations
    while (true) {
        if (check(TokenType::LEFT_PAREN)) {
            int line = currentToken().line;
            int column = currentToken().column;
            std::string functionName = expr->toString();
            advance(); // consume '('
            auto arguments = parseArgumentList();
            consume(TokenType::RIGHT_PAREN,
                    "Expected ')' after function arguments");
            expr = std::make_unique<FunctionCall>(
                functionName, std::move(arguments), line, column);
        } else if (check(TokenType::LEFT_BRACKET)) {
            expr = parseArrayAccess(std::move(expr));
        } else if (check(TokenType::DOT)) {
            advance(); // consume '.'
            if (!check(TokenType::IDENTIFIER)) {
                throw ParseError("Expected member name after '.'",
                                 currentToken().line, currentToken().column);
            }
            Token &token = tokens[position];
            expr = std::make_unique<MemberAccess>(std::move(expr), token.value,
                                                  token.line, token.column);
            advance();
        } else {
            break;
        }
    }

    return expr;
}

std::unique_ptr<Statement> Parser::parseStatement() {
    if (isAtEnd()) {
        return nullptr;
    }

    try {
        if (match(TokenType::IMPORT)) {
            return parseImportStatement();
        }

        if (check(TokenType::NAMESPACE)) {
            return parseNamespaceDeclaration();
        }

        if (check(TokenType::FN)) {
            return parseFunctionDeclaration();
        }

        if (check(TokenType::LAYOUT)) {
            return parseLayoutDeclaration();
        }

        if (check(TokenType::INT) || check(TokenType::CHAR) ||
            check(TokenType::REFERENCE)) {
            return parseVariableDeclaration();
        }

        // Check for layout type variable declarations (identifier followed by
        // identifier)
        if (check(TokenType::IDENTIFIER)) {
            // Look ahead to see if this is a variable declaration (identifier
            // followed by identifier)
            if (position + 1 < tokens.size() &&
                tokens[position + 1].type == TokenType::IDENTIFIER) {
                return parseVariableDeclaration();
            }
        }

        if (check(TokenType::IF)) {
            return parseIfStatement();
        }

        if (check(TokenType::WHILE)) {
            return parseWhileStatement();
        }

        if (check(TokenType::RET)) {
            return parseReturnStatement();
        }

        if (check(TokenType::LEFT_BRACE)) {
            return parseBlockStatement();
        }

        // Try to parse as assignment or expression statement
        size_t savedPosition = position;
        try {
            auto expr = parseExpression();
            if (!expr) {
                position = savedPosition;
                throw ParseError("Failed to parse expression",
                                 currentToken().line, currentToken().column);
            }

            if (match(TokenType::ASSIGN)) {
                auto value = parseExpression();
                if (!value) {
                    position = savedPosition;
                    throw ParseError("Failed to parse assignment value",
                                     currentToken().line,
                                     currentToken().column);
                }
                consume(TokenType::SEMICOLON, "Expected ';' after assignment");
                return std::make_unique<Assignment>(std::move(expr),
                                                    std::move(value),
                                                    expr->line, expr->column);
            } else {
                consume(TokenType::SEMICOLON, "Expected ';' after expression");
                return std::make_unique<ExpressionStatement>(
                    std::move(expr), expr->line, expr->column);
            }
        } catch (const ParseError &e) {
            position = savedPosition;

            // Special case: if we see a pointer type pattern in expression
            // context, suggest it might be a missing variable declaration
            if (check(TokenType::REFERENCE) && position + 1 < tokens.size()) {
                TokenType nextToken = tokens[position + 1].type;
                if (nextToken == TokenType::INT ||
                    nextToken == TokenType::CHAR ||
                    nextToken == TokenType::IDENTIFIER) {
                    throw ParseError("Unexpected pointer type. Did you forget "
                                     "to declare this as a variable? (e.g., "
                                     "missing type before identifier)",
                                     currentToken().line,
                                     currentToken().column);
                }
            }

            throw ParseError("Expected statement", currentToken().line,
                             currentToken().column);
        }
    } catch (const ParseError &e) {
        // Re-throw with additional context
        throw ParseError("Statement parsing failed: " + std::string(e.what()),
                         e.getLine(), e.getColumn());
    }
}

std::unique_ptr<Statement> Parser::parseVariableDeclaration() {
    int startLine = currentToken().line;
    int startColumn = currentToken().column;

    auto type = parseType();

    if (!check(TokenType::IDENTIFIER)) {
        throw ParseError("Expected identifier in variable declaration",
                         currentToken().line, currentToken().column);
    }

    std::string name = currentToken().value;
    advance();

    std::unique_ptr<Expression> initializer = nullptr;
    if (match(TokenType::ASSIGN)) {
        try {
            initializer = parseExpression();
            if (!initializer) {
                throw ParseError("Invalid initializer expression",
                                 currentToken().line, currentToken().column);
            }
        } catch (const ParseError &e) {
            // Add more context to initialization errors
            throw ParseError("Error in variable initialization: " +
                                 std::string(e.what()),
                             currentToken().line, currentToken().column);
        }
    }

    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration");

    return std::make_unique<VariableDeclaration>(
        std::move(type), name, std::move(initializer), startLine, startColumn);
}

std::unique_ptr<Statement> Parser::parseAssignment() {
    auto target = parseExpression();
    consume(TokenType::ASSIGN, "Expected '=' in assignment");
    auto value = parseExpression();
    consume(TokenType::SEMICOLON, "Expected ';' after assignment");

    return std::make_unique<Assignment>(std::move(target), std::move(value),
                                        target->line, target->column);
}

std::unique_ptr<Statement> Parser::parseIfStatement() {
    int startLine = currentToken().line;
    int startColumn = currentToken().column;

    consume(TokenType::IF, "Expected 'if'");
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'if'");
    auto condition = parseExpression();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after if condition");

    auto thenStatement = parseStatement();

    std::unique_ptr<Statement> elseStatement = nullptr;
    if (match(TokenType::ELSE)) {
        elseStatement = parseStatement();
    }

    return std::make_unique<IfStatement>(
        std::move(condition), std::move(thenStatement),
        std::move(elseStatement), startLine, startColumn);
}

std::unique_ptr<Statement> Parser::parseWhileStatement() {
    int startLine = currentToken().line;
    int startColumn = currentToken().column;

    consume(TokenType::WHILE, "Expected 'while'");
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'while'");
    auto condition = parseExpression();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after while condition");

    auto body = parseStatement();

    return std::make_unique<WhileStatement>(
        std::move(condition), std::move(body), startLine, startColumn);
}

std::unique_ptr<Statement> Parser::parseReturnStatement() {
    int startLine = currentToken().line;
    int startColumn = currentToken().column;

    consume(TokenType::RET, "Expected 'ret'");

    std::unique_ptr<Expression> value = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        value = parseExpression();
    }

    consume(TokenType::SEMICOLON, "Expected ';' after return statement");

    return std::make_unique<ReturnStatement>(std::move(value), startLine,
                                             startColumn);
}

std::unique_ptr<Statement> Parser::parseBlockStatement() {
    int startLine = currentToken().line;
    int startColumn = currentToken().column;

    consume(TokenType::LEFT_BRACE, "Expected '{'");

    auto block = std::make_unique<BlockStatement>(startLine, startColumn);

    // Keep track of brace nesting level
    int braceLevel = 1;

    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        try {
            auto stmt = parseStatement();
            if (stmt) {
                block->statements.push_back(std::move(stmt));
            }
        } catch (const ParseError &e) {
            // Print error for debugging with line content and highlighting
            std::cerr << formatErrorMessage(e, "Parse Error in block: " +
                                                   std::string(e.what()))
                      << std::endl;

            // Store current position to ensure we make progress
            size_t errorPosition = position;

            // Error recovery: skip to next statement boundary or matching brace
            while (!isAtEnd()) {
                if (check(TokenType::LEFT_BRACE)) {
                    braceLevel++;
                    advance();
                } else if (check(TokenType::RIGHT_BRACE)) {
                    braceLevel--;
                    if (braceLevel == 0)
                        break;
                    advance();
                } else if (braceLevel == 1 &&
                           (check(TokenType::SEMICOLON) ||
                            check(TokenType::FN) || check(TokenType::INT) ||
                            check(TokenType::CHAR) ||
                            check(TokenType::REFERENCE) ||
                            check(TokenType::LAYOUT))) {
                    break;
                } else {
                    advance();
                }
            }

            // Skip the semicolon if we found one at the current brace level
            if (braceLevel == 1 && check(TokenType::SEMICOLON)) {
                advance();
            }

            // Ensure we've made progress - if not, force advance to avoid
            // infinite loop
            if (position == errorPosition && !isAtEnd()) {
                std::cerr << "Warning: Error recovery stuck in block, forcing "
                             "advance past token: "
                          << currentToken().value << std::endl;
                advance();
            }

            // Continue parsing the block
            continue;
        }
    }

    consume(TokenType::RIGHT_BRACE, "Expected '}' after block");

    return block;
}

std::unique_ptr<Statement> Parser::parseExpressionStatement() {
    auto expr = parseExpression();
    consume(TokenType::SEMICOLON, "Expected ';' after expression");

    return std::make_unique<ExpressionStatement>(std::move(expr), expr->line,
                                                 expr->column);
}

std::unique_ptr<Program> Parser::parseProgram() {
    auto program = std::make_unique<Program>(1, 1);

    // Keep track of brace nesting level for the whole program
    int braceLevel = 0;

    while (!isAtEnd()) {
        try {
            auto stmt = parseStatement();
            if (stmt) {
                program->statements.push_back(std::move(stmt));
            }
        } catch (const ParseError &e) {
            // Print error for debugging with line content and highlighting
            std::cerr << formatErrorMessage(e, "Parse Error: " +
                                                   std::string(e.what()))
                      << std::endl;

            // Store current position to ensure we make progress
            size_t errorPosition = position;

            // Error recovery: skip to next statement boundary or matching brace
            while (!isAtEnd()) {
                if (check(TokenType::LEFT_BRACE)) {
                    braceLevel++;
                    advance();
                } else if (check(TokenType::RIGHT_BRACE)) {
                    braceLevel--;
                    advance();
                    if (braceLevel <= 0) {
                        braceLevel = 0;
                        break;
                    }
                } else if (braceLevel == 0 &&
                           (check(TokenType::SEMICOLON) ||
                            check(TokenType::FN) || check(TokenType::INT) ||
                            check(TokenType::CHAR) ||
                            check(TokenType::REFERENCE) ||
                            check(TokenType::LAYOUT))) {
                    break;
                } else {
                    advance();
                }
            }

            // Skip the semicolon if we found one at the top level
            if (braceLevel == 0 && check(TokenType::SEMICOLON)) {
                advance();
            }

            // Ensure we've made progress - if not, force advance to avoid
            // infinite loop
            if (position == errorPosition && !isAtEnd()) {
                std::cerr << "Warning: Error recovery stuck, forcing advance "
                             "past token: "
                          << currentToken().value << std::endl;
                advance();
            }

            // Continue parsing
            continue;
        } catch (const std::exception &e) {
            std::cerr << "Unexpected error: " << e.what() << std::endl;
            break;
        } catch (...) {
            std::cerr << "Unknown error occurred during parsing" << std::endl;
            break;
        }
    }

    return program;
}

std::unique_ptr<Statement> Parser::parseFunctionDeclaration() {
    int startLine = currentToken().line;
    int startColumn = currentToken().column;

    consume(TokenType::FN, "Expected 'fn'");

    // Parse return type
    auto returnType = parseType();

    // Parse function name
    if (!check(TokenType::IDENTIFIER)) {
        throw ParseError("Expected function name", currentToken().line,
                         currentToken().column);
    }

    std::string name = currentToken().value;
    advance();

    // Parse parameter list
    consume(TokenType::LEFT_PAREN, "Expected '(' after function name");
    auto parameters = parseParameterList();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after parameters");

    // Parse function body
    auto body = std::unique_ptr<BlockStatement>(
        static_cast<BlockStatement *>(parseBlockStatement().release()));

    consume(TokenType::SEMICOLON, "Expected ';' after function definition");

    return std::make_unique<FunctionDeclaration>(
        std::move(returnType), name, std::move(parameters), std::move(body),
        startLine, startColumn);
}

std::unique_ptr<Statement> Parser::parseLayoutDeclaration() {
    int startLine = currentToken().line;
    int startColumn = currentToken().column;

    consume(TokenType::LAYOUT, "Expected 'layout'");

    // Parse layout name
    if (!check(TokenType::IDENTIFIER)) {
        throw ParseError("Expected layout name", currentToken().line,
                         currentToken().column);
    }

    std::string name = currentToken().value;
    advance();

    // Parse member list
    consume(TokenType::LEFT_BRACE, "Expected '{' after layout name");
    auto members = parseLayoutMemberList();
    consume(TokenType::RIGHT_BRACE, "Expected '}' after layout members");

    consume(TokenType::SEMICOLON, "Expected ';' after layout declaration");

    return std::make_unique<LayoutDeclaration>(name, std::move(members),
                                               startLine, startColumn);
}

std::unique_ptr<Parameter> Parser::parseParameter() {
    int startLine = currentToken().line;
    int startColumn = currentToken().column;

    auto type = parseType();

    if (!check(TokenType::IDENTIFIER)) {
        throw ParseError("Expected parameter name", currentToken().line,
                         currentToken().column);
    }

    std::string name = currentToken().value;
    advance();

    return std::make_unique<Parameter>(std::move(type), name, startLine,
                                       startColumn);
}

std::vector<std::unique_ptr<Parameter>> Parser::parseParameterList() {
    std::vector<std::unique_ptr<Parameter>> parameters;

    // Empty parameter list
    if (check(TokenType::RIGHT_PAREN)) {
        return parameters;
    }

    // Parse first parameter
    parameters.push_back(parseParameter());

    // Parse remaining parameters
    while (match(TokenType::COMMA)) {
        parameters.push_back(parseParameter());
    }

    return parameters;
}

std::unique_ptr<LayoutMember> Parser::parseLayoutMember() {
    int startLine = currentToken().line;
    int startColumn = currentToken().column;

    auto type = parseType();

    if (!check(TokenType::IDENTIFIER)) {
        throw ParseError("Expected member name", currentToken().line,
                         currentToken().column);
    }

    std::string name = currentToken().value;
    advance();

    consume(TokenType::SEMICOLON, "Expected ';' after layout member");

    return std::make_unique<LayoutMember>(std::move(type), name, startLine,
                                          startColumn);
}

std::vector<std::unique_ptr<LayoutMember>> Parser::parseLayoutMemberList() {
    std::vector<std::unique_ptr<LayoutMember>> members;

    // Parse members until we hit the closing brace
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        members.push_back(parseLayoutMember());
    }

    return members;
}

std::vector<std::unique_ptr<Expression>> Parser::parseArgumentList() {
    std::vector<std::unique_ptr<Expression>> arguments;

    // Empty argument list
    if (check(TokenType::RIGHT_PAREN)) {
        return arguments;
    }

    // Parse first argument
    arguments.push_back(parseExpression());

    // Parse remaining arguments
    while (match(TokenType::COMMA)) {
        arguments.push_back(parseExpression());
    }

    return arguments;
}

std::unique_ptr<Expression> Parser::parseArrayAllocation() {
    int line = currentToken().line;
    int column = currentToken().column;

    // The ~ has already been consumed, now parse the element type (int, char,
    // or layout type)
    auto elementType = parseType();

    consume(TokenType::LEFT_BRACKET,
            "Expected '[' after array type in ~Type[size] allocation");
    auto size = parseExpression();
    consume(TokenType::RIGHT_BRACKET,
            "Expected ']' after array size in ~Type[size] allocation");

    return std::make_unique<ArrayAllocation>(std::move(elementType),
                                             std::move(size), line, column);
}

std::unique_ptr<Expression>
Parser::parseArrayAccess(std::unique_ptr<Expression> array) {
    int line = currentToken().line;
    int column = currentToken().column;

    consume(TokenType::LEFT_BRACKET, "Expected '[' for array access");
    auto index = parseExpression();
    consume(TokenType::RIGHT_BRACKET, "Expected ']' after array index");

    return std::make_unique<ArrayAccess>(std::move(array), std::move(index),
                                         line, column);
}

std::unique_ptr<Expression> Parser::parseSyscallExpression(int line,
                                                           int column) {
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'syscall'");
    auto arguments = parseArgumentList();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after syscall arguments");

    return std::make_unique<SyscallExpression>(std::move(arguments), line,
                                               column);
}

std::unique_ptr<Expression> Parser::parseLayoutInitialization() {
    int line = currentToken().line;
    int column = currentToken().column;
    
    consume(TokenType::LEFT_BRACE, "Expected '{' for layout initialization");
    
    std::vector<std::unique_ptr<Expression>> values;
    
    // Empty initialization list
    if (check(TokenType::RIGHT_BRACE)) {
        consume(TokenType::RIGHT_BRACE, "Expected '}'");
        return std::make_unique<LayoutInitialization>(std::move(values), line, column);
    }
    
    // Parse initialization values
    values.push_back(parseExpression());
    
    while (match(TokenType::COMMA)) {
        values.push_back(parseExpression());
    }
    
    consume(TokenType::RIGHT_BRACE, "Expected '}' after layout initialization");
    
    return std::make_unique<LayoutInitialization>(std::move(values), line, column);
}

std::unique_ptr<Statement> Parser::parseImportStatement() {
    int startLine = currentToken().line;
    int startColumn = currentToken().column;

    if (!check(TokenType::STRING_LITERAL)) {
        throw ParseError("Expected string literal for import path",
                         currentToken().line, currentToken().column);
    }

    std::string path = currentToken().value;
    advance();

    consume(TokenType::SEMICOLON, "Expected ';' after import statement");

    return std::make_unique<ImportStatement>(path, startLine, startColumn);
}

// Add new method for parsing namespace declarations
std::unique_ptr<Statement> Parser::parseNamespaceDeclaration() {
    int startLine = currentToken().line;
    int startColumn = currentToken().column;

    consume(TokenType::NAMESPACE, "Expected 'namespace' keyword");

    if (!check(TokenType::IDENTIFIER)) {
        throw ParseError("Expected namespace name", currentToken().line,
                         currentToken().column);
    }

    std::string name = currentToken().value;
    advance();

    consume(TokenType::LEFT_BRACE, "Expected '{' after namespace name");

    std::vector<std::unique_ptr<Statement>> statements;
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        auto stmt = parseStatement();
        if (stmt) {
            statements.push_back(std::move(stmt));
        }
    }

    consume(TokenType::RIGHT_BRACE, "Expected '}' after namespace body");
    consume(TokenType::SEMICOLON, "Expected ';' after namespace declaration");

    return std::make_unique<NamespaceDeclaration>(name, std::move(statements),
                                                  startLine, startColumn);
}

// Helper function to get line content and format error message with ANSI colors
std::string Parser::formatErrorMessage(const ParseError &e,
                                       const std::string &message) {
    // Find the token at the error position
    Token *errorToken = nullptr;
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i].line == e.getLine() &&
            tokens[i].column == e.getColumn()) {
            errorToken = &tokens[i];
            break;
        }
    }

    // Build the error message
    std::ostringstream error;
    if (errorToken && !errorToken->sourceFile.empty()) {
        error << message << " in file '" << errorToken->sourceFile
              << "' at line " << e.getLine() << ", column " << e.getColumn()
              << "\n";
    } else {
        error << message << " at line " << e.getLine() << ", column "
              << e.getColumn() << "\n";
    }

    // Find the line content
    std::string lineContent;
    size_t lineStartPos = 0;
    size_t lineEndPos = 0;
    int currentLine = 1;

    // Find the start of the error line
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i].line == e.getLine()) {
            // Found first token on error line
            lineStartPos = i;
            break;
        }
    }

    // Find the end of the error line
    for (size_t i = lineStartPos; i < tokens.size(); i++) {
        if (tokens[i].line > e.getLine()) {
            // Found first token of next line
            lineEndPos = i - 1;
            break;
        }
        if (i == tokens.size() - 1) {
            // Last line
            lineEndPos = i;
        }
    }

    // Build the line content with error highlighting
    error << "  ";
    for (size_t i = lineStartPos; i <= lineEndPos; i++) {
        if (errorToken && &tokens[i] == errorToken) {
            // Highlight error token in red
            error << "\033[1;31m" << tokens[i].value << "\033[0m";
        } else {
            error << tokens[i].value;
        }

        // Add space between tokens
        if (i < lineEndPos) {
            error << " ";
        }
    }
    error << "\n";

    // Add caret line pointing to the error
    error << "  " << std::string(e.getColumn() - 1, ' ')
          << "\033[1;31m^\033[0m";

    return error.str();
}

} // namespace calpha