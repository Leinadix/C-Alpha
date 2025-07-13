#include "lsp_server.hpp"

namespace calpha {
namespace lsp {

void to_json(json& j, const Position& p) {
    j = json{{"line", p.line}, {"character", p.character}};
}

void from_json(const json& j, Position& p) {
    j.at("line").get_to(p.line);
    j.at("character").get_to(p.character);
}

void to_json(json& j, const Range& r) {
    j = json{{"start", r.start}, {"end", r.end}};
}

void from_json(const json& j, Range& r) {
    j.at("start").get_to(r.start);
    j.at("end").get_to(r.end);
}

void to_json(json& j, const Location& l) {
    j = json{{"uri", l.uri}, {"range", l.range}};
}

void from_json(const json& j, Location& l) {
    j.at("uri").get_to(l.uri);
    j.at("range").get_to(l.range);
}

void to_json(json& j, const Diagnostic& d) {
    j = json{
        {"range", d.range},
        {"severity", d.severity},
        {"message", d.message}
    };
}

void from_json(const json& j, Diagnostic& d) {
    j.at("range").get_to(d.range);
    j.at("severity").get_to(d.severity);
    j.at("message").get_to(d.message);
}

} // namespace lsp
} // namespace alpha 