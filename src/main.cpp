
#include "config/ConfigParser.hpp"
#include <exception>

int main()
{
    //Token for testing(aggregate initialization so class Tokens is not called )
    std::vector<Token> tokens;
    Token tok;

    /* server { */
    tok.type = WORD;
    tok.value = "server";
    tokens.push_back(tok);

    tok.type = LBRACE;
    tok.value = "{";
    tokens.push_back(tok);

    /* listen 8080 ; */
    tok.type = WORD;
    tok.value = "listen";
    tokens.push_back(tok);

    tok.type = WORD;
    tok.value = "8080";
    tokens.push_back(tok);

    tok.type = SEMICOLON;
    tok.value = ";";
    tokens.push_back(tok);

    /* root /var/www ; */
    tok.type = WORD;
    tok.value = "root";
    tokens.push_back(tok);

    tok.type = WORD;
    tok.value = "/var/www";
    tokens.push_back(tok);

    tok.type = SEMICOLON;
    tok.value = ";";
    tokens.push_back(tok);

    /* location / { */
    tok.type = WORD;
    tok.value = "location";
    tokens.push_back(tok);

    tok.type = WORD;
    tok.value = "/";
    tokens.push_back(tok);

    tok.type = LBRACE;
    tok.value = "{";
    tokens.push_back(tok);

    /* index index.html ; */
    tok.type = WORD;
    tok.value = "index";
    tokens.push_back(tok);

    tok.type = WORD;
    tok.value = "index.html";
    tokens.push_back(tok);

    tok.type = SEMICOLON;
    tok.value = ";";
    tokens.push_back(tok);

    /* } } */
    tok.type = RBRACE;
    tok.value = "}";
    tokens.push_back(tok);

    tok.type = RBRACE;
    tok.value = "}";
    tokens.push_back(tok);

    ConfigParser Parse(tokens);
    try 
    {
        ConfigAST config1 = Parse.parse();
    }
    catch(std::exception &e)
    {
        std::cerr << "Caught parse error: " << e.what() << std::endl;
    }
}
/*
#include <iostream>
#include "config/ConfigLoader.hpp"
#include "config/ConfigErrors.hpp"

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file_path>" << std::endl;
        return 1;
    }
    const std::string configPath = argv[1];
    try {
        ConfigLoader::load(configPath);
    } catch (const ConfigError& e) {
        std::cerr << "Config Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
*/