#include "httpCore/ResponseBuilder.hpp"

static bool endsWith(const std::string&s, const std::string& suf)
{
    if (s.size() < suf.size()) return false;
    return s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
}

static std::string guessContentType(const std::string& path)
{
    if (endsWith(path, ".html") || endsWith(path, ".htm")) return "text/html";
    if (endsWith(path, ".css")) return "text/css";
    if (endsWith(path, ".js")) return "application/javascript";
    if (endsWith(path, ".png")) return "image/png";
    if (endsWith(path, ".jpg") || endsWith(path, ".jpeg")) return "image/jpeg";
    if (endsWith(path, ".gif")) return "image/gif";
    if (endsWith(path, ".pdf")) return "application/pdf";
    if (endsWith(path, ".json")) return "application/json";
    if (endsWith(path, ".xml")) return "application/xml";
    if (endsWith(path, ".txt")) return "text/plain";
    if (endsWith(path, ".svg")) return "image/svg+xml";
    if (endsWith(path, ".ico")) return "image/x-icon";
    if (endsWith(path, ".bin")) return "application/octet-stream";
    return "application/octet-stream";
}

HttpResponse ResponseBuilder::ok(const std::string& body, const std::string& path) {
    HttpResponse res;
    res.status_code = 200;
    res.reason_phrase = "OK";
    res.body = body;
    res.headers["Content-Type"] = guessContentType(path);
    return res;
}

HttpResponse ResponseBuilder::redirect(int code, const std::string& location) {
    HttpResponse res;
    res.status_code = code;
    if (code == 301) res.reason_phrase = "Moved Permanently";
    else if (code == 302) res.reason_phrase = "Found";
    else res.reason_phrase = "Redirect";
    res.headers["Location"] = location;
    res.body = "Redirection to " + location + "\n";
    return res;
}
