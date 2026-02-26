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

        // fd -> SocketKey for listening sockets
        std::map<int, SocketKey> _listeningSockets;

        // fd -> Connection - active clients
        std::map<int, Connection> _connections;

        // poll descriptors
        std::vector<pollfd> _pollfds;

        // Self-pipe for signal handling
        static int _signalPipe[2];

        void createListeningSockets();
        void setupSocket(const SocketKey& key);
        void setupSignalHandling();

        void eventLoop();
        void handlePollEvent(size_t index);

        bool getSocketKey(int fd, SocketKey& key) const;
        void acceptConnection(int serverFd);
        void closeConnection(int clientFd);
        void cleanup();

        static void signalHandler(int signum);

    public:
        ServerEngine(const RuntimeConfig& config);
        ~ServerEngine();

        void start();
};
