#ifndef AST_PRINTER_HPP
# define AST_PRINTER_HPP

#include <iostream>
#include <iomanip>
#include "config/AST.hpp"

class ASTPrinter {
public:
    static void print(const ConfigAST& ast);

private:
    static void printServer(const ServerNode& server, int index);
    static void printLocation(const LocationNode& location);
    static void printDirective(const Directive& directive);
};

#endif