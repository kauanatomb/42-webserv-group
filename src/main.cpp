
#include "config/ConfigParser.hpp"

int main()
{
    //Token for testing(aggregate initialization so class Tokens is not called )
    std::vector<Token> tokens =
    {
        { WORD, "server" },
        { LBRACE, "{" },

        { WORD, "listen" },
        { WORD, "8080" },
        { SEMICOLON, ";" },

        { WORD, "root" },
        { WORD, "/var/www" },
        { SEMICOLON, ";" },

        { WORD, "location" },
        { WORD, "/" },
        { LBRACE, "{" },

        { WORD, "index" },
        { WORD, "index.html" },
        { SEMICOLON, ";" },

        { RBRACE, "}" },
        { RBRACE, "}" }
    };


    ConfigParser Parse(tokens);

    ConfigAST config1 = Parse.parse();
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