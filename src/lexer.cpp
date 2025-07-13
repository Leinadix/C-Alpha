#include "lexer.hpp"
#include <cctype>
#include <unordered_map>

namespace calpha {

Lexer::Lexer(const std::string& source, const std::string& mainFile) 
    : source(source), position(0), line(1), column(1), currentFile(mainFile) {
    // Initialize first line with main file
    lineToFile[1] = mainFile;
    importStack.push_back(mainFile);
}

void Lexer::updateSourceFile(const std::string& line) {
    // Check for file markers in comments
    const std::string startMarker = "// Start of imported file: ";
    const std::string endMarker = "// End of imported file: ";
    
    if (line.find(startMarker) == 0) {
        // Extract filename from comment - take the rest of the line after the marker
        std::string importedFile = line.substr(startMarker.length());
        // Remove any trailing whitespace
        importedFile = importedFile.substr(0, importedFile.find_last_not_of(" \n\r\t") + 1);
        
        // Push the imported file onto the stack
        importStack.push_back(importedFile);
        currentFile = importedFile;
        lineToFile[this->line + 1] = currentFile; // Apply to next line
    } else if (line.find(endMarker) == 0) {
        // Pop the current file from the stack
        if (!importStack.empty()) {
            importStack.pop_back();
            // Return to the previous file in the stack
            currentFile = importStack.back();
            lineToFile[this->line + 1] = currentFile; // Apply to next line
        }
    }
}

std::string Lexer::getSourceFile(int line) const {
    // Find the most recent file declaration before or at this line
    int lastLine = 1;
    for (const auto& [l, f] : lineToFile) {
        if (l <= line && l > lastLine) {
            lastLine = l;
        }
    }
    auto it = lineToFile.find(lastLine);
    return it != lineToFile.end() ? it->second : "";
}

char Lexer::currentChar() const {
    if (position >= source.length()) return '\0';
    return source[position];
}

char Lexer::peek(int offset) const {
    size_t pos = position + offset;
    if (pos >= source.length()) return '\0';
    return source[pos];
}

void Lexer::advance() {
    if (position < source.length()) {
        if (source[position] == '\n') {
            // Before advancing to next line, check if we need to update source file
            std::string currentLine = source.substr(position - column + 1, column - 1);
            updateSourceFile(currentLine);
            
            line++;
            column = 1;
        } else {
            column++;
        }
        position++;
    }
}

void Lexer::skipWhitespace() {
    while (std::isspace(currentChar())) {
        advance();
    }
}

void Lexer::skipComment() {
    if (currentChar() == '/' && peek() == '/') {
        while (currentChar() != '\n' && currentChar() != '\0') {
            advance();
        }
    }
}

Token Lexer::scanNumber() {
    int startLine = line;
    int startColumn = column;
    std::string value;
    
    while (std::isdigit(currentChar())) {
        value += currentChar();
        advance();
    }
    
    return Token(TokenType::INTEGER, value, startLine, startColumn, getSourceFile(startLine));
}

Token Lexer::scanCharacter() {
    int startLine = line;
    int startColumn = column;
    
    advance(); // Skip opening quote '
    std::string value;
    
    if (currentChar() != '\'' && currentChar() != '\0') {
        if (currentChar() == '\\') {
            // Handle escape sequences
            advance(); // Skip the backslash
            char nextChar = currentChar();
            switch (nextChar) {
                case 'n':
                    value += '\n'; // Newline
                    break;
                case 't':
                    value += '\t'; // Tab
                    break;
                case 'r':
                    value += '\r'; // Carriage return
                    break;
                case '0':
                    value += '\0'; // Null terminator
                    break;
                case '\\':
                    value += '\\'; // Backslash
                    break;
                case '\'':
                    value += '\''; // Single quote
                    break;
                default:
                    // For unknown escape sequences, keep the backslash and the character
                    value += '\\';
                    value += nextChar;
                    break;
            }
            advance();
        } else {
            value += currentChar();
            advance();
        }
    }
    
    if (currentChar() == '\'') {
        advance(); // Skip closing quote '
    }
    
    return Token(TokenType::CHARACTER, value, startLine, startColumn, getSourceFile(startLine));
}

Token Lexer::scanStringLiteral() {
    int startLine = line;
    int startColumn = column;
    
    advance(); // Skip opening quote "
    std::string value;
    
    while (currentChar() != '"' && currentChar() != '\0') {
        if (currentChar() == '\\') {
            // Handle escape sequences
            advance(); // Skip the backslash
            char nextChar = currentChar();
            switch (nextChar) {
                case 'n':
                    value += '\n'; // Newline
                    break;
                case 't':
                    value += '\t'; // Tab
                    break;
                case 'r':
                    value += '\r'; // Carriage return
                    break;
                case '0':
                    value += '\0'; // Null terminator
                    break;
                case '\\':
                    value += '\\'; // Backslash
                    break;
                case '"':
                    value += '"'; // Double quote
                    break;
                default:
                    // For unknown escape sequences, keep the backslash and the character
                    value += '\\';
                    value += nextChar;
                    break;
            }
            advance();
        } else {
            value += currentChar();
            advance();
        }
    }
    
    if (currentChar() == '"') {
        advance(); // Skip closing quote "
    }
    
    return Token(TokenType::STRING_LITERAL, value, startLine, startColumn, getSourceFile(startLine));
}

Token Lexer::scanIdentifier() {
    int startLine = line;
    int startColumn = column;
    std::string value;
    
    while (std::isalnum(currentChar()) || currentChar() == '_') {
        value += currentChar();
        advance();
    }
    
    TokenType type = isKeyword(value) ? getKeywordType(value) : TokenType::IDENTIFIER;
    return Token(type, value, startLine, startColumn, getSourceFile(startLine));
}

Token Lexer::scanOperator() {
    int startLine = line;
    int startColumn = column;
    char c = currentChar();
    std::string sourceFile = getSourceFile(startLine);
    
    switch (c) {
        case '+':
            advance();
            return Token(TokenType::PLUS, "+", startLine, startColumn, sourceFile);
        case '-':
            advance();
            if (currentChar() == '>') {
                advance();
                return Token(TokenType::REFERENCE, "->", startLine, startColumn, sourceFile);
            }
            return Token(TokenType::MINUS, "-", startLine, startColumn, sourceFile);
        case '*':
            advance();
            return Token(TokenType::MULTIPLY, "*", startLine, startColumn, sourceFile);
        case '/':
            advance();
            return Token(TokenType::DIVIDE, "/", startLine, startColumn, sourceFile);
        case '%':
            advance();
            return Token(TokenType::MODULO, "%", startLine, startColumn, sourceFile);
        case '&':
            advance();
            return Token(TokenType::BITWISE_AND, "&", startLine, startColumn, sourceFile);
        case '|':
            advance();
            return Token(TokenType::BITWISE_OR, "|", startLine, startColumn, sourceFile);
        case '^':
            advance();
            return Token(TokenType::BITWISE_XOR, "^", startLine, startColumn, sourceFile);
        case '~':
            advance();
            return Token(TokenType::BITWISE_NOT, "~", startLine, startColumn, sourceFile);
        case '=':
            advance();
            if (currentChar() == '=') {
                advance();
                return Token(TokenType::EQUAL, "==", startLine, startColumn, sourceFile);
            }
            return Token(TokenType::ASSIGN, "=", startLine, startColumn, sourceFile);
        case '!':
            advance();
            if (currentChar() == '=') {
                advance();
                return Token(TokenType::NOT_EQUAL, "!=", startLine, startColumn, sourceFile);
            }
            return Token(TokenType::INVALID, "!", startLine, startColumn, sourceFile);
        case '<':
            advance();
            if (currentChar() == '=') {
                advance();
                return Token(TokenType::LESS_EQUAL, "<=", startLine, startColumn, sourceFile);
            } else if (currentChar() == '-') {
                advance();
                return Token(TokenType::DEREFERENCE, "<-", startLine, startColumn, sourceFile);
            }
            return Token(TokenType::LESS_THAN, "<", startLine, startColumn, sourceFile);
        case '>':
            advance();
            if (currentChar() == '=') {
                advance();
                return Token(TokenType::GREATER_EQUAL, ">=", startLine, startColumn, sourceFile);
            }
            return Token(TokenType::GREATER_THAN, ">", startLine, startColumn, sourceFile);
        case ';':
            advance();
            return Token(TokenType::SEMICOLON, ";", startLine, startColumn, sourceFile);
        case '(':
            advance();
            return Token(TokenType::LEFT_PAREN, "(", startLine, startColumn, sourceFile);
        case ')':
            advance();
            return Token(TokenType::RIGHT_PAREN, ")", startLine, startColumn, sourceFile);
        case '{':
            advance();
            return Token(TokenType::LEFT_BRACE, "{", startLine, startColumn, sourceFile);
        case '}':
            advance();
            return Token(TokenType::RIGHT_BRACE, "}", startLine, startColumn, sourceFile);
        case '[':
            advance();
            return Token(TokenType::LEFT_BRACKET, "[", startLine, startColumn, sourceFile);
        case ']':
            advance();
            return Token(TokenType::RIGHT_BRACKET, "]", startLine, startColumn, sourceFile);
        case ',':
            advance();
            return Token(TokenType::COMMA, ",", startLine, startColumn, sourceFile);
        case '.':
            advance();
            return Token(TokenType::DOT, ".", startLine, startColumn, sourceFile);
        default:
            advance();
            return Token(TokenType::INVALID, std::string(1, c), startLine, startColumn, sourceFile);
    }
}

Token Lexer::nextToken() {
    skipWhitespace();
    
    if (currentChar() == '/' && peek() == '/') {
        skipComment();
        return nextToken();
    }
    
    if (currentChar() == '\0') {
        return Token(TokenType::END_OF_FILE, "", line, column, getSourceFile(line));
    }
    
    if (std::isdigit(currentChar())) {
        return scanNumber();
    }
    
    if (currentChar() == '"') {
        return scanStringLiteral();
    }
    
    if (currentChar() == '\'') {
        return scanCharacter();
    }
    
    if (std::isalpha(currentChar()) || currentChar() == '_') {
        return scanIdentifier();
    }
    
    return scanOperator();
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    Token token = nextToken();
    
    while (token.type != TokenType::END_OF_FILE) {
        tokens.push_back(token);
        token = nextToken();
    }
    
    tokens.push_back(token); // Add EOF token
    return tokens;
}

std::string Lexer::tokenTypeToString(TokenType type) {
    static const std::unordered_map<TokenType, std::string> tokenNames = {
        {TokenType::INTEGER, "INTEGER"},
        {TokenType::CHARACTER, "CHARACTER"},
        {TokenType::STRING_LITERAL, "STRING_LITERAL"},
        {TokenType::IDENTIFIER, "IDENTIFIER"},
        {TokenType::INT, "INT"},
        {TokenType::CHAR, "CHAR"},
        {TokenType::IF, "IF"},
        {TokenType::ELSE, "ELSE"},
        {TokenType::WHILE, "WHILE"},
        {TokenType::FN, "FN"},
        {TokenType::RET, "RET"},
        {TokenType::PLUS, "PLUS"},
        {TokenType::MINUS, "MINUS"},
        {TokenType::MULTIPLY, "MULTIPLY"},
        {TokenType::DIVIDE, "DIVIDE"},
        {TokenType::MODULO, "MODULO"},
        {TokenType::BITWISE_AND, "BITWISE_AND"},
        {TokenType::BITWISE_OR, "BITWISE_OR"},
        {TokenType::BITWISE_XOR, "BITWISE_XOR"},
        {TokenType::BITWISE_NOT, "BITWISE_NOT"},
        {TokenType::ASSIGN, "ASSIGN"},
        {TokenType::REFERENCE, "REFERENCE"},
        {TokenType::DEREFERENCE, "DEREFERENCE"},
        {TokenType::EQUAL, "EQUAL"},
        {TokenType::NOT_EQUAL, "NOT_EQUAL"},
        {TokenType::LESS_THAN, "LESS_THAN"},
        {TokenType::GREATER_THAN, "GREATER_THAN"},
        {TokenType::LESS_EQUAL, "LESS_EQUAL"},
        {TokenType::GREATER_EQUAL, "GREATER_EQUAL"},
        {TokenType::SEMICOLON, "SEMICOLON"},
        {TokenType::LEFT_PAREN, "LEFT_PAREN"},
        {TokenType::RIGHT_PAREN, "RIGHT_PAREN"},
        {TokenType::LEFT_BRACE, "LEFT_BRACE"},
        {TokenType::RIGHT_BRACE, "RIGHT_BRACE"},
        {TokenType::LEFT_BRACKET, "LEFT_BRACKET"},
        {TokenType::RIGHT_BRACKET, "RIGHT_BRACKET"},
        {TokenType::COMMA, "COMMA"},
        {TokenType::DOT, "DOT"},
        {TokenType::LAYOUT, "LAYOUT"},
        {TokenType::SYSCALL, "SYSCALL"},
        {TokenType::END_OF_FILE, "END_OF_FILE"},
        {TokenType::INVALID, "INVALID"}
    };
    
    auto it = tokenNames.find(type);
    return it != tokenNames.end() ? it->second : "UNKNOWN";
}

bool Lexer::isKeyword(const std::string& identifier) {
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"int", TokenType::INT},
        {"char", TokenType::CHAR},
        {"if", TokenType::IF},
        {"else", TokenType::ELSE},
        {"while", TokenType::WHILE},
        {"fn", TokenType::FN},
        {"ret", TokenType::RET},
        {"layout", TokenType::LAYOUT},
        {"syscall", TokenType::SYSCALL},
        {"import", TokenType::IMPORT},
        {"namespace", TokenType::NAMESPACE}  // Add namespace keyword
    };
    
    return keywords.find(identifier) != keywords.end();
}

TokenType Lexer::getKeywordType(const std::string& identifier) {
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"int", TokenType::INT},
        {"char", TokenType::CHAR},
        {"if", TokenType::IF},
        {"else", TokenType::ELSE},
        {"while", TokenType::WHILE},
        {"fn", TokenType::FN},
        {"ret", TokenType::RET},
        {"layout", TokenType::LAYOUT},
        {"syscall", TokenType::SYSCALL},
        {"import", TokenType::IMPORT},
        {"namespace", TokenType::NAMESPACE}  // Add namespace keyword
    };
    
    auto it = keywords.find(identifier);
    return it != keywords.end() ? it->second : TokenType::IDENTIFIER;
}

} // namespace calpha 