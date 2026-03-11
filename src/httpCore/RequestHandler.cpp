#include "resolver/RuntimeServer.hpp"
#include "httpCore/RequestHandler.hpp"
#include "httpCore/UploadHandler.hpp"
#include "resolver/RuntimeLocation.hpp"
#include "httpCore/ErrorHandler.hpp"
#include "httpCore/ResponseBuilder.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <iostream>
#include <dirent.h>
#include <algorithm>
#include <sys/types.h>
#include <cstdio>

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

static bool hasTrailingSlash(const std::string& s) {
    return (!s.empty() && s[s.size() - 1] == '/');
}

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

static std::string stripLocationPrefix(const std::string& uri, const std::string& locPath)
{
    //uri starts with locPath, matchLocation() guarantee
    if (locPath == "/")
        return uri; // keep as is
    if (uri.size() == locPath.size())
        return "/"; //exact match
    return uri.substr(locPath.size()); // starts with /
}

// Resolves .. and . components without requiring the path to exist
static std::string normalizePath(const std::string& path)
{
    std::vector<std::string> components;
    bool isAbsolute = (!path.empty() && path[0] == '/');
    std::string segment;
    for (size_t i = (isAbsolute ? 1 : 0); i <= path.size(); ++i)
    {
        if (i == path.size() || path[i] == '/')
        {
            if (segment == "..") {
                if (!components.empty()) components.pop_back();
            } else if (segment != "." && !segment.empty()) {
                components.push_back(segment);
            }
            segment.clear();
        }
        else
            segment += path[i];
    }
    std::string result;
    if (isAbsolute) result = "/";
    for (size_t i = 0; i < components.size(); ++i)
    {
        if (i > 0) result += "/";
        result += components[i];
    }
    return result.empty() ? (isAbsolute ? "/" : ".") : result;
}

// Returns empty string if path traversal is detected
static std::string resolvePath(const HttpRequest& req, const RuntimeLocation* loc)
{
    std::string root = loc->getRoot();
    std::string suffix = stripLocationPrefix(req.path, loc->getPath());
    std::string raw = joinPath(root, suffix);

    // Get canonical absolute root (root must exist — it's from config)
    char buf[PATH_MAX];
    if (realpath(root.c_str(), buf) == NULL)
        return "";
    std::string absRoot(buf);

    // Make raw path absolute for normalization
    std::string absRaw = raw;
    if (raw.empty() || raw[0] != '/')
    {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) == NULL)
            return "";
        absRaw = std::string(cwd) + "/" + raw;
    }

    std::string normalized = normalizePath(absRaw);

    // Traversal check: normalized path must be inside absRoot
    if (normalized.size() < absRoot.size() ||
        normalized.compare(0, absRoot.size(), absRoot) != 0 ||
        (normalized.size() > absRoot.size() && normalized[absRoot.size()] != '/'))
        return "";

    return normalized;
}

static HttpResponse handleDELETE(const HttpRequest& req, const RuntimeLocation* loc)
{
    std::string target = resolvePath(req, loc);
    if (target.empty())
        return ErrorHandler::build(400, loc);

    struct stat st;
    if (stat(target.c_str(), &st) != 0)
        return ErrorHandler::build(404, loc);

    // do not delete directories
    if (S_ISDIR(st.st_mode))
        return ErrorHandler::build(403, "Cannot delete a directory\n", loc);

    //check permission write on file
    if (access(target.c_str(), W_OK) != 0)
        return ErrorHandler::build(403, loc);

    if (std::remove(target.c_str()) != 0)
        return ErrorHandler::build(500, "Failed to delete file\n", loc);
    
    HttpResponse res;
    res.status_code = 204;
    res.reason_phrase = "No Content";
    res.body = "";
    return res;
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

static std::string buildFileList(const std::string& dirPath, const std::string& uriBase,
                                 const std::string& excludeFile)
{
    DIR* d = opendir(dirPath.c_str());
    if (!d) return "";

    std::vector<std::string> entries;
    for (struct dirent* ent = readdir(d); ent; ent = readdir(d))
    {
        std::string name = ent->d_name;
        if (name != "." && name != ".." && name != excludeFile)
            entries.push_back(name);
    }
    closedir(d);
    std::sort(entries.begin(), entries.end());

    std::ostringstream out;
    out << "<ul class=\"autoindex\">";
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
            << htmlEscape(name) << "</a>"
            << " <button class=\"delete-btn\" data-path=\"" << htmlEscape(href)
            << "\">&#x2716;</button></li>";
    }
    out << "</ul>";
    return out.str();
}

static std::string readFileToString(const std::string& path)
{
    std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
    if (!in.is_open()) return "";
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static HttpResponse buildAutoindex(const std::string& dirPath, const std::string& uriBase,
                                    const RuntimeLocation* loc, const std::string& indexPath)
{
    // extract the index filename to exclude it from the listing
    std::string excludeFile;
    if (!indexPath.empty())
    {
        size_t slash = indexPath.rfind('/');
        excludeFile = (slash != std::string::npos) ? indexPath.substr(slash + 1) : indexPath;
    }

    std::string fileList = buildFileList(dirPath, uriBase, excludeFile);
    if (fileList.empty())
        return ErrorHandler::build(403, loc);

    // if an index template with <!-- AUTOINDEX --> exists, inject the list there
    if (!indexPath.empty())
    {
        std::string tpl = readFileToString(indexPath);
        std::string marker = "<!-- AUTOINDEX -->";
        size_t pos = tpl.find(marker);
        if (pos != std::string::npos)
        {
            std::string page = tpl.substr(0, pos) + fileList + tpl.substr(pos + marker.size());
            return ResponseBuilder::ok(page, ".html");
        }
    }

    // fallback: plain generated page
    std::ostringstream out;
    out << "<!doctype html><html><head><meta charset=\"utf-8\">"
        << "<title>Index of " << htmlEscape(uriBase) << "</title></head><body>"
        << "<h1>Index of " << htmlEscape(uriBase) << "</h1>"
        << fileList
        << "</body></html>\n";
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
    std::string uriBase = req.path;
    if (!hasTrailingSlash(uriBase)) uriBase += "/";

    // find index file candidate
    std::string indexPath;
    const std::vector<std::string>& idx = loc->getIndex();
    if (!idx.empty())
    {
        struct stat st;
        std::string candidate = joinPath(dirPath, idx[0]);
        if (stat(candidate.c_str(), &st) == 0 && S_ISREG(st.st_mode))
            indexPath = candidate;
    }

    // if autoindex is on, generate listing (using template if available)
    if (loc && loc->getAutoindex())
        return buildAutoindex(dirPath, uriBase, loc, indexPath);

    // autoindex off: serve static index if it exists
    if (!indexPath.empty())
        return serveRegularFile(indexPath, loc);

    return ErrorHandler::build(403, "Directory listing denied\n", loc);
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
        std::string basePath = resolvePath(req, loc);
        if (basePath.empty())
            return ErrorHandler::build(400, loc);
        return serveFileGET(basePath, req, loc);
    }
    
    if (req.method == "POST") return UploadHandler::handle(req, loc);

    if (req.method == "DELETE") return handleDELETE(req, loc);

    return ErrorHandler::build(501, "Fallback\n", loc);
}
