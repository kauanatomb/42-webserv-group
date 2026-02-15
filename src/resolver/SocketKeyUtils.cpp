#include "resolver/SocketKeyUtils.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstdio>

SocketKey SocketKeyUtils::fromString(const std::string& addr_port) {
    SocketKey socket;
    size_t colon_pos = addr_port.rfind(':');
    
    std::string ip_str;
    std::string port_str;
    
    if (colon_pos == std::string::npos) {
        ip_str = "0.0.0.0";
        port_str = addr_port;
    } else {
        ip_str = addr_port.substr(0, colon_pos);
        port_str = addr_port.substr(colon_pos + 1);
    }
    
    // Convert IP (xxx.xxx.xxx.xxx) to uint32_t
    unsigned int a, b, c, d_ip;
    std::sscanf(ip_str.c_str(), "%u.%u.%u.%u", &a, &b, &c, &d_ip);
    socket.ip = (a << 24) | (b << 16) | (c << 8) | d_ip;
    // Convert port to uint16_t
    socket.port = static_cast<uint16_t>(std::strtoul(port_str.c_str(), 0, 10));
    
    return socket;
}

void SocketKeyUtils::extractIPOctets(uint32_t ip, unsigned char& a, unsigned char& b,
                                      unsigned char& c, unsigned char& d) {
    a = (ip >> 24) & 0xFF;
    b = (ip >> 16) & 0xFF;
    c = (ip >> 8) & 0xFF;
    d = ip & 0xFF;
}

std::string SocketKeyUtils::formatIP(uint32_t ip) {
    unsigned char a, b, c, d;
    extractIPOctets(ip, a, b, c, d);
    
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%u.%u.%u.%u",
                  static_cast<int>(a), static_cast<int>(b),
                  static_cast<int>(c), static_cast<int>(d));
    return std::string(buffer);
}

std::string SocketKeyUtils::toString(const SocketKey& sk) {
    std::string result = formatIP(sk.ip);
    result += ":";
    
    char port_buf[6];
    std::snprintf(port_buf, sizeof(port_buf), "%u", sk.port);
    result += std::string(port_buf);
    
    return result;
}

bool SocketKeyUtils::exists(const std::vector<SocketKey>& listens, const SocketKey& key) {
    for (std::vector<SocketKey>::const_iterator it = listens.begin(); it != listens.end(); ++it) {
        if (equals(*it, key))
            return true;
    }
    return false;
}

bool SocketKeyUtils::equals(const SocketKey& a, const SocketKey& b) {
    return a.ip == b.ip && a.port == b.port;
}
