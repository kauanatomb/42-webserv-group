#pragma once
#include "RuntimeConfig.hpp"
#include "RuntimeServer.hpp"

class ServerResolver {
    public:
        static const RuntimeServer* resolve(
            const RuntimeConfig& config,
            const SocketKey& socket_key,
            const std::string& host_header
        );
    
    private:
        static std::string extractHostname(const std::string& host_header);
};
