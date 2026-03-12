#include "resolver/HandlerResolver.hpp"
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>

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
