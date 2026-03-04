#include "httpCore/ErrorHandler.hpp"
#include <fstream>
#include <sstream>
#include <limits.h>
#include <stdlib.h>

std::string ErrorHandler::getReasonPhrase(int code)
{
    switch (code)
    {
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 408: return "Request Timeout";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 504: return "Gateway Timeout";
        default:  return "Error";
    }
}

std::string ErrorHandler::resolvePathError(int code, const RuntimeLocation* loc)
{
    if (loc) {
        const std::map<int, std::string>& locPages = loc->getErrorPages();
        std::map<int, std::string>::const_iterator it = locPages.find(code);
        if (it != locPages.end()) {
            return it->second;
        }
    }
    return "";
}

static std::string buildAllowHeader(const std::set<HttpMethod>& allowed)
{
    std::string allow;
    for (std::set<HttpMethod>::const_iterator it = allowed.begin(); it != allowed.end(); ++it)
    {
        if (!allow.empty()) allow += ", ";
        if (*it == GET) allow += "GET";
        else if (*it == POST) allow += "POST";
        else if (*it == DELETE) allow += "DELETE";
    }
    return allow;
}

HttpResponse ErrorHandler::build405(const std::set<HttpMethod>& allowed, const RuntimeLocation* loc)
{
    HttpResponse res = build(405, loc);
    res.headers["Allow"] = buildAllowHeader(allowed);
    return res;
}

HttpResponse ErrorHandler::build(int code, const RuntimeLocation* loc)
{
    return build(code, getReasonPhrase(code) + "\n", loc);
}

std::string ErrorHandler::readFile(const std::string& path, bool& success)
{
    std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
    if (!in.is_open()) {
        success = false;
        return "";
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    success = true;
    return ss.str();
}

HttpResponse ErrorHandler::build(int code, const std::string& fallbackMsg, const RuntimeLocation* loc)
{
    HttpResponse res;
    res.status_code = code;
    res.reason_phrase = getReasonPhrase(code);

    std::string pagePath = resolvePathError(code, loc);
    bool fileOk = false;
    std::string content;
    if (!pagePath.empty()) {
        content = readFile(pagePath, fileOk);
        if (fileOk) {
            res.body = content;
            res.headers["Content-Type"] = "text/html";
            return res;
        }
    }
    // Fallback
    res.body = fallbackMsg;
    res.headers["Content-Type"] = "text/plain";
    return res;
}