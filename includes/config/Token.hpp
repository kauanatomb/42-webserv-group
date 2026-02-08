#pragma once
#include <string>

enum TokenType {
    WORD,
    LBRACE,   // {
    RBRACE,   // }
    SEMICOLON, // ;
    EOT
};

struct Token {
    TokenType type;
    std::string value;

    Token() : type(EOT), value ("") {}
    Token(TokenType t, const std::string& v) : type(t), value(v) {}
};
