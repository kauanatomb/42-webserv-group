#pragma once
#include <map>
#include <vector>
#include <string>
#include "RuntimeServer.hpp"

struct RuntimeConfig {
    std::map<SocketKey, std::vector<RuntimeServer> > servers;
};
