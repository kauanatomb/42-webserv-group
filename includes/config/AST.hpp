#pragma once
#include <string>
#include <vector>

struct Directive {
    std::string name;
    std::vector<std::string> args;
};

struct LocationNode {
    std::string path;
    std::vector<Directive> directives;
};

struct ServerNode {
    std::vector<Directive> directives;
    std::vector<LocationNode> locations;
};

struct ConfigAST {
    std::vector<ServerNode> servers;
};


//why ifndef & define 