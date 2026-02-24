#include "network/ServerEngine.hpp"
#include "network/RuntimeError.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <csignal>
#include <iostream>

int ServerEngine::_signalPipe[2] = {-1, -1};

ServerEngine::ServerEngine(const RuntimeConfig& config) : _config(config) {}

ServerEngine::~ServerEngine() {
    cleanup();
}

void ServerEngine::signalHandler(int signum) {
    (void)signum;
    char c = 'x';
    write(_signalPipe[1], &c, 1);
}

void ServerEngine::setupSignalHandling() {
    if (pipe(_signalPipe) < 0)
        throw RuntimeError("pipe() failed for signal handling");
    fcntl(_signalPipe[0], F_SETFL, O_NONBLOCK);
    fcntl(_signalPipe[1], F_SETFL, O_NONBLOCK);

    struct sigaction sa;
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);

    pollfd pfd;
    pfd.fd = _signalPipe[0];
    pfd.events = POLLIN;
    pfd.revents = 0;
    _pollfds.push_back(pfd);
}

void ServerEngine::cleanup() {
    for (std::map<int, Connection>::iterator it = _connections.begin();
            it != _connections.end(); ++it) {
        close(it->first);
    }
    _connections.clear();

    for (std::map<int, SocketKey>::iterator it = _listeningSockets.begin();
            it != _listeningSockets.end(); ++it) {
        close(it->first);
    }
    _listeningSockets.clear();
    _pollfds.clear();

    if (_signalPipe[0] != -1) {
        close(_signalPipe[0]);
        close(_signalPipe[1]);
        _signalPipe[0] = -1;
        _signalPipe[1] = -1;
    }
}

void ServerEngine::start() {
    setupSignalHandling();
    createListeningSockets();
    for (std::map<int, SocketKey>::iterator it = _listeningSockets.begin();
            it != _listeningSockets.end(); ++it) {
        pollfd pfd;
        pfd.fd = it->first;
        pfd.events = POLLIN;
        pfd.revents = 0;
        _pollfds.push_back(pfd);
    }
    eventLoop();
    cleanup();
}

void ServerEngine::createListeningSockets() {
    const std::map<SocketKey, std::vector<RuntimeServer> >& servers = _config.getServers();

    for (std::map<SocketKey, std::vector<RuntimeServer> >::const_iterator it = servers.begin();
            it != servers.end(); ++it) {
        setupSocket(it->first);
    }
}

void ServerEngine::setupSocket(const SocketKey& key) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        throw RuntimeError("socket() failed");
    
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(fd);
        throw RuntimeError("setsockopt() failed");
    }
    
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(key.ip);
    addr.sin_port = htons(key.port);

    if (bind(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(fd);
        throw RuntimeError("bind() failed");
    }
    if (listen(fd, SOMAXCONN) < 0) {
        close(fd);
        throw RuntimeError("listen() failed");
    }
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        close(fd);
        throw RuntimeError("fcntl F_GETFL failed");
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(fd);
        throw RuntimeError("fcntl O_NONBLOCK failed");
    }
    _listeningSockets[fd] = key;
}

void ServerEngine::eventLoop() {
    while (true) {
        int ready = poll(&_pollfds[0], _pollfds.size(), -1);
        if (ready < 0) {
            if (errno == EINTR)
                continue;
            break;
        }
        if (ready == 0)
            continue;
        for (size_t i = 0; i < _pollfds.size(); ++i) {
            if (_pollfds[i].fd == _signalPipe[0] && (_pollfds[i].revents & POLLIN)) {
                char c;
                read(_signalPipe[0], &c, 1);
                std::cout << "\nShutting down gracefully..." << std::endl;
                return;
            }
        }
        for (size_t i = _pollfds.size(); i > 0; --i) {
            if (_pollfds[i - 1].revents != 0 && _pollfds[i - 1].fd != _signalPipe[0])
                handlePollEvent(i - 1);
        }
    }
}

void ServerEngine::handlePollEvent(size_t index) {
    pollfd& pfd = _pollfds[index];
    if (pfd.revents & (POLLERR | POLLHUP)) {
        closeConnection(pfd.fd);
        return;
    }
    std::map<int, Connection>::iterator it = _connections.find(pfd.fd);
    if (it == _connections.end()) {
        if (pfd.revents & POLLIN)
            acceptConnection(pfd.fd);
        return;
    }
    Connection& conn = it->second;
    if (pfd.revents & POLLIN)
        conn.onReadable();
    if (pfd.revents & POLLOUT)
        conn.onWritable();
    if (conn.isClosed()) {
        closeConnection(pfd.fd);
        return;
    }
    if (conn.wantsWrite())
        pfd.events = POLLOUT;
    else
        pfd.events = POLLIN;
}

bool ServerEngine::getSocketKey(int fd, SocketKey& key) const {
    std::map<int, SocketKey>::const_iterator it = _listeningSockets.find(fd);
    if (it == _listeningSockets.end())
        return false;
    key = it->second;
    return true;
}

void ServerEngine::acceptConnection(int serverFd) {
    SocketKey key;
    if (!getSocketKey(serverFd, key))
        return;
    
    int clientFd = accept(serverFd, NULL, NULL);
    if (clientFd < 0)
        return;
    fcntl(clientFd, F_SETFL, O_NONBLOCK);
    _connections.insert(std::make_pair(clientFd, Connection(clientFd, _config, key)));
    pollfd pfd;
    pfd.fd = clientFd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _pollfds.push_back(pfd);
}

void ServerEngine::closeConnection(int clientFd) {
    close(clientFd);
    _connections.erase(clientFd);
    for (std::vector<pollfd>::iterator it = _pollfds.begin(); it != _pollfds.end(); ++it) {
        if (it->fd == clientFd) {
            _pollfds.erase(it);
            break;
        }
    }
}   
