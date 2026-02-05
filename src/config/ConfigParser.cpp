#include "ConfigParser.hpp"


//ConfigParser::ConfigParser(void)
//{
//    return ;
//}

ConfigParser::ConfigParser(const std::vector<Token>& tokens) : _pos(0), _tokens(tokens)
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


//Methods
ConfigAST ConfigParser::parse(void)
{
    ConfigAST  config;
    while (_pos < _tokens.size())
    {
        if (_tokens[_pos].value == "server") // & _tokens[_pos].type == WORD
            std::cout << "First word okay ! " << std::endl; //parseServer();
        else
            throw std::runtime_error("Unexpected token ");
        //For first test set position to final
        _pos = _tokens.size();
    }

    return (config);
}
/*
loop over token vector 
    if server 
        server Node
    else 
        throw

*/

//Helper Functions 

ServerNode parseServer(){}
LocationNode parseLocation(){}
Directive parseDirective(){}



//Getters and Setters
//size_t getPos() const
//{
//    retu
//}
//setPos(size_t newSize);