#pragma once
#include "./config/AST.hpp"
#include "RuntimeConfig.hpp"

class ConfigResolver {
    public:
        static RuntimeConfig resolve(const ConfigAST& ast);
    private:
        static RuntimeServer buildServer(const ServerNode& node);
        static RuntimeLocation buildLocation(const LocationNode& node, const RuntimeServer& parent);
        static void applyServerDirectives(RuntimeServer& server, const std::vector<Directive>& directives);
        static void setDefaults(RuntimeServer& server);
        static void ApplyLocationDirectives(const Directive& dir, RuntimeLocation& loc);
        static void ApplyInheritance(RuntimeLocation& loc, const RuntimeServer& parent);
        // Debug methods
        static void debugPrintServer(const RuntimeServer& server);
        static void debugPrintServerBasicInfo(const RuntimeServer& server);
        static void debugPrintLocation(const RuntimeLocation& loc, size_t index);
};
