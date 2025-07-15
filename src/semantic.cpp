#include "semantic.hpp"
#include <iostream>
#include <sstream>

namespace calpha {

// SymbolTable Implementation

Symbol *SymbolTable::findSymbol(const std::string &name) {

    if (name.empty() || name == "global") {
        return nullptr; // No name provided
    }

    // Check if the name is a Fully Qualified Domain Name (FQDN)
    if (name.find("::") != std::string::npos) {
        return findSymbolByFQDN(name);
    }

    // Search from the current (innermost) scope outwards to the global scope
    for (int i = static_cast<int>(scopes.size()) - 1; i >= 0; --i) {
        const auto &scope = scopes[i];
        auto it = scope->symbols.find(name);
        if (it != scope->symbols.end()) {
            return it->second.get();
        }
    }

    // If not found in active scopes, check archived scopes as a fallback
    for (const auto &scope : archivedScopes) {
        auto it = scope->symbols.find(name);
        if (it != scope->symbols.end()) {
            return it->second.get();
        }
    }

    std::cout << "Symbol '" << name << "' not found in any scope." << '\n';

    return nullptr;
}

// SemanticAnalyzer Implementation

bool SemanticAnalyzer::analyze(const Program *program) {
    errors.clear();
    visitProgram(program);
    return errors.empty();
}

const std::vector<SemanticError> &SemanticAnalyzer::getErrors() const {
    return errors;
}

void SemanticAnalyzer::printErrors() const {
    for (const auto &error : errors) {
        std::cout << error.toString() << '\n';
    }
}

void SemanticAnalyzer::printSymbolTable() const {
    std::cout << "=== Symbol Table ===" << '\n';
    std::cout << "Current scope: " << symbolTable.getCurrentScopeName() << '\n';

    // Print all symbols in all scopes
    auto allScopes = symbolTable.getAllScopes();
    for (const auto *scope : allScopes) {

        if (scope->symbols.empty()) {
            continue;
        }
        std::cout << "\nScope: " << scope->scopeName << '\n';
        for (const auto &symbolPair : scope->symbols) {
            const Symbol *symbol = symbolPair.second.get();
            std::cout << "  " << symbol->toString() << " (line " << symbol->line
                      << ", col " << symbol->column << ")";
            if (symbol->symbolKind == SymbolKind::VARIABLE) {
                std::cout << " ["
                          << (symbol->isInitialized ? "initialized"
                                                    : "uninitialized")
                          << "]";
            }
            std::cout << '\n';
        }
    }
}

void SemanticAnalyzer::addError(const std::string &message, int line,
                                int column) {
    errors.emplace_back(message, line, column);
}

std::unique_ptr<SemanticType>
SemanticAnalyzer::convertType(const Type *astType) {
    if (astType == nullptr) {
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
    }

    switch (astType->nodeType) {
    case NodeType::BASIC_TYPE: {
        const auto *basicType = dynamic_cast<const BasicType *>(astType);
        switch (basicType->baseType) {
        case TokenType::INT:
            return std::make_unique<BasicSemanticType>(SemanticTypeKind::INT);
        case TokenType::CHAR:
            return std::make_unique<BasicSemanticType>(SemanticTypeKind::CHAR);
        default:
            return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
        }
    }
    case NodeType::POINTER_TYPE: {
        const auto *ptrType = dynamic_cast<const PointerType *>(astType);
        auto pointsTo = convertType(ptrType->pointsTo.get());
        return std::make_unique<PointerSemanticType>(std::move(pointsTo));
    }
    case NodeType::LAYOUT_TYPE: {
        const auto *layoutType = dynamic_cast<const LayoutType *>(astType);
        std::string layoutName = layoutType->layoutName;

        // Check if this is a namespace-qualified layout type (e.g.,
        // heap.ArrayHeader)
        size_t dotPos = layoutName.find('.');
        if (dotPos != std::string::npos) {
            std::string namespaceName = layoutName.substr(0, dotPos);
            std::string typeName = layoutName.substr(dotPos + 1);

            // Push namespace scope
            symbolTable.pushScope("namespace_" + namespaceName);

            // Look up the layout in the namespace scope
            Symbol *layoutSymbol = symbolTable.findSymbol(typeName);

            // Pop namespace scope
            symbolTable.popScope();

            if ((layoutSymbol == nullptr) ||
                layoutSymbol->symbolKind != SymbolKind::LAYOUT) {
                addError("Undefined layout type '" + typeName +
                             "' in namespace '" + namespaceName + "'",
                         astType->line, astType->column);
                return std::make_unique<BasicSemanticType>(
                    SemanticTypeKind::ERROR);
            }

            return layoutSymbol->type->clone();
        }

        // Non-namespace-qualified layout type
        Symbol *layoutSymbol = symbolTable.findSymbol(layoutName);
        if ((layoutSymbol == nullptr) ||
            layoutSymbol->symbolKind != SymbolKind::LAYOUT) {
            addError("Undefined layout type '" + layoutName + "'",
                     astType->line, astType->column);
            return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
        }
        
        // For self-referential layouts, create a new type with the correct FQDN
        // instead of cloning the potentially incomplete forward declaration
        const auto *existingLayout = dynamic_cast<const LayoutSemanticType *>(layoutSymbol->type.get());
        if (existingLayout->members.empty()) {
            // This might be a forward declaration, create a reference by name
            std::cout << "DEBUG: Found forward declaration for " << layoutName 
                      << ", creating name-based reference" << std::endl;
            return std::make_unique<LayoutSemanticType>(layoutSymbol->fqdn, 
                std::vector<std::unique_ptr<LayoutSemanticType::Member>>());
        }
        
        return layoutSymbol->type->clone();
    }
    default:
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
    }
}

void SemanticAnalyzer::visitProgram(const Program *program) {
    for (const auto &statement : program->statements) {
        visitStatement(statement.get());
    }
}

void SemanticAnalyzer::visitStatement(const Statement *stmt) {
    if (stmt == nullptr)
        return;

    switch (stmt->nodeType) {
    case NodeType::NAMESPACE_DECLARATION:
        visitNamespaceDeclaration(
            dynamic_cast<const NamespaceDeclaration *>(stmt));
        break;
    case NodeType::VARIABLE_DECLARATION:
        visitVariableDeclaration(
            dynamic_cast<const VariableDeclaration *>(stmt));
        break;
    case NodeType::FUNCTION_DECLARATION:
        visitFunctionDeclaration(
            dynamic_cast<const FunctionDeclaration *>(stmt));
        break;
    case NodeType::LAYOUT_DECLARATION:
        visitLayoutDeclaration(dynamic_cast<const LayoutDeclaration *>(stmt));
        break;
    case NodeType::ASSIGNMENT:
        visitAssignment(dynamic_cast<const Assignment *>(stmt));
        break;
    case NodeType::IF_STATEMENT:
        visitIfStatement(dynamic_cast<const IfStatement *>(stmt));
        break;
    case NodeType::WHILE_STATEMENT:
        visitWhileStatement(dynamic_cast<const WhileStatement *>(stmt));
        break;
    case NodeType::RETURN_STATEMENT:
        visitReturnStatement(dynamic_cast<const ReturnStatement *>(stmt));
        break;
    case NodeType::BLOCK_STATEMENT:
        visitBlockStatement(dynamic_cast<const BlockStatement *>(stmt));
        break;
    case NodeType::EXPRESSION_STATEMENT:
        visitExpressionStatement(
            dynamic_cast<const ExpressionStatement *>(stmt));
        break;
    default:
        addError("Unknown statement type", stmt->line, stmt->column);
        break;
    }
}

void SemanticAnalyzer::visitNamespaceDeclaration(
    const NamespaceDeclaration *namespaceDecl) {
    // Push new scope for namespace
    symbolTable.pushScope("namespace_" + namespaceDecl->name);

    // Process all statements in the namespace
    for (const auto &stmt : namespaceDecl->statements) {
        visitStatement(stmt.get());
    }

    // Pop namespace scope
    symbolTable.popScope();
}

void SemanticAnalyzer::visitVariableDeclaration(
    const VariableDeclaration *varDecl) {
    if (symbolTable.hasSymbolInCurrentScope(varDecl->name)) {
        addError("Variable '" + varDecl->name +
                     "' already declared in current scope",
                 varDecl->line, varDecl->column);
        return;
    }

    auto semanticType = convertType(varDecl->type.get());

    bool isInitialized = false;
    if (varDecl->initializer) {
        auto initType = visitExpression(varDecl->initializer.get());

        // Special case: layout initialization to layout variable
        if (varDecl->initializer->nodeType == NodeType::LAYOUT_INITIALIZATION &&
            semanticType->isLayout()) {
            const auto *layoutInit = dynamic_cast<const LayoutInitialization *>(varDecl->initializer.get());
            const auto *layoutSemanticType = dynamic_cast<const LayoutSemanticType *>(semanticType.get());
            
            // Check that the number of values matches the number of layout members
            if (layoutInit->values.size() != layoutSemanticType->members.size()) {
                addError("Layout initialization has " + std::to_string(layoutInit->values.size()) +
                         " values but layout '" + layoutSemanticType->layoutName + "' has " +
                         std::to_string(layoutSemanticType->members.size()) + " members",
                         varDecl->line, varDecl->column);
            } else {
                // Check that each initialization value is compatible with its corresponding member
                bool allCompatible = true;
                for (size_t i = 0; i < layoutInit->values.size(); ++i) {
                    auto initValueType = visitExpression(layoutInit->values[i].get());
                    const auto &member = layoutSemanticType->members[i];
                    
                    if (initValueType && !member->type->isCompatibleWith(initValueType.get())) {
                        addError("Type mismatch in layout initialization for member '" + member->name +
                                 "'. Expected " + member->type->toString() + ", got " + initValueType->toString(),
                                 layoutInit->values[i]->line, layoutInit->values[i]->column);
                        allCompatible = false;
                    }
                }
                
                if (allCompatible) {
                    isInitialized = true;
                }
            }
        }
        // Special case: string literal assignment to char pointer
        else if (varDecl->initializer->nodeType == NodeType::STRING_LITERAL &&
            semanticType->isPointer()) {
            const auto *ptrType =
                dynamic_cast<const PointerSemanticType *>(semanticType.get());
            if (ptrType->pointsTo->kind == SemanticTypeKind::CHAR) {
                // This is valid: ->char text = "Hello";
                isInitialized = true;
            } else {
                addError("String literal can only be assigned to char pointer",
                         varDecl->line, varDecl->column);
            }
        }
        // Check for invalid string literal assignment to single char
        else if (varDecl->initializer->nodeType == NodeType::STRING_LITERAL &&
                 semanticType->kind == SemanticTypeKind::CHAR) {
            addError("Cannot assign string literal to single char variable. "
                     "Use single quotes for character literals (e.g., '\\n') "
                     "or declare as char pointer (e.g., ->char)",
                     varDecl->line, varDecl->column);
        } else if (initType &&
                   !semanticType->isCompatibleWith(initType.get())) {
            addError("Type mismatch in variable initialization for '" +
                         varDecl->name + "'. Expected " +
                         semanticType->toString() + ", got " +
                         initType->toString(),
                     varDecl->line, varDecl->column);
        } else {
            isInitialized = true;
        }
    } else if (semanticType->isLayout()) {
        // Layout variables are considered initialized even without explicit
        // initializer This allows access to their members
        isInitialized = true;
    }

    auto symbol = std::make_unique<Symbol>(
        varDecl->name, SymbolKind::VARIABLE, std::move(semanticType),
        varDecl->line, varDecl->column, isInitialized);
    symbolTable.addSymbol(std::move(symbol));
}

void SemanticAnalyzer::visitFunctionDeclaration(
    const FunctionDeclaration *funcDecl) {
    if (symbolTable.hasSymbolInCurrentScope(funcDecl->name)) {
        addError("Function '" + funcDecl->name +
                     "' already declared in current scope",
                 funcDecl->line, funcDecl->column);
        return;
    }

    auto returnType = convertType(funcDecl->returnType.get());

    std::vector<std::unique_ptr<SemanticType>> paramTypes;
    paramTypes.reserve(funcDecl->parameters.size());
    for (const auto &param : funcDecl->parameters) {
        paramTypes.push_back(convertType(param->type.get()));
    }

    auto funcType = std::make_unique<FunctionSemanticType>(
        returnType->clone(), std::move(paramTypes));

    auto symbol = std::make_unique<Symbol>(funcDecl->name, SymbolKind::FUNCTION,
                                           std::move(funcType), funcDecl->line,
                                           funcDecl->column, true);
    symbolTable.addSymbol(std::move(symbol));

    symbolTable.pushScope("function_" + funcDecl->name);
    currentFunctionReturnType = std::move(returnType);

    for (const auto &param : funcDecl->parameters) {
        auto paramType = convertType(param->type.get());
        auto paramSymbol = std::make_unique<Symbol>(
            param->name, SymbolKind::PARAMETER, std::move(paramType),
            param->line, param->column, true);
        symbolTable.addSymbol(std::move(paramSymbol));
    }

    visitBlockStatement(funcDecl->body.get());

    currentFunctionReturnType.reset();
    symbolTable.popScope();
}

void SemanticAnalyzer::visitLayoutDeclaration(
    const LayoutDeclaration *layoutDecl) {
    if (symbolTable.hasSymbolInCurrentScope(layoutDecl->name)) {
        addError("Layout '" + layoutDecl->name +
                     "' already declared in current scope",
                 layoutDecl->line, layoutDecl->column);
        return;
    }

    std::cout << "\n> Processing layout declaration: " << layoutDecl->name
              << " at line " << layoutDecl->line << ", column "
              << layoutDecl->column << '\n';

    // Get the FQDN that the symbol table will generate for this layout
    // We need to manually build the FQDN since the symbol isn't added yet
    std::string fqdn = symbolTable.buildFQDN(layoutDecl->name);

    std::cout << "\t> Generated FQDN for layout: " << fqdn << '\n';

    // Always create a forward declaration first, then replace with complete
    // layout This ensures the layout type is available for processing members
    // and other statements
    auto forwardLayoutType = std::make_unique<LayoutSemanticType>(
        fqdn, std::vector<std::unique_ptr<LayoutSemanticType::Member>>());
    auto forwardSymbol = std::make_unique<Symbol>(
        layoutDecl->name, SymbolKind::LAYOUT, std::move(forwardLayoutType),
        layoutDecl->line, layoutDecl->column, true);
    symbolTable.addSymbol(std::move(forwardSymbol));

    std::cout << "\t> Forward layout declaration: " << fqdn << '\n';
    // Now process the members
    std::vector<std::unique_ptr<LayoutSemanticType::Member>> members;

    for (const auto &member : layoutDecl->members) {
        auto memberType = convertType(member->type.get());
        members.push_back(std::make_unique<LayoutSemanticType::Member>(
            member->name, std::move(memberType)));

        // std::cout << "\t[] Layout member: " << member->name << " of type " <<
        // members.back()->type->toString() << '\n';
    }

    // Create the complete layout type
    auto layoutType =
        std::make_unique<LayoutSemanticType>(fqdn, std::move(members));

    // std::cout << "\t> Complete layout type: " << fqdn << '\n';

    auto symbol = std::make_unique<Symbol>(
        layoutDecl->name, SymbolKind::LAYOUT, std::move(layoutType),
        layoutDecl->line, layoutDecl->column, true);

    // Replace the forward declaration with the complete one
    symbolTable.replaceSymbol(layoutDecl->name, std::move(symbol));
}

void SemanticAnalyzer::visitAssignment(const Assignment *assignment) {
    auto targetType = visitExpression(assignment->target.get());
    auto valueType = visitExpression(assignment->value.get());

    if (targetType && valueType) {
        // Special case: layout initialization to layout variable
        if (assignment->value->nodeType == NodeType::LAYOUT_INITIALIZATION &&
            targetType->isLayout()) {
            const auto *layoutInit = dynamic_cast<const LayoutInitialization *>(assignment->value.get());
            const auto *layoutTargetType = dynamic_cast<const LayoutSemanticType *>(targetType.get());
            
            // Check that the number of values matches the number of layout members
            if (layoutInit->values.size() != layoutTargetType->members.size()) {
                addError("Layout initialization has " + std::to_string(layoutInit->values.size()) +
                         " values but layout '" + layoutTargetType->layoutName + "' has " +
                         std::to_string(layoutTargetType->members.size()) + " members",
                         assignment->line, assignment->column);
                return;
            }
            
            // Check that each initialization value is compatible with its corresponding member
            for (size_t i = 0; i < layoutInit->values.size(); ++i) {
                auto initValueType = visitExpression(layoutInit->values[i].get());
                const auto &member = layoutTargetType->members[i];
                
                if (initValueType && !member->type->isCompatibleWith(initValueType.get())) {
                    addError("Type mismatch in layout initialization for member '" + member->name +
                             "'. Expected " + member->type->toString() + ", got " + initValueType->toString(),
                             layoutInit->values[i]->line, layoutInit->values[i]->column);
                }
            }
            
            // Mark as initialized
            if (assignment->target->nodeType == NodeType::IDENTIFIER) {
                const auto *id = dynamic_cast<const Identifier *>(assignment->target.get());
                Symbol *symbol = symbolTable.findSymbol(id->name);
                if (symbol != nullptr) {
                    symbol->isInitialized = true;
                }
            }
            return;
        }
        
        // Special case: string literal assignment to char pointer
        if (assignment->value->nodeType == NodeType::STRING_LITERAL &&
            targetType->isPointer()) {
            const auto *ptrType =
                dynamic_cast<const PointerSemanticType *>(targetType.get());
            if (ptrType->pointsTo->kind == SemanticTypeKind::CHAR) {
                // This is valid: messageB = "Hello";
                if (assignment->target->nodeType == NodeType::IDENTIFIER) {
                    const auto *id = dynamic_cast<const Identifier *>(
                        assignment->target.get());
                    Symbol *symbol = symbolTable.findSymbol(id->name);
                    if (symbol != nullptr) {
                        symbol->isInitialized = true;
                    }
                }
                return;
            }
        }

        if (!targetType->isCompatibleWith(valueType.get())) {
            addError("Type mismatch in assignment. Expected " +
                         targetType->toString() + ", got " +
                         valueType->toString(),
                     assignment->line, assignment->column);
        }
    }

    if (assignment->target->nodeType == NodeType::IDENTIFIER) {
        const auto *id =
            dynamic_cast<const Identifier *>(assignment->target.get());
        Symbol *symbol = symbolTable.findSymbol(id->name);
        if (symbol != nullptr) {
            symbol->isInitialized = true;
        }
    }
}

void SemanticAnalyzer::visitIfStatement(const IfStatement *ifStmt) {
    auto condType = visitExpression(ifStmt->condition.get());
    if (condType && !condType->isNumeric()) {
        addError("If condition must be a numeric type", ifStmt->line,
                 ifStmt->column);
    }

    visitStatement(ifStmt->thenStatement.get());
    if (ifStmt->elseStatement) {
        visitStatement(ifStmt->elseStatement.get());
    }
}

void SemanticAnalyzer::visitWhileStatement(const WhileStatement *whileStmt) {
    auto condType = visitExpression(whileStmt->condition.get());
    if (condType && !condType->isNumeric()) {
        addError("While condition must be a numeric type", whileStmt->line,
                 whileStmt->column);
    }

    visitStatement(whileStmt->body.get());
}

void SemanticAnalyzer::visitReturnStatement(const ReturnStatement *retStmt) {
    if (!currentFunctionReturnType) {
        addError("Return statement outside of function", retStmt->line,
                 retStmt->column);
        return;
    }

    if (retStmt->value) {
        auto valueType = visitExpression(retStmt->value.get());
        if (valueType &&
            !currentFunctionReturnType->isCompatibleWith(valueType.get())) {
            addError("Return type mismatch. Expected " +
                         currentFunctionReturnType->toString() + ", got " +
                         valueType->toString(),
                     retStmt->line, retStmt->column);
        }
    } else {
        if (currentFunctionReturnType->kind != SemanticTypeKind::VOID) {
            addError("Missing return value. Expected " +
                         currentFunctionReturnType->toString(),
                     retStmt->line, retStmt->column);
        }
    }
}

void SemanticAnalyzer::visitBlockStatement(const BlockStatement *block) {
    symbolTable.pushScope("block");

    for (const auto &stmt : block->statements) {
        visitStatement(stmt.get());
    }

    symbolTable.popScope();
}

void SemanticAnalyzer::visitExpressionStatement(
    const ExpressionStatement *exprStmt) {
    visitExpression(exprStmt->expression.get());
}

std::unique_ptr<SemanticType>
SemanticAnalyzer::visitExpression(const Expression *expr) {
    if (expr == nullptr) {
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
    }

    switch (expr->nodeType) {
    case NodeType::NAMESPACE_ACCESS:
        return visitNamespaceAccess(dynamic_cast<const NamespaceAccess *>(expr));
        break;
    case NodeType::LITERAL:
        return visitLiteral(dynamic_cast<const Literal *>(expr));
    case NodeType::STRING_LITERAL:
        return visitStringLiteral(dynamic_cast<const StringLiteral *>(expr));
    case NodeType::IDENTIFIER:
        return visitIdentifier(dynamic_cast<const Identifier *>(expr));
    case NodeType::BINARY_EXPRESSION:
        return visitBinaryExpression(
            dynamic_cast<const BinaryExpression *>(expr));
    case NodeType::UNARY_EXPRESSION:
        return visitUnaryExpression(dynamic_cast<const UnaryExpression *>(expr));
    case NodeType::FUNCTION_CALL:
        return visitFunctionCall(dynamic_cast<const FunctionCall *>(expr));
    case NodeType::ARRAY_ALLOCATION:
        return visitArrayAllocation(dynamic_cast<const ArrayAllocation *>(expr));
    case NodeType::ARRAY_ACCESS:
        return visitArrayAccess(dynamic_cast<const ArrayAccess *>(expr));
    case NodeType::MEMBER_ACCESS:
        return visitMemberAccess(dynamic_cast<const MemberAccess *>(expr));
    case NodeType::SYSCALL_EXPRESSION:
        return visitSyscallExpression(
            dynamic_cast<const SyscallExpression *>(expr));
    case NodeType::TYPE_CAST:
        return visitTypeCast(dynamic_cast<const TypeCast *>(expr));
    case NodeType::LAYOUT_INITIALIZATION:
        return visitLayoutInitialization(dynamic_cast<const LayoutInitialization *>(expr));
    default:
        addError("Unknown expression type", expr->line, expr->column);
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
    }
}

std::unique_ptr<SemanticType>
SemanticAnalyzer::visitNamespaceAccess(const NamespaceAccess *namespaceAccess) {
    // Find the namespace scope
    std::string namespaceFQDN = "global::" + namespaceAccess->namespaceName;
    Symbol *namespaceSymbol = symbolTable.findSymbolByFQDN(namespaceFQDN);

    if (namespaceSymbol == nullptr) {
        addError("Undefined namespace '" + namespaceAccess->namespaceName + "'",
                 namespaceAccess->line, namespaceAccess->column);
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
    }

    // Visit the member expression within the namespace scope
    symbolTable.pushScope("namespace_" + namespaceAccess->namespaceName);
    auto memberType = visitExpression(namespaceAccess->member.get());
    symbolTable.popScope();

    return memberType;
}

std::unique_ptr<SemanticType>
SemanticAnalyzer::visitTypeCast(const TypeCast *typeCast) {
    auto targetType = convertType(typeCast->targetType.get());
    auto exprType = visitExpression(typeCast->expression.get());

    if (!targetType || !exprType) {
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
    }

    // Only allow casting between numeric types (int and char)
    if (!((targetType->isNumeric() && exprType->isNumeric()) ||
          (targetType->isNumeric() && exprType->isPointer()) ||
          (targetType->isPointer() && exprType->isNumeric()) ||
          (targetType->isPointer() && exprType->isPointer()))) {
        addError("Type cast only supported between numeric types (int and "
                 "char) or pointer types",
                 typeCast->line, typeCast->column);
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
    }

    // Warn about potential data loss when casting from int to char
    if (targetType->kind == SemanticTypeKind::CHAR &&
        exprType->kind == SemanticTypeKind::INT) {
        std::cerr << "Warning: Possible data loss when casting from int to "
                     "char at line "
                  << typeCast->line << ", column " << typeCast->column << '\n';
    }

    return targetType->clone();
}

std::unique_ptr<SemanticType>
SemanticAnalyzer::visitLiteral(const Literal *literal) {
    switch (literal->literalType) {
    case TokenType::INTEGER:
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::INT);
    case TokenType::CHARACTER:
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::CHAR);
    default:
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
    }
}

std::unique_ptr<SemanticType>
SemanticAnalyzer::visitStringLiteral(const StringLiteral *stringLiteral) {
    // String literals are char pointers (->char)
    auto charType = std::make_unique<BasicSemanticType>(SemanticTypeKind::CHAR);
    return std::make_unique<PointerSemanticType>(std::move(charType));
}

std::unique_ptr<SemanticType>
SemanticAnalyzer::visitIdentifier(const Identifier *id) {
    Symbol *symbol = symbolTable.findSymbol(id->name);
    if (symbol == nullptr) {
        addError("Undefined identifier '" + id->name + "'", id->line,
                 id->column);
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
    }

    if (!symbol->isInitialized && symbol->symbolKind == SymbolKind::VARIABLE) {
        addError("Use of uninitialized variable '" + id->name + "'", id->line,
                 id->column);
    }

    return symbol->type->clone();
}

std::unique_ptr<SemanticType>
SemanticAnalyzer::visitBinaryExpression(const BinaryExpression *binExpr) {
    auto leftType = visitExpression(binExpr->left.get());
    auto rightType = visitExpression(binExpr->right.get());

    if (!leftType || !rightType) {
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
    }

    // Special case: Check for char vs string literal comparison
    bool leftIsChar = (leftType->kind == SemanticTypeKind::CHAR);
    bool rightIsChar = (rightType->kind == SemanticTypeKind::CHAR);
    bool leftIsStringLiteral =
        (binExpr->left->nodeType == NodeType::STRING_LITERAL);
    bool rightIsStringLiteral =
        (binExpr->right->nodeType == NodeType::STRING_LITERAL);

    if ((leftIsChar && rightIsStringLiteral) ||
        (rightIsChar && leftIsStringLiteral)) {
        if (binExpr->operator_ == TokenType::EQUAL ||
            binExpr->operator_ == TokenType::NOT_EQUAL) {
            addError(
                "Cannot compare char with string literal. Use single quotes "
                "for character comparison (e.g., '\\0' instead of \"\\0\")",
                binExpr->line, binExpr->column);
        }
    }

    // Handle arithmetic operations
    switch (binExpr->operator_) {
    case TokenType::PLUS:
    case TokenType::MINUS:
    case TokenType::MULTIPLY:
    case TokenType::DIVIDE:
    case TokenType::MODULO:
        if (!leftType->isNumeric() || !rightType->isNumeric()) {
            addError("Arithmetic operators require numeric types",
                     binExpr->line, binExpr->column);
            return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
        }
        // Result type is the "larger" of the two types (int > char)
        if (leftType->kind == SemanticTypeKind::INT ||
            rightType->kind == SemanticTypeKind::INT) {
            return std::make_unique<BasicSemanticType>(SemanticTypeKind::INT);
        }
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::CHAR);

    case TokenType::BITWISE_AND:
    case TokenType::BITWISE_OR:
    case TokenType::BITWISE_XOR:
        if (!leftType->isNumeric() || !rightType->isNumeric()) {
            addError("Bitwise operators require numeric types", binExpr->line,
                     binExpr->column);
            return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
        }
        return leftType->clone();

    case TokenType::EQUAL:
    case TokenType::NOT_EQUAL:
    case TokenType::LESS_THAN:
    case TokenType::GREATER_THAN:
    case TokenType::LESS_EQUAL:
    case TokenType::GREATER_EQUAL:
        // Comparison operators return int (0 or 1)
        if (!leftType->isCompatibleWith(rightType.get()) &&
            !rightType->isCompatibleWith(leftType.get())) {
            addError("Cannot compare incompatible types: " +
                         leftType->toString() + " and " + rightType->toString(),
                     binExpr->line, binExpr->column);
            return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
        }
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::INT);

    default:
        addError("Unknown binary operator", binExpr->line, binExpr->column);
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
    }
}

std::unique_ptr<SemanticType>
SemanticAnalyzer::visitUnaryExpression(const UnaryExpression *unExpr) {
    auto operandType = visitExpression(unExpr->operand.get());
    if (!operandType) {
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
    }

    switch (unExpr->operator_) {
    case TokenType::MINUS:
    case TokenType::BITWISE_NOT:
        if (!operandType->isNumeric()) {
            addError("Unary arithmetic operators require numeric types",
                     unExpr->line, unExpr->column);
            return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
        }
        return operandType->clone();

    case TokenType::REFERENCE:
        return std::make_unique<PointerSemanticType>(operandType->clone());

    case TokenType::DEREFERENCE: {
        if (!operandType->isPointer()) {
            addError("Dereference operator requires pointer type", unExpr->line,
                     unExpr->column);
            return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
        }
        const auto *ptrType =
            dynamic_cast<const PointerSemanticType *>(operandType.get());
        
        // Debug: Check what we're dereferencing
        std::cout << "DEBUG: Dereferencing pointer to: " << ptrType->pointsTo->toString() << std::endl;
        if (ptrType->pointsTo->isLayout()) {
            const auto *layoutType = dynamic_cast<const LayoutSemanticType *>(ptrType->pointsTo.get());
            std::cout << "DEBUG: Original layout '" << layoutType->layoutName 
                      << "' has " << layoutType->members.size() << " members" << std::endl;
            
            // If the layout has no members, it might be a forward declaration
            // Try to resolve it from the symbol table
            if (layoutType->members.empty()) {
                std::cout << "DEBUG: Layout has no members, attempting to resolve from symbol table" << std::endl;
                Symbol *layoutSymbol = symbolTable.findSymbol(layoutType->layoutName);
                if (layoutSymbol && layoutSymbol->symbolKind == SymbolKind::LAYOUT) {
                    const auto *completeLayout = dynamic_cast<const LayoutSemanticType *>(layoutSymbol->type.get());
                    std::cout << "DEBUG: Found complete layout with " << completeLayout->members.size() << " members" << std::endl;
                    return completeLayout->clone();
                }
            }
        }
        
        auto result = ptrType->pointsTo->clone();
        
        // Debug: Check what the clone produced
        if (result->isLayout()) {
            const auto *clonedLayoutType = dynamic_cast<const LayoutSemanticType *>(result.get());
            std::cout << "DEBUG: Cloned layout '" << clonedLayoutType->layoutName 
                      << "' has " << clonedLayoutType->members.size() << " members" << std::endl;
        }
        
        return result;
    }

    default:
        addError("Unknown unary operator", unExpr->line, unExpr->column);
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
    }
}

std::unique_ptr<SemanticType>
SemanticAnalyzer::visitFunctionCall(const FunctionCall *funcCall) {
    // Check if this is a namespace-qualified function call
    size_t dotPos = funcCall->functionName.find('.');
    if (dotPos != std::string::npos) {
        std::string namespaceName = funcCall->functionName.substr(0, dotPos);
        std::string functionName = funcCall->functionName.substr(dotPos + 1);

        // Push namespace scope
        symbolTable.pushScope("namespace_" + namespaceName);

        // Look up the function in the namespace scope
        Symbol *symbol = symbolTable.findSymbol(functionName);
        if (symbol == nullptr) {
            addError("Undefined function '" + functionName +
                         "' in namespace '" + namespaceName + "'",
                     funcCall->line, funcCall->column);
            symbolTable.popScope();
            return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
        }

        if (!symbol->type->isFunction()) {
            addError("'" + functionName + "' in namespace '" + namespaceName +
                         "' is not a function",
                     funcCall->line, funcCall->column);
            symbolTable.popScope();
            return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
        }

        const auto *funcType =
            dynamic_cast<const FunctionSemanticType *>(symbol->type.get());

        // Check arguments
        if (funcCall->arguments.size() != funcType->parameterTypes.size()) {
            addError("Function '" + namespaceName + "." + functionName +
                         "' expects " +
                         std::to_string(funcType->parameterTypes.size()) +
                         " arguments, got " +
                         std::to_string(funcCall->arguments.size()),
                     funcCall->line, funcCall->column);
            symbolTable.popScope();
            return funcType->returnType->clone();
        }

        for (size_t i = 0; i < funcCall->arguments.size(); ++i) {
            auto argType = visitExpression(funcCall->arguments[i].get());
            if (argType && funcType->parameterTypes[i]) {
                if (!funcType->parameterTypes[i]->isCompatibleWith(
                        argType.get())) {
                    addError("Argument " + std::to_string(i + 1) +
                                 " type mismatch in function '" +
                                 namespaceName + "." + functionName +
                                 "'. Expected " +
                                 funcType->parameterTypes[i]->toString() +
                                 ", got " + argType->toString(),
                             funcCall->line, funcCall->column);
                }
            }
        }

        symbolTable.popScope();
        return funcType->returnType->clone();
    }

    // Regular (non-namespace) function call
    Symbol *symbol = symbolTable.findSymbol(funcCall->functionName);
    if (symbol == nullptr) {
        addError("Undefined function '" + funcCall->functionName + "'",
                 funcCall->line, funcCall->column);
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
    }

    if (!symbol->type->isFunction()) {
        addError("'" + funcCall->functionName + "' is not a function",
                 funcCall->line, funcCall->column);
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
    }

    const auto *funcType =
        dynamic_cast<const FunctionSemanticType *>(symbol->type.get());

    if (funcCall->arguments.size() != funcType->parameterTypes.size()) {
        addError("Function '" + funcCall->functionName + "' expects " +
                     std::to_string(funcType->parameterTypes.size()) +
                     " arguments, got " +
                     std::to_string(funcCall->arguments.size()),
                 funcCall->line, funcCall->column);
        return funcType->returnType->clone();
    }

    for (size_t i = 0; i < funcCall->arguments.size(); ++i) {
        auto argType = visitExpression(funcCall->arguments[i].get());
        if (argType && funcType->parameterTypes[i]) {
            if (!funcType->parameterTypes[i]->isCompatibleWith(argType.get())) {
                addError("Argument " + std::to_string(i + 1) +
                             " type mismatch in function '" +
                             funcCall->functionName + "'. Expected " +
                             funcType->parameterTypes[i]->toString() +
                             ", got " + argType->toString(),
                         funcCall->line, funcCall->column);
            }
        }
    }

    return funcType->returnType->clone();
}

std::unique_ptr<SemanticType>
SemanticAnalyzer::visitArrayAllocation(const ArrayAllocation *arrayAlloc) {
    auto sizeType = visitExpression(arrayAlloc->size.get());
    if (sizeType && !sizeType->isNumeric()) {
        addError("Array size must be numeric", arrayAlloc->line,
                 arrayAlloc->column);
    }

    auto elementType = convertType(arrayAlloc->elementType.get());
    return std::make_unique<PointerSemanticType>(std::move(elementType));
}

std::unique_ptr<SemanticType>
SemanticAnalyzer::visitArrayAccess(const ArrayAccess *arrayAccess) {
    auto arrayType = visitExpression(arrayAccess->array.get());
    auto indexType = visitExpression(arrayAccess->index.get());

    if (indexType && !indexType->isNumeric()) {
        addError("Array index must be numeric", arrayAccess->line,
                 arrayAccess->column);
    }

    if (!arrayType) {
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
    }

    if (!arrayType->isPointer() && !arrayType->isArray()) {
        addError("Array access requires pointer/array type", arrayAccess->line,
                 arrayAccess->column);
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
    }

    if (arrayType->isPointer()) {
        const auto *ptrType =
            dynamic_cast<const PointerSemanticType *>(arrayType.get());
        return ptrType->pointsTo->clone();
    }
    if (arrayType->isArray()) {
        const ArraySemanticType *arrType =
            dynamic_cast<const ArraySemanticType *>(arrayType.get());
        return arrType->elementType->clone();
    }

    return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
}

std::unique_ptr<SemanticType>
SemanticAnalyzer::visitMemberAccess(const MemberAccess *memberAccess) {
    auto objectType = visitExpression(memberAccess->object.get());

    if (!objectType) {
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
    }

    // Handle member access on pointer types (auto-dereference)
    if (objectType->isPointer()) {
        const auto *ptrType =
            dynamic_cast<const PointerSemanticType *>(objectType.get());
        objectType = ptrType->pointsTo->clone();
    }

    if (!objectType->isLayout()) {
        addError("Member access requires layout type", memberAccess->line,
                 memberAccess->column);
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
    }

    const auto *layoutType =
        dynamic_cast<const LayoutSemanticType *>(objectType.get());
    
    // Debug: Print layout information
    std::cout << "DEBUG: Looking for member '" << memberAccess->memberName 
              << "' in layout '" << layoutType->layoutName << "'" << std::endl;
    std::cout << "DEBUG: Layout has " << layoutType->members.size() << " members:" << std::endl;
    for (const auto &layoutMember : layoutType->members) {
        std::cout << "  - " << layoutMember->name << " (" << layoutMember->type->toString() << ")" << std::endl;
    }
    
    const LayoutSemanticType::Member *member =
        layoutType->findMember(memberAccess->memberName);

    if (member == nullptr) {
        addError("Layout '" + layoutType->layoutName + "' has no member '" +
                     memberAccess->memberName + "'",
                 memberAccess->line, memberAccess->column);
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
    }

    return member->type->clone();
}

std::unique_ptr<SemanticType>
SemanticAnalyzer::visitSyscallExpression(const SyscallExpression *syscallExpr) {
    // syscall expects exactly 7 arguments (based on user example: syscall(0, 1,
    // 2, 3, 4, 5, 8))
    if (syscallExpr->arguments.size() != 7) {
        addError("syscall expects exactly 7 arguments, got " +
                     std::to_string(syscallExpr->arguments.size()),
                 syscallExpr->line, syscallExpr->column);
        return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
    }

    // syscall returns an integer value
    return std::make_unique<BasicSemanticType>(SemanticTypeKind::INT);
}

std::unique_ptr<SemanticType>
SemanticAnalyzer::visitLayoutInitialization(const LayoutInitialization *layoutInit) {
    // Layout initialization returns a special type that can be assigned to layout variables
    // We'll return an ERROR type for now since we can't determine the specific layout type
    // without more context. The actual type checking will happen during assignment.
    
    // Validate all initialization values
    for (const auto &value : layoutInit->values) {
        auto valueType = visitExpression(value.get());
        if (!valueType || valueType->isError()) {
            addError("Invalid expression in layout initialization",
                     value->line, value->column);
        }
    }
    
    // Return a special marker type - this will be resolved during assignment
    return std::make_unique<BasicSemanticType>(SemanticTypeKind::ERROR);
}

} // namespace calpha