#ifndef TOKENBUILDER_HPP
#define TOKENBUILDER_HPP

#include "config/Token.hpp"
#include <string>


class TokenBuilder {
public:
    static Token makeWord(const std::string& value);
    static Token makeLBrace();
    static Token makeRBrace() ;
    static Token makeSemicolon();
    static Token makeEOT();
};

#endif 