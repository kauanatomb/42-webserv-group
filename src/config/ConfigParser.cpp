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
        else
            throw std::runtime_error("Unexpected token");

        _pos = _tokens.size();
    }

    if (config.servers.empty()) {
        throw std::runtime_error("Config must contain at least one server block");
    }
    return (config);
}

/***************************Helper Functions************************************/
//Grammar server:        → "server" "{" (directive | location)* "}"
ServerNode ConfigParser::parseServer()
{   
    ServerNode newServer;
    continueIfMatchValue("server"); 
    continueIfMatchType(LBRACE);   
    while(getCurrentTokenType() != RBRACE)
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
    continueIfMatchType(RBRACE);
    return (newServer);
}

//Grammar location:      → "location" WORD "{" directive* "}"
LocationNode ConfigParser::parseLocation()
{
    LocationNode newLocation;

    continueIfMatchValue("location");
    if (getCurrentTokenType() == WORD)
        newLocation.path = getCurrentTokenValue();
    continueIfMatchType(WORD);
    continueIfMatchType(LBRACE);
    while(getCurrentTokenType() != RBRACE) // will it cause brake in case there is no directive
    {
        Directive newDirective = parseDirective();
        newLocation.directives.push_back(newDirective);
    }
    continueIfMatchType(RBRACE);
    return (newLocation);
}

// Grammar directive:     → WORD WORD* ";"
Directive ConfigParser::parseDirective()
{
    Directive newDirective;

    newDirective.name = getCurrentTokenValue();
    continueIfMatchType(WORD);
    while(getCurrentTokenType() == WORD && _pos < _tokens.size() )
    {
        if (getCurrentTokenType() == WORD)
            newDirective.args.push_back(getCurrentTokenValue());
        _pos++;
    }
    continueIfMatchType(SEMICOLON);
    return (newDirective);
}

/***************************Utils************************************/
void ConfigParser::continueIfMatchType(TokenType type)
{ 
    if (getCurrentTokenType() != type)
        throw std::runtime_error("Unexpected token");
    _pos++;
    return;
}

void ConfigParser::continueIfMatchValue(const std::string &value)
{
    if (getCurrentTokenValue() != value)
        throw std::runtime_error("Unexpected token");
    _pos++;
    return;
}
        
/***************************Getters and Setters************************************/
//**Getters and Setters
TokenType ConfigParser::getCurrentTokenType()
{
    if (_pos >= _tokens.size())
        throw std::runtime_error("Token out range");
    else
        return (_tokens[this->_pos].type );
}

std::string ConfigParser::getCurrentTokenValue()
{
    if (_pos >= _tokens.size())
        throw std::runtime_error("Trying to access token out of range");
    else
        return (_tokens[this->_pos].value );
}