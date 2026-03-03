#include "httpCore/HttpRequest.hpp"
#include <iostream>

std::string HttpRequest::getHeader(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = headers.find(key);
    if (it != headers.end())
        return it->second;
    return "";
}

void HttpRequest::print() const {
    std::cout << "========== HTTP REQUEST ==========" << std::endl;
    std::cout << "Method:  " << method << std::endl;
    std::cout << "URI:     " << uri << std::endl;
    std::cout << "Version: " << version << std::endl;
    std::cout << "---------- Headers ----------" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
            it != headers.end(); ++it) {
        std::cout << it->first << ": " << it->second << std::endl;
    }
    if (!body.empty()) {
        std::cout << "---------- Body ----------" << std::endl;
        std::cout << body << std::endl;
    }
    std::cout << "==================================" << std::endl;
}