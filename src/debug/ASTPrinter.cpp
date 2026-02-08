#include "debug/ASTPrinter.hpp"
#include <iostream>
#include <string>

void ASTPrinter::print(const ConfigAST& ast) {
    std::cout << "=== CONFIG AST ===" << std::endl;
    std::cout << "Total servers: " << ast.servers.size() << std::endl;
    std::cout << std::endl;
    
    for (size_t i = 0; i < ast.servers.size(); i++) {
        std::cout << "SERVER #" << (i + 1) << std::endl;
        printServer(ast.servers[i], 1);
        std::cout << std::endl;
    }
}
void ASTPrinter::printServer(const ServerNode& server, int indent) {
    std::string ind = getIndent(indent);
    
    std::cout << ind << "Directives: " << server.directives.size() << std::endl;
    for (size_t i = 0; i < server.directives.size(); i++) {
        printDirective(server.directives[i], indent + 1);
    }
    
    std::cout << ind << "Locations: " << server.locations.size() << std::endl;
    for (size_t i = 0; i < server.locations.size(); i++) {
        printLocation(server.locations[i], indent + 1);
    }
}

void ASTPrinter::printLocation(const LocationNode& location, int indent) {
    std::string ind = getIndent(indent);
    
    std::cout << ind << "LOCATION: \"" << location.path << "\"" << std::endl;
    std::cout << ind << "  Directives: " << location.directives.size() << std::endl;
    
    for (size_t i = 0; i < location.directives.size(); i++) {
        printDirective(location.directives[i], indent + 2);
    }
}

void ASTPrinter::printDirective(const Directive& directive, int indent) {
    std::string ind = getIndent(indent);
    
    std::cout << ind << directive.name;
    for (size_t i = 0; i < directive.args.size(); i++) {
        std::cout << " " << directive.args[i];
    }
    std::cout << ";" << std::endl;
}

std::string ASTPrinter::getIndent(int level) {
    return std::string(level * 2, ' ');
}