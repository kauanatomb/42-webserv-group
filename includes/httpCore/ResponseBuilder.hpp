#pragma once

#include "HttpResponse.hpp"
#include <string>
#include <map>

class ResponseBuilder {
    public:
        static HttpResponse ok(const std::string& body, const std::string& path);
    //missmatch with the implementation, so i'm sticking to the prototype of the function implemented
    //    static HttpResponse ok(const std::string& body, const std::string& contentType);
        static HttpResponse redirect(int code, const std::string& location);
};
