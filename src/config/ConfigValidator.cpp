#include "config/ConfigValidator.hpp"
#include "config/ConfigErrors.hpp"
#include <limits>
#include <set>
#include <cctype>
#include <cstdlib>

static DirectiveRule makeRule(bool s, bool l, size_t min, size_t max) {
    DirectiveRule r;
    r.allowed_in_server = s;
    r.allowed_in_location = l;
    r.min_args = min;
    r.max_args = max;
    return r;
}

static void validateListenStatic(const Directive& d) {
    if (d.args.empty())
        throw ValidationError("listen requires interface:port format");
    
    std::string addr_port = d.args[0];
    size_t colon_pos = addr_port.rfind(':');
    
    if (colon_pos == std::string::npos)
        throw ValidationError("listen: format must be interface:port");
    
    std::string port_str = addr_port.substr(colon_pos + 1);
    if (port_str.empty())
        throw ValidationError("listen: port is empty");
    
    for (size_t i = 0; i < port_str.size(); ++i) {
        if (!std::isdigit(port_str[i]))
            throw ValidationError("listen: invalid port format");
    }
    
    int port = std::atoi(port_str.c_str());
    if (port < 1 || port > 65535)
        throw ValidationError("listen: port out of range (1-65535)");
}

static void validateClientMaxBodyStatic(const Directive& d) {
    if (d.args.size() != 1)
        throw ValidationError("client_max_body requires exactly one argument");
    
    std::string size_str = d.args[0];
    if (size_str.empty())
        throw ValidationError("client_max_body: size is empty");
    
    size_t i = 0;
    for (; i < size_str.size(); ++i) {
        if (!std::isdigit(size_str[i]))
            break;
    }
    
    if (i == 0)
        throw ValidationError("client_max_body: invalid format");
    
    if (i < size_str.size()) {
        std::string suffix = size_str.substr(i);
        if (suffix != "K" && suffix != "M" && suffix != "G" && suffix != "KB" && suffix != "MB" && suffix != "GB")
            throw ValidationError("client_max_body: invalid suffix (use K, M, G, KB, MB, GB");
    }
}

static void validateErrorPageStatic(const Directive& d) {
    if (d.args.size() < 2)
        throw ValidationError("error_page requires at least code and path");
    
    for (size_t i = 0; i < d.args.size() - 1; ++i) {
        const std::string& code = d.args[i];
        if (code.size() != 3 || !std::isdigit(code[0]) || !std::isdigit(code[1]) || !std::isdigit(code[2]))
            throw ValidationError("error_page: invalid HTTP error code: " + code);
        
        int error_code = std::atoi(code.c_str());
        if (error_code < 400 || error_code > 599)
            throw ValidationError("error_page: error code must be between 400-599");
    }
}

static void validateCgiExecStatic(const Directive& d) {
    if (d.args.size() < 2)
        throw ValidationError("cgi_exec requires extension and path");
    
    const std::string& ext = d.args[0];
    if (ext.empty() || ext[0] != '.')
        throw ValidationError("cgi_exec: extension must start with dot (.)");
}

static void validateReturnStatic(const Directive& d) {
    if (d.args.size() < 1 || d.args.size() > 2)
        throw ValidationError("return requires code and optional text");
    
    const std::string& code = d.args[0];
    if (code.size() != 3 || !std::isdigit(code[0]) || !std::isdigit(code[1]) || !std::isdigit(code[2]))
        throw ValidationError("return: invalid HTTP code: " + code);
    
    int http_code = std::atoi(code.c_str());
    if (http_code < 100 || http_code > 599)
        throw ValidationError("return: HTTP code must be between 100-599");
}

static void validateAllowMethodsStatic(const Directive& d) {
    static std::set<std::string> allowed;
    if (allowed.empty()) {
        allowed.insert("GET");
        allowed.insert("POST");
        allowed.insert("DELETE");
    }

    for (size_t i = 0; i < d.args.size(); ++i) {
        if (allowed.count(d.args[i]) == 0)
            throw ValidationError("invalid HTTP method: " + d.args[i]);
    }
}

static void validateOnOffStatic(const Directive& d) {
    static std::set<std::string> allowed;
    if (allowed.empty()) {
        allowed.insert("on");
        allowed.insert("off");
    }

    for (size_t i = 0; i < d.args.size(); ++i) {
        if (allowed.count(d.args[i]) == 0)
            throw ValidationError("invalid enable or disable arg for: " + d.args[i]);
    }
}

ConfigValidator::ConfigValidator() {
    _rules["listen"] = makeRule(true, false, 1, 2);
    _rules["server_name"] = makeRule(true, false, 1, SIZE_MAX);
    _rules["root"] = makeRule(true, true, 1, 1);
    _rules["index"] = makeRule(true, true, 1, SIZE_MAX);
    _rules["client_max_body_size"] = makeRule(true, false, 1, 1);
    _rules["return"] = makeRule(true, true, 1, 2);
    _rules["error_page"] = makeRule(true, true, 2, SIZE_MAX);
    _rules["allow_methods"] = makeRule(false, true, 1, SIZE_MAX);
    _rules["autoindex"] = makeRule(true, true, 1, 1);
    _rules["upload"] = makeRule(false, true, 1, 1);
    _rules["upload_store"] = makeRule(false, true, 1, 1);
    _rules["cgi"] = makeRule(false, true, 1, 1);
    _rules["cgi_exec"] = makeRule(false, true, 2, 2);

    _argValidators["allow_methods"] = &validateAllowMethodsStatic;
    _argValidators["autoindex"] = &validateOnOffStatic;
    _argValidators["cgi"] = &validateOnOffStatic;
    _argValidators["upload"] = &validateOnOffStatic;
    _argValidators["cgi_exec"] = &validateCgiExecStatic;
    _argValidators["return"] = &validateReturnStatic;
    _argValidators["listen"] = &validateListenStatic;
    _argValidators["client_max_body_size"] = &validateClientMaxBodyStatic;
    _argValidators["error_page"] = &validateErrorPageStatic;
}

void ConfigValidator::validate(const ConfigAST& ast) {
    if (ast.servers.empty())
        throw ValidationError("no server defined");
    
    for(size_t i = 0; i < ast.servers.size(); i++)
        validateServer(ast.servers[i]);
}

void ConfigValidator::validateServer(const ServerNode& server) {
    bool has_listen = false;

    for(size_t i = 0; i < server.directives.size(); i++) {
        validateDirective(server.directives[i], true, false);
        if (server.directives[i].name == "listen")
            has_listen = true;
    }
    if (!has_listen)
        throw ValidationError("server without listen");
    checkCardinality(server.directives);
    checkConcurrence(server.directives);
    validateContextualRules(server.directives);
    for(size_t i = 0; i < server.locations.size(); i++)
        validateLocation(server.locations[i]);
}

void ConfigValidator::validateDirective(const Directive& d, bool in_server, bool in_location) {
    RuleMap::iterator it = _rules.find(d.name);
    if (it == _rules.end())
        throw ValidationError("unknown directive: " + d.name);
        
    const DirectiveRule& rule = it->second;
    if (in_server && !rule.allowed_in_server)
        throw ValidationError(d.name + " not allowed in server");
    if (in_location && !rule.allowed_in_location)
        throw ValidationError(d.name + " not allowed in location");
    if (d.args.size() < rule.min_args || d.args.size() > rule.max_args)
        throw ValidationError("invalid args count for " + d.name);
    
    std::map<std::string, ArgValidator>::iterator validator_it = _argValidators.find(d.name);
    if (validator_it != _argValidators.end())
        validator_it->second(d);
}

void ConfigValidator::checkCardinality(const std::vector<Directive>& directives) {
    std::map<std::string, size_t> count;

    for(size_t i = 0; i < directives.size(); i++)
        count[directives[i].name]++;

    if (count["root"] > 1)
        throw ValidationError("root duplicated");
    if (count["index"] > 1)
        throw ValidationError("index duplicated");
    if (count["client_max_body_size"] > 1)
        throw ValidationError("client_max_body_size duplicated");
}

void ConfigValidator::checkConcurrence(const std::vector<Directive>& directives) {
    bool has_root, has_index, has_return = false;

    for(size_t i = 0; i < directives.size(); i++) {
        if (directives[i].name == "root") has_root = true;
        if (directives[i].name == "index") has_index = true;
        if (directives[i].name == "return") has_return = true;
    }

    if (has_return && (has_root || has_index))
        throw ValidationError("return cannot coexist with index/root");
}

void ConfigValidator::validateLocation(const LocationNode& location) {
    if (location.path.empty())
        throw ValidationError("location without path");
    for(size_t i = 0; i < location.directives.size(); i++)
        validateDirective(location.directives[i], false, true);
    checkCardinality(location.directives);
    checkConcurrence(location.directives);
    validateContextualRules(location.directives);
}

void ConfigValidator::validateContextualRules(const std::vector<Directive>& directives) {
    bool has_cgi_on = false;
    bool has_cgi_exec = false;
    bool has_upload_on = false;
    bool has_upload_store = false;
    bool has_cgi_path = false;

    for(size_t i = 0; i < directives.size(); i++) {
        if (directives[i].name == "cgi")
            has_cgi_on = (directives[i].args[0] == "on");
        if (directives[i].name == "cgi_exec")
            has_cgi_exec = true;
        if (directives[i].name == "upload")
            has_upload_on = (directives[i].args[0] == "on");
        if (directives[i].name == "upload_store")
            has_upload_store = true;
        if (directives[i].name == "cgi_path")
            has_cgi_path = true;
    }

    if (has_cgi_on && !has_cgi_path)
        throw ValidationError("cgi on requires cgi path");
    if (has_cgi_on && !has_cgi_exec)
        throw ValidationError("cgi on requires cgi_exec");
    if (has_cgi_exec && !has_cgi_on)
        throw ValidationError("cgi_exec requires cgi on");
    if (has_upload_store && !has_upload_on)
        throw ValidationError("upload_store requires upload on");
}
