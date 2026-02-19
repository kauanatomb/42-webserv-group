#include "network/ServerEngine.hpp"
#include "network/RuntimeError.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

ServerEngine::ServerEngine(const RuntimeConfig& config) : _config(config) {}

void ServerEngine::start() {
    createListeningSockets();
    for (size_t i = 0; i < _listeningSockets.size(); ++i) {
        pollfd pfd;
        pfd.fd = _listeningSockets[i].fd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        _pollfds.push_back(pfd);
    }
    eventLoop();
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
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw RuntimeError("setsockopt() failed");
    
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(key.ip);
    addr.sin_port = htons(key.port);

    if (bind(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0)
        throw RuntimeError("bind() failed");
    if (listen(fd, SOMAXCONN) < 0)
        throw RuntimeError("listen() failed");
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        throw RuntimeError("fcntl F_GETFL failed");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        throw RuntimeError("fcntl O_NONBLOCK failed");
    ListeningSocket ls;
    ls.key = key;
    ls.fd = fd;
    _listeningSockets.push_back(ls);
}

void ServerEngine::eventLoop() {
    while (true) {
        int ready = poll(&_pollfds[0], _pollfds.size(), -1);
        if (ready <= 0)
            continue;
        for (size_t i = _pollfds.size(); i > 0; --i) {
            if (_pollfds[i - 1].revents != 0)
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

void ServerEngine::acceptConnection(int serverFd) {
    //1- find SocketKey associated with this listening fd
    SocketKey key;
    bool found = false;

    for (std::vector<ListeningSocket>::const_iterator it = _listeningSockets.begin();
        it != _listeningSockets.end(); ++ it)
        {
            if (it->fd == serverFd)
            {
                key = it->key;
                found = true;
                break;
            }
        }

        // if not found, refuse accepting:
        if (!found)
            return;
    //2- accept the client:        
    int clientFd = accept(serverFd, NULL, NULL);
    if (clientFd < 0)
        return;
    fcntl(clientFd, F_SETFL, O_NONBLOCK);
    //3- create the connection with the listen key 
    _connections.insert(std::make_pair(clientFd, Connection(clientFd, _config, key))); //every connection knows where it came from.
    
    //add to poll
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
