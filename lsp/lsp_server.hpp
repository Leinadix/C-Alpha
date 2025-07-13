#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>
#include <nlohmann/json.hpp>

namespace calpha {
namespace lsp {

using json = nlohmann::json;

struct Position {
    int line;
    int character;
};

struct Range {
    Position start;
    Position end;
};

struct Location {
    std::string uri;
    Range range;
};

struct Diagnostic {
    Range range;
    int severity;
    std::string message;
};

// Forward declarations
class NotificationHandler;

// JSON serialization function declarations
void to_json(json& j, const Position& p);
void from_json(const json& j, Position& p);
void to_json(json& j, const Range& r);
void from_json(const json& j, Range& r);
void to_json(json& j, const Location& l);
void from_json(const json& j, Location& l);
void to_json(json& j, const Diagnostic& d);
void from_json(const json& j, Diagnostic& d);

class LSPServer {
public:
    LSPServer();
    ~LSPServer();

    // Set notification handler
    void setNotificationHandler(std::shared_ptr<NotificationHandler> handler);

    // LSP lifecycle methods
    void initialize();
    void shutdown();
    void exit();

    // Document sync methods
    void didOpen(const std::string& uri, const std::string& text);
    void didChange(const std::string& uri, const std::string& text);
    void didClose(const std::string& uri);
    void didSave(const std::string& uri);

    // Language features
    std::vector<Location> getDefinition(const std::string& uri, Position position);
    std::vector<Location> getReferences(const std::string& uri, Position position);
    std::optional<std::string> getHover(const std::string& uri, Position position);
    std::vector<Diagnostic> getDiagnostics(const std::string& uri);

private:
    // Document storage
    std::unordered_map<std::string, std::string> documents_;
    
    // Parser and semantic analysis integration
    void analyzeDocument(const std::string& uri);
    void clearDiagnostics(const std::string& uri);
    
    bool initialized_;
    bool shutdownRequested_;
    std::shared_ptr<NotificationHandler> notificationHandler_;
};

} // namespace lsp
} // namespace alpha 