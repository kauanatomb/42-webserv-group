#pragma once
#include <stdexcept>

class ConfigError : public std::runtime_error {
    public:
        explicit ConfigError(const std::string& msg)
            : std::runtime_error(msg) {}
};

class SyntaxError : public ConfigError {
    public:
        explicit SyntaxError(const std::string& msg)
            : ConfigError(msg) {}
};

class ValidationError : public ConfigError {
    public:
        explicit ValidationError(const std::string& msg)
            : ConfigError(msg) {}
};
