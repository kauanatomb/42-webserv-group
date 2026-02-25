#pragma once

#include "HttpResponse.hpp"
#include <string>
#include <map>

class ResponseBuilder {
    public:
        static HttpResponse ok(const std::string& body, const std::string& contentType);
        static HttpResponse redirect(int code, const std::string& location);
};
