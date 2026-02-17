#pragma once
#include <string>
#include <stdint.h>

enum HttpMethod {
    GET,
    POST,
    DELETE
};

struct ReturnRule {
    int status_code;
    std::string target;
};

struct SocketKey {
    uint32_t ip;
    uint16_t port;
    
    bool operator<(const SocketKey& other) const {
        if (ip != other.ip) return ip < other.ip;
        return port < other.port;
    }
};
