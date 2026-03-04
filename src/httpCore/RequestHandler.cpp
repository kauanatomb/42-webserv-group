#include "resolver/RuntimeServer.hpp"
#include "httpCore/RequestHandler.hpp"
#include "resolver/RuntimeLocation.hpp"
#include "httpCore/ErrorHandler.hpp"
#include "httpCore/ResponseBuilder.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <dirent.h>
#include <algorithm>

static bool methodFromString (const std::string& s, HttpMethod& out)
{
    if (s == "GET") { out = GET; return true; }
    if (s == "POST") {out = POST; return true; }
    if (s == "DELETE") {out = DELETE; return true; }
    return false;
}

static int checkMethod(const HttpRequest& req, const RuntimeLocation* loc)
{
    HttpMethod m;
    if (!methodFromString(req.method, m))
        return 501;

    if (loc)
    {
        const std::set<HttpMethod>& allowed = loc->getAllowedMethods();
        if (!allowed.empty() && allowed.find(m) == allowed.end())
            return 405;
    }
    return 0;
}

static bool isDirectory(const std::string& path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return false;
    return S_ISDIR(st.st_mode);
}

static std::string joinPath(const std::string& a, const std::string& b)
{
    if (a.empty()) return b;
    if (b.empty()) return a;
    if (a[a.size() - 1] == '/' && b[0] == '/')
        return a + b.substr(1);
    if (a[a.size() - 1] != '/' && b[0] != '/')
        return a + "/" + b;
    return a + b;
}

static std::string stripLocationPrefix(const std::string& uri, const std::string& locPath)
{
    //uri starts with locPath, matchLocation() guarantee
    if (locPath == "/")
        return uri; // keep as is
    if (uri.size() == locPath.size())
        return "/"; //exact match
    return uri.substr(locPath.size()); // starts with /
}

static bool hasTrailingSlash(const std::string& s) {
    return (!s.empty() && s[s.size() - 1] == '/');
}

static std::string resolvePath(const HttpRequest& req, const RuntimeLocation* loc) {
    std::string root = loc->getRoot();
    std::string suffix = stripLocationPrefix(req.path, loc->getPath());
    return joinPath(root, suffix);
}

static std::string joinUri(const std::string& base, const std::string& name)
{
    if (base.empty()) return name;
    if (base[base.size() - 1] == '/')
        return base + name;
    return base + "/" + name;
}

static std::string htmlEscape(const std::string& s)
{
    std::string out;
    size_t i = 0;
    while (i < s.size())
    {
        char c = s[i];
        if (c == '&') out += "&amp;";
        else if (c == '<') out += "&lt;";
        else if (c == '>') out += "&gt;";
        else if (c == '"') out += "&quot;";
        else if (c == '\'') out += "&#39;";
        else out += c;
        ++i;
    }
    return out;
}

static HttpResponse buildAutoindex (const std::string& dirPath, const std::string& uriBase, const RuntimeLocation* loc)
{
    DIR* d = opendir(dirPath.c_str());
    if (!d)
        return ErrorHandler::build(403, loc);

    std::vector<std::string> entries;
    for (struct dirent* ent = readdir(d); ent; ent = readdir(d))
    {
        std::string name = ent->d_name;
        if (name != "." && name != "..")
            entries.push_back(name);
    }
    closedir(d);
    std::sort(entries.begin(), entries.end());

    std::ostringstream out;
    out << "<!doctype html><html><head><meta charset=\"utf-8\">"
        << "<title>Index of " << htmlEscape(uriBase) << "</title></head><body>"
        << "<h1>Index of " << htmlEscape(uriBase) << "</h1><ul>";

    for (size_t i = 0; i < entries.size(); ++i)
    {
        std::string name = entries[i];
        std::string href = joinUri(uriBase, name);
        std::string fullEntryPath = joinPath(dirPath, name);
        if (isDirectory(fullEntryPath))
        {
            if (href[href.size() - 1] != '/') href += "/";
            name += "/";
        }
        out << "<li><a href=\"" << htmlEscape(href) << "\">"
            << htmlEscape(name) << "</a></li>";
    }
    
    out << "</ul></body></html>\n";

    //serve as HTML
    return ResponseBuilder::ok(out.str(), ".html");
}

static HttpResponse serveRegularFile(const std::string& filePath, const RuntimeLocation* loc)
{
    if (access(filePath.c_str(), R_OK) != 0)
        return ErrorHandler::build(403, loc);

    std::ifstream in(filePath.c_str(), std::ios::in | std::ios::binary);
    if (!in.is_open())
        return ErrorHandler::build(500, loc);

    std::ostringstream ss;
    ss << in.rdbuf();
    return ResponseBuilder::ok(ss.str(), filePath);
}

static HttpResponse serveDirectory(const std::string& dirPath, const HttpRequest& req, const RuntimeLocation* loc)
{
    // try index file first
    const std::vector<std::string>& idx = loc->getIndex();
    if (!idx.empty())
    {
        struct stat st;
        std::string candidate = joinPath(dirPath, idx[0]);
        if (stat(candidate.c_str(), &st) == 0 && S_ISREG(st.st_mode))
            return serveRegularFile(candidate, loc);
    }
    // fallback to autoindex
    if (!loc || !loc->getAutoindex())
        return ErrorHandler::build(403, "Directory listing denied\n", loc);

    std::string uriBase = req.path;
    if (!hasTrailingSlash(uriBase)) uriBase += "/";
    return buildAutoindex(dirPath, uriBase, loc);
}

static HttpResponse serveFileGET(const std::string& basePath, const HttpRequest& req, const RuntimeLocation* loc)
{
    struct stat st;
    if (stat(basePath.c_str(), &st) != 0)
        return ErrorHandler::build(404, loc);

    if (S_ISDIR(st.st_mode))
        return serveDirectory(basePath, req, loc);

    return serveRegularFile(basePath, loc);
}

HttpResponse RequestHandler::handle(const HttpRequest& req, const RuntimeLocation* loc) {
    //1-redirection handling (return directive) 
    if (loc->getHasReturn()) {
        const ReturnRule& r = loc->getRedirect();
        if (r.target.empty())
            return ErrorHandler::build(500, loc);
        return ResponseBuilder::redirect(r.status_code, r.target);
    }
    // 2-method check 405/501
    int methodErr = checkMethod(req, loc);
    if (methodErr == 501)
        return ErrorHandler::build(501, loc);
    if (methodErr == 405)
    return ErrorHandler::build405(loc->getAllowedMethods(), loc);

    // 3- resolve filesystem path
    if (req.method == "GET")
    {   
        //build base path (no index)
        std::string basePath = resolvePath(req, loc);
        return serveFileGET(basePath, req, loc);
    }
    
    // TEMP until post/delete:
    if (req.method == "POST" || req.method == "DELETE")
        return ErrorHandler::build(501, loc);

    return ErrorHandler::build(501, "Fallback\n", loc);
}
