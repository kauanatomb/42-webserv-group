#include <fstream>
#include <iostream>
#include "config/ConfigLoader.hpp"
#include "config/Tokenizer.hpp"
#include "config/ConfigParser.hpp"
#include "config/ConfigValidator.hpp"
#include "config/ConfigErrors.hpp"

ConfigAST ConfigLoader::load(const std::string& path) {
    if (path.empty())
        throw ConfigError("config path is empty");

    std::ifstream file(path.c_str());
    if (!file.is_open())
        throw ConfigError("cannot open config file");

    if (file.peek() == std::ifstream::traits_type::eof())
        throw ConfigError("config file is empty");

    Tokenizer tokenizer(file);
    std::vector<Token> tokens = tokenizer.tokenizer();
    //print tokens for debug
    // Tokenizer::printTokens(tokens);

    ConfigParser parser(tokens);
    ConfigAST ast = parser.parse();

    // print AST for debug
    // ConfigParser::printAST(ast);
    ConfigValidator validator;
    validator.validate(ast);

    return ast;
}