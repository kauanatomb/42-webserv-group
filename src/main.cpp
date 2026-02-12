
#include <iostream>
#include "config/ConfigLoader.hpp"
#include "config/ConfigErrors.hpp"
#include "resolver/RuntimeConfig.hpp"
#include "resolver/ConfigResolver.hpp"

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file_path>" << std::endl;
        return 1;
    }
    const std::string configPath = argv[1];
    try {
        ConfigAST ast = ConfigLoader::load(configPath);
        const RuntimeConfig runtime = ConfigResolver::resolve(ast);
    } catch (const ConfigError& e) {
        std::cerr << "Config Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
