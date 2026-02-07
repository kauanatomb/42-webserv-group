#include "debug/TokenBuilder.hpp"

 Token TokenBuilder::makeWord(const std::string& value) {
    Token tok;
    tok.type = WORD;
    tok.value = value;
    return tok;
}

 Token TokenBuilder::makeLBrace() {
    Token tok;
    tok.type = LBRACE;
    tok.value = "{";
    return tok;
}

 Token TokenBuilder::makeRBrace() {
    Token tok;
    tok.type = RBRACE;
    tok.value = "}";
    return tok;
}

 Token TokenBuilder::makeSemicolon() {
    Token tok;
    tok.type = SEMICOLON;
    tok.value = ";";
    return tok;
}

