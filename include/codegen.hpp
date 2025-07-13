#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <sstream>
#include <memory>
#include "parser.hpp"
#include "semantic.hpp"

namespace calpha {

class RegisterAllocator {
private:
    std::unordered_set<int> availableRegisters;
    std::unordered_map<std::string, int> variableToRegister;
    
public:
    RegisterAllocator();
    
    int allocateRegister(const std::string& variableName = "");
    void deallocateRegister(int registerIndex);
    void deallocateVariable(const std::string& variableName);
    bool isVariableInRegister(const std::string& variableName) const;
    int getVariableRegister(const std::string& variableName) const;
    std::string getRegisterName(int registerIndex) const;
    bool hasAvailableRegister() const;
    void clearAll();
};

// Memory management for variables and layout members with proper scoping
class MemoryManager {
private:
    int nextMemoryAddress;
    
    // Scope stack for variable management using FQDNs
    std::vector<std::unordered_map<std::string, int>> scopeStack;
    
    // Layout member offsets (global)
    std::unordered_map<std::string, std::unordered_map<std::string, int>> layoutMemberOffsets;
    
    // Track current scope path for FQDN construction
    std::vector<std::string> currentScopePath;
    
    std::string getCurrentFQDNPrefix() const {
        if (currentScopePath.empty()) return "global";
        std::string prefix;
        for (const auto& scope : currentScopePath) {
            if (!prefix.empty()) prefix += "::";
            prefix += scope;
        }
        return prefix;
    }
    
public:
    MemoryManager() : nextMemoryAddress(1) {
        // Start at address 1 (0 might be reserved)
        // Create global scope
        scopeStack.push_back(std::unordered_map<std::string, int>());
        currentScopePath.push_back("global");
    }

    int getNextMemoryAddress() const {
        return nextMemoryAddress;
    }

    // Scope management
    void pushScope(const std::string& scopeName) {
        scopeStack.push_back(std::unordered_map<std::string, int>());
        currentScopePath.push_back(scopeName);
    }
    
    void popScope() {
        if (scopeStack.size() <= 1) {
            throw std::runtime_error("Cannot pop global scope");
        }
        scopeStack.pop_back();
        currentScopePath.pop_back();
    }
    
    // Variable memory management with FQDN support
    int allocateMemory(const std::string& fqdn, int size = 1) {
        if (hasVariableInCurrentScope(fqdn)) {
            throw std::runtime_error("Variable '" + fqdn + "' already has memory allocated in current scope");
        }
        
        int address = nextMemoryAddress;
        scopeStack.back()[fqdn] = address;
        nextMemoryAddress += size;
        return address;
    }
    
    int allocateArray(int size) {
        int address = nextMemoryAddress;
        nextMemoryAddress += size;
        return address;
    }
    
    int getVariableAddress(const std::string& fqdn) const {
        // Search from current scope upwards
        for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
            auto varIt = it->find(fqdn);
            if (varIt != it->end()) {
                return varIt->second;
            }
        }
        
        throw std::runtime_error("Variable '" + fqdn + "' not found in memory");
    }
    
    bool hasVariable(const std::string& fqdn) const {
        // Search from current scope upwards
        for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
            if (it->find(fqdn) != it->end()) {
                return true;
            }
        }
        return false;
    }
    
    bool hasVariableInCurrentScope(const std::string& fqdn) const {
        if (scopeStack.empty()) {
            return false;
        }
        return scopeStack.back().find(fqdn) != scopeStack.back().end();
    }
    
    // Layout management with FQDN support
    void setLayoutMemberOffset(const std::string& layoutFQDN, const std::string& memberName, int offset) {
        layoutMemberOffsets[layoutFQDN][memberName] = offset;
    }
    
    int getLayoutMemberOffset(const std::string& layoutFQDN, const std::string& memberName) const {
        auto layoutIt = layoutMemberOffsets.find(layoutFQDN);
        if (layoutIt != layoutMemberOffsets.end()) {
            auto memberIt = layoutIt->second.find(memberName);
            if (memberIt != layoutIt->second.end()) {
                return memberIt->second;
            }
        }
        throw std::runtime_error("Member '" + memberName + "' not found in layout '" + layoutFQDN + "'");
    }
    
    void clearAll() {
        nextMemoryAddress = 1;
        scopeStack.clear();
        layoutMemberOffsets.clear();
        currentScopePath.clear();
        // Reinitialize global scope
        scopeStack.push_back(std::unordered_map<std::string, int>());
        currentScopePath.push_back("global");
    }
};

// Label generation for control flow
class LabelGenerator {
private:
    int nextLabelIndex;
    
public:
    LabelGenerator();
    
    std::string generateLabel(const std::string& prefix = "L");
    void reset();
};

// Main code generator class
class CodeGenerator {
private:
    std::ostringstream output;
    RegisterAllocator registerAllocator;
    MemoryManager memoryManager;
    LabelGenerator labelGenerator;
    SemanticAnalyzer* semanticAnalyzer;
    
    // Stack management
    int stackDepth;
    std::stack<std::string> stackComments; // For debugging
    
    // Control flow
    std::vector<std::string> breakLabels;
    std::vector<std::string> continueLabels;
    
    // Function management
    std::string currentFunction;
    std::unordered_map<std::string, int> functionParameterCounts;
    
    // Variable type tracking for layout member access using FQDNs
    std::unordered_map<std::string, std::string> variableLayoutTypes;  // FQDN -> Layout FQDN
    
    // Helper methods
    void emit(const std::string& instruction);
    void emitComment(const std::string& comment);
    void emitLabel(const std::string& label);
    
    // FQDN helper methods
    std::string getVariableFQDN(const std::string& name);
    std::string getLayoutFQDN(const std::string& name);
    void trackVariableLayout(const std::string& varFQDN, const std::string& layoutFQDN);
    std::string getVariableLayoutType(const std::string& varFQDN);
    
    // Stack operations
    void pushToStack(const std::string& comment = "");
    void popFromStack(const std::string& comment = "");
    void pushRegisterToStack(int registerIndex, const std::string& comment = "");
    void popStackToRegister(int registerIndex, const std::string& comment = "");
    void emitStackOperation(const std::string& operation, const std::string& comment = "");
    
    // Memory operations with FQDN support
    void loadFromMemory(int registerIndex, const std::string& varFQDN);
    void storeToMemory(const std::string& varFQDN, int registerIndex);
    void loadFromMemoryToStack(const std::string& varFQDN, const std::string& comment = "");
    void storeFromStackToMemory(const std::string& varFQDN, const std::string& comment = "");
    
    // Expression evaluation
    void generateExpression(const Expression* expr);
    void generateBinaryExpression(const BinaryExpression* binExpr);
    void generateUnaryExpression(const UnaryExpression* unExpr);
    void generateIdentifier(const Identifier* id);
    void generateLiteral(const Literal* lit);
    void generateStringLiteral(const StringLiteral* stringLit);
    void generateFunctionCall(const FunctionCall* funcCall);
    void generateArrayAccess(const ArrayAccess* arrayAccess);
    void generateMemberAccess(const MemberAccess* memberAccess);
    
    void generateSyscallExpression(const SyscallExpression* syscallExpr);
    void generateArrayAllocation(const ArrayAllocation* arrayAlloc);
    void generateTypeCast(const TypeCast* typeCast);
    
    // Statement generation
    void generateStatement(const Statement* stmt);
    void generateVariableDeclaration(const VariableDeclaration* varDecl);
    void generateAssignment(const Assignment* assignment);
    void generateFunctionDeclaration(const FunctionDeclaration* funcDecl);
    void generateReturnStatement(const ReturnStatement* retStmt);
    void generateIfStatement(const IfStatement* ifStmt);
    void generateWhileStatement(const WhileStatement* whileStmt);
    void generateBlockStatement(const BlockStatement* blockStmt);
    void generateExpressionStatement(const ExpressionStatement* exprStmt);
    void generateLayoutDeclaration(const LayoutDeclaration* layoutDecl);
    void generateNamespaceDeclaration(const NamespaceDeclaration* namespaceDecl);
    
    // Type and layout management
    void setupLayoutMembers(const std::string& layoutName, const std::vector<std::unique_ptr<LayoutMember>>& members);
    int calculateLayoutSize(const std::string& layoutName);
    
    // Utility methods
    std::string getOperatorInstruction(TokenType op);
    bool isComparisonOperator(TokenType op);
    void generateComparison(TokenType op, const std::string& trueLabel, const std::string& falseLabel);
    void generateComparisonResult(TokenType op);
    
    // Utility to get layout name from AST type (handles pointers)
    std::string extractLayoutName(const Type* astType) const;

public:
    CodeGenerator(SemanticAnalyzer* analyzer) 
        : semanticAnalyzer(analyzer), stackDepth(0) {}
    
    std::string generate(const Program* program);
    void reset();
};

class CodeGeneratorError : public std::exception {
    private:
        std::string message;

    public:
        CodeGeneratorError(const std::string& msg) : message(msg) {
            std::cout << "CodeGeneratorError: " << message << std::endl;
        }

        const char* what() const noexcept override { return message.c_str(); }
    };

} // namespace calpha 