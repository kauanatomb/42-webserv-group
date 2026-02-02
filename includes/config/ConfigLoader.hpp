#pragma once

#include <string>
#include "AST.hpp"

class ConfigLoader {
    public:
        static void load(const std::string& path);
};
