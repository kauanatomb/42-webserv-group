#include "config/ConfigParser.hpp"
#include "debug/ASTPrinter.hpp"
#include "debug/TokenBuilder.hpp"
#include <exception>



int main()
{
    //Token for testing(aggregate initialization so class Tokens is not called )
    /* Test1 : Minimal valid config
        server {
            listen 8080;
        }
    */
    std::vector<Token> test1;
    test1.push_back(TokenBuilder::makeWord("server"));
    test1.push_back(TokenBuilder::makeLBrace());
    test1.push_back(TokenBuilder::makeWord("listen"));
    test1.push_back(TokenBuilder::makeWord("8080"));
    test1.push_back(TokenBuilder::makeSemicolon());
    test1.push_back(TokenBuilder::makeRBrace());
    /*Test2*/


    ConfigParser Parse(test1);
    try 
    {
        ConfigAST config1 = Parse.parse();
        ASTPrinter::print(config1);
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