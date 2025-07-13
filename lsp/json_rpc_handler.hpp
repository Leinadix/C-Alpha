#pragma once

#include <string>
#include <functional>
#include <memory>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace calpha {
namespace lsp {

using json = nlohmann::json;

class JSONRPCHandler {
public:
    using MessageHandler = std::function<json(const json&)>;

    JSONRPCHandler();
    ~JSONRPCHandler();

    // Register method handlers
    void registerHandler(const std::string& method, MessageHandler handler);

    // Process incoming message
    std::string handleMessage(const std::string& message);

    // Send notification (no response expected)
    void sendNotification(const std::string& method, const json& params);

private:
    // Message handling
    json handleRequest(const json& request);
    void handleNotification(const json& notification);
    
    // Response creation
    json createResponse(int id, const json& result);
    json createErrorResponse(int id, int code, const std::string& message);

    std::unordered_map<std::string, MessageHandler> handlers_;
    int nextId_;
};

} // namespace lsp
} // namespace alpha 