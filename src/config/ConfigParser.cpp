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
        throw std::runtime_error("Unexpected token:" + getCurrentTokenValue() + ", instead of \"server\"");
    _pos++;

    //checl LBRACE : case of wrong tokens or no more tokens
    if(_pos >= _tokens.size()) 
        throw std::runtime_error("without {");

    if (getCurrentTokenType() != LBRACE) 
        throw std::runtime_error("without {"); //merge in one 
    _pos++;

    std::cout << "F" << std::endl;
    if(_pos >= _tokens.size()) 
        throw std::runtime_error("without }");
    

    while( _tokens[this->_pos].type != RBRACE && _pos >= _tokens.size())
    {
        if (getCurrentTokenValue() == "location")
        {
            //LocationNode newLocation = parseLocation();
            //newServer.locations.push_back(newLocation);
            throw std::runtime_error("End");
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
    
    return (newServer);
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
        throw std::runtime_error("Unexpected token:" + getCurrentTokenValue() + ", instead of \";\"");
    return (newDirective);
}


//Grammar location:      → "location" WORD "{" directive* "}"
//LocationNode ConfigParser::parseLocation()
//{
//    LocationNode newLocation;
//
//    continueIfMatchValue("location");
//    if (getCurrentTokenType() == WORD)
//        newLocation.path = getCurrentTokenValue();
//    continueIfMatchType(WORD);
//    continueIfMatchType(LBRACE);
//    while(getCurrentTokenType() != RBRACE) // will it cause brake in case there is no directive
//    {
//        Directive newDirective = parseDirective();
//        newLocation.directives.push_back(newDirective);
//    }
//    continueIfMatchType(RBRACE);
//    return (newLocation);
//}



        
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

