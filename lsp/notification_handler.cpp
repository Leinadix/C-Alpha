#include "notification_handler.hpp"

namespace calpha {
namespace lsp {

NotificationHandler::NotificationHandler() = default;
NotificationHandler::~NotificationHandler() = default;

void NotificationHandler::setNotificationCallback(NotificationCallback callback) {
    callback_ = std::move(callback);
}

void NotificationHandler::publishDiagnostics(const std::string& uri, const std::vector<Diagnostic>& diagnostics) {
    if (!callback_) {
        return;
    }

    nlohmann::json params = {
        {"uri", uri},
        {"diagnostics", diagnostics}
    };

    callback_("textDocument/publishDiagnostics", params);
}

void NotificationHandler::logMessage(const std::string& message, int type) {
    if (!callback_) {
        return;
    }

    nlohmann::json params = {
        {"type", type},
        {"message", message}
    };

    callback_("window/logMessage", params);
}

} // namespace lsp
} // namespace alpha 