#pragma once
#include "RuntimeConfig.hpp"
#include "RuntimeServer.hpp"
#include "httpCore/HttpRequest.hpp"

class HandlerResolver {
    public:
        static const RuntimeLocation* resolve(
            const RuntimeConfig& config,
            const SocketKey& socket_key,
            HttpRequest& req,
            int socket_fd
        );
    
    private:
        static std::string extractHostname(const std::string& host_header);
        static void resolveCgiPathInfo(HttpRequest& req, const RuntimeLocation* loc);
        static void fillRequestMeta(HttpRequest& req, const SocketKey& socket_key, int socket_fd);
};
