// tokenizer class

#pragma once
#include <vector>
#include <istream>
#include "Token.hpp"

class Tokenizer
{
    public:
        explicit Tokenizer(std::istream& in);
        std::vector<Token> tokenizer();
        static void printTokens(const std::vector<Token>& tokens);
        
    private:
        std::istream& _in;

        int peekChar();
        int getChar();
        void skipWhitespace();
        bool isDelimiter(int ch) const;
        Token makeSymbolToken(int ch);
    
};