#include "config/ConfigParser.hpp"


/************************************OCF***************************************/
//ConfigParser::ConfigParser(void)
//{
//    return ;
//}

ConfigParser::ConfigParser(const std::vector<Token>& tokens) :  _tokens(tokens), _pos(0)
{
    return ;
}

//ConfigParser::ConfigParser(const ConfigParser &other)
//{
//    std::cout << "Copy constructor called" << std::endl;
//    (void) other;
//    return ;
//}
//
//ConfigParser &ConfigParser::operator=(const ConfigParser &other)
//{
//    std::cout << "Assignment operator called" << std::endl;
//    (void) other;
//    return (*this);
//}

ConfigParser::~ConfigParser(void)
{
    return ;
}


/***********************************Main Methods************************************/

ConfigAST ConfigParser::parse(void)
{
    ConfigAST  config;

    while (_pos < _tokens.size())
    {
        if (getCurrentTokenValue() == "server") // & _tokens[_pos].type == WORD 
            config.servers.push_back(parseServer());
        else if (getCurrentTokenValue() == "location") 
            throw std::runtime_error("Location outside of server");
        else
            throw std::runtime_error("Unexpected token");

        _pos = _tokens.size();
    }
    if (config.servers.empty()) { //discuss: case of empty config
        throw std::runtime_error("Config must contain at least one server block");
    }
    return (config);
}

/***************************Helper Functions************************************/
//Grammar server:        → "server" "{" (directive | location)* "}"
ServerNode ConfigParser::parseServer()
{   
    ServerNode newServer;

    if (getCurrentTokenValue() != "server") 
        throw std::runtime_error("Unexpected token value:" + TokenTypeToString(getCurrentTokenType())  + ", instead of \"server\"");
    _pos++;

    //checl LBRACE : case of wrong tokens or no more tokens
    if(_pos >= _tokens.size()) 
        throw std::runtime_error("without {");

    if (getCurrentTokenType() != LBRACE) 
        throw std::runtime_error("without {"); //merge in one 
    _pos++;

    if(_pos >= _tokens.size()) 
        throw std::runtime_error("without }");
    

    while( _tokens[this->_pos].type != RBRACE && _pos < _tokens.size())
    {
        if (getCurrentTokenValue() == "location")
        {
            LocationNode newLocation = parseLocation();
            newServer.locations.push_back(newLocation);
        }
        else if (getCurrentTokenType() == WORD)
        {
            Directive newDirective = parseDirective();
            newServer.directives.push_back(newDirective);
        }
    }

    if(_pos >= _tokens.size()) 
        throw std::runtime_error("without }");

    if (getCurrentTokenType() != RBRACE) 
        throw std::runtime_error("without }"); //merge in one 
    _pos++;

    if(_pos < _tokens.size())
        throw std::runtime_error("Unexpected token " + getCurrentTokenValue());
    return (newServer);
}



std::string ConfigParser::TokenTypeToString(TokenType type) const
{
    switch (type) {
        case WORD:      return "word";
        case LBRACE:    return "'{'";
        case RBRACE:    return "'}'";
        case SEMICOLON: return "';'";
        default:        return "unknown";
    }
}

// Grammar directive:     → WORD WORD* ";"
Directive ConfigParser::parseDirective()
{
    Directive newDirective;

    newDirective.name = getCurrentTokenValue(); //assign name with existence verified
        _pos++;

    if(_pos >= _tokens.size()) 
        throw std::runtime_error("Directive without ;");

    while(getCurrentTokenType() == WORD && _pos < _tokens.size() )
    {
        newDirective.args.push_back(getCurrentTokenValue());
        _pos++;
    }

    if(_pos >= _tokens.size()) 
        throw std::runtime_error("Directive without ;");
    
    if (getCurrentTokenType() == SEMICOLON)
        _pos++;
    else
        throw std::runtime_error("Unexpected token:" + TokenTypeToString(getCurrentTokenType()) + ", instead of \";\"");
    return (newDirective);
}


//Grammar location:      → "location" WORD "{" directive* "}"
LocationNode ConfigParser::parseLocation()
{
    LocationNode newLocation;

    if (getCurrentTokenValue() == "location")
        _pos++;

    if(_pos >= _tokens.size()) 
        throw std::runtime_error("Location without path");

    if (getCurrentTokenType() != WORD) 
        throw std::runtime_error("Unexpected token:" + TokenTypeToString(getCurrentTokenType()) + ", instead of WORD");
    _pos++;
    
    if(_pos >= _tokens.size()) 
        throw std::runtime_error("Location without {");

    if (getCurrentTokenType() != LBRACE) 
        throw std::runtime_error("Unexpected token:" + TokenTypeToString(getCurrentTokenType()) + ", instead of {");
    _pos++;
    
    while(getCurrentTokenType() != RBRACE && _pos < _tokens.size()) // will it cause brake in case there is no directive
    {
        Directive newDirective = parseDirective();
        newLocation.directives.push_back(newDirective);
    }
    if(_pos >= _tokens.size()) 
        throw std::runtime_error("Location without }");

    if (getCurrentTokenType() != RBRACE) 
        throw std::runtime_error("Unexpected token:" + TokenTypeToString(getCurrentTokenType()) + ", instead of }");
    _pos++;
    return (newLocation);
}



        
/***************************Getters and Setters************************************/
//**Getters and Setters
TokenType ConfigParser::getCurrentTokenType()
{
    if (_pos >= _tokens.size())
        throw std::runtime_error("Unexpected end of file while parsing");
    else
        return (_tokens[this->_pos].type );
}

std::string ConfigParser::getCurrentTokenValue()
{
    if (_pos >= _tokens.size())
        throw std::runtime_error("Unexpected end of file while parsing");
    else
        return (_tokens[this->_pos].value );
}

