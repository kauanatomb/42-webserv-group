#include <iostream>
#include "config/ConfigLoader.hpp"
#include "config/ConfigErrors.hpp"
#include "resolver/RuntimeConfig.hpp"
#include "resolver/ConfigResolver.hpp"
#include "network/ServerEngine.hpp"
#include "network/RuntimeError.hpp"

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file_path>" << std::endl;
        return 1;
    }
    const std::string configPath = argv[1];
    try {
        ConfigAST ast = ConfigLoader::load(configPath);
        const RuntimeConfig runtime = ConfigResolver::resolve(ast);

        ServerEngine engine(runtime);
        engine.start();
    } catch (const ConfigError& e) {
        std::cerr << "Config Error: " << e.what() << std::endl;
        return 1;
    } catch (const RuntimeError& e) {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
