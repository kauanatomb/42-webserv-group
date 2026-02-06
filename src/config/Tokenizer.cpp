// tokenizer implementation

#include "config/Tokenizer.hpp"
#include "config/ConfigErrors.hpp"
#include <iostream>
#include <cctype>
#include <vector>
#include <cstdio>
#include <sstream>

Tokenizer::Tokenizer(std::istream& in)
    : _in(in), _line(1), _col(1) {}

int Tokenizer::peekChar()
{
    return _in.peek();
}

int Tokenizer::getChar()
{
    int ch = _in.get();
    if (ch == '\n')
    {
        _line++;
        _col = 1;
    }
    else if (ch != EOF)
    {
        _col++;
    }
    return ch;
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
        if (ch == EOF)
            return;
        if (!std::isspace(static_cast<unsigned char>(ch)))
            return;
        getChar();
    }
}

Token Tokenizer::makeSymbolToken(int ch, int line, int col)
{
    if (ch == '{') return Token(TOK_LBRACE, "{", line, col);
    if (ch == '}') return Token(TOK_RBRACE, "}", line, col);
    return Token(TOK_SEMICOLON, ";", line, col);
}

std::vector<Token> Tokenizer::tokenizer() {
    std::vector<Token> tokens;

    while (true) {
        skipWhitespace();

        int ch = peekChar();
        if(ch == EOF)
            break;
        
        //record token start position : line/col
        int startLine = _line;
        int startCol = _col;

        // delimiter { } ;
        if (isDelimiter(ch)) {
            getChar();
            tokens.push_back(makeSymbolToken(ch, startLine, startCol));
            continue;
        }

        //WORD: read until whitespace or delimiter
        std::string word;
        while (true) {
            ch = peekChar();
            if (ch == EOF)
                break;
            if (std::isspace(static_cast<unsigned char>(ch)) || isDelimiter(ch))
                break;
            //consume char
            int c = getChar();
            if (c == EOF)
                break;
            word.push_back(static_cast<char>(c));
        }


        if (word.empty()) {
            // to avoid infite loop
            std::ostringstream oss;
            oss << "tokenizer stuck at line " << _line << ", col " << _col;
            throw SyntaxError(oss.str());
        }
        tokens.push_back(Token(TOK_WORD, word, startLine, startCol));
    }
    tokens.push_back(Token(TOK_END, "", _line, _col));
    return tokens;
}

void Tokenizer::printTokens(const std::vector<Token>& tokens)
{
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        const Token& t = tokens[i];
        std::cout << "[" << i << "] "
                  << "line " << t.line << ", col " << t.col << " ";

        switch (t.type) {
            case TOK_WORD:        std::cout << "WORD"; break;
            case TOK_LBRACE:      std::cout << "LBRACE"; break;
            case TOK_RBRACE:      std::cout << "RBRACE"; break;
            case TOK_SEMICOLON:   std::cout << "SEMICOLON"; break;
            case TOK_END:         std::cout <<  "END"; break;
            default:              std::cout << "UNKNOWN"; break;
        }
        std::cout << " value='" << t.value << "'\n";
    }
}