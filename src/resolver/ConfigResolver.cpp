#include "resolver/ConfigResolver.hpp"
#include "resolver/SocketKeyUtils.hpp"
#include "config/ConfigErrors.hpp"
#include <cstdlib>
#include <cctype>
#include <limits>
#include <iostream>
#include <algorithm>
#include <cstdio>

static size_t parseClientMaxBodySize(const std::string& value) {
    size_t i = 0;
    while (i < value.size() && std::isdigit(value[i]))
        ++i;
    if (i == 0)
        return 0;

    unsigned long base = std::strtoul(value.substr(0, i).c_str(), 0, 10);
    std::string suffix = value.substr(i);
    size_t multiplier = 1;

    if (suffix == "K" || suffix == "KB")
        multiplier = 1024;
    else if (suffix == "M" || suffix == "MB")
        multiplier = 1024 * 1024;
    else if (suffix == "G" || suffix == "GB")
        multiplier = 1024 * 1024 * 1024;

    // overflow check
    if (base > std::numeric_limits<size_t>::max() / multiplier)
        return std::numeric_limits<size_t>::max();
    
    return static_cast<size_t>(base) * multiplier;
}

RuntimeConfig ConfigResolver::resolve(const ConfigAST& ast) {
    RuntimeConfig runtime;
    for(std::vector<ServerNode>::const_iterator it = ast.servers.begin(); it != ast.servers.end(); ++it) {
        RuntimeServer server = buildServer(*it);
        const std::vector<SocketKey>& listens = server.getListens();
        for(std::vector<SocketKey>::const_iterator sk = listens.begin(); sk != listens.end(); ++sk) {
            runtime.servers[*sk].push_back(server);
                // debugPrintServer(server, *sk);
        }
    }
    return runtime;
}

RuntimeServer ConfigResolver::buildServer(const ServerNode& node) {
    RuntimeServer server;
    applyServerDirectives(server, node.directives);
    for(std::vector<LocationNode>::const_iterator locNode = node.locations.begin();
        locNode != node.locations.end(); ++locNode) {
            RuntimeLocation loc = buildLocation(*locNode, server);
            server.addLocation(loc);
    }
    server.sortLocations();
    return server;
}

void ConfigResolver::applyServerDirectives(RuntimeServer& server, const std::vector<Directive>& directives) {
    for(std::vector<Directive>::const_iterator it = directives.begin(); it != directives.end(); ++it) {
        if ((*it).name == "listen") {
            SocketKey key = SocketKeyUtils::fromString((*it).args[0]);
            if (SocketKeyUtils::exists(server.getListens(), key)) {
                throw ValidationError("Duplicate listen directive: " + SocketKeyUtils::toString(key));
            }
            server.addListen(key);
        }
        else if ((*it).name == "server_name")
            server.addServerNames((*it).args);
        else if ((*it).name == "root")
            server.setRoot((*it).args[0]);
        else if ((*it).name == "index")
            server.addIndex((*it).args);
        else if ((*it).name == "client_max_body_size")
            server.setClientMaxBodySizeServer(parseClientMaxBodySize((*it).args[0]));
        else if ((*it).name == "error_page") {
            const std::string& path = (*it).args.back();
            for (size_t i = 0; i + 1 < (*it).args.size(); ++i) {
                int code = std::atoi((*it).args[i].c_str());
                server.addErrorPage(code, path);
            }
        }
    }
    setDefaults(server);
}

void ConfigResolver::setDefaults(RuntimeServer& server) {
    if (server.getRoot().empty())
        server.setRoot("./www/html");
    
    if (server.getIndex().empty()) {
        std::vector<std::string> default_index;
        default_index.push_back("index.html");
        server.addIndex(default_index);
    }
    
    if (server.getClientMaxBodySize() == 0)
        server.setClientMaxBodySizeServer(1024 * 1024);
}

void ConfigResolver::ApplyLocationDirectives(const Directive& dir, RuntimeLocation& loc) {
    if (dir.name == "root")
        loc.setRoot(dir.args[0]);
    else if (dir.name == "index")
        loc.setIndex(dir.args);
    else if (dir.name == "client_max_body_size")
        loc.setClientMaxBodySizeLoc(parseClientMaxBodySize(dir.args[0]));
    else if (dir.name == "error_page") {
        const std::string& path = dir.args.back();
        for (size_t i = 0; i + 1 < dir.args.size(); ++i) {
            int code = std::atoi(dir.args[i].c_str());
            loc.addErrorPageLoc(code, path);
        }
    }
    else if (dir.name == "upload" || dir.name == "cgi" || (dir.name == "autoindex" && dir.args[0] == "on"))
        loc.changeStatus(dir.name);
    else if (dir.name == "allow_methods")
        loc.methodsHTTP(dir.args);
    else if (dir.name == "return")
        loc.setReturn(dir.args);
    else if (dir.name == "upload_store")
        loc.setUploadStore(dir.args[0]);
    else if (dir.name == "cgi_exec")
        loc.setCGI(dir.args);
}

RuntimeLocation ConfigResolver::buildLocation(const LocationNode& node, const RuntimeServer& parent) {
    RuntimeLocation loc(node.path);
    for(std::vector<Directive>::const_iterator it = node.directives.begin(); it != node.directives.end(); ++it) {
        ApplyLocationDirectives(*it, loc);
    }
    ApplyInheritance(loc, parent);
    return loc;
}

void ConfigResolver::ApplyInheritance(RuntimeLocation& loc, const RuntimeServer& parent) {
    if (!loc.getHasReturn()) {
        if (loc.getRoot().empty())
            loc.setRoot(parent.getRoot());
        if (loc.getIndex().empty())
            loc.setIndex(parent.getIndex());
    }
    loc.mergeErrorPage(parent.getErrorPages(), parent.getRoot());
    if (loc.getClientMaxBodySize() == 0)
        loc.setClientMaxBodySizeLoc(parent.getClientMaxBodySize());
}

// ---------------------------------------------------------------------------------------------------------------------
// Debug Methods

void ConfigResolver::debugPrintServerBasicInfo(const RuntimeServer& server, const SocketKey& sk) {
    std::cout << "=== Server Configuration ===" << std::endl;
    
    // Socket Key
    std::cout << "Socket: " << SocketKeyUtils::toString(sk) << std::endl;
    
    // Listen addresses
    const std::vector<SocketKey>& listens = server.getListens();
    std::cout << "Listen: ";
    for (size_t i = 0; i < listens.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << SocketKeyUtils::toString(listens[i]);
    }
    std::cout << std::endl;
    
    // Server names
    const std::vector<std::string>& server_names = server.getServerNames();
    std::cout << "Server Name: ";
    for (size_t i = 0; i < server_names.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << server_names[i];
    }
    std::cout << std::endl;
    
    // Root and Index
    std::cout << "Root: " << server.getRoot() << std::endl;
    
    const std::vector<std::string>& index = server.getIndex();
    std::cout << "Index: ";
    for (size_t i = 0; i < index.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << index[i];
    }
    std::cout << std::endl;
    
    // Client max body size
    std::cout << "Client Max Body Size: " << server.getClientMaxBodySize() << " bytes" << std::endl;
    
    // Error pages
    const std::map<int, std::string>& error_pages = server.getErrorPages();
    if (!error_pages.empty()) {
        std::cout << "Error Pages: ";
        for (std::map<int, std::string>::const_iterator it = error_pages.begin(); 
                it != error_pages.end(); ++it) {
            if (it != error_pages.begin()) std::cout << ", ";
            std::cout << it->first << ":" << it->second;
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void ConfigResolver::debugPrintLocation(const RuntimeLocation& loc, size_t index) {
    std::cout << "\nLocation [" << index << "]: " << loc.getPath() << std::endl;
    
    if (!loc.getRoot().empty())
        std::cout << "  Root: " << loc.getRoot() << std::endl;
    
    const std::vector<std::string>& loc_index = loc.getIndex();
    if (!loc_index.empty()) {
        std::cout << "  Index: ";
        for (size_t j = 0; j < loc_index.size(); ++j) {
            if (j > 0) std::cout << ", ";
            std::cout << loc_index[j];
        }
        std::cout << std::endl;
    }
    
    if (loc.getAutoindex())
        std::cout << "  Autoindex: on" << std::endl;
    
    const std::set<HttpMethod>& methods = loc.getAllowedMethods();
    if (!methods.empty()) {
        std::cout << "  Allowed Methods: ";
        bool first = true;
        for (std::set<HttpMethod>::const_iterator it = methods.begin(); it != methods.end(); ++it) {
            if (!first) std::cout << ", ";
            if (*it == GET) std::cout << "GET";
            else if (*it == POST) std::cout << "POST";
            else if (*it == DELETE) std::cout << "DELETE";
            first = false;
        }
        std::cout << std::endl;
    }
    
    // Client max body size
    std::cout << "  Client Max Body Size: " << loc.getClientMaxBodySize() << " bytes" << std::endl;
    
    // Error pages
    const std::map<int, std::string>& error_pages = loc.getErrorPages();
    if (!error_pages.empty()) {
        std::cout << "  Error Pages: ";
        for (std::map<int, std::string>::const_iterator it = error_pages.begin(); 
                it != error_pages.end(); ++it) {
            if (it != error_pages.begin()) std::cout << ", ";
            std::cout << it->first << ":" << it->second;
        }
        std::cout << std::endl;
    }

    if (loc.getHasReturn()) {
        const ReturnRule& redirect = loc.getRedirect();
        std::cout << "  Return: " << redirect.status_code;
        if (!redirect.target.empty())
            std::cout << " " << redirect.target;
        std::cout << std::endl;
    }
    
    if (loc.getHasUpload()) {
        std::cout << "  Upload: on" << std::endl;
        if (!loc.getUploadStore().empty())
            std::cout << "  Upload Store: " << loc.getUploadStore() << std::endl;
    }
    
    if (loc.getHasCGI()) {
        const std::map<std::string, std::string>& cgi_exec = loc.getCGIExec();
        if (!cgi_exec.empty()) {
            std::cout << "  CGI Exec: ";
            for (std::map<std::string, std::string>::const_iterator it = cgi_exec.begin();
                    it != cgi_exec.end(); ++it) {
                if (it != cgi_exec.begin()) std::cout << ", ";
                std::cout << it->first << " -> " << it->second;
            }
            std::cout << std::endl;
        }
    }
}

void ConfigResolver::debugPrintServer(const RuntimeServer& server, const SocketKey& sk) {
    debugPrintServerBasicInfo(server, sk);
    
    const std::vector<RuntimeLocation>& locations = server.getLocations();
    if (!locations.empty()) {
        std::cout << "=== Locations (" << locations.size() << ") ===" << std::endl;
        for (size_t i = 0; i < locations.size(); ++i) {
            debugPrintLocation(locations[i], i + 1);
        }
        std::cout << std::endl;
    }
}

// End Debug Methods
// ---------------------------------------------------------------------------------------------------------------------
