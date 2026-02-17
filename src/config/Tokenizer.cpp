#include "config/Tokenizer.hpp"
#include <iostream>
#include <cctype>
#include <vector>
#include <cstdio>  // For EOF

Tokenizer::Tokenizer(std::istream& in) : _in(in) {}

int Tokenizer::peekChar()
{
    return _in.peek();
}

int Tokenizer::getChar()
{
    return _in.get();
}

bool Tokenizer::isDelimiter(int ch) const
{
    return (ch == '{' || ch == '}' || ch == ';');
}

void Tokenizer::skipWhitespace()
{
    while(true)
    {
        int ch = peekChar();
        if (ch == '#')
        {
            while (ch != '\n' && ch != EOF)
            {
                getChar();
                ch = peekChar();
            }
            continue;
        }
        if (ch == EOF)
            return;
        if (ch >= 0 && !std::isspace(static_cast<unsigned char>(ch)))
            return;
        getChar();
    }
}

Token Tokenizer::makeSymbolToken(int ch)
{
    if (ch == '{') return Token(LBRACE, "{");
    if (ch == '}') return Token(RBRACE, "}");
    return Token(SEMICOLON, ";");
}

std::vector<Token> Tokenizer::tokenizer()
{
    std::vector<Token> tokens;

    while (true) 
    {
        skipWhitespace();

        int ch = peekChar();
        if(ch == EOF)
            break;
    
        // delimiter { } ;
        if (isDelimiter(ch)) {
            getChar();
            tokens.push_back(makeSymbolToken(ch));
            continue;
        }

        //WORD: read until whitespace or delimiter or #
        std::string word;
        word.reserve(32); // Reserve space to avoid reallocations
        while (true)
        {
            ch = peekChar();
            if (ch == EOF)
                break;
            if (ch == '#' || isDelimiter(ch) || (ch >= 0 && std::isspace(static_cast<unsigned char>(ch))))
                break;
            word.push_back(static_cast<char>(getChar()));
        }
        if (!word.empty())
            tokens.push_back(Token(WORD, word));
    }

    return tokens;
}

void Tokenizer::printTokens(const std::vector<Token>& tokens)
{
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        const Token& t = tokens[i];
        std::cout << "[" << i << "] ";

        switch (t.type) {
            case WORD:        std::cout << "WORD"; break;
            case LBRACE:      std::cout << "LBRACE"; break;
            case RBRACE:      std::cout << "RBRACE"; break;
            case SEMICOLON:   std::cout << "SEMICOLON"; break;
            default:              std::cout << "UNKNOWN"; break;
        }
        std::cout << " value='" << t.value << "'\n";
    }
}
