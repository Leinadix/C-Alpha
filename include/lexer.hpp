#ifndef LEXER_HPP
#define LEXER_HPP
#include <memory>
#include <string>
#include <unordered_map> // Added for lineToFile
#include <utility>
#include <vector>

namespace calpha {

enum class TokenType {
    // Literals
    INTEGER,
    CHARACTER,
    STRING_LITERAL, // New token type for string literals

    // Identifiers
    IDENTIFIER,

    // Keywords
    INT,
    CHAR,
    IF,
    ELSE,
    WHILE,
    FN,
    RET,
    LAYOUT,
    SYSCALL,
    IMPORT,    // New import keyword
    NAMESPACE, // Namespace keyword

    // Operators
    PLUS,        // +
    MINUS,       // -
    MULTIPLY,    // *
    DIVIDE,      // /
    MODULO,      // %
    BITWISE_AND, // &
    BITWISE_OR,  // |
    BITWISE_XOR, // ^
    BITWISE_NOT, // ~
    ASSIGN,      // =

    // Pointer operators
    REFERENCE,   // ->
    DEREFERENCE, // <-

    // Comparison
    EQUAL,         // ==
    NOT_EQUAL,     // !=
    LESS_THAN,     // <
    GREATER_THAN,  // >
    LESS_EQUAL,    // <=
    GREATER_EQUAL, // >=

    // Delimiters
    SEMICOLON,     // ;
    LEFT_PAREN,    // (
    RIGHT_PAREN,   // )
    LEFT_BRACE,    // {
    RIGHT_BRACE,   // }
    LEFT_BRACKET,  // [
    RIGHT_BRACKET, // ]
    COMMA,         // ,
    DOT,           // .

    // Special
    END_OF_FILE,
    INVALID
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
    std::string sourceFile; // Track which file this token came from

    Token(const TokenType typ, std::string val, const int lin, const int col,
          std::string file = "")
        : type(typ), value(std::move(val)), line(lin), column(col), sourceFile(std::move(file)) {
    }
};

class Lexer {
  private:
    std::string source;
    size_t position{0};
    int line{1};
    int column{1};
    std::string currentFile; // Track current source file
    std::unordered_map<int, std::string>
        lineToFile;                       // Map line numbers to source files
    std::vector<std::string> importStack; // Stack to track nested imports

    [[nodiscard]] char currentChar() const;
    [[nodiscard]] char peek(int offset = 1) const;
    void advance();
    void skipWhitespace();
    void skipComment();

    Token scanNumber();
    Token scanCharacter();
    Token scanStringLiteral();
    Token scanIdentifier();
    Token scanOperator();

    void updateSourceFile(
        const std::string &line); // New helper to update source file tracking

  public:
    explicit Lexer(std::string source, const std::string &mainFile = "");

    Token nextToken();
    std::vector<Token> tokenize();

    // Utility
    static std::string tokenTypeToString(TokenType type);
    static bool isKeyword(const std::string &identifier);
    static TokenType getKeywordType(const std::string &identifier);

    // Source file tracking
    [[nodiscard]] std::string getSourceFile(int line) const;
};

} // namespace calpha

#endif // LEXER_HPP