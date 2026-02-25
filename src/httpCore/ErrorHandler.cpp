#include "httpCore/ErrorHandler.hpp"
#include <fstream>
#include <sstream>

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

std::string ErrorHandler::readFile(const std::string& path)
{
    std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
    if (!in.is_open())
        return "";
    
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::string ErrorHandler::resolvePathError(int code, const RuntimeServer* server)
{
    if (!server)
        return "";

    const std::map<int, std::string>& pages = server->getErrorPages();
    std::map<int, std::string>::const_iterator it = pages.find(code);
    
    if (it == pages.end())
        return "";

    const std::string& root = server->getRoot();
    const std::string& page = it->second;

    // Join path
    if (root.empty())
        return page;
    if (root[root.size() - 1] == '/' && !page.empty() && page[0] == '/')
        return root + page.substr(1);
    if (root[root.size() - 1] != '/' && !page.empty() && page[0] != '/')
        return root + "/" + page;
    return root + page;
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

HttpResponse ErrorHandler::build405(const std::set<HttpMethod>& allowed, const RuntimeServer* server)
{
    HttpResponse res = build(405, server);
    res.headers["Allow"] = buildAllowHeader(allowed);
    return res;
}

HttpResponse ErrorHandler::build(int code, const RuntimeServer* server)
{
    return build(code, getReasonPhrase(code) + "\n", server);
}

HttpResponse ErrorHandler::build(int code, const std::string& fallbackMsg, const RuntimeServer* server)
{
    HttpResponse res;
    res.status_code = code;
    res.reason_phrase = getReasonPhrase(code);

    std::string pagePath = resolvePathError(code, server);
    if (!pagePath.empty())
    {
        std::string content = readFile(pagePath);
        if (!content.empty())
        {
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