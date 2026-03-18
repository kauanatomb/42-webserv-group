#include "network/ServerEngine.hpp"
#include "network/RuntimeError.hpp"
#include "logger/Logger.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <csignal>
#include <iostream>
#include <errno.h>
#include <sys/wait.h>

int ServerEngine::_signalPipe[2] = {-1, -1};
volatile sig_atomic_t ServerEngine::shutdownFlag = 0;

ServerEngine::ServerEngine(const RuntimeConfig& config) : _config(config) {}

ServerEngine::~ServerEngine() {
    cleanup();
}

void ServerEngine::signalHandler(int signum) {
    (void)signum;
    shutdownFlag = 1;
    char c = 'x';
    write(_signalPipe[1], &c, 1);
}

void ServerEngine::setupSignalHandling() {
    if (pipe(_signalPipe) < 0)
        throw RuntimeError("pipe() failed for signal handling");
    fcntl(_signalPipe[0], F_SETFL, O_NONBLOCK);
    fcntl(_signalPipe[1], F_SETFL, O_NONBLOCK);

    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

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
        if (it->second.hasCgi())
            it->second.killCgi();
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

    struct in_addr inAddr;
    inAddr.s_addr = htonl(key.ip);
    Logger::serverInit(inet_ntoa(inAddr), key.port);

    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        close(fd);
        throw RuntimeError("fcntl O_NONBLOCK failed");
    }
    _listeningSockets[fd] = key;
}

void ServerEngine::eventLoop() {
    while (true) {
        int ready = poll(&_pollfds[0], _pollfds.size(), POLL_TICK_MS);
        if (ready < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (ready > 0) {
            // snapshot
            std::vector<std::pair<int,short> > ready_fds;
            for (size_t i = 0; i < _pollfds.size(); ++i) {
                if (_pollfds[i].revents != 0)
                    ready_fds.push_back(
                        std::make_pair(_pollfds[i].fd, _pollfds[i].revents));
            }
            for (size_t i = 0; i < ready_fds.size(); ++i) {
                if (ready_fds[i].first == _signalPipe[0]) {
                    char buf[16];
                    while (read(_signalPipe[0], buf, sizeof(buf)) > 0);
                    std::cout << "\nShutting down gracefully..." << std::endl;
                    return;
                }
                handlePollEvent(ready_fds[i].first, ready_fds[i].second);
            }
        }
        checkTimeouts();
    }
}

void ServerEngine::handlePollEvent(int fd, short revents) {

    // 1) CGI pipe
    std::map<int,int>::iterator cit = _cgiToSocket.find(fd);
    if (cit != _cgiToSocket.end()) {
        std::map<int, Connection>::iterator itConn = _connections.find(cit->second);
        if (itConn == _connections.end()) {
            _cgiToSocket.erase(cit);
            return;
        }

        Connection& conn = itConn->second;
        const int inFd  = conn.getCgiStdinFd();
        const int outFd = conn.getCgiStdoutFd();

        if (fd == outFd && (revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)))
            conn.onCgiReadable();

        if (fd == inFd && (revents & POLLOUT))
            conn.onCgiWritable();

        if (inFd != -1 && conn.getCgiStdinFd() == -1)
            _cgiToSocket.erase(inFd);

        if (conn.hasCgi() && (conn.isCgiDone() || (revents & (POLLERR | POLLHUP | POLLNVAL))))
            closeCgi(conn);

        rebuildPollfds();
        return;
    }

    // 2) socket normal
    if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
        closeConnection(fd);
        return;
    }

    std::map<int, Connection>::iterator it = _connections.find(fd);
    if (it == _connections.end()) {
        if (revents & POLLIN)
            acceptConnection(fd);
        return;
    }

    Connection& conn = it->second;
    if (revents & POLLIN)
        conn.onReadable();

    if (conn.hasCgi() && !_cgiToSocket.count(conn.getCgiStdoutFd())) {
        registerCgi(fd, conn.getCgiStdinFd(), conn.getCgiStdoutFd());
        rebuildPollfds();
        return;
    }

    if ((revents & POLLOUT) && conn.wantsWrite())
        conn.onWritable();

    if (conn.isClosed()) {
        closeConnection(fd);
        return;
    }

    if (conn.wantsWrite())
        rebuildPollfds();
}

void ServerEngine::closeConnection(int clientFd) {
    std::map<int, Connection>::iterator itConn = _connections.find(clientFd);
    if (itConn != _connections.end()) {
        int inFd = itConn->second.getCgiStdinFd();
        int outFd = itConn->second.getCgiStdoutFd();

        unregisterCgi(inFd, outFd);
        if (inFd != -1)  close(inFd);
        if (outFd != -1) close(outFd);
    }

    close(clientFd);
    _connections.erase(clientFd);
    Logger::disconnection(clientFd);
    rebuildPollfds();
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
    
    while (true)
    {
        int clientFd = accept(serverFd, NULL, NULL);
        if (clientFd < 0)
            break;

        struct sockaddr_in peerAddr;
        socklen_t peerLen = sizeof(peerAddr);
        std::string clientIp = "unknown";
        if (getpeername(clientFd, reinterpret_cast<struct sockaddr*>(&peerAddr), &peerLen) == 0)
            clientIp = inet_ntoa(peerAddr.sin_addr);
        Logger::newConnection(clientFd, clientIp);

        fcntl(clientFd, F_SETFL, O_NONBLOCK);
        _connections.insert(
            std::make_pair(clientFd, Connection(clientFd, _config, key)));
        pollfd pfd;
        pfd.fd = clientFd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        _pollfds.push_back(pfd);
    }
} 

void ServerEngine::registerCgi(int socketFd, int stdinFd, int stdoutFd) {
    if (stdinFd  != -1) _cgiToSocket[stdinFd]  = socketFd;
    if (stdoutFd != -1) _cgiToSocket[stdoutFd] = socketFd;
}

void ServerEngine::unregisterCgi(int stdinFd, int stdoutFd) {
    if (stdinFd  != -1) _cgiToSocket.erase(stdinFd);
    if (stdoutFd != -1) _cgiToSocket.erase(stdoutFd);
}

void ServerEngine::closeCgi(Connection& conn) {
    unregisterCgi(conn.getCgiStdinFd(), conn.getCgiStdoutFd());
    conn.finalizeCgi();
    rebuildPollfds();
}

void ServerEngine::rebuildPollfds() {
    _pollfds.clear();
    
    pollfd sigpfd;
    sigpfd.fd = _signalPipe[0];
    sigpfd.events = POLLIN;
    sigpfd.revents = 0;
    _pollfds.push_back(sigpfd);
    
    for (std::map<int,SocketKey>::iterator it = _listeningSockets.begin();
            it != _listeningSockets.end(); ++it) {
        pollfd pfd;
        pfd.fd = it->first;
        pfd.events = POLLIN;
        pfd.revents = 0;
        _pollfds.push_back(pfd);
    }
    for (std::map<int,Connection>::iterator it = _connections.begin();
            it != _connections.end(); ++it) {
        Connection& conn = it->second;
        pollfd pfd;
        pfd.revents = 0;
        pfd.fd = it->first;
        pfd.events = POLLIN;
        if (conn.wantsWrite())
            pfd.events |= POLLOUT;
        _pollfds.push_back(pfd);

        if (conn.hasCgi()) {
            if (conn.getCgiStdinFd() != -1) {
                pollfd p;
                p.fd = conn.getCgiStdinFd();
                p.events = POLLOUT;
                p.revents = 0;
                _pollfds.push_back(p);
            }
            if (conn.getCgiStdoutFd() != -1) {
                pollfd p;
                p.fd = conn.getCgiStdoutFd();
                p.events = POLLIN;
                p.revents = 0;
                _pollfds.push_back(p);
            }
        }
    }
}

void ServerEngine::checkTimeouts() {
    time_t now = time(NULL);
    std::vector<int> toClose;
    bool needRebuild = false;

    for (std::map<int, Connection>::iterator it = _connections.begin();
            it != _connections.end(); ++it) {
        Connection& conn = it->second;

        if (conn.hasCgi() && difftime(now, conn.getCgiStartTime()) >= CGI_TIMEOUT_SEC) {
            unregisterCgi(conn.getCgiStdinFd(), conn.getCgiStdoutFd());
            conn.handleCgiTimeout();
            needRebuild = true;
            continue;
        }

        if (!conn.hasCgi() && !conn.wantsWrite() &&
                conn.isIdleSince(now, CONN_IDLE_TIMEOUT_SEC))
            toClose.push_back(it->first);
    }

    for (size_t i = 0; i < toClose.size(); ++i)
        closeConnection(toClose[i]);

    if (needRebuild && toClose.empty())
        rebuildPollfds();
}