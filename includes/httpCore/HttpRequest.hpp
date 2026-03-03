#pragma once

#include <string>
#include <map>

class HttpRequest 
{
    public:
        std::string method; // GET POST DELETE
        std::string uri;    // /images/cat.png
        std::string version; // Http/1.1
        std::map<std::string, std::string> headers;
        std::string body;

        std::string getHeader(const std::string& key) const;
        void print() const;
};
