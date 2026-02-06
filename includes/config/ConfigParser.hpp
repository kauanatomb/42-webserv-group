#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

# include <iostream>
# include <vector>
# include "AST.hpp"
# include "Token.hpp"
# include <exception>

class ConfigParser
{
    private: 
        const std::vector<Token>& _tokens;
        size_t _pos;
    
        /***************************Helper Functions*****************************/
        ServerNode parseServer();
        LocationNode parseLocation();
        Directive parseDirective();

        /***************************Utils************************************/
        /**
        @brief Function that cuts the code flow if token type is not found, 
                else passes to next token
        */
        void continueIfMatchType(TokenType type);

        /**
        @brief Function that cuts the code flow if token value is not found, 
                else passes to next token
        */
        void continueIfMatchValue(const std::string &value);
    
    public:
        //***************************OCF************************************/
        //ConfigParser(void);
        ConfigParser(const std::vector<Token>& tokens); //added
        //ConfigParser &operator=(const ConfigParser &other);
        ~ConfigParser();

        /***************************Methods************************************/
        /**
        @brief Function that returns a ConfigAST structure containing the server nodes, given a tokenized version of the configuration file
        */
        ConfigAST parse(void);
            /*
            set configAST 
            detect server keyword and call parserServer
            */

        
        /***************************Getters and Setters************************************/
        //**Getters and Setters
        TokenType getCurrentTokenType();
        std::string getCurrentTokenValue();
        //size_t getPos() const ;
        //setPos(size_t newSize);
        
};



#endif

