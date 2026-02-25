#include "resolver/HandlerResolver.hpp"

std::string HandlerResolver::extractHostname(const std::string& host_header) {
    size_t pos = host_header.find(':');
    return (pos != std::string::npos) ? host_header.substr(0, pos) : host_header;
}

static bool isPrefixMatch(const std::string& uri, const std::string& locPath)
{
    if (uri == locPath)
        return true;
    if (locPath == "/")
        return true;
    if (uri.size() < locPath.size())
        return false;
    if (uri.compare(0, locPath.size(), locPath) != 0)
        return false;
    return (uri[locPath.size()] == '/');
}

static const RuntimeLocation* matchLocation (const std::string& uri, const RuntimeServer& server)
{
    const std::vector<RuntimeLocation>& locs = server.getLocations();
    const RuntimeLocation* best = NULL;
    size_t bestLen = 0;

    for (size_t i = 0; i < locs.size(); i++)
    {
        const RuntimeLocation& loc = locs[i];
        const std::string& p = loc.getPath();

        if (isPrefixMatch(uri, p) && p.size() >= bestLen)
        {
            best = &loc;
            bestLen = p.size();
        }
    }
    return best;
}

const RuntimeLocation* HandlerResolver::resolve(const RuntimeConfig& config, const SocketKey& socket_key, const HttpRequest& req) {
    const std::map<SocketKey, std::vector<RuntimeServer> >& servers = config.getServers();
    std::map<SocketKey, std::vector<RuntimeServer> >::const_iterator it = servers.find(socket_key);
    
    if (it == servers.end() || it->second.empty())
        return NULL;
    
    std::string hostname = extractHostname(req.getHeader("Host"));
    const std::vector<RuntimeServer>& server_list = it->second;
    
    for (size_t i = 0; i < server_list.size(); ++i) {
        const std::vector<std::string>& names = server_list[i].getServerNames();
        for (size_t j = 0; j < names.size(); ++j) {
            if (names[j] == hostname)
                return matchLocation(req.uri, server_list[i]);
        }
    }
    return matchLocation(req.uri, server_list[0]);
}
