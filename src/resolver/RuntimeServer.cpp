#include "resolver/RuntimeServer.hpp"
#include <algorithm>

RuntimeServer::RuntimeServer() : client_max_body_size(0) {}

//getters
const std::vector<SocketKey>& RuntimeServer::getListens() const { return listens; }

const std::string& RuntimeServer::getRoot() const { return root; }

const std::vector<std::string>& RuntimeServer::getIndex() const { return index; }

size_t RuntimeServer::getClientMaxBodySize() const { return client_max_body_size; }


const std::map<int, std::string>& RuntimeServer::getErrorPages() const { 
    return error_pages; 
}

//setters
void RuntimeServer::addListen(const SocketKey& key) { listens.push_back(key); }

void RuntimeServer::addServerNames(const std::vector<std::string>& names) {
    server_names.insert(server_names.end(), names.begin(), names.end());
}

void RuntimeServer::setRoot(const std::string& value) { root = value; }

void RuntimeServer::addIndex(const std::vector<std::string>& values) {
    index.insert(index.end(), values.begin(), values.end());
}

void RuntimeServer::setClientMaxBodySizeServer(size_t value) { client_max_body_size = value; }

void RuntimeServer::addErrorPage(int status, const std::string& path) {
    error_pages[status] = path;
}

const std::vector<std::string>& RuntimeServer::getServerNames() const { 
    return server_names; 
}

const std::vector<RuntimeLocation>& RuntimeServer::getLocations() const {
    return locations;
}

void RuntimeServer::addLocation(const RuntimeLocation& loc) { locations.push_back(loc); }

// helpers
static bool isMoreSpecificLocation(const RuntimeLocation& left, const RuntimeLocation& right) {
    const std::string& leftPath = left.getPath();
    const std::string& rightPath = right.getPath();
    if (leftPath.size() != rightPath.size())
        return leftPath.size() > rightPath.size();
    return leftPath < rightPath;
}

// others
void RuntimeServer::sortLocations() {
    std::sort(locations.begin(), locations.end(), isMoreSpecificLocation);
}
