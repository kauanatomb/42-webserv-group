#ifndef AST_PRINTER_HPP
# define AST_PRINTER_HPP

#include <iostream>
#include <iomanip>
#include "config/AST.hpp"

#include <iostream>
#include <string>

class ASTPrinter {
public:
    static void print(const ConfigAST& ast);

private:
    static void printServer(const ServerNode& server, int indent);
    static void printLocation(const LocationNode& location, int indent);
    static void printDirective(const Directive& directive, int indent);
    static std::string getIndent(int level);
};

#endif