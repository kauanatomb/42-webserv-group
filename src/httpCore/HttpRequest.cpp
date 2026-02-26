#include "httpCore/HttpRequest.hpp"

std::string HttpRequest::getHeader(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = headers.find(key);
    if (it != headers.end())
        return it->second;
    return "";
}