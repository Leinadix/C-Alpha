#include "lsp_server.hpp"
#include "notification_handler.hpp"
#include <lexer.hpp>
#include <parser.hpp>
#include <semantic.hpp>
#include <stdexcept>
#include <unordered_map>
#include <memory>

namespace calpha {
namespace lsp {

struct DocumentState {
    std::string content;
    std::vector<Diagnostic> diagnostics;
    std::shared_ptr<Parser> parser;
    std::shared_ptr<SemanticAnalyzer> analyzer;
};

LSPServer::LSPServer() 
    : initialized_(false)
    , shutdownRequested_(false)
    , notificationHandler_(nullptr) {
}

LSPServer::~LSPServer() {
    if (!shutdownRequested_) {
        shutdown();
    }
}

void LSPServer::setNotificationHandler(std::shared_ptr<NotificationHandler> handler) {
    notificationHandler_ = std::move(handler);
}

void LSPServer::initialize() {
    if (initialized_) {
        throw std::runtime_error("LSP server already initialized");
    }
    initialized_ = true;
    
    if (notificationHandler_) {
        notificationHandler_->logMessage("LSP server initialized", 3);
    }
}

void LSPServer::shutdown() {
    if (!initialized_) {
        throw std::runtime_error("LSP server not initialized");
    }
    shutdownRequested_ = true;
    documents_.clear();
    
    if (notificationHandler_) {
        notificationHandler_->logMessage("LSP server shutting down", 3);
    }
}

void LSPServer::exit() {
    if (!shutdownRequested_) {
        throw std::runtime_error("Shutdown must be called before exit");
    }
}

void LSPServer::didOpen(const std::string& uri, const std::string& text) {
    if (!initialized_) {
        throw std::runtime_error("LSP server not initialized");
    }
    documents_[uri] = text;
    
    if (notificationHandler_) {
        notificationHandler_->logMessage("Document opened: " + uri, 4);
    }
    
    analyzeDocument(uri);
}

void LSPServer::didChange(const std::string& uri, const std::string& text) {
    if (!initialized_) {
        throw std::runtime_error("LSP server not initialized");
    }
    auto it = documents_.find(uri);
    if (it == documents_.end()) {
        throw std::runtime_error("Document not found: " + uri);
    }
    documents_[uri] = text;
    
    if (notificationHandler_) {
        notificationHandler_->logMessage("Document changed: " + uri, 4);
    }
    
    analyzeDocument(uri);
}

void LSPServer::didClose(const std::string& uri) {
    if (!initialized_) {
        throw std::runtime_error("LSP server not initialized");
    }
    documents_.erase(uri);
    clearDiagnostics(uri);
    
    if (notificationHandler_) {
        notificationHandler_->logMessage("Document closed: " + uri, 4);
    }
}

void LSPServer::didSave(const std::string& uri) {
    if (!initialized_) {
        throw std::runtime_error("LSP server not initialized");
    }
    
    if (notificationHandler_) {
        notificationHandler_->logMessage("Document saved: " + uri, 4);
    }
    
    analyzeDocument(uri);
}

std::vector<Location> LSPServer::getDefinition(const std::string& uri, Position position) {
    if (!initialized_) {
        throw std::runtime_error("LSP server not initialized");
    }

    auto it = documents_.find(uri);
    if (it == documents_.end()) {
        return {};
    }

    try {
        // Create lexer and parser
        Lexer lexer(it->second);
        auto tokens = lexer.tokenize();
        
        // Find the token at the given position
        const Token* targetToken = nullptr;
        for (const auto& token : tokens) {
            if (token.line == position.line + 1) { // LSP is 0-based, our lexer is 1-based
                if (token.column <= position.character && 
                    token.column + token.value.length() > position.character) {
                    targetToken = &token;
                    break;
                }
            }
        }
        
        if (!targetToken || targetToken->type != TokenType::IDENTIFIER) {
            return {};
        }

        // Parse the program
        auto parser = std::make_shared<Parser>(tokens);
        auto program = parser->parseProgram();

        // Create semantic analyzer
        SemanticAnalyzer analyzer;
        if (!analyzer.analyze(program.get())) {
            return {};
        }

        // Get all scopes from the symbol table
        auto scopes = analyzer.getSymbolTable().getAllScopes();
        
        // Search for the symbol definition
        for (const auto* scope : scopes) {
            for (const auto& [name, symbol] : scope->symbols) {
                if (name == targetToken->value) {
                    // Convert 1-based line/column to 0-based for LSP
                    Location location;
                    location.uri = uri;
                    location.range.start.line = symbol->line - 1;
                    location.range.start.character = symbol->column - 1;
                    location.range.end.line = symbol->line - 1;
                    location.range.end.character = symbol->column - 1 + symbol->name.length();
                    return {location};
                }
            }
        }

        return {};
    } catch (const std::exception& e) {
        if (notificationHandler_) {
            notificationHandler_->logMessage("Error in getDefinition: " + std::string(e.what()), 1);
        }
        return {};
    }
}

std::vector<Location> LSPServer::getReferences(const std::string& uri, Position position) {
    if (!initialized_) {
        throw std::runtime_error("LSP server not initialized");
    }

    auto it = documents_.find(uri);
    if (it == documents_.end()) {
        return {};
    }

    try {
        // Create lexer and parser
        Lexer lexer(it->second);
        auto tokens = lexer.tokenize();
        
        // Find the token at the given position
        const Token* targetToken = nullptr;
        for (const auto& token : tokens) {
            if (token.line == position.line + 1) { // LSP is 0-based, our lexer is 1-based
                if (token.column <= position.character && 
                    token.column + token.value.length() > position.character) {
                    targetToken = &token;
                    break;
                }
            }
        }
        
        if (!targetToken || targetToken->type != TokenType::IDENTIFIER) {
            return {};
        }

        // Find all occurrences of the identifier
        std::vector<Location> references;
        for (const auto& token : tokens) {
            if (token.type == TokenType::IDENTIFIER && token.value == targetToken->value) {
                Location location;
                location.uri = uri;
                location.range.start.line = token.line - 1;
                location.range.start.character = token.column - 1;
                location.range.end.line = token.line - 1;
                location.range.end.character = token.column - 1 + token.value.length();
                references.push_back(location);
            }
        }

        return references;
    } catch (const std::exception& e) {
        if (notificationHandler_) {
            notificationHandler_->logMessage("Error in getReferences: " + std::string(e.what()), 1);
        }
        return {};
    }
}

std::optional<std::string> LSPServer::getHover(const std::string& uri, Position position) {
    if (!initialized_) {
        throw std::runtime_error("LSP server not initialized");
    }

    auto it = documents_.find(uri);
    if (it == documents_.end()) {
        return std::nullopt;
    }

    try {
        // Create lexer and parser
        Lexer lexer(it->second);
        auto tokens = lexer.tokenize();
        
        // Find the token at the given position
        const Token* targetToken = nullptr;
        for (const auto& token : tokens) {
            if (token.line == position.line + 1) { // LSP is 0-based, our lexer is 1-based
                if (token.column <= position.character && 
                    token.column + token.value.length() > position.character) {
                    targetToken = &token;
                    break;
                }
            }
        }
        
        if (!targetToken || targetToken->type != TokenType::IDENTIFIER) {
            return std::nullopt;
        }

        // Parse the program
        auto parser = std::make_shared<Parser>(tokens);
        auto program = parser->parseProgram();

        // Create semantic analyzer
        SemanticAnalyzer analyzer;
        if (!analyzer.analyze(program.get())) {
            return std::nullopt;
        }

        // Get all scopes from the symbol table
        auto scopes = analyzer.getSymbolTable().getAllScopes();
        
        // Search for the symbol
        for (const auto* scope : scopes) {
            for (const auto& [name, symbol] : scope->symbols) {
                if (name == targetToken->value) {
                    std::string hoverText = "**" + name + "**\n\n";
                    
                    // Add symbol kind
                    switch (symbol->symbolKind) {
                        case SymbolKind::VARIABLE:
                            hoverText += "Variable\n";
                            break;
                        case SymbolKind::FUNCTION:
                            hoverText += "Function\n";
                            break;
                        case SymbolKind::PARAMETER:
                            hoverText += "Parameter\n";
                            break;
                        case SymbolKind::LAYOUT:
                            hoverText += "Layout\n";
                            break;
                    }
                    
                    // Add type information
                    if (symbol->type) {
                        hoverText += "\nType: `" + symbol->type->toString() + "`";
                    }
                    
                    // Add scope information
                    hoverText += "\nDefined in scope: `" + scope->scopeName + "`";
                    
                    return hoverText;
                }
            }
        }

        return std::nullopt;
    } catch (const std::exception& e) {
        if (notificationHandler_) {
            notificationHandler_->logMessage("Error in getHover: " + std::string(e.what()), 1);
        }
        return std::nullopt;
    }
}

std::vector<Diagnostic> LSPServer::getDiagnostics(const std::string& uri) {
    if (!initialized_) {
        throw std::runtime_error("LSP server not initialized");
    }

    auto it = documents_.find(uri);
    if (it == documents_.end()) {
        return {};
    }

    try {
        std::vector<Diagnostic> diagnostics;

        // Create lexer and parser
        Lexer lexer(it->second);
        auto tokens = lexer.tokenize();
        auto parser = std::make_shared<Parser>(tokens);

        try {
            auto program = parser->parseProgram();
        } catch (const Parser::ParseError& e) {
            // Handle parser errors
            Diagnostic diagnostic;
            diagnostic.range = Range{
                Position{e.getLine(), e.getColumn()},
                Position{e.getLine(), e.getColumn() + 1} // Assuming 1 character for now
            };
            diagnostic.severity = 1; // Error
            diagnostic.message = e.what();
            diagnostics.push_back(diagnostic);
        }

        // TODO: Add semantic analyzer errors
        // This will require adding error collection to your semantic analyzer

        return diagnostics;
    } catch (const std::exception& e) {
        if (notificationHandler_) {
            notificationHandler_->logMessage("Error in getDiagnostics: " + std::string(e.what()), 1);
        }
        
        // If we get a fatal error, return it as a diagnostic
        Diagnostic diagnostic;
        diagnostic.range = Range{Position{0, 0}, Position{0, 0}};
        diagnostic.severity = 1; // Error
        diagnostic.message = e.what();
        return {diagnostic};
    }
}

void LSPServer::analyzeDocument(const std::string& uri) {
    auto it = documents_.find(uri);
    if (it == documents_.end()) {
        return;
    }

    try {
        // Create lexer and parser
        Lexer lexer(it->second);
        auto tokens = lexer.tokenize();
        auto parser = std::make_shared<Parser>(tokens);

        try {
            auto program = parser->parseProgram();
            // Get diagnostics
            auto diagnostics = getDiagnostics(uri);

            // Publish diagnostics to the client
            if (notificationHandler_) {
                notificationHandler_->publishDiagnostics(uri, diagnostics);
            }
        } catch (const Parser::ParseError& e) {
            // Handle parser errors
            Diagnostic diagnostic;
            diagnostic.range = Range{
                Position{e.getLine(), e.getColumn()},
                Position{e.getLine(), e.getColumn() + 1} // Assuming 1 character for now
            };
            diagnostic.severity = 1; // Error
            diagnostic.message = e.what();

            // Publish diagnostic to the client
            if (notificationHandler_) {
                notificationHandler_->publishDiagnostics(uri, {diagnostic});
                notificationHandler_->logMessage("Parse error: " + std::string(e.what()), 1);
            }
        }
    } catch (const std::exception& e) {
        // Handle fatal errors
        Diagnostic diagnostic;
        diagnostic.range = Range{Position{0, 0}, Position{0, 0}};
        diagnostic.severity = 1; // Error
        diagnostic.message = e.what();

        // Publish diagnostic to the client
        if (notificationHandler_) {
            notificationHandler_->publishDiagnostics(uri, {diagnostic});
            notificationHandler_->logMessage("Fatal error in analyzeDocument: " + std::string(e.what()), 1);
        }
    }
}

void LSPServer::clearDiagnostics(const std::string& uri) {
    // Publish empty diagnostics to clear them in the editor
    if (notificationHandler_) {
        notificationHandler_->publishDiagnostics(uri, {});
    }
}

} // namespace lsp
} // namespace calpha 