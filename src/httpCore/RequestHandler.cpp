#include "resolver/RuntimeServer.hpp"
#include "httpCore/RequestHandler.hpp"
#include "httpCore/HttpResponse.hpp"
#include "resolver/RuntimeLocation.hpp"
#include "httpCore/ErrorHandler.hpp"
#include "httpCore/ResponseBuilder.hpp"
//includes for file existence/type and reading
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <dirent.h>

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
    //1- get uri path (basic)
    std::string uri = req.uri;
    std::string::size_type q = uri.find('?');
    if (q != std::string::npos)
        uri = uri.substr(0, q);

    //2- take suffix after location prefix
    std::string suffix = stripLocationPrefix(uri, loc->getPath()); //begins with /

    //3- filesystem path = root + suffix
    std::string full = joinPath(root, suffix);

    //4- if uri ends with / or points to a directory -> append index[0] if exists
    const std::vector<std::string>& idx = loc->getIndex(); 
    if ((hasTrailingSlash(uri) || isDirectory(full)) && !idx.empty())
        full = joinPath(full, idx[0]);

    return full;
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


static HttpResponse buildAutoindex (const std::string& dirPath,
                                    const std::string& uriBase,
                                    const RuntimeLocation* loc)
{
    DIR* d = opendir(dirPath.c_str());
    if (!d)
        return ErrorHandler::build(403, loc);
    
    std::ostringstream out;
    out << "<!doctype html><html><head><meta charset=\"utf-8\">"
        << "<title>Index of " << htmlEscape(uriBase) << "</title></head><body>"
        << "<h1>Index of " << htmlEscape(uriBase) << "</h1><ul>";
    
    for (struct dirent* ent = readdir(d); ent; ent = readdir(d))
    {
        std::string name = ent->d_name;

        if (name == "." || name == "..")
            continue;
        std::string href = joinUri(uriBase, name);
        //add trainling slash for dir if poss
        std::string fullEntryPath = joinPath(dirPath, name);
        if (isDirectory(fullEntryPath))
        {
            if (href[href.size() - 1] != '/') href += "/";
            name += "/";
        }
        out << "<li><a href=\"" << htmlEscape(href) << "\">"
            << htmlEscape(name) << "</a></li>";
    }
    closedir(d);
    
    out << "</ul></body></html>\n";

    //serve as HTML
    return ResponseBuilder::ok(out.str(), ".html");
}

static HttpResponse serveFileGET(const std::string& finalPath, 
                                 const HttpRequest& req, 
                                 const RuntimeLocation* loc,
                                 const std::string& basePath)
{
    struct stat st;
    //if finalPath doesn't exist -> autoindex fallback

    if (stat(finalPath.c_str(), &st) != 0)
    {
        if (errno == ENOENT && loc && loc->getAutoindex() && isDirectory(basePath))
        {
            std::string uriBase = req.uri;
            if (!hasTrailingSlash(uriBase)) uriBase += "/";
            return (buildAutoindex(basePath, uriBase, loc));
        }
        return ErrorHandler::build(404, loc);
    }
    // if finalPath is a dirctory: autoindex or deny:
    if (S_ISDIR(st.st_mode))
    {
        if (!loc || !loc->getAutoindex())
            return ErrorHandler::build(403, "Directory listing denied\n", loc);
        
        //using request URI base to have correct links:
        std::string uriBase = req.uri;
        if (!hasTrailingSlash(uriBase)) uriBase += "/";
        return buildAutoindex(finalPath, uriBase, loc);
    }

    if (access(finalPath.c_str(), R_OK) != 0)
        return ErrorHandler::build(403, loc);

    std::ifstream in(finalPath.c_str(), std::ios::in | std::ios::binary);
    if (!in.is_open())
        return ErrorHandler::build(500, loc);

    std::ostringstream ss;
    ss << in.rdbuf();

    return ResponseBuilder::ok(ss.str(), finalPath);
}






HttpResponse RequestHandler::handle(const HttpRequest& req, const RuntimeLocation* loc) {
    //1-redirection handling (return directive) 
    //need NULL check?
    if (!loc)
        return ErrorHandler::build(500, "No location resolved\n", loc);

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
    if (req.method == "GET") // I believe Fran will create get functions for request than we can change it
    {
        // std::string path = resolvePath(req, loc);
        // return serveFileGET(path, req, loc);
        
        //build base path (no index)
        std::string uri = req.uri;
        std::string::size_type q = uri.find('?');
        if (q != std::string::npos)
            uri = uri.substr(0, q);

        std::string suffix = stripLocationPrefix(uri, loc->getPath());
        std::string basePath = joinPath(loc->getRoot(), suffix);

        //final path (adding index if needed)
        std::string finalPath = resolvePath(req, loc);

        return serveFileGET(finalPath, req, loc, basePath);
    }
    
    // TEMP until post/delete:

    if (req.method == "POST" || req.method == "DELETE")
        return ErrorHandler::build(501, loc);

    return ErrorHandler::build(501, "Fallback\n", loc);
}
