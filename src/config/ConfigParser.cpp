#include "config/ConfigParser.hpp"


/************************************OCF***************************************/
ConfigParser::ConfigParser(const std::vector<Token>& tokens) :  _tokens(tokens), _pos(0)
{
    return ;
}


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
        if (getCurrentTokenValue() == "server") 
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

    checkMandatoryToken(LBRACE, "without {");

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
    checkMandatoryToken(RBRACE, "without }");
    if(_pos < _tokens.size()) //for extra closing braces
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
    checkMandatoryToken(SEMICOLON, "without ;");
    return (newDirective);
}


//Grammar location:      → "location" WORD "{" directive* "}"
LocationNode ConfigParser::parseLocation()
{
    LocationNode newLocation;
    
    if (getCurrentTokenValue() == "location")
        _pos++;

    checkMandatoryToken(WORD, "Location : Unexpected token" );
    checkMandatoryToken(LBRACE, "Without {" );
    while(getCurrentTokenType() != RBRACE && _pos < _tokens.size()) 
    {
        Directive newDirective = parseDirective();
        newLocation.directives.push_back(newDirective);
    }
    checkMandatoryToken(RBRACE, "Location without }");
    return (newLocation);
}


void ConfigParser::checkMandatoryToken(TokenType type, std::string errorMessage)
{
    if(_pos >= _tokens.size()) 
        throw std::runtime_error(errorMessage);
    if (getCurrentTokenType() != type) 
        throw std::runtime_error(errorMessage); 
    _pos++;
}
        
/***************************Getters and Setters************************************/
//**Getters and Setters
const TokenType &ConfigParser::getCurrentTokenType() const
{
    if (_pos >= _tokens.size())
        throw std::runtime_error("Unexpected end of file while parsing");
    else
        return (_tokens[this->_pos].type );
}

const std::string &ConfigParser::getCurrentTokenValue() const
{
    if (_pos >= _tokens.size())
        throw std::runtime_error("Unexpected end of file while parsing");
    else
        return (_tokens[this->_pos].value );
}

