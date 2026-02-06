#pragma once
#include <string>

enum TokenType 
{
    TOK_WORD,
    TOK_LBRACE,    // {
    TOK_RBRACE,    // }
    TOL_SEMICOLON, // ;
    TOK_END        // EOF
};

struct Token 
{
    TokenType   type;
    std::string value;
    int         line;
    int         col;

    Token() : type(TOK_END), value (""), line (1), col (1) {};
    Token(TokenType t, const std::string& v, int l, int c)
        : type(t), value(v), line(l), col(c) {}
};