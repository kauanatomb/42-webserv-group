#include "config/ConfigParser.hpp"


//*** OCF
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


/***************************Main Methods************************************/
ConfigAST ConfigParser::parse(void)
{
    ConfigAST  config;
    while (_pos < _tokens.size())
    {
        if (getCurrentTokenValue() == "server") // & _tokens[_pos].type == WORD
        {
            _pos++; //skip to next token -> function  
            config.servers.push_back(parseServer());
        }
        else
            throw std::runtime_error("Unexpected token ");

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
    ServerNode severN;

    continueIfMatch(LBRACE);   
    while(getCurrentTokenType() != RBRACE)
    {
        if (getCurrentTokenValue() == "location")
            parseLocation();
        else if (getCurrentTokenType() == WORD)
            parseDirective();
    }
    continueIfMatch(RBRACE);
    //empty check?
    return (severN);
}

//Grammar location:      → "location" WORD "{" directive* "}"
LocationNode ConfigParser::parseLocation()
{
    LocationNode locationN;

    locationN.path = getCurrentTokenValue();
    _pos++//skip first word
    continueIfMatchType(WORD);
    continueIfMatchType(LBRACE);
    while(getCurrentTokenType() != RBRACE)
    {

    }
    continueIfMatchType(RBRACE);
}

// Grammar directive:     → WORD WORD* ";"
Directive ConfigParser::parseDirective()
{
    Directive directiveN;

    directiveN.name = getCurrentTokenValue();
    _pos++//skip first word
    while(getCurrentTokenType() != SEMICOLON)
    {
        if (getCurrentTokenType() == WORD)
            directiveN.args.push_back(getCurrentTokenValue());
        pos++
    }
    continueIfMatchType(SEMICOLON);
}

/***************************Utils************************************/
void ConfigParser::continueIfMatchType(TokenType type)
{
    if (_tokens[this->_pos].type != type)
        throw std::runtime_error("Unexpected token type");
    _pos++;
    return;
}

void continueIfMatchValue(const std::string &value)
{
    if (_tokens[this->_pos].value != value)
        throw std::runtime_error("Unexpected token value");
    _pos++;
    return;
}


        
/***************************Getters and Setters************************************/
//**Getters and Setters
TokenType getCurrentTokenType()
{
    return (_tokens[this->_pos].type );
}

std::string getCurrentTokenValue()
{
    return (_tokens[this->_pos].value );
}