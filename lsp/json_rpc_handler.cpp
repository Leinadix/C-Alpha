#include "json_rpc_handler.hpp"
#include <stdexcept>

namespace calpha {
namespace lsp {

JSONRPCHandler::JSONRPCHandler() : nextId_(1) {}

JSONRPCHandler::~JSONRPCHandler() = default;

void JSONRPCHandler::registerHandler(const std::string& method, MessageHandler handler) {
    handlers_[method] = std::move(handler);
}

std::string JSONRPCHandler::handleMessage(const std::string& message) {
    try {
        json j = json::parse(message);
        
        // Check if it's a request or notification
        bool isRequest = j.contains("id");
        
        if (isRequest) {
            json response = handleRequest(j);
            return response.dump();
        } else {
            handleNotification(j);
            return "";
        }
    } catch (const std::exception& e) {
        // Create error response for malformed JSON
        json error = createErrorResponse(-1, -32700, "Parse error: " + std::string(e.what()));
        return error.dump();
    }
}

void JSONRPCHandler::sendNotification(const std::string& method, const json& params) {
    json notification = {
        {"jsonrpc", "2.0"},
        {"method", method},
        {"params", params}
    };
    
    // TODO: Implement actual sending mechanism (e.g., stdout for now)
    // std::cout << notification.dump() << std::endl;
}

json JSONRPCHandler::handleRequest(const json& request) {
    int id = request["id"].get<int>();
    std::string method = request["method"].get<std::string>();
    
    auto it = handlers_.find(method);
    if (it == handlers_.end()) {
        return createErrorResponse(id, -32601, "Method not found: " + method);
    }
    
    try {
        json params = request.contains("params") ? request["params"] : json::object();
        json result = it->second(params);
        return createResponse(id, result);
    } catch (const std::exception& e) {
        return createErrorResponse(id, -32603, "Internal error: " + std::string(e.what()));
    }
}

void JSONRPCHandler::handleNotification(const json& notification) {
    std::string method = notification["method"].get<std::string>();
    
    auto it = handlers_.find(method);
    if (it != handlers_.end()) {
        try {
            json params = notification.contains("params") ? notification["params"] : json::object();
            it->second(params);
        } catch (const std::exception& e) {
            // Log error but don't send response for notifications
            // TODO: Implement proper error logging
        }
    }
}

json JSONRPCHandler::createResponse(int id, const json& result) {
    return {
        {"jsonrpc", "2.0"},
        {"id", id},
        {"result", result}
    };
}

json JSONRPCHandler::createErrorResponse(int id, int code, const std::string& message) {
    return {
        {"jsonrpc", "2.0"},
        {"id", id},
        {"error", {
            {"code", code},
            {"message", message}
        }}
    };
}

} // namespace lsp
} // namespace alpha 