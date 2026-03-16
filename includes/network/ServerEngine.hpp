#pragma once

#include <vector>
#include <map>
#include <poll.h>
#include <csignal>
#include "resolver/RuntimeConfig.hpp"
#include "Connection.hpp"

class ServerEngine {
    private:
        const RuntimeConfig& _config;

        // fd -> SocketKey for listening sockets
        std::map<int, SocketKey> _listeningSockets;

        // fd -> Connection - active clients
        std::map<int, Connection> _connections;

        // poll descriptors
        std::vector<pollfd> _pollfds;

        std::map<int, int> _cgiToSocket; // pipe_fd → socket_fd of Connection

        // Self-pipe for signal handling
        static int _signalPipe[2];

        void createListeningSockets();
        void setupSocket(const SocketKey& key);
        void setupSignalHandling();

        void eventLoop();
        void handlePollEvent(int fd, short revents);

        bool getSocketKey(int fd, SocketKey& key) const;
        void acceptConnection(int serverFd);
        void closeConnection(int clientFd);
        void cleanup();

        static void signalHandler(int signum);

        void closeCgi(Connection& conn);
        void rebuildPollfds();
        void registerCgi(int socketFd, int stdinFd, int stdoutFd);
        void unregisterCgi(int stdinFd, int stdoutFd);

        void checkTimeouts();

        enum {
            POLL_TICK_MS = 500,
            CONN_IDLE_TIMEOUT_SEC = 30,
            CGI_TIMEOUT_SEC = 10
        };

    public:
        static volatile sig_atomic_t shutdownFlag;

        ServerEngine(const RuntimeConfig& config);
        ~ServerEngine();

        void start();
};
