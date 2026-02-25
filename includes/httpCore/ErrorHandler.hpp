#pragma once

#include "HttpResponse.hpp"
#include "resolver/RuntimeServer.hpp"
#include <string>

class ErrorHandler {
    public:
        static HttpResponse build(int code, const RuntimeServer* server);
        static HttpResponse build(int code, const std::string& fallbackMsg, const RuntimeServer* server);
        static HttpResponse build405(const std::set<HttpMethod>& allowed, const RuntimeServer* server);
        static std::string getReasonPhrase(int code);

    private:
        ErrorHandler();
        
        static std::string readFile(const std::string& path);
        static std::string resolvePathError(int code, const RuntimeServer* server);
};