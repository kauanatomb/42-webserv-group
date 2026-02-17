#pragma once
#include <string>
#include <vector>
#include "RuntimeStruct.hpp"

class SocketKeyUtils {
    public:
        static SocketKey fromString(const std::string& addr_port);
        static std::string toString(const SocketKey& sk);
        static bool exists(const std::vector<SocketKey>& listens, const SocketKey& key);
        static bool equals(const SocketKey& a, const SocketKey& b);
        
    private:
        static void extractIPOctets(uint32_t ip, unsigned char& a, unsigned char& b, 
                                unsigned char& c, unsigned char& d);
        static std::string formatIP(uint32_t ip);
};
