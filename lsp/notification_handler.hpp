#pragma once

#include <string>
#include <functional>
#include <nlohmann/json.hpp>
#include "lsp_server.hpp"

namespace calpha {
namespace lsp {

class NotificationHandler {
public:
    using NotificationCallback = std::function<void(const std::string&, const nlohmann::json&)>;

    NotificationHandler();
    ~NotificationHandler();

    // Set callback for sending notifications
    void setNotificationCallback(NotificationCallback callback);

    // Send notifications
    void publishDiagnostics(const std::string& uri, const std::vector<Diagnostic>& diagnostics);
    void logMessage(const std::string& message, int type = 3); // 1=error, 2=warning, 3=info, 4=log

private:
    NotificationCallback callback_;
};

} // namespace lsp
} // namespace alpha 