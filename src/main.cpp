#include <iostream>
#include <vector>
#include "config/ConfigLoader.hpp"
#include "config/ConfigErrors.hpp"
#include "config/Tokenizer.hpp"
#include <fstream>

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



//main to test tokenizer
// int main(int argc, char **argv)
// {
//     if (argc != 2) {
//         std::cerr << "Usage: " << argv[0] << " <config_file_path>" << std::endl;
//         return 1;
//     }

//     std::ifstream file (argv[1]);
//     Tokenizer tokenizer(file);
//     std::vector<Token> tokns = tokenizer.tokenize();
//     Tokenizer::printTokens(tokns);
//     return 0;
// }