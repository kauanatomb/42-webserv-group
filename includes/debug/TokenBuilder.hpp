#ifndef TOKENBUILDER_HPP
# define TOKENBUILDER_HPP

#include "debug/TokenBuilder.hpp"

class TokenBuilder {
public:
    static Token makeWord(const std::string& value);
    static Token makeLBrace();
    static Token makeRBrace() ;
    static Token makeSemicolon();
};

#endif 