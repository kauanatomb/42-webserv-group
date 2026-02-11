#include "config/ConfigParser.hpp"


/************************************OCF***************************************/
ConfigParser::ConfigParser(const std::vector<Token>& tokens) :  _tokens(tokens), _pos(0)
{
}


ConfigParser::~ConfigParser(void)
{
}


/***********************************Main Methods************************************/

ConfigAST ConfigParser::parse(void)
{
    ConfigAST  config;

    while (_pos < _tokens.size())
    {
        if (getCurrentTokenValue() == "server" && getCurrentTokenType() == WORD) 
            config.servers.push_back(parseServer());
        else
            throw SyntaxError("Unexpected token");
    }
    return (config);
}

/***************************Helper Functions************************************/
//Grammar server:        → "server" "{" (directive | location)* "}"
ServerNode ConfigParser::parseServer()
{   
    ServerNode newServer;

    _pos++;

    checkMandatoryToken(LBRACE, "without {");

    while(_pos < _tokens.size() && getCurrentTokenType() != RBRACE)
    {
        if (getCurrentTokenValue() == "location" && getCurrentTokenType() == WORD)
        {
            LocationNode newLocation = parseLocation();
            newServer.locations.push_back(newLocation);
        }
        else if (getCurrentTokenType() == WORD)
        {
            Directive newDirective = parseDirective();
            newServer.directives.push_back(newDirective);
        }
        else
            throw SyntaxError("Server: Unexpected token");
    }
    checkMandatoryToken(RBRACE, "without }");
    return (newServer);
}



// Grammar directive:     → WORD WORD* ";"
Directive ConfigParser::parseDirective()
{
    Directive newDirective;

    if (getCurrentTokenType() == WORD)
    {
        newDirective.name = getCurrentTokenValue(); 
        _pos++;
    }
    else
        throw SyntaxError("Unexpected token");

    if(_pos >= _tokens.size()) 
        throw SyntaxError("Directive without ;");

    while(_pos < _tokens.size() && getCurrentTokenType() == WORD)
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
    
    _pos++; // Skip "location" keyword

    if(_pos >= _tokens.size()) 
        throw SyntaxError("Location: missing path");
    if (getCurrentTokenType() != WORD) 
        throw SyntaxError("Location: path must be a word"); 
    newLocation.path = getCurrentTokenValue();
    if (newLocation.path.empty())
        throw SyntaxError("Location: path cannot be empty");
    _pos++;

    checkMandatoryToken(LBRACE, "Without {" );
    while(_pos < _tokens.size() && getCurrentTokenType() != RBRACE) 
    {
        Directive newDirective = parseDirective();
        newLocation.directives.push_back(newDirective);
    }
    checkMandatoryToken(RBRACE, "Location without }");
    return (newLocation);
}


void ConfigParser::checkMandatoryToken(TokenType type, const std::string& errorMessage)
{
    if(_pos >= _tokens.size()) 
        throw SyntaxError(errorMessage);
    if (getCurrentTokenType() != type) 
        throw SyntaxError(errorMessage); 
    _pos++;
}
        
/***************************Getters and Setters************************************/
//**Getters and Setters
const TokenType &ConfigParser::getCurrentTokenType() const
{
    if (_pos >= _tokens.size())
        throw SyntaxError("Unexpected end of file while parsing");
    else
        return (_tokens[this->_pos].type );
}

const std::string &ConfigParser::getCurrentTokenValue() const
{
    if (_pos >= _tokens.size())
        throw SyntaxError("Unexpected end of file while parsing");
    else
        return (_tokens[this->_pos].value );
}
