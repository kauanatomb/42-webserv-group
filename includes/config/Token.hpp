#pragma once
#include <string>

enum TokenType {
    WORD,
    LBRACE,   // {
    RBRACE,   // }
    SEMICOLON // ;
};

struct Token {
    TokenType type;
    std::string value;
};
