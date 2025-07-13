#include "lsp_server.hpp"
#include "json_rpc_handler.hpp"
#include "notification_handler.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <memory>

using namespace calpha::lsp;

// Read a message from stdin following LSP protocol
std::string readMessage() {
    std::string header;
    std::string line;
    int contentLength = 0;
    
    // Read headers
    while (std::getline(std::cin, line) && line != "\r" && line != "") {
        if (line.find("Content-Length: ") == 0) {
            contentLength = std::stoi(line.substr(16));
        }
    }
    
    if (contentLength == 0) {
        return "";
    }
    
    // Read content
    std::string content(contentLength, ' ');
    std::cin.read(&content[0], contentLength);
    
    return content;
}

// Write a message to stdout following LSP protocol
void writeMessage(const std::string& content) {
    if (content.empty()) {
        return;
    }
    
    std::cout << "Content-Length: " << content.length() << "\r\n\r\n";
    std::cout << content;
    std::cout.flush();
}

int main() {
    // Disable stdin/stdout buffering
    std::ios::sync_with_stdio(false);
    
    auto server = std::make_shared<LSPServer>();
    auto rpcHandler = std::make_shared<JSONRPCHandler>();
    auto notificationHandler = std::make_shared<NotificationHandler>();
    
    // Set up notification handler
    notificationHandler->setNotificationCallback(
        [&rpcHandler](const std::string& method, const json& params) {
            json notification = {
                {"jsonrpc", "2.0"},
                {"method", method},
                {"params", params}
            };
            writeMessage(notification.dump());
        }
    );
    
    // Set notification handler in server
    server->setNotificationHandler(notificationHandler);
    
    // Register LSP method handlers
    rpcHandler->registerHandler("initialize", [&](const json& params) -> json {
        server->initialize();
        return {
            {"capabilities", {
                {"textDocumentSync", 1}, // Full sync
                {"definitionProvider", true},
                {"referencesProvider", true},
                {"hoverProvider", true}
            }}
        };
    });
    
    rpcHandler->registerHandler("shutdown", [&](const json& params) -> json {
        server->shutdown();
        return json::object();
    });
    
    rpcHandler->registerHandler("exit", [&](const json& params) -> json {
        server->exit();
        std::exit(0);
        return json::object();
    });
    
    // Document sync handlers
    rpcHandler->registerHandler("textDocument/didOpen", [&](const json& params) -> json {
        auto doc = params["textDocument"];
        server->didOpen(doc["uri"].get<std::string>(), doc["text"].get<std::string>());
        return json::object();
    });
    
    rpcHandler->registerHandler("textDocument/didChange", [&](const json& params) -> json {
        auto doc = params["textDocument"];
        auto changes = params["contentChanges"];
        if (!changes.empty()) {
            server->didChange(doc["uri"].get<std::string>(), changes[0]["text"].get<std::string>());
        }
        return json::object();
    });
    
    rpcHandler->registerHandler("textDocument/didClose", [&](const json& params) -> json {
        auto doc = params["textDocument"];
        server->didClose(doc["uri"].get<std::string>());
        return json::object();
    });
    
    rpcHandler->registerHandler("textDocument/didSave", [&](const json& params) -> json {
        auto doc = params["textDocument"];
        server->didSave(doc["uri"].get<std::string>());
        return json::object();
    });
    
    // Language feature handlers
    rpcHandler->registerHandler("textDocument/definition", [&](const json& params) -> json {
        auto doc = params["textDocument"];
        auto pos = params["position"];
        Position position{pos["line"].get<int>(), pos["character"].get<int>()};
        
        auto locations = server->getDefinition(doc["uri"].get<std::string>(), position);
        json result = json::array();
        for (const auto& loc : locations) {
            result.push_back(loc);
        }
        return result;
    });
    
    rpcHandler->registerHandler("textDocument/references", [&](const json& params) -> json {
        auto doc = params["textDocument"];
        auto pos = params["position"];
        Position position{pos["line"].get<int>(), pos["character"].get<int>()};
        
        auto locations = server->getReferences(doc["uri"].get<std::string>(), position);
        json result = json::array();
        for (const auto& loc : locations) {
            result.push_back(loc);
        }
        return result;
    });
    
    rpcHandler->registerHandler("textDocument/hover", [&](const json& params) -> json {
        auto doc = params["textDocument"];
        auto pos = params["position"];
        Position position{pos["line"].get<int>(), pos["character"].get<int>()};
        
        auto hover = server->getHover(doc["uri"].get<std::string>(), position);
        if (hover) {
            return {{"contents", *hover}};
        }
        return json::object();
    });
    
    // Main message loop
    while (true) {
        std::string message = readMessage();
        if (message.empty()) {
            continue;
        }
        
        std::string response = rpcHandler->handleMessage(message);
        if (!response.empty()) {
            writeMessage(response);
        }
    }
    
    return 0;
} 