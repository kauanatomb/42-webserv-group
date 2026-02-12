#pragma once

#include "AST.hpp"
#include <map>
#include <cstddef>

struct DirectiveRule {
    bool allowed_in_server;
    bool allowed_in_location;
    size_t min_args;
    size_t max_args; // use SIZE_MAX for "no limit"
};

typedef std::map<std::string, DirectiveRule> RuleMap;
typedef void (*ArgValidator)(const Directive&);

class ConfigValidator {
    public:
        ConfigValidator();
        void validate(const ConfigAST& ast);

    private:
        RuleMap _rules;
        std::map<std::string, ArgValidator> _argValidators;

        void validateServer(const ServerNode& server);
        void validateLocation(const LocationNode& location);

        void validateDirective(const Directive& d, bool in_server, bool in_location);
        void checkCardinality(const std::vector<Directive>& directives);
        void checkConcurrence(const std::vector<Directive>& directives);
        void validateContextualRules(const std::vector<Directive>& directives);
};
