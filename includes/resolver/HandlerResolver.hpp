#pragma once
#include "RuntimeConfig.hpp"
#include "RuntimeServer.hpp"
#include "httpCore/HttpRequest.hpp"

class HandlerResolver {
    public:
        static const RuntimeLocation* resolve(
            const RuntimeConfig& config,
            const SocketKey& socket_key,
            const HttpRequest& req
        );
    
    private:
        static std::string extractHostname(const std::string& host_header);
};
