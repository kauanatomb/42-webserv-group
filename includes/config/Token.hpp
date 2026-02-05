#pragma once

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
