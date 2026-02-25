#pragma once

#include "HttpResponse.hpp"
#include "resolver/RuntimeServer.hpp"
#include <string>

class ErrorHandler {
    public:
        static HttpResponse build(int code, const RuntimeLocation* loc);
        static HttpResponse build(int code, const std::string& fallbackMsg, const RuntimeLocation* loc);
        static HttpResponse build405(const std::set<HttpMethod>& allowed, const RuntimeLocation* loc);
        static std::string getReasonPhrase(int code);

    private:
        ErrorHandler();
        
        static std::string readFile(const std::string& path, bool& success);
        static std::string resolvePathError(int code, const RuntimeLocation* loc);
};