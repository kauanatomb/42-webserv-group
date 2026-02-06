#include "debug/ASTPrinter.hpp"


 void ASTPrinter::print(const ConfigAST& ast) {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════╗" << std::endl;
    std::cout << "║          CONFIGURATION AST            ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝" << std::endl;
    std::cout << "\n";
    
    std::cout << "📊 Total Servers: " << ast.servers.size() << "\n\n";
    
    for (size_t i = 0; i < ast.servers.size(); i++) {
        printServer(ast.servers[i], i + 1);
    }
}

 void ASTPrinter::printServer(const ServerNode& server, int index) {
    std::cout << "┌─────────────────────────────────────────┐" << std::endl;
    std::cout << "│ 🖥️  SERVER #" << index << std::setw(28) << "│" << std::endl;
    std::cout << "└─────────────────────────────────────────┘" << std::endl;
    
    // Print server directives
    if (!server.directives.empty()) {
        std::cout << "\n  📝 Server Directives:" << std::endl;
        for (size_t i = 0; i < server.directives.size(); i++) {
            std::cout << "    ";
            printDirective(server.directives[i]);
        }
    }
    
    // Print locations
    if (!server.locations.empty()) {
        std::cout << "\n  📍 Locations (" << server.locations.size() << "):" << std::endl;
        for (size_t i = 0; i < server.locations.size(); i++) {
            printLocation(server.locations[i]);
        }
    }
    
    std::cout << "\n";
}

 void ASTPrinter::printLocation(const LocationNode& location) {
    std::cout << "\n    ├─ Path: \"" << location.path << "\"" << std::endl;
    
    if (!location.directives.empty()) {
        std::cout << "    │  Directives:" << std::endl;
        for (size_t i = 0; i < location.directives.size(); i++) {
            std::cout << "    │    ";
            printDirective(location.directives[i]);
        }
    }
}

 void ASTPrinter::printDirective(const Directive& directive) {
    std::cout << "• " << directive.name;
    for (size_t i = 0; i < directive.args.size(); i++) {
        std::cout << " " << directive.args[i];
    }
    std::cout << ";" << std::endl;
}
