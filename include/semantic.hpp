#ifndef SEMANTIC_HPP
#define SEMANTIC_HPP

#include "parser.hpp"
#include <memory>
#include <stack>
#include <unordered_map>
#include <utility>
#include <vector>

namespace calpha {

// Forward declarations
class SemanticType;
class Symbol;
class SymbolTable;
class SemanticAnalyzer;

// Semantic Type System
enum class SemanticTypeKind {
    INT,
    CHAR,
    POINTER,
    ARRAY,
    FUNCTION,
    LAYOUT,
    VOID,
    ERROR
};

class SemanticType {
  public:
    SemanticTypeKind kind;

    SemanticType(SemanticTypeKind k) : kind(k) {
    }
    virtual ~SemanticType() = default;

    [[nodiscard]] virtual std::string toString() const = 0;
    virtual bool isCompatibleWith(const SemanticType *other) const = 0;
    [[nodiscard]] virtual std::unique_ptr<SemanticType> clone() const = 0;

    [[nodiscard]] bool isPointer() const {
        return kind == SemanticTypeKind::POINTER;
    }
    [[nodiscard]] bool isArray() const {
        return kind == SemanticTypeKind::ARRAY;
    }
    [[nodiscard]] bool isFunction() const {
        return kind == SemanticTypeKind::FUNCTION;
    }
    [[nodiscard]] bool isLayout() const {
        return kind == SemanticTypeKind::LAYOUT;
    }
    [[nodiscard]] bool isNumeric() const {
        return kind == SemanticTypeKind::INT || kind == SemanticTypeKind::CHAR;
    }
    [[nodiscard]] bool isError() const {
        return kind == SemanticTypeKind::ERROR;
    }
};

class BasicSemanticType : public SemanticType {
  public:
    BasicSemanticType(SemanticTypeKind k) : SemanticType(k) {
    }

    [[nodiscard]] std::string toString() const override {
        switch (kind) {
        case SemanticTypeKind::INT:
            return "int";
        case SemanticTypeKind::CHAR:
            return "char";
        case SemanticTypeKind::VOID:
            return "void";
        case SemanticTypeKind::ERROR:
            return "error";
        default:
            return "unknown";
        }
    }

    bool isCompatibleWith(const SemanticType *other) const override {
        if (other == nullptr)
            return false;
        if (kind == SemanticTypeKind::ERROR ||
            other->kind == SemanticTypeKind::ERROR) {
            return true; // Error type is compatible with everything
        }

        // Exact match
        if (kind == other->kind)
            return true;

        // C-Alpha allows char to int conversion (implicit widening)
        if (kind == SemanticTypeKind::INT &&
            other->kind == SemanticTypeKind::CHAR) {
            return true;
        }

        return false;
    }

    [[nodiscard]] std::unique_ptr<SemanticType> clone() const override {
        return std::make_unique<BasicSemanticType>(kind);
    }
};

class PointerSemanticType : public SemanticType {
  public:
    std::unique_ptr<SemanticType> pointsTo;

    PointerSemanticType(std::unique_ptr<SemanticType> pointsTo)
        : SemanticType(SemanticTypeKind::POINTER),
          pointsTo(std::move(pointsTo)) {
    }

    [[nodiscard]] std::string toString() const override {
        return "->" + (pointsTo ? pointsTo->toString() : "unknown");
    }

    bool isCompatibleWith(const SemanticType *other) const override {
        if (other == nullptr)
            return false;
        if (kind == SemanticTypeKind::ERROR ||
            other->kind == SemanticTypeKind::ERROR) {
            return true;
        }
        if (other->kind != SemanticTypeKind::POINTER)
            return false;

        const auto *otherPtr = static_cast<const PointerSemanticType *>(other);
        if (!pointsTo || !otherPtr->pointsTo)
            return false;

        return pointsTo->isCompatibleWith(otherPtr->pointsTo.get());
    }

    [[nodiscard]] std::unique_ptr<SemanticType> clone() const override {
        return std::make_unique<PointerSemanticType>(
            pointsTo ? pointsTo->clone() : nullptr);
    }
};

class ArraySemanticType : public SemanticType {
  public:
    std::unique_ptr<SemanticType> elementType;
    int size; // -1 for unknown/dynamic size

    ArraySemanticType(std::unique_ptr<SemanticType> elementType, int size = -1)
        : SemanticType(SemanticTypeKind::ARRAY),
          elementType(std::move(elementType)), size(size) {
    }

    [[nodiscard]] std::string toString() const override {
        std::string sizeStr = (size >= 0) ? std::to_string(size) : "?";
        return (elementType ? elementType->toString() : "unknown") + "[" +
               sizeStr + "]";
    }

    bool isCompatibleWith(const SemanticType *other) const override {
        if (other == nullptr)
            return false;
        if (kind == SemanticTypeKind::ERROR ||
            other->kind == SemanticTypeKind::ERROR) {
            return true;
        }
        if (other->kind != SemanticTypeKind::ARRAY)
            return false;

        const auto *otherArray = static_cast<const ArraySemanticType *>(other);
        if (!elementType || !otherArray->elementType)
            return false;

        return elementType->isCompatibleWith(otherArray->elementType.get());
    }

    [[nodiscard]] std::unique_ptr<SemanticType> clone() const override {
        return std::make_unique<ArraySemanticType>(
            elementType ? elementType->clone() : nullptr, size);
    }
};

class FunctionSemanticType : public SemanticType {
  public:
    std::unique_ptr<SemanticType> returnType;
    std::vector<std::unique_ptr<SemanticType>> parameterTypes;

    FunctionSemanticType(
        std::unique_ptr<SemanticType> returnType,
        std::vector<std::unique_ptr<SemanticType>> parameterTypes)
        : SemanticType(SemanticTypeKind::FUNCTION),
          returnType(std::move(returnType)),
          parameterTypes(std::move(parameterTypes)) {
    }

    [[nodiscard]] std::string toString() const override {
        std::string result = "fn ";
        result += returnType ? returnType->toString() : "unknown";
        result += "(";
        for (size_t i = 0; i < parameterTypes.size(); ++i) {
            if (i > 0)
                result += ", ";
            result +=
                parameterTypes[i] ? parameterTypes[i]->toString() : "unknown";
        }
        result += ")";
        return result;
    }

    bool isCompatibleWith(const SemanticType *other) const override {
        if (other == nullptr)
            return false;
        if (kind == SemanticTypeKind::ERROR ||
            other->kind == SemanticTypeKind::ERROR) {
            return true;
        }
        if (other->kind != SemanticTypeKind::FUNCTION)
            return false;

        const auto *otherFunc =
            static_cast<const FunctionSemanticType *>(other);

        // Check return type
        if (!returnType || !otherFunc->returnType)
            return false;
        if (!returnType->isCompatibleWith(otherFunc->returnType.get()))
            return false;

        // Check parameter count
        if (parameterTypes.size() != otherFunc->parameterTypes.size())
            return false;

        // Check parameter types
        for (size_t i = 0; i < parameterTypes.size(); ++i) {
            if (!parameterTypes[i] || !otherFunc->parameterTypes[i])
                return false;
            if (!parameterTypes[i]->isCompatibleWith(
                    otherFunc->parameterTypes[i].get())) {
                return false;
            }
        }

        return true;
    }

    [[nodiscard]] std::unique_ptr<SemanticType> clone() const override {
        std::vector<std::unique_ptr<SemanticType>> clonedParams;
        clonedParams.reserve(parameterTypes.size());
        for (const auto &param : parameterTypes) {
            clonedParams.push_back(param ? param->clone() : nullptr);
        }
        return std::make_unique<FunctionSemanticType>(
            returnType ? returnType->clone() : nullptr,
            std::move(clonedParams));
    }
};

class LayoutSemanticType : public SemanticType {
  public:
    struct Member {
        std::string name;
        std::unique_ptr<SemanticType> type;

        Member(std::string name, std::unique_ptr<SemanticType> type)
            : name(std::move(name)), type(std::move(type)) {
        }
    };

    std::string layoutName;
    std::vector<std::unique_ptr<Member>> members;

    LayoutSemanticType(std::string name,
                       std::vector<std::unique_ptr<Member>> members)
        : SemanticType(SemanticTypeKind::LAYOUT), layoutName(std::move(name)),
          members(std::move(members)) {
    }

    [[nodiscard]] std::string toString() const override {
        return "layout " + layoutName;
    }

    bool isCompatibleWith(const SemanticType *other) const override {
        if (other == nullptr)
            return false;
        if (kind == SemanticTypeKind::ERROR ||
            other->kind == SemanticTypeKind::ERROR) {
            return true;
        }
        if (other->kind != SemanticTypeKind::LAYOUT)
            return false;

        const auto *otherLayout =
            static_cast<const LayoutSemanticType *>(other);
        return layoutName == otherLayout->layoutName;
    }

    [[nodiscard]] std::unique_ptr<SemanticType> clone() const override {
        std::vector<std::unique_ptr<Member>> clonedMembers;
        clonedMembers.reserve(members.size());
        for (const auto &member : members) {
            clonedMembers.push_back(std::make_unique<Member>(
                member->name, member->type ? member->type->clone() : nullptr));
        }
        return std::make_unique<LayoutSemanticType>(layoutName,
                                                    std::move(clonedMembers));
    }

    [[nodiscard]] const Member *findMember(const std::string &name) const {
        for (const auto &member : members) {
            if (member->name == name) {
                return member.get();
            }
        }
        return nullptr;
    }
};

// Symbol System
enum class SymbolKind { VARIABLE, FUNCTION, PARAMETER, LAYOUT };

class Symbol {
  public:
    std::string name;
    std::string fqdn; // Fully Qualified Domain Name for unique identification
    SymbolKind symbolKind;
    std::unique_ptr<SemanticType> type;
    int line, column;
    bool isInitialized;

    Symbol(std::string name, SymbolKind kind,
           std::unique_ptr<SemanticType> type, int line, int column,
           bool initialized = false)
        : name(std::move(name)), symbolKind(kind), type(std::move(type)),
          line(line), column(column), isInitialized(initialized) {
    }

    [[nodiscard]] std::string toString() const {
        std::string kindStr;
        switch (symbolKind) {
        case SymbolKind::VARIABLE:
            kindStr = "variable";
            break;
        case SymbolKind::FUNCTION:
            kindStr = "function";
            break;
        case SymbolKind::PARAMETER:
            kindStr = "parameter";
            break;
        case SymbolKind::LAYOUT:
            kindStr = "layout";
            break;
        }
        return kindStr + " " + fqdn + ": " +
               (type ? type->toString() : "unknown");
    }
};

// Symbol Table with Scope Management
class Scope {
  public:
    std::unordered_map<std::string, std::unique_ptr<Symbol>> symbols;
    std::string scopeName;

    explicit Scope(std::string name) : scopeName(std::move(name)) {
    }

    void addSymbol(std::unique_ptr<Symbol> symbol) {
        symbols[symbol->name] = std::move(symbol);
    }

    Symbol *findSymbol(const std::string &name) {
        auto iter = symbols.find(name);
        return (iter != symbols.end()) ? iter->second.get() : nullptr;
    }

    bool hasSymbol(const std::string &name) const {
        return symbols.find(name) != symbols.end();
    }
};

class SymbolTable {
  private:
    // Actively open scopes (scope stack)
    std::vector<std::unique_ptr<Scope>> scopes;
    // Scopes that have been closed but kept for debug/inspection purposes
    std::vector<std::unique_ptr<Scope>> archivedScopes;

    [[nodiscard]] std::string buildFQDN(const std::string &name) const {
        if (scopes.empty())
            return "global::" + name;

        std::string fqdn;
        for (const auto &scope : scopes) {
            if (!fqdn.empty())
                fqdn += "::";
            fqdn += scope->scopeName;
        }
        return fqdn + "::" + name;
    }

  public:
    SymbolTable() {
        pushScope("global");
    }

    // Scope stack management
    // ---------------------------------------------------
    void pushScope(const std::string &name) {
        scopes.push_back(std::make_unique<Scope>(name));
    }

    void popScope() {
        if (!scopes.empty()) {
            // move the finished scope into the archive for later inspection
            archivedScopes.push_back(std::move(scopes.back()));
            scopes.pop_back();
        }
    }

    void addSymbol(std::unique_ptr<Symbol> symbol) {
        if (!scopes.empty()) {
            symbol->fqdn = buildFQDN(symbol->name);
            scopes.back()->addSymbol(std::move(symbol));
        }
    }

    Symbol *findSymbol(const std::string &name);

    Symbol *findSymbolByFQDN(const std::string &fqdn) {
        // Search all scopes for exact FQDN match
        for (const auto &scope : scopes) {
            for (const auto &symbolPair : scope->symbols) {
                if (symbolPair.second->fqdn == fqdn) {
                    return symbolPair.second.get();
                }
            }
        }
        for (const auto &scope : archivedScopes) {
            for (const auto &symbolPair : scope->symbols) {
                if (symbolPair.second->fqdn == fqdn) {
                    return symbolPair.second.get();
                }
            }
        }
        return nullptr;
    }

    [[nodiscard]] bool hasSymbolInCurrentScope(const std::string &name) const {
        return !scopes.empty() && scopes.back()->hasSymbol(name);
    }

    [[nodiscard]] std::string getCurrentScopeName() const {
        return !scopes.empty() ? scopes.back()->scopeName : "";
    }

    // Accessors
    // ---------------------------------------------------------------- Active
    // (open) scopes – mostly just the global one after analysis
    [[nodiscard]] const std::vector<std::unique_ptr<Scope>> &getScopes() const {
        return scopes;
    }
    // Closed scopes preserved for debugging / code-gen introspection
    [[nodiscard]] const std::vector<std::unique_ptr<Scope>> &
    getArchivedScopes() const {
        return archivedScopes;
    }
    // Convenience: active + archived combined
    [[nodiscard]] std::vector<const Scope *> getAllScopes() const {
        std::vector<const Scope *> all;
        all.reserve(scopes.size());
        for (const auto &s : scopes)
            all.push_back(s.get());
        for (const auto &s : archivedScopes)
            all.push_back(s.get());
        return all;
    }

    void replaceSymbol(const std::string &name,
                       std::unique_ptr<Symbol> symbol) {
        if (!scopes.empty()) {
            symbol->fqdn = buildFQDN(name);
            scopes.back()->symbols[name] = std::move(symbol);
        }
    }
};

// Semantic Error System
class SemanticError {
  public:
    std::string message;
    int line, column;

    SemanticError(std::string msg, int line, int column)
        : message(std::move(msg)), line(line), column(column) {
    }

    [[nodiscard]] std::string toString() const {
        return "Semantic Error at line " + std::to_string(line) + ", column " +
               std::to_string(column) + ": " + message;
    }
};

// Semantic Analyzer - Main class that walks the AST
class SemanticAnalyzer {
  private:
    SymbolTable symbolTable;
    std::vector<SemanticError> errors;
    std::unique_ptr<SemanticType> currentFunctionReturnType;

    void addError(const std::string &message, int line, int column);
    std::unique_ptr<SemanticType> convertType(const Type *astType);

    // AST Visitor Methods
    void visitProgram(const Program *program);
    void visitStatement(const Statement *stmt);
    void visitVariableDeclaration(const VariableDeclaration *varDecl);
    void visitFunctionDeclaration(const FunctionDeclaration *funcDecl);
    void visitLayoutDeclaration(const LayoutDeclaration *layoutDecl);
    void visitNamespaceDeclaration(const NamespaceDeclaration *namespaceDecl);
    void visitAssignment(const Assignment *assignment);
    void visitIfStatement(const IfStatement *ifStmt);
    void visitWhileStatement(const WhileStatement *whileStmt);
    void visitReturnStatement(const ReturnStatement *retStmt);
    void visitBlockStatement(const BlockStatement *block);
    void visitExpressionStatement(const ExpressionStatement *exprStmt);

    // Expression visitors
    std::unique_ptr<SemanticType> visitExpression(const Expression *expr);
    static std::unique_ptr<SemanticType> visitLiteral(const Literal *literal);
    static std::unique_ptr<SemanticType>
    visitStringLiteral(const StringLiteral *stringLiteral);
    std::unique_ptr<SemanticType> visitIdentifier(const Identifier *id);
    std::unique_ptr<SemanticType>
    visitBinaryExpression(const BinaryExpression *binExpr);
    std::unique_ptr<SemanticType>
    visitUnaryExpression(const UnaryExpression *unExpr);
    std::unique_ptr<SemanticType>
    visitFunctionCall(const FunctionCall *funcCall);
    std::unique_ptr<SemanticType>
    visitArrayAllocation(const ArrayAllocation *arrayAlloc);
    std::unique_ptr<SemanticType>
    visitArrayAccess(const ArrayAccess *arrayAccess);
    std::unique_ptr<SemanticType>
    visitMemberAccess(const MemberAccess *memberAccess);
    std::unique_ptr<SemanticType>
    visitNamespaceAccess(const NamespaceAccess *namespaceAccess);
    std::unique_ptr<SemanticType>
    visitSyscallExpression(const SyscallExpression *syscallExpr);
    std::unique_ptr<SemanticType> visitTypeCast(const TypeCast *typeCast);

  public:
    SemanticAnalyzer() = default;

    // Main analysis entry point
    bool analyze(const Program *program);

    [[nodiscard]] const std::vector<SemanticError> &getErrors() const;
    void printErrors() const;
    void printSymbolTable() const;

    // Add getter for symbol table
    [[nodiscard]] const SymbolTable &getSymbolTable() const {
        return symbolTable;
    }

    // Public access to symbol table
    SymbolTable &getSymbolTable() {
        return symbolTable;
    }
};

} // namespace calpha

#endif // SEMANTIC_HPP