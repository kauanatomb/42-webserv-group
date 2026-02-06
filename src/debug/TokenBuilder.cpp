#include "debug/TokenBuilder.hpp"

static Token makeWord(const std::string& value) {
    Token tok;
    tok.type = WORD;
    tok.value = value;
    return tok;
}

static Token makeLBrace() {
    Token tok;
    tok.type = LBRACE;
    tok.value = "{";
    return tok;
}

static Token makeRBrace() {
    Token tok;
    tok.type = RBRACE;
    tok.value = "}";
    return tok;
}

static Token makeSemicolon() {
    Token tok;
    tok.type = SEMICOLON;
    tok.value = ";";
    return tok;
}
