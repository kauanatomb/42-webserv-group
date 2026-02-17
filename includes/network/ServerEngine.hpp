#pragma once

#include <vector>
#include <map>
#include <poll.h>
#include "resolver/RuntimeConfig.hpp"
#include "Connection.hpp"
// #include "RequestHandler.hpp"

class ServerEngine {
    private:
        const RuntimeConfig& _config;

        // Listening sockets
        struct ListeningSocket {
            SocketKey key;
            int fd;
        };

        std::vector<ListeningSocket> _listeningSockets;

        // fd -> Connection - active clients
        std::map<int, Connection> _connections;

        // poll descriptors
        std::vector<pollfd> _pollfds;

        void createListeningSockets();
        void setupSocket(const SocketKey& key);

        void eventLoop();
        void handlePollEvent(size_t index);

        void acceptConnection(int serverFd);
        void closeConnection(int clientFd);

    public:
        ServerEngine(const RuntimeConfig& config);

        void start();
};
