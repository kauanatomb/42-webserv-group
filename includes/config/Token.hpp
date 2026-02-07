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
};
