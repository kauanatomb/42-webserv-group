#pragma once
#include <string>
#include <vector>
#include <map>
#include "RuntimeLocation.hpp"

class RuntimeServer {
    private:
        std::vector<SocketKey> listens;
        std::vector<std::string> server_names;
        std::string root;
        std::vector<std::string> index;
        size_t client_max_body_size;
        std::map<int, std::string> error_pages;
        std::vector<RuntimeLocation> locations;
    public:
        RuntimeServer();
        const std::vector<SocketKey>& getListens() const;
        const std::vector<std::string>& getServerNames() const;
        const std::vector<std::string>& getIndex() const;
        const std::string& getRoot() const;
        size_t getClientMaxBodySize() const;
        const std::map<int, std::string>& getErrorPages() const;
        const std::vector<RuntimeLocation>& getLocations() const;

        void addListen(const SocketKey& key);
        void addServerNames(const std::vector<std::string>& names);
        void setRoot(const std::string& value);
        void addIndex(const std::vector<std::string>& values);
        void setClientMaxBodySize(size_t value);
        void addErrorPage(int status, const std::string& path);
        void addLocation(const RuntimeLocation& loc);
        void sortLocations();
};
