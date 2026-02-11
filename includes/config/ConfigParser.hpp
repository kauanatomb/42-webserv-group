#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

# include <iostream>
# include <vector>
# include "AST.hpp"
# include "Token.hpp"
# include "ConfigErrors.hpp"

class ConfigParser
{
    private: 
        const std::vector<Token>& _tokens;
        size_t _pos;
    
        /***************************Helper Functions*****************************/
        ServerNode parseServer();
        LocationNode parseLocation();
        Directive parseDirective();
        void checkMandatoryToken(TokenType type, const std::string& errorMessage);

    public:
        //***************************OCF************************************/
        ConfigParser(const std::vector<Token>& tokens);
        ~ConfigParser();

        /***************************Methods************************************/
        /**
        @brief Function that returns a ConfigAST structure containing the server nodes, given a tokenized version of the configuration file
        */
        ConfigAST parse(void);
        
        /***************************Getters and Setters************************************/
        const TokenType &getCurrentTokenType() const;
        const std::string &getCurrentTokenValue() const;

        static void printAST(const ConfigAST& ast);
};

#endif
