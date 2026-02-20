#include "resolver/ServerResolver.hpp"

std::string ServerResolver::extractHostname(const std::string& host_header) {
    size_t pos = host_header.find(':');
    return (pos != std::string::npos) ? host_header.substr(0, pos) : host_header;
}

const RuntimeServer* ServerResolver::resolve(const RuntimeConfig& config, const SocketKey& socket_key,
    const std::string& host_header
) {
    const std::map<SocketKey, std::vector<RuntimeServer> >& servers = config.getServers();
    std::map<SocketKey, std::vector<RuntimeServer> >::const_iterator it = servers.find(socket_key);
    
    if (it == servers.end() || it->second.empty())
        return NULL;
    
    std::string hostname = extractHostname(host_header);
    const std::vector<RuntimeServer>& server_list = it->second;
    
    for (size_t i = 0; i < server_list.size(); ++i) {
        const std::vector<std::string>& names = server_list[i].getServerNames();
        for (size_t j = 0; j < names.size(); ++j) {
            if (names[j] == hostname)
                return &server_list[i];
        }
    }
    
    return &server_list[0];
}
