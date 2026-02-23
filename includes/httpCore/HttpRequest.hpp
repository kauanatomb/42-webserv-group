#pragma once

class HttpRequest 
{
    public:
        std::string method;
        std::string uri;
        std::string version;
        std::map<std::string, std::string> headers;
        std::string body;
};
