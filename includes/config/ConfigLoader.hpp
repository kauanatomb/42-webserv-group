#pragma once

#include <string>
#include "AST.hpp"

class ConfigLoader {
    public:
        static ConfigAST load(const std::string& path);
};
