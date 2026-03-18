#include "resolver/HandlerResolver.hpp"
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


std::string HandlerResolver::extractHostname(const std::string& host_header) {
    size_t pos = host_header.find(':');
    return (pos != std::string::npos) ? host_header.substr(0, pos) : host_header;
}

static bool isPrefixMatch(const std::string& path, const std::string& locPath)
{
    if (path == locPath)
        return true;
    if (locPath == "/")
        return true;
    if (path.size() < locPath.size())
        return false;
    if (path.compare(0, locPath.size(), locPath) != 0)
        return false;
    return (path[locPath.size()] == '/');
}

static const RuntimeLocation* matchLocation (const std::string& path, const RuntimeServer& server)
{
    const std::vector<RuntimeLocation>& locs = server.getLocations();
    const RuntimeLocation* best = NULL;
    size_t bestLen = 0;

    for (size_t i = 0; i < locs.size(); i++)
    {
        const RuntimeLocation& loc = locs[i];
        const std::string& p = loc.getPath();

        if (isPrefixMatch(path, p) && p.size() >= bestLen)
        {
            best = &loc;
            bestLen = p.size();
        }
    }
    return best;
}

void HandlerResolver::fillRequestMeta(HttpRequest& req, const SocketKey& socket_key, int socket_fd) {
    req.serverPort = socket_key.port;
    struct sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);
    if (getpeername(socket_fd, (struct sockaddr*)&addr, &addrLen) == 0)
        req.clientIp = inet_ntoa(addr.sin_addr);
    else
        req.clientIp = "0.0.0.0";
}

static const RuntimeServer& findServer(const std::vector<RuntimeServer>& servers, const std::string& hostname) {
    for (size_t i = 0; i < servers.size(); ++i) {
        const std::vector<std::string>& names = servers[i].getServerNames();
        for (size_t j = 0; j < names.size(); ++j) {
            if (names[j] == hostname)
                return servers[i];
        }
    }
    return servers[0];
}

const RuntimeLocation* HandlerResolver::resolve(const RuntimeConfig& config, const SocketKey& socket_key, HttpRequest& req, int socket_fd) {
    const std::map<SocketKey, std::vector<RuntimeServer> >& servers = config.getServers();
    std::map<SocketKey, std::vector<RuntimeServer> >::const_iterator it = servers.find(socket_key);
    
    if (it == servers.end() || it->second.empty())
        return NULL;
    
    std::string hostname = extractHostname(req.getHeader("Host"));
    const RuntimeLocation* loc = matchLocation(req.path, findServer(it->second, hostname));
    resolveCgiPathInfo(req, loc);
    fillRequestMeta(req, socket_key, socket_fd);
    return loc;
}

// Splits req.path into SCRIPT_NAME + PATH_INFO when the location has CGI.
// Called once after location matching — the only place that mutates the request.
void HandlerResolver::resolveCgiPathInfo(HttpRequest& req, const RuntimeLocation* loc) {
    if (!loc || !loc->getHasCGI())
        return;
    const std::map<std::string, std::string>& cgi_exec = loc->getCGIExec();
    const std::string& locPath = loc->getPath();
    const std::string& root = loc->getRoot();

    // strip location prefix to get the suffix (ex. "/script.py/extra/path")
    std::string suffix;
    if (locPath == "/")
        suffix = req.path;
    else if (req.path.size() == locPath.size())
        suffix = "/";
    else
        suffix = req.path.substr(locPath.size());

    for (size_t i = 1; i <= suffix.size(); ++i) {
        if (i < suffix.size() && suffix[i] != '/')
            continue;
        std::string candidate = suffix.substr(0, i);
        size_t dot = candidate.rfind('.');
        if (dot == std::string::npos)
            continue;
        std::string ext = candidate.substr(dot);
        if (cgi_exec.find(ext) == cgi_exec.end())
            continue;
        // build filesystem path and check it exists
        std::string fsPath = root;
        if (!fsPath.empty() && fsPath[fsPath.size() - 1] != '/' && !candidate.empty() && candidate[0] != '/')
            fsPath += "/";
        fsPath += candidate;
        struct stat st;
        if (stat(fsPath.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
            req.pathInfo = (i < suffix.size()) ? suffix.substr(i) : "";
            if (locPath == "/")
                req.path = candidate;
            else
                req.path = locPath + candidate;
            return;
        }
    }
}

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

std::string HandlerResolver::joinPath(const std::string& a, const std::string& b)
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

std::string HandlerResolver::resolvePath(const HttpRequest& req, const RuntimeLocation* loc)
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